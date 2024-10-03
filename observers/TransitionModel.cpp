// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria
#include "observers/TransitionModel.h"

#include <algorithm>
#include <iostream>
#include <numeric>

#include "Eigen/Core"
#include "kiss_fft/kiss_fft.h"
#include "spdlog/spdlog.h"
#include "upkie/cpp/utils/low_pass_filter.h"

using upkie::cpp::utils::low_pass_filter;

void hann_window(std::vector<kiss_fft_cpx> *in) {
  int N = in->size();
  for (int i = 0; i < N; ++i) {
    double w = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (N - 1)));
    in->at(i).r *= w;
    in->at(i).i *= w;
  }
}

double sigmoid(double x, double offset, double scale, double margin) {
  double sigm_ = 1.0 / (1.0 + std::exp(-(x - offset) / scale));
  return margin + (1.0 - 2.0 * margin) * sigm_;
}

std::vector<double> output_frequencies(double dt, size_t window_size) {
  int N_freqs = window_size / 2;
  double fs = 1.0 / dt;

  std::vector<double> freqs{};

  for (int i = 0; i < N_freqs; ++i) {
    freqs.push_back(i * fs / window_size);
  }
  return freqs;
}

TransitionModel::~TransitionModel() {
  kiss_fft_free(cfg);
  cfg = nullptr;
}

void TransitionModel::read(const Dictionary &observation) {
  // Read the z-component of the linear acceleration.
  double pitch = 0.0;

  // Check if base_orientation is in the observation dictionary.
  auto keys = observation.keys();
  if (std::find(keys.begin(), keys.end(), "base_orientation") != keys.end()) {
    pitch = observation("base_orientation")("pitch");
  }

  double acc_z =
      observation("imu")("linear_acceleration").as<Eigen::Vector3d>()(2);

  acc_z = acc_z * std::cos(pitch);

  double acc_xy_norm = observation("imu")("linear_acceleration")
                           .as<Eigen::Vector3d>()
                           .head<2>()
                           .norm();

  acc_z += acc_xy_norm * std::sin(pitch);

  // Very lightly filter the acceleration.
  // acc_z =
  //    low_pass_filter(acc_buf.back(), 3e-3, acc_z, 1e-3);

  // Rotate the buffer to the left.
  std::rotate(acc_buf.begin(), acc_buf.begin() + 1, acc_buf.end());

  // Add the new data point to the buffer.
  acc_buf[kWindowSize - 1] = acc_z;

  // Compute the mean of the acceleration.
  double mean_acc = 0.0;
  // std::accumulate(acc_buf.begin(), acc_buf.end(), 0.0) / kWindowSize;

  // Subtract the mean from the acceleration, and write it to the input buffer.
  for (int i = 0; i < kWindowSize; ++i) {
    in[i] = kiss_fft_cpx{.r = acc_buf[i] - mean_acc, .i = 0.0};
  }
}

void TransitionModel::write(Dictionary &observation) {
  // Update the model.
  update();

  // Write the mean and median frequencies to the observation dictionary.
  observation(prefix())("mean_frequency") = mean_freq;
  observation(prefix())("median_frequency") = median_freq;
  observation(prefix())("power") = power;
  observation(prefix())("acc_buf") = acc_buf.back();

  // Compute the power times the median frequency.
  // This is just a heuristic that we monitor, and is not used in the transition
  // model.
  observation(prefix())("power_freq") = power * median_freq;

  // Compute transition probabilities
  double p_switch = sigmoid(power, params.switch_offset, params.switch_scale);

  // Filter the median frequency, to have some memory of the previous
  // transitions. Do not include the median frequency if the power is low (i.e.
  // no switch, and the acceleration is mostly noise).
  filtered_median_freq =
      low_pass_filter(filtered_median_freq, 1e-1, median_freq * p_switch, 1e-3);

  observation(prefix())("filtered_median_freq") = filtered_median_freq;

  // Filter the median frequency, to have some memory of the previous
  // transitions. Do not include the median frequency if the power is low (i.e.
  // no switch, and the acceleration is mostly noise).
  filtered_median_freq =
      low_pass_filter(filtered_median_freq, 1e-1, median_freq * p_switch, 1e-3);

  filtered_mean_freq =
      low_pass_filter(filtered_mean_freq, 1e-1, mean_freq * p_switch, 1e-3);

  observation(prefix())("filtered_median_freq") = filtered_median_freq;
  observation(prefix())("filtered_mean_freq") = filtered_mean_freq;

  // Compute the takeoff probability conditioned on the switch probability
  double p_landing = sigmoid(filtered_median_freq, params.landing_offset,
                             params.landing_scale);

  double p_landing_p_switch = p_landing * p_switch;
  double p_takeoff_p_switch = (1.0 - p_landing) * p_switch;

  // Write the probabilities to the observation dictionary.
  observation(prefix())("p_switch") = p_switch;
  observation(prefix())("p_landing") = p_landing;
  observation(prefix())("p_landing_p_switch") = p_landing_p_switch;
  observation(prefix())("p_takeoff_p_switch") = p_takeoff_p_switch;
}

void TransitionModel::update() {
  // Perform the FFT
  kiss_fft(cfg, in.data(), out.data());

  // Compute the mean and median frequencies.
  mean_freq = mean_frequency();
  median_freq = median_frequency();
  power = compute_power();
}

double TransitionModel::mean_frequency() const {
  int N_bins = freqs.size();
  double weight_sum{}, mean_freq{};

  // Compute the magnitude-weigted mean of frequencies
  for (int i = 0; i < N_bins; ++i) {
    double weight = cpx_mag(out.at(i));
    weight_sum += weight;
    mean_freq += weight * freqs.at(i);
  }
  return mean_freq / weight_sum;
}

double TransitionModel::median_frequency() const {
  int N_bins = freqs.size();
  std::vector<double> mags(N_bins);

  // Compute the magnitude of the FFT
  double mag_sum{};
  for (int i = 0; i < N_bins; ++i) {
    double mag = cpx_mag(out.at(i));
    mags[i] = mag;
    mag_sum += mag;
  }

  // Normalize the magnitudes
  for (int i = 0; i < N_bins; ++i) {
    mags[i] /= mag_sum;
  }

  // Cumsum
  for (int i = 1; i < N_bins; ++i) {
    mags[i] += mags[i - 1];
  }

  // Find the median frequency, which is the frequency where the cumulative sum
  // is 0.5. This is found by linear interpolation, as the cumulative sum is not
  // guaranteed to be exactly 0.5 for any frequency bin.
  if (mags[0] >= 0.5) {
    return freqs.at(0);
  }

  for (int i = 1; i < N_bins; ++i) {
    if (mags[i] >= 0.5) {
      double f1 = freqs.at(i - 1);
      double f2 = freqs.at(i);

      double c1 = mags.at(i - 1);
      double c2 = mags.at(i);

      // Linear interpolation
      return f1 + (f2 - f1) * (0.5 - c1) / (c2 - c1);
    }
  }
  return freqs.back();
}

double TransitionModel::compute_power() const {
  double energy = std::accumulate(acc_buf.begin(), acc_buf.end(), 0.0,
                                  [](const double &acc_sum, const double &acc) {
                                    return acc_sum + pow(acc, 2);
                                  });

  return energy / params.window_size;
}

void print_vector(const std::vector<double> &vec, const std::string &name) {
  size_t N = vec.size();

  std::cout << name << std::endl;
  for (size_t i = 0; i < N; ++i) {
    std::cout << "[" << i << "] " << vec[i] << std::endl;
  }
}
