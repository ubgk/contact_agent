#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import gymnasium as gym
import numpy as np
import upkie.envs
from upkie.utils.raspi import configure_agent_process, on_raspi

upkie.envs.register()


if __name__ == "__main__":
    if on_raspi():
        configure_agent_process()

    with gym.make("UpkieGroundVelocity-v3", frequency=200.0) as env:
        observation, _ = env.reset()  # connects to the spine
        gain = np.array([10.0, 1.0, 0.0, 0.1])
        for step in range(1_000_000):
            action = gain.dot(observation).reshape((1,))
            observation, _, terminated, truncated, _ = env.step(action)
            if terminated or truncated:
                observation, _ = env.reset()
