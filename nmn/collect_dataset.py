#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: Apache-2.0
# Copyright 2024 Inria

"""
This script regenerates the training and test sets in data/.
"""

import argparse
import os

import torch
import upkie.envs
from lib import SimulationDataset

upkie.envs.register()
script_dir = os.path.dirname(__file__)


def parse_command_line_arguments() -> argparse.Namespace:
    """Get command-line argument parser."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "nb_steps", type=int, help="Number of training steps to add"
    )
    return parser.parse_args()


def load_dataset():
    try:
        os.mkdir(f"{script_dir}/data")
    except FileExistsError:
        pass
    data = {}
    for key in ("training", "validation"):
        try:
            data[key] = torch.load(f"{script_dir}/data/{key}.pt")
        except Exception:
            data[key] = SimulationDataset()
        print(f'Loaded {key} dataset from "{script_dir}/data/{key}.pt"')
        print(f"- Size of {key} dataset: {len(data[key])}")
    return data


def save_dataset(data):
    for key, subset in data.items():
        torch.save(subset, f"{script_dir}/data/{key}.pt")
        print(f'New {key} dataset saved to "{script_dir}/data/{key}.pt"')
        print(f"- Size of {key} dataset: {len(subset)}")


if __name__ == "__main__":
    args = parse_command_line_arguments()
    data = load_dataset()
    data["training"].extend(nb_steps=args.nb_steps)
    data["validation"].extend(nb_steps=args.nb_steps // 10)
    save_dataset(data)
