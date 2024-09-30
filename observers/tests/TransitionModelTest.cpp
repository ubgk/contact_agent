// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria
#include <stdlib.h>

#include <numeric>

#include "gtest/gtest.h"
#include "observers/TransitionModel.h"
#include "palimpsest/Dictionary.h"

constexpr double kNearTolerance = 1e-10;

namespace {
class TransitionModelTest : public testing::Test {
 protected:
  TransitionModel::Parameters params{.dt = 0.001, .window_size = 128};
  TransitionModel transition_model;

  TransitionModelTest() : transition_model(TransitionModel(params)) {}
};

TEST_F(TransitionModelTest, TestPrefix) {
  // Check the axis and value keys
  ASSERT_EQ(transition_model.prefix(), "transition_model");
}

TEST_F(TransitionModelTest, TestRead) {
  palimpsest::Dictionary observation;

  observation("imu")("linear_acceleration") = Eigen::Vector3d(0.0, 0.0, 1.0);
  transition_model.read(observation);

  // Check the internal state of the transition model
  ASSERT_EQ(transition_model.in[params.window_size - 1].r, 1.0);
  ASSERT_EQ(transition_model.in[params.window_size - 1].i, 0.0);

  observation("imu")("linear_acceleration") = Eigen::Vector3d(1.0, 2.0, 3.0);
  transition_model.read(observation);

  // Check the internal state of the transition model
  ASSERT_EQ(transition_model.in[params.window_size - 1].r, 3.0);
  ASSERT_EQ(transition_model.in[params.window_size - 1].i, 0.0);
  ASSERT_EQ(transition_model.in[params.window_size - 2].r, 1.0);
  ASSERT_EQ(transition_model.in[params.window_size - 2].i, 0.0);

  // Check the input buffer is zeroed out for the first params.window_size - 2
  // elements
  for (size_t i = 0; i < params.window_size - 2; ++i) {
    ASSERT_EQ(transition_model.in[i].r, 0.0);
    ASSERT_EQ(transition_model.in[i].i, 0.0);
  }

  // Check the output buffer is zeroed out for all elements
  for (size_t i = 0; i < params.window_size; ++i) {
    ASSERT_EQ(transition_model.out[i].r, 0.0);
    ASSERT_EQ(transition_model.out[i].i, 0.0);
  }
}

TEST_F(TransitionModelTest, CheckOutputFrequencies) {
  std::vector<double> freqs = output_frequencies(params.dt, params.window_size);
  double nyquist_freq = 1.0 / (2 * params.dt);
  double last_freq = nyquist_freq - 2 * (nyquist_freq) / params.window_size;

  ASSERT_NEAR(freqs[0], 0.0, kNearTolerance);
  ASSERT_NEAR(freqs.back(), last_freq, kNearTolerance);
  ASSERT_EQ(freqs.size(), 64);

  // Check that the frequencies are in increasing order and have a constant
  // difference
  const double df = freqs[1] - freqs[0];
  for (size_t i = 1; i < params.window_size / 2; ++i) {
    ASSERT_EQ(freqs[i] - freqs[i - 1], df);
  }
}

TEST_F(TransitionModelTest, CheckRandomFFT) {
  palimpsest::Dictionary observation;
  std::vector<double> random_data(params.window_size);
  unsigned int seed = 42;
  // Populate the random data
  for (size_t i = 0; i < params.window_size; ++i) {
    random_data[i] =
        (static_cast<double>(rand_r(&seed)) / RAND_MAX) * 20.0 - 10.0;
  }

  observation("imu")("linear_acceleration") = Eigen::Vector3d(0.0, 0.0, 0.0);
  double *p = &observation("imu")("linear_acceleration")
                   .as<Eigen::Vector3d>()
                   .data()[2];

  auto start_time = std::chrono::high_resolution_clock::now();
  // Read the random data into the observation buffer
  for (size_t i = 0; i < params.window_size; ++i) {
    // observation("imu")("linear_acceleration").as<Eigen::Vector3d>().data()[2]
    // = random_data[i];

    // Using a pointer instead to bypass Palimpsest
    *p = random_data[i];
    transition_model.read(observation);

    // Check that the data was read
    ASSERT_EQ(transition_model.in.back().r, *p);
  }
  auto elapsed_time = std::chrono::high_resolution_clock::now() - start_time;
  std::cout << "Read time microsecs: "
            << std::chrono::duration_cast<std::chrono::microseconds>(
                   elapsed_time)
                   .count()
            << std::endl;

  start_time = std::chrono::high_resolution_clock::now();
  // Perform the discrete Fourier transform
  transition_model.update();
  elapsed_time = std::chrono::high_resolution_clock::now() - start_time;
  std::cout << "Update time microsecs: "
            << std::chrono::duration_cast<std::chrono::microseconds>(
                   elapsed_time)
                   .count()
            << std::endl;

  // Check against naive FFT
  std::vector<std::complex<double>> dft_out(params.window_size);

  start_time = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < params.window_size; ++i) {
    for (size_t j = 0; j < params.window_size; ++j) {
      double angle = 2 * M_PI * i * j / params.window_size;
      dft_out[i] += std::complex(random_data[j], 0.0) *
                    std::complex(cos(angle), -sin(angle));
    }
  }
  elapsed_time = std::chrono::high_resolution_clock::now() - start_time;
  std::cout << "Naive DFT time microsecs: "
            << std::chrono::duration_cast<std::chrono::microseconds>(
                   elapsed_time)
                   .count()
            << std::endl;

