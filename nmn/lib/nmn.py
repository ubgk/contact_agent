#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: Apache-2.0
# Copyright 2024 Inria

from typing import Tuple

import numpy as np
import torch
from numpy.typing import NDArray
from torch import nn

# Noise values identified from real-robot logs
LINEAR_ACCELERATION_NOISE = np.array([0.02, 0.02, 0.02])
ANGULAR_VELOCITY_NOISE = np.array([0.01, 0.02, 0.05])
JOINT_POSITION_NOISE = 4e-3 * np.ones(6)
JONT_VELOCITY_NOISE = 0.1 * np.ones(6)
PREVIOUS_ACTION_NOISE = 0.0


class NMN(nn.Module):
    """Neural measurement network.

    Based on "Legged Robot State Estimation With Invariant Extended Kalman
    Filter Using Neural Measurement Network", https://arxiv.org/abs/2402.00366.
    """

    def __init__(
        self,
        nb_joints: int,
        hidden_size: int = 128,  # from paper
        mlp_sizes: Tuple[int, int] = (256, 128),  # from paper
    ):
        super(NMN, self).__init__()
        input_size = 3 + 3 + nb_joints + nb_joints + 1
        self.hidden_size = hidden_size
        self.gru = nn.GRU(input_size, hidden_size, batch_first=True)
        self.mlp = nn.Sequential(
            nn.Linear(hidden_size, mlp_sizes[0]),
            nn.ReLU(),  # activation function not specified by the paper
            nn.Linear(mlp_sizes[0], mlp_sizes[1]),
            nn.ReLU(),
        )
        self.contact = nn.Sequential(
            nn.Linear(mlp_sizes[1], 1),
            nn.Sigmoid(),
        )
        self.linear_velocity = nn.Sequential(
            nn.Linear(mlp_sizes[1], 3),
            nn.Softsign(),
        )

    def forward(self, x, h) -> Tuple[torch.Tensor, torch.Tensor]:
        """Forward pass through the network.

        Args:
            x: Input tensor of shape (I,) for unbatched input and (N, I) for
                batched input.
            h: Initial hidden state of the input sequence, of dimension (1, I)
                for unbatched and (1, N, I) for batched input.

        Returns:
            Output tensor and new hidden state.
        """
        if len(x.shape) == 1:
            x = x[None, :]
        latent, h = self.gru(x, h)
        latent = self.mlp(latent)  # (N, L, mlp_sizes[1])
        contact_state = self.contact(latent)
        linear_velocity = self.linear_velocity(latent)
        output = torch.cat((contact_state, linear_velocity), dim=-1)
        return output, h

    @staticmethod
    def get_input(observation: dict, action: NDArray[float]) -> NDArray[float]:
        """Get NMN input vector from an observation dictionary and action.

        Args:
            observation: Full spine observation dictionary.
            action: Previous action.

        Returns:
            NMN input vector.
        """
        imu = observation["imu"]
        servo = observation["servo"]
        linear_acceleration = imu["linear_acceleration"]
        angular_velocity = imu["angular_velocity"]
        joint_positions = np.array(
            [
                servo["left_hip"]["position"],
                servo["left_knee"]["position"],
                servo["left_wheel"]["position"],
                servo["right_hip"]["position"],
                servo["right_knee"]["position"],
                servo["right_wheel"]["position"],
            ]
        )
        joint_velocities = np.array(
            [
                servo["left_hip"]["velocity"],
                servo["left_knee"]["velocity"],
                servo["left_wheel"]["velocity"],
                servo["right_hip"]["velocity"],
                servo["right_knee"]["velocity"],
                servo["right_wheel"]["velocity"],
            ]
        )
        previous_action = action
        clear_input = np.hstack(
            [
                linear_acceleration,
                angular_velocity,
                joint_positions,
                joint_velocities,
                previous_action,
            ],
            dtype=float,
        )
        noise = np.random.normal(
            scale=np.hstack(
                [
                    LINEAR_ACCELERATION_NOISE,
                    ANGULAR_VELOCITY_NOISE,
                    JOINT_POSITION_NOISE,
                    JONT_VELOCITY_NOISE,
                    PREVIOUS_ACTION_NOISE,
                ]
            ),
        )
        return clear_input + noise

    @staticmethod
    def get_contact_state(observation: dict) -> bool:
        """Get contact state from an observation dictionary.

        Args:
            observation: Full spine observation dictionary.

        Returns:
            Contact state boolean.
        """
        contact = observation["sim"]["contact"]
        left_wheel_contact = (
            contact["left_wheel_tire"]["num_contact_points"] > 0
        )
        right_wheel_contact = (
            contact["right_wheel_tire"]["num_contact_points"] > 0
        )
        return left_wheel_contact or right_wheel_contact

    @staticmethod
    def get_linear_velocity(observation: dict) -> NDArray[float]:
        """Get linear velocity of the IMU frame from an observation dictionary.

        Args:
            observation: Full spine observation dictionary.

        Returns:
            Linear velocity of the IMU frame in the IMU frame.
        """
        return observation["sim"]["imu"]["linear_velocity"]
