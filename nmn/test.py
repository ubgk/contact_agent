#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: Apache-2.0
# Copyright 2024 Inria

"""
Test NMN inference on a simple balancing agent.
"""

import os

import gymnasium as gym
import numpy as np
import torch
import upkie.envs

from lib import NMN, SPINE_CONFIG, UpkieLiftSchedule

upkie.envs.register()

LIFT = False
EPISODE_LEN = 100_000
TORCH_DEVICE = "cuda" if torch.cuda.is_available() else "cpu"


if __name__ == "__main__":
    script_dir = os.path.dirname(__file__)
    model = torch.load(
        f"{script_dir}/data/nmn.pt",
        map_location=torch.device("cpu"),
        weights_only=False,
    ).to(TORCH_DEVICE)
    model.eval()
    with gym.make(
        "UpkieGroundVelocity-v3",
        spine_config=SPINE_CONFIG,
        frequency=200.0,
        frequency_checks=False,
    ) as env:
        env.reset()  # connects to the spine at first call
        action = np.zeros(env.action_space.shape)
        hidden_state = None
        bullet_action = {
            "external_forces": {
                "torso": {
                    "force": [0.0, 0.0, 0.0],
                    "local": False,
                }
            }
        }
        torso_force = bullet_action["external_forces"]["torso"]["force"]
        lift = UpkieLiftSchedule(env.unwrapped.dt, EPISODE_LEN)
        for step in range(EPISODE_LEN):
            torso_force[2] = lift.get_vertical_force(step) if LIFT else 0.0
            env.unwrapped.bullet_extra(bullet_action)
            observation, _, terminated, truncated, info = env.step(action)
            if terminated or truncated:
                observation, info = env.reset()
                hidden_state = None
                lift.reset()

            spine_observation = info["spine_observation"]
            m = NMN.get_input(spine_observation, action)
            m = torch.tensor(m, dtype=torch.float32).to(TORCH_DEVICE)
            y, hidden_state = model.forward(m, hidden_state)
            contact_state = y.squeeze()[0].item()
            linear_velocity = y.squeeze()[1:4]
            env.unwrapped.log("nmn_input", m)
            env.unwrapped.log("contact_state", contact_state)
            env.unwrapped.log("linear_velocity", linear_velocity)

            bullet = spine_observation["sim"]
            contact = spine_observation["sim"]["contact"]
            left_wheel_num = contact["left_wheel_tire"]["num_contact_points"]
            right_wheel_num = contact["right_wheel_tire"]["num_contact_points"]
            true_contact_state = left_wheel_num > 0 or right_wheel_num > 0
            env.unwrapped.log("true_contact_state", true_contact_state)
            env.unwrapped.log(
                "true_linear_velocity", bullet["imu"]["linear_velocity"]
            )

            pitch = observation[0]
            ground_position = observation[1]
            ground_velocity = observation[3]
            v = 10.0 * pitch + 1.0 * ground_position + 0.1 * ground_velocity
            action[0] = v  # action is the next commanded ground velocity
