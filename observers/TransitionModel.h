// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria

#pragma once

#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "kiss_fft/kiss_fft.h"
#include "palimpsest/Dictionary.h"
#include "upkie/cpp/observers/Observer.h"

using palimpsest::Dictionary;
using upkie::cpp::observers::Observer;

inline constexpr int kWindowSize = 128;

// Apply a Hann window to the input signal in place.
void hann_window(std::vector<kiss_fft_cpx> *in);

// Computes the magnitude of a complex value
inline double cpx_mag(const kiss_fft_cpx &cpx) {
  return sqrt(cpx.r * cpx.r + cpx.i * cpx.i);
}

/*!
 * Compute the sigmoid function: `1 / (1 + exp(-(x - offset) / scale))`.
 * \param[in] x Input value.
 * \param[in] offset Offset of the sigmoid function. Default is 0.0.
 * \param[in] scale Scale of the sigmoid function. Default is 1.0.
 * \param[in] margin Margin of the sigmoid function. Default is 1e-6, which
 * ensures that the output is in the range `[margin, 1 - margin]`.
 * \return Sigmoid function value.
 */
double sigmoid(double x, double offset, double scale, double margin = 1e-10);

/*! Compute the output frequencies given a time step.
 * \param[in] dt Time step.
 * \return Vector of output frequencies. The vector has `N//2` elements, where
 * `N` is the size of the input signal.
 */
std::vector<double> output_frequencies(double dt,
                                       size_t window_size = kWindowSize);

//! Observe contact between the wheels and the floor.
class TransitionModel : public Observer {
public:
  struct Parameters {
    //! Time step between observations
    double dt = 0.001;
    size_t window_size = kWindowSize;

    // Sigmoid parameters for the transition model
    double switch_offset = 50.0;
    double switch_scale = 5.0;
    double landing_offset = 8.0;
    double landing_scale = 3;
  };

  /*! Initialize observer.
   *
   */
  explicit TransitionModel(const Parameters &params)
      : params(params), acc_buf(params.window_size, 0.0),
        in(params.window_size, kiss_fft_cpx{0.0, 0.0}),
        out(params.window_size, kiss_fft_cpx{0.0, 0.0}),
        filtered_median_freq(0.0), filtered_mean_freq(0.0),
        freqs(output_frequencies(params.dt)), sampling_freq(1 / params.dt),
        nyquist_freq(sampling_freq / 2),
        cfg(kiss_fft_alloc(params.window_size, false, nullptr, nullptr)) {}

  // Destructor
  ~TransitionModel() override;

  //! Prefix of outputs in the observation dictionary.
  inline std::string prefix() const noexcept final {
    return "transition_model";
  }

  /*! Read inputs from other observations.
   * \param[in] observation Dictionary to read other observations from.
   */
  void read(const Dictionary &observation) override;

  /*! Write outputs, called if reading was successful.
   *
   * \param[out] observation Dictionary to write observations to.
   */
  void write(Dictionary &observation) override;

  //! Update the internal state of the observer, e.g. perform an FFT.
  void update();

  //! Compute the mean frequency
  double mean_frequency() const;

  //! Compute the median frequency
  double median_frequency() const;

  //! Compute the power of the signal
  double compute_power() const;

  // private:

  //! Parameters of the model
  const Parameters params{};

  //! Accelerometer buffer
  std::vector<double> acc_buf{};

  //! FFT input buffer
  std::vector<kiss_fft_cpx> in{};

  //! FFT output buffer
  std::vector<kiss_fft_cpx> out{};

  //! Mean, median, and power of the FFT
  double mean_freq{};
  double median_freq{};
  double power{};

  //! Filter the median frequency
  double filtered_median_freq{};

  //! Filter the mean frequency
  double filtered_mean_freq{};

  //! Output frequencies given dt = 0.001
  const std::vector<double> freqs{};

  //! Sampling frequency
  const double sampling_freq{};

  //! Nyquist-Shannon frequency
  const double nyquist_freq{};

  //! FFT configuration
  kiss_fft_cfg cfg;
};

void print_vector(const std::vector<double> &vec, const std::string &name);
