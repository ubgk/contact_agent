// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria

#pragma once

#include <string>
#include <vector>

#include "btwxt/btwxt.h"
/*
 * Compute the product of a vector of size_t values.
 *
 * \param[in] vec Vector of size_t values.
 * \returns The product of the values in the vector.
 *
 */
inline size_t vec_prod(const std::vector<size_t> &vec) {
  size_t prod = 1;
  for (size_t val : vec) {
    prod *= val;
  }
  return prod;
}

class NpzInterpolator {
 public:
  explicit NpzInterpolator(std::string npz_path,
                           std::vector<std::string> axis_keys,
                           std::vector<std::string> value_keys);

  const std::vector<double> interpolate(const std::vector<double> &point);

  const std::vector<double> operator()(const std::vector<double> &point) {
    return interpolate(point);
  }

  std::string npz_path_;
  std::vector<std::string> axis_keys;
  std::vector<std::string> value_keys;

  std::vector<size_t> axis_sizes;
  std::vector<std::vector<size_t>> value_sizes;

  std::vector<Btwxt::GridAxis> axes;
  std::vector<Btwxt::GridPointDataSet> datasets;

 private:
  Btwxt::RegularGridInterpolator interpolator;
};
