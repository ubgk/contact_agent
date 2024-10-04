// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria

#define EIGEN_RUNTIME_NO_MALLOC

#include "observers/MeasurementModel.h"

#include <fstream>
#include <iostream>

#include "Eigen/Core"
#include "observers/utils.h"
#include "spdlog/spdlog.h"
#include "tools/cpp/runfiles/runfiles.h"
#include "upkie/cpp/utils/low_pass_filter.h"

using bazel::tools::cpp::runfiles::Runfiles;
using upkie::cpp::utils::low_pass_filter;

MeasurementModel::MeasurementModel(const Parameters &params)
    : leg_name(params.leg_name),
      joint_names(params.joint_names),
      cutoff_periods(params.cutoff_periods),
      filtered_torques(params.joint_names.size()),
      dt(params.dt) {
  if (dt <= 0.0) {
    throw std::invalid_argument("Time step must be strictly positive!");
  }
  // Load the measurement model
  std::string model_path = find_model_path(params.argv0, params.model_path);

  // Load the NpzInterpolator
  interpolator = std::make_unique<NpzInterpolator>(
      /* npz_path = */ model_path,
      /* axis_keys = */ params.axis_keys,
      /* value_keys = */ params.value_keys);
}

void MeasurementModel::read(const Dictionary &observation) {
  // Read the torques from the observation
  for (size_t i = 0; i < joint_names.size(); ++i) {
    auto tau = observation("servo")(leg_name + "_" + joint_names[i])("torque")
                   .as<double>();
    filtered_torques[i] = low_pass_filter(
        /* prev_output = */ filtered_torques[i],
        /* cutoff_period = */ cutoff_periods[i],
        /* new_input = */ tau,
        /* dt = */ dt);
  }

  // Interpolate the contact likelihood
  likelihoods = query_likelihoods(filtered_torques);
}

void MeasurementModel::write(Dictionary &observation) {
  observation(prefix())("contact_likelihood") = likelihoods.contact;
  observation(prefix())("no_contact_likelihood") = likelihoods.no_contact;
  observation(prefix())("p_contact") =
      likelihoods.contact / (likelihoods.contact + likelihoods.no_contact);

  for (size_t i = 0; i < joint_names.size(); ++i) {
    observation(prefix())(leg_name + "_" + joint_names[i])("torque") =
        filtered_torques[i];
  }
}

MeasurementModel::Likelihoods MeasurementModel::query_likelihoods(
    const std::vector<double> &point) const {
  std::vector<double> likelihoods = interpolator->interpolate(point);
  return MeasurementModel::Likelihoods{.contact = likelihoods.at(0),
                                       .no_contact = likelihoods.at(1)};
}
