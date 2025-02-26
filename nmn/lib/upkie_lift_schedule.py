#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: Apache-2.0
# Copyright 2024 Inria

import numpy as np

MIN_LIFT_HEIGHT = 0.1  # [m]
MAX_LIFT_HEIGHT = 0.5  # [m]


class UpkieLiftSchedule:

    def __init__(self, dt: float, episode_len: int):
        self.dt = dt
        self.episode_len = episode_len
        #
        self.reset()

    def reset(self):
        episode_len = self.episode_len
        height = np.random.uniform(MIN_LIFT_HEIGHT, MAX_LIFT_HEIGHT)
        hold_steps = np.random.randint(1, episode_len // 2)
        lift_steps = np.random.randint(1, episode_len // 4)
        start = np.random.randint(-episode_len // 2, episode_len // 2)
        lift_duration = self.dt * lift_steps
        self.delta = 2 * height / (9.81 * lift_duration**2)
        self.start = start
        self.lift_steps = lift_steps
        self.hold_steps = hold_steps

    def get_vertical_force(self, step: int) -> float:
        a = 0.0
        if step < self.start:
            a = 0.0
        elif step - self.start < self.lift_steps // 2:
            a = 1 + self.delta
        elif step - self.start < self.lift_steps:
            a = 1 - self.delta
        elif step - self.start - self.lift_steps < self.hold_steps:
            a = 1.0
        else:
            a = 0.0
        return a * 5.375 * 9.81