  // Check the FFT output against the naive DFT
  for (size_t i = 0; i < params.window_size; ++i) {
    ASSERT_NEAR(transition_model.out[i].r, dft_out[i].real(), kNearTolerance);
    ASSERT_NEAR(transition_model.out[i].i, dft_out[i].imag(), kNearTolerance);
  }

  double energy_time{}, energy_dft{}, energy_fft{};

  // Compute the energy in the time domain
  for (size_t i = 0; i < params.window_size; ++i) {
    energy_time += random_data[i] * random_data[i];

    energy_dft += dft_out[i].real() * dft_out[i].real() +
                  dft_out[i].imag() * dft_out[i].imag();

    energy_fft += pow(cpx_mag(transition_model.out[i]), 2);
  }

  // Normalize the energy by the window size
  energy_dft /= params.window_size;
  energy_fft /= params.window_size;

  const double reference_energy =
      transition_model.compute_power() * transition_model.params.window_size;

  // Check the energy in time and frequency domains
  ASSERT_NEAR(reference_energy, energy_time, kNearTolerance);
  ASSERT_NEAR(reference_energy, energy_fft, kNearTolerance);
  ASSERT_NEAR(reference_energy, energy_dft, kNearTolerance);
}

TEST_F(TransitionModelTest, TestFrequencySanity) {
  // Pick a random frequency from freqs, excluding the DC component
  unsigned int seed = 42;
  int random_index = (rand_r(&seed) % (transition_model.freqs.size() - 1)) + 1;
  double random_freq = transition_model.freqs.at(random_index);

  // Set the magnitude of the output at the random frequency
  transition_model.out.at(random_index) = kiss_fft_cpx{.r = 1.0, .i = 0.0};

  // Compute the mean and median frequencies
  double mean_freq = transition_model.mean_frequency();
  double median_freq = transition_model.median_frequency();
  double expected_median_freq = (transition_model.freqs.at(random_index) +
                                 transition_model.freqs.at(random_index - 1)) /
                                2;

  // Check the computed frequencies
  ASSERT_NEAR(mean_freq, random_freq, kNearTolerance);
  ASSERT_NEAR(median_freq, expected_median_freq, kNearTolerance);
}

TEST_F(TransitionModelTest, ReadWriteTest) {
  palimpsest::Dictionary observation;

  // We need integer number of periods in the window to
  // prevent spectral leakage
  const size_t kNumPeriods = 10;
  const double target_freq = kNumPeriods / (params.dt * params.window_size);
  const double df = 1.0 / (params.dt * params.window_size);

  // Produce a sine wave at target_freq Hz
  std::vector<double> sine_wave(params.window_size);

  for (size_t i = 0; i < params.window_size; ++i) {
    // target_freq Hz sine wave
    sine_wave[i] = sin(2 * M_PI * target_freq * i * params.dt);
  }

  // Read the sine wave into the observation buffer
  for (size_t i = 0; i < params.window_size; ++i) {
    observation("imu")("linear_acceleration") =
        Eigen::Vector3d(0.0, 0.0, sine_wave[i]);
    transition_model.read(observation);
  }

  // Write the model output to the observation
  transition_model.write(observation);

  // Check the mean and median frequencies
  ASSERT_NEAR(observation("transition_model")("mean_frequency").as<double>(),
              target_freq, kNearTolerance);
  ASSERT_NEAR(observation("transition_model")("median_frequency").as<double>(),
              target_freq - df / 2, kNearTolerance);

  // Check the power
  // Integral of sin^2(x) from 0 to 2pi is pi.
  // Integral of sin^2(x) from 0 to 2pi * kNumPeriods is kNumPeriods * pi
  // If we normalize the integral (kNumPeriods * pi) by the window length (2pi *
  // kNumPeriods), we get 1/2. This will hold true for any integer kNumPeriods.
  double expected_power = 0.5;

  ASSERT_NEAR(observation("transition_model")("power").as<double>(),
              expected_power, kNearTolerance);
}
}  // namespace
