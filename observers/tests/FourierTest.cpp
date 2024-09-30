// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria
#include "observers/TransitionModel.h"
#include "gtest/gtest.h"

TEST(FourierTests, FrequencyArray) {
  constexpr double dt = 0.001;
  std::array<double, (kWindowSize / 2)> freqs{output_frequencies(dt)};

  std::cout << "Frequencies: ";
  for (int i = 0; i < freqs.size(); ++i) {
    std::cout << i << " " << freqs[i] << std::endl;
  }
}

TEST(FourierTests, FourierTransform) {
  std::array<double, kWindowSize> in{};
  std::array<std::complex<double>, (kWindowSize / 2)> out{};

  for (int i = 0; i < in.size(); ++i) {
    in[i] = std::sin(2.0 * M_PI * 10.0 * i / kWindowSize);
  }

  discrete_fourier_transform(in, out);

  std::cout << "Fourier transform: ";
  for (int i = 0; i < out.size(); ++i) {
    std::cout << i << " " << out[i] << std::endl;
  }
}
