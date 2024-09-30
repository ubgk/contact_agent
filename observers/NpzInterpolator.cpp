// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria

#include "observers/NpzInterpolator.h"
#include "cnpy/cnpy.h"
#include "spdlog/spdlog.h"

NpzInterpolator::NpzInterpolator(std::string npz_path,
                                 std::vector<std::string> axis_keys,
                                 std::vector<std::string> value_keys)
    : npz_path_(npz_path), axis_keys(axis_keys), value_keys(value_keys) {
  // Load the npz file
  cnpy::npz_t arrs = cnpy::npz_load(npz_path_);

  spdlog::debug("Loaded {} arrays from \"{}\"", arrs.size(), npz_path_);

  for (const auto &pair : arrs) {
    spdlog::debug("Array name: {}", pair.first);
  }

  // Load the axes
  for (const auto &axis_key : axis_keys) {
    spdlog::debug("Loading requested axis \"{}\"", axis_key);
    cnpy::NpyArray arr = arrs.at(axis_key);

    // We only support arrays with a single dimension, i.e. vectors
    // Singular axes are supported, e.g. shape = (N, 1) or (N, 1, 1) etc.
    if (vec_prod(arr.shape) != arr.shape[0]) {
      throw std::runtime_error("Axis \"" + axis_key +
                               "\" has more than one non-singular dimension. "
                               "Only singular axes are supported.");
    }

    if (arr.shape.size() > 1) {
      spdlog::warn("Axis \"{}\" has shape {} which is not a vector. "
                   "Only the first dimension will be used",
                   axis_key, arr.shape.size());
    }

    double *ptr = arr.data<double>();
    size_t num_vals = arr.num_vals;
    std::vector<double> axis(ptr, ptr + num_vals);
    axes.emplace_back(
        /* values = */ axis,
        /* interpolation_method = */ Btwxt::InterpolationMethod::linear,
        /* extrapolation_method = */ Btwxt::ExtrapolationMethod::constant);
    axis_sizes.push_back(num_vals);
  }

  // Load the values
  for (const auto &value_key : value_keys) {
    cnpy::NpyArray arr = arrs.at(value_key);

    spdlog::debug("Loading value key: \"{}\"", value_key);

    if (vec_prod(arr.shape) == 0) {
      spdlog::error("Value key \"{}\" has no data", value_key);
    } else if (vec_prod(arr.shape) != vec_prod(axis_sizes)) {
      throw std::runtime_error(
          "Value key \"" + value_key +
          "\" has shape which does not match the axes' dimensions");
    }

    assert(vec_prod(arr.shape) == vec_prod(axis_sizes));

    std::vector<size_t> shape = arr.shape;
    size_t num_vals = arr.num_vals;
    std::vector<double> data(arr.data<double>(), arr.data<double>() + num_vals);

    datasets.emplace_back(data, value_key);
    value_sizes.push_back(shape);
  }

  // Create the interpolator
  interpolator = Btwxt::RegularGridInterpolator(axes, datasets);
}

const std::vector<double>
NpzInterpolator::interpolate(const std::vector<double> &point) {
  return interpolator.get_values_at_target(point);
}
