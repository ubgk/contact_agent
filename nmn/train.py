#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: Apache-2.0
# Copyright 2024 Inria

"""
Train a new NMN from training and validation sets saved in data/.
"""

import logging
import os
from typing import List, Tuple

import pylab
import torch
import upkie.envs
from torch.nn import functional as F

from lib import NMN

upkie.envs.register()

TORCH_DEVICE = "cuda" if torch.cuda.is_available() else "cpu"

# From paper
NB_ITER = 20  # "during the first 200 iterations"
BATCH_SIZE = 400  # "all 400 environments"
EPISODE_LEN = 400  # "a trajectory of 400 control steps"
NB_EPOCHS = 32  # "and 32 epochs"
ADAM_LEARNING_RATE = 5e-4  # "with a learning rate of 5e-4"

SEQ_LEN = 10


def nmn_loss(nmn_output, target):
    contact_state = nmn_output[:, :, 0:1]
    contact_state_target = target[:, :, 0:1]
    linear_velocity = nmn_output[:, :, 1:4]
    linear_velocity_target = target[:, :, 1:4]
    contact_loss = F.binary_cross_entropy(contact_state, contact_state_target)
    velocity_loss = F.l1_loss(linear_velocity, linear_velocity_target)
    return contact_loss + velocity_loss


@torch.no_grad()
def estimate_loss(model, data, nb_iters: int = 1):
    model.eval()
    total_loss = 0.0
    total_batches = 0
    for _ in range(nb_iters):
        hidden_state = None
        for batch in data.batch(BATCH_SIZE, SEQ_LEN):
            inputs, targets = batch
            outputs, hidden_state = model(inputs, hidden_state)
            loss = nmn_loss(outputs, targets)
            total_loss += loss.item()
            total_batches += 1
    model.train()
    return total_loss / total_batches


def plot_losses(losses: List[Tuple[int, float, float]]):
    xrange, y1, y2 = list(zip(*losses))
    pylab.plot(xrange, y1)
    pylab.plot(xrange, y2)
    pylab.legend(("Training loss", "Validation loss"))
    pylab.xlabel("Epoch")
    pylab.show()


def train_new_model(training_data, validation_data):
    model = NMN(nb_joints=6).to(TORCH_DEVICE)
    model.train()
    optimizer = torch.optim.Adam(model.parameters(), lr=ADAM_LEARNING_RATE)
    losses = []
    if len(training_data) < NB_ITER * BATCH_SIZE * EPISODE_LEN:
        raise ValueError(
            "Dataset too small: we want at least NB_ITER * "
            "BATCH_SIZE * EPISODE_LEN = "
            f"{NB_ITER * BATCH_SIZE * EPISODE_LEN} samples"
        )
    for it, training_split in enumerate(training_data.split(NB_ITER)):
        validation_loss = estimate_loss(model, validation_data)
        logging.info(
            "iteration=%d: len(training_split)=%d validation_loss=%.4f",
            it,
            len(training_split),
            validation_loss,
        )
        for epoch in range(NB_EPOCHS):
            total_loss = 0.0
            nb_steps = 0
            hidden_state = None
            for step in training_split.batch(BATCH_SIZE, SEQ_LEN):
                inputs, targets = step
                outputs, hidden_state = model(inputs, hidden_state)
                hidden_state = hidden_state.detach().clone()
                loss = nmn_loss(outputs, targets)
                optimizer.zero_grad(set_to_none=True)
                loss.backward()
                optimizer.step()
                total_loss += loss.item()
                nb_steps += 1
            training_loss = total_loss / nb_steps
            losses.append(
                (it * NB_EPOCHS + epoch, training_loss, validation_loss)
            )
            if epoch % 10 == 0:
                logging.info(
                    "- epoch=%2d: nb_steps=%d, training_loss=%.4f",
                    epoch,
                    nb_steps,
                    training_loss,
                )

    return model, losses


if __name__ == "__main__":
    script_dir = os.path.dirname(__file__)
    training_data = torch.load(f"{script_dir}/data/training.pt", weights_only=False)
    validation_data = torch.load(f"{script_dir}/data/validation.pt", weights_only=False)
    print(f"Size of training dataset: {len(training_data)}")
    print(f"Size of validation dataset: {len(validation_data)}")
    model, losses = train_new_model(training_data, validation_data)
    plot_losses(losses)
    torch.save(model, f"{script_dir}/data/nmn.pt")
    print('New estimator saved to "data/nmn.pt"')
