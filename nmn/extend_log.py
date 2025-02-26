#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: Apache-2.0
# Copyright 2024 Inria

"""
This utility extends an existing Upkie log with NMN outputs.
"""

import argparse
import logging
import os

import numpy as np
import torch
from mpacklog import SyncLogger, read_log

from lib import NMN

CURDIR = os.path.dirname(os.path.abspath(__file__))
TORCH_DEVICE = "cuda" if torch.cuda.is_available() else "cpu"
WHEEL_RADIUS = 0.06  # [m]


def parse_command_line_arguments() -> argparse.Namespace:
    """Get command-line argument parser."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log_path", type=str, help="Path to input log file")
    return parser.parse_args()


class NMNProcessor:

    def __init__(self):
        model = torch.load(f"{CURDIR}/data/nmn.pt").to(TORCH_DEVICE)
        model.eval()
        self.hidden_state = None
        self.last_action = np.inf
        self.last_input = None
        self.last_output = None
        self.model = model

    def add_nmn_output(self, unpacked: dict) -> None:
        """Add NMN outputs to logged dictionaries.

        Args:
            unpacked: Input dictionary (will be updated in-place).
        """
        action_dict = unpacked["action"]
        action = action_dict["servo"]["left_wheel"]["velocity"] * WHEEL_RADIUS
        if abs(action - self.last_action) > 1e-4:
            observation = unpacked["observation"]
            m = NMN.get_input(observation, action)
            m = torch.tensor(m, dtype=torch.float32).to(TORCH_DEVICE)
            y, h = self.model.forward(m, self.hidden_state)
            self.hidden_state = h
            self.last_action = action
            self.last_input = m
            self.last_output = y
        unpacked["observation"]["nmn"] = {
            "input_vector": self.last_input,
            "contact_state": self.last_output.squeeze()[0].item(),
            "linear_velocity": self.last_output.squeeze()[1:4],
        }
        return unpacked

    def run(self, input_path: str, output_path: str) -> None:
        logger = SyncLogger(output_path)
        nb_processed = 0
        for unpacked in read_log(args.log_path):
            if nb_processed % 10_000 == 0:
                logging.info("Processed %d inputs so far...", nb_processed)
            logger.put(self.add_nmn_output(unpacked))
            nb_processed += 1
        logger.write()
        logging.info('Extended log written to "%s"', output_path)


if __name__ == "__main__":
    logging.getLogger().setLevel(logging.INFO)
    args = parse_command_line_arguments()
    assert args.log_path.endswith(".mpack"), "Not a valid log file"
    output_path = args.log_path[:-6] + "_nmn.mpack"
    NMNProcessor().run(args.log_path, output_path)
