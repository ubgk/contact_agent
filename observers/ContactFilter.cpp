// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria
#define EIGEN_RUNTIME_NO_MALLOC

#include "observers/ContactFilter.h"

#include <iostream>

#include "upkie/cpp/utils/low_pass_filter.h"

using upkie::cpp::utils::low_pass_filter;

void ContactFilter::read(const Dictionary &observation) {
  double contact_likelihood =
      observation("measurement_model")("contact_likelihood");
  double no_contact_likelihood =
      observation("measurement_model")("no_contact_likelihood");

  // Add a small constant to avoid division by zero.
  // We would only encounter zero if torques are outside the range of the
  // KDE, which means they are abnormally large.
  contact_likelihood += 1e-20;

  spdlog::debug("p_contact: " + std::to_string(p_contact));

  double knee_torque = observation("measurement_model")("left_knee")("torque");
  double wheel_torque =
      observation("measurement_model")("left_wheel")("torque");

  if (std::isnan(contact_likelihood)) {
    spdlog::error("contact_likelihood is NaN!");
  } else if (std::isnan(no_contact_likelihood)) {
    spdlog::error("no_contact_likelihood is NaN!");
  }

  double p_switch = observation("transition_model")("p_switch");
  double p_landing =
      observation("transition_model")("p_landing"); // CONDITIONED ON switch!
  double power = observation("transition_model")("power");

  spdlog::debug("p_switch: " + std::to_string(p_switch) + " p_landing: " +
                std::to_string(p_landing) + " power: " + std::to_string(power));

  double contact_belief = p_contact;
  double no_contact_belief = 1.0 - p_contact;

  spdlog::debug("contact_belief: " + std::to_string(contact_belief) +
                " no_contact_belief: " + std::to_string(no_contact_belief));

  // Equation 1a
  double tmp_contact_belief = (p_switch * p_landing) * no_contact_belief +
                              (1 - p_switch) * contact_belief;

  no_contact_belief = (p_switch * (1 - p_landing)) * contact_belief +
                      (1 - p_switch) * no_contact_belief;

  contact_belief = tmp_contact_belief;

  spdlog::debug("contact_belief: " + std::to_string(contact_belief) +
                " no_contact_belief: " + std::to_string(no_contact_belief));

  // Equation 1b
  contact_belief = contact_likelihood * contact_belief;
  no_contact_belief = no_contact_likelihood * no_contact_belief;

  // Equation 2
  double norm_term = contact_belief + no_contact_belief;
  contact_belief = contact_belief / norm_term;

  if (!std::isnan(contact_belief)) {
    spdlog::debug("updating the contact belief!");
    p_contact = contact_belief;
  } else {
    spdlog::error("contact_belief was NaN, p_contact was not updated!");
    exit(1);
  }
  // Apply a very gentle low-pass filter to the contact belief, to smooth it
  // out.
  p_contact_smooth = low_pass_filter(p_contact_smooth, 1e-2, p_contact, 1e-3);

}

void ContactFilter::write(Dictionary &observation) {
