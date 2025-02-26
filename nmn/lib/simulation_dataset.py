#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: Apache-2.0
# Copyright 2024 Inria

import logging
from typing import Generator, Optional, Tuple

import gymnasium as gym
import numpy as np
import torch
from upkie.utils.robot_state import RobotState, RobotStateRandomization

from .nmn import NMN
from .spine_config import SPINE_CONFIG
from .upkie_lift_schedule import UpkieLiftSchedule

MAX_EPISODE_STEPS = 2_000  # 2000 * 0.005 = 10 seconds
TORCH_DEVICE = "cuda" if torch.cuda.is_available() else "cpu"


class SimulationDataset(torch.utils.data.Dataset):
    def __init__(
        self,
        inputs: Optional[torch.Tensor] = None,
        targets: Optional[torch.Tensor] = None,
    ):
        """Generate a new simulation dataset."""
        self.bullet_action = {
            "external_forces": {
                "torso": {
                    "force": [0.0, 0.0, 0.0],
                    "local": False,
                }
            }
        }
        self.inputs = inputs
        self.targets = targets

    def __len__(self) -> int:
        """Size of the dataset.

        Returns:
            Size of the dataset.
        """
        if self.inputs is None:
            return 0
        return len(self.inputs)

    def __getitem__(self, idx: int) -> Tuple[torch.Tensor, torch.Tensor]:
        x = self.inputs[idx]
        y = self.targets[idx]
        return x, y

    def __rollout(self, nb_steps: int):
        rollout = []
        torso_force = self.bullet_action["external_forces"]["torso"]["force"]
        with gym.make(
            "UpkieGroundVelocity-v3",
            spine_config=SPINE_CONFIG,
            frequency=200.0,
            frequency_checks=False,
            init_state=RobotState(
                position_base_in_world=np.array([0.0, 0.0, 0.7]),
                randomization=RobotStateRandomization(
                    z=0.3,
                    linear_velocity=np.array([0.3, 0.1, 0.0]),
                ),
            ),
        ) as env:
            env.reset()  # connects to the spine at first call
            action = np.zeros(env.action_space.shape)
            reporting_steps = nb_steps // 10
            episode_step = 0
            lift = UpkieLiftSchedule(env.unwrapped.dt, MAX_EPISODE_STEPS)
            for total_step in range(nb_steps):
                if total_step % reporting_steps == 0:
                    logging.info(
                        "Now at %d / %d simulation steps",
                        total_step,
                        nb_steps,
                    )
                torso_force[2] = lift.get_vertical_force(episode_step)
                env.unwrapped.bullet_extra(self.bullet_action)
                observation, _, terminated, truncated, info = env.step(action)
                if terminated or truncated or episode_step > MAX_EPISODE_STEPS:
                    observation, info = env.reset()
                    episode_step = 0
                    lift.reset()

                spine_observation = info["spine_observation"]
                inputs = NMN.get_input(spine_observation, action)
                contact_state = NMN.get_contact_state(spine_observation)
                linear_velocity = NMN.get_linear_velocity(spine_observation)
                targets = np.hstack([contact_state, linear_velocity])
                rollout.append((inputs, targets))

                pitch = observation[0]
                action[0] = 10.0 * pitch  # 1D action: [ground_velocity]
                episode_step += 1
        inputs, targets = list(zip(*rollout))
        inputs = torch.tensor(np.array(inputs), dtype=torch.float32)
        targets = torch.tensor(np.array(targets), dtype=torch.float32)
        return inputs, targets

    def extend(self, nb_steps: int):
        """Extend dataset with new simulation steps.

        Args:
            nb_steps: Number of simulation steps.
        """
        new_inputs, new_targets = self.__rollout(nb_steps)
        if self.inputs is None:
            self.inputs = new_inputs
            self.targets = new_targets
        else:  # self.inputs is not None
            self.inputs = torch.cat([self.inputs, new_inputs])
            self.targets = torch.cat([self.targets, new_targets])

    def batch(
        self, batch_size: int, seq_len: int
    ) -> Generator[Tuple[torch.Tensor, torch.Tensor], None, None]:
        """Get batches from the dataset.

        Args:
            batch_size: Number of parallel batch trajectories.
            seq_len: Length of input and target sequences.

        Returns:
            Batches generator, a batch being a pair (x, y) of inputs and
            targets. Inputs have shape (batch_size, seq_len, input_size) while
            targets have shape (batch_size, seq_len, output_size).

        Note:
            The number of generated batches depends on the size of the dataset.
        """
        batch_length = len(self) // batch_size
        starts = [k * batch_length for k in range(batch_size)]
        assert batch_length >= seq_len
        X, Y = self.inputs, self.targets
        for t in range(0, batch_length - seq_len + 1, seq_len):
            x = torch.stack([X[s + t : s + t + seq_len] for s in starts])
            y = torch.stack([Y[s + t : s + t + seq_len] for s in starts])
            x = x.to(TORCH_DEVICE)
            y = y.to(TORCH_DEVICE)
            yield x, y

    def split(
        self, nb_splits: int
    ) -> Generator["SimulationDataset", None, None]:
        """Split the dataset into consecutive subsets.

        Args:
            nb_splits: Number of chunks to split the dataset into.

        Returns:
        """
        split_length = len(self) // nb_splits
        for split_index in range(nb_splits):
            split_slice = slice(
                split_index * split_length,
                (split_index + 1) * split_length,
            )
            yield SimulationDataset(
                inputs=self.inputs[split_slice],
                targets=self.targets[split_slice],
            )
