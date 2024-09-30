// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria

#include "observers/NpzInterpolator.h"
#include "observers/utils.h"
#include "spdlog/spdlog.h"
#include "tools/cpp/runfiles/runfiles.h"
#include "gtest/gtest.h"

using bazel::tools::cpp::runfiles::Runfiles;

constexpr double kNearTolerance = 1e-8;

namespace {
class NpzInterpolatorTest : public testing::Test {
protected:
  std::unique_ptr<NpzInterpolator> interpolator;
  std::vector<std::string> axis_keys_;
  std::vector<std::string> value_keys;

  NpzInterpolatorTest() : axis_keys_{"x_axis", "y_axis"}, value_keys{"X", "Y"} {
    std::string model_path = find_model_path(
        "observers/tests/NpzInterpolatorTest",
        "contact_agent/observers/tests/data/coordinate_grid.npz");

    spdlog::debug("NpzInterpolatorTest Model path: {}", model_path);

    // Create the interpolator
    interpolator =
        std::make_unique<NpzInterpolator>(model_path, axis_keys_, value_keys);
  }
};

TEST_F(NpzInterpolatorTest, TestKeys) {
  // Check the axis and value keys
  ASSERT_EQ(axis_keys_, interpolator->axis_keys);
  ASSERT_EQ(value_keys, interpolator->value_keys);
}

TEST_F(NpzInterpolatorTest, TestSizes) {
  // Check that sizes match between axes and values
  size_t expected_num_vals = vec_prod(interpolator->axis_sizes);
  for (const auto &value_size : interpolator->value_sizes) {
    ASSERT_EQ(expected_num_vals, vec_prod(value_size));
  }
  // Check that the number of axes and values match
  ASSERT_EQ(interpolator->axis_keys.size(), interpolator->axes.size());

  // Check that the number of values match
  ASSERT_EQ(interpolator->value_keys.size(), interpolator->datasets.size());

  // Check that the number of axes and values match
  ASSERT_EQ(interpolator->axis_keys.size(), interpolator->axis_sizes.size());

  // Check that the number of values match
  ASSERT_EQ(interpolator->value_keys.size(), interpolator->value_sizes.size());
}

TEST_F(NpzInterpolatorTest, TestInterpolationInGrid) {
  /* The test grid is a 2D grid with 200 points in each axis,
   * with the both axes ranging from 0 to 199.
   * The values are X = x and Y = 100 + y.
   * Example: The interpolated values at (1, 190.7) should be (1, 290.7).
   */
  std::vector<double> point = {1, 190.7};
  std::vector<double> interpolated_values = (*interpolator)({1, 190.7});

  ASSERT_NEAR(interpolated_values[0], 1, kNearTolerance);
  ASSERT_NEAR(interpolated_values[1], 290.7, kNearTolerance);

  // Test the interpolation at the corners
  point = {0, 0};
  interpolated_values = (*interpolator)(point);
  ASSERT_NEAR(interpolated_values[0], 0, kNearTolerance);
  ASSERT_NEAR(interpolated_values[1], 100, kNearTolerance);

  point = {199, 0};
  interpolated_values = (*interpolator)(point);
  ASSERT_NEAR(interpolated_values[0], 199, kNearTolerance);
  ASSERT_NEAR(interpolated_values[1], 100, kNearTolerance);

  point = {0, 199};
  interpolated_values = (*interpolator)(point);
  ASSERT_NEAR(interpolated_values[0], 0, kNearTolerance);
  ASSERT_NEAR(interpolated_values[1], 299, kNearTolerance);

  point = {199, 199};
  interpolated_values = (*interpolator)(point);
  ASSERT_NEAR(interpolated_values[0], 199, kNearTolerance);
  ASSERT_NEAR(interpolated_values[1], 299, kNearTolerance);

  // Test the interpolation at the center
  point = {100, 100};
  interpolated_values = (*interpolator)(point);
  ASSERT_NEAR(interpolated_values[0], 100, kNearTolerance);
  ASSERT_NEAR(interpolated_values[1], 200, kNearTolerance);
}

TEST_F(NpzInterpolatorTest, TestInterpolationOutsideGrid) {
  // Test the interpolation outside the grid
  // Note that the grid is 200x200, so the maximum values are (199, 299)
  // We expect the interpolator to return the values at the closest grid point
  std::vector<double> point = {-100, -100};
  std::vector<double> interpolated_values = (*interpolator)(point);
  ASSERT_NEAR(interpolated_values[0], 0, kNearTolerance);
  ASSERT_NEAR(interpolated_values[1], 100, kNearTolerance);

  // East
  point = {300, 100};
  interpolated_values = (*interpolator)(point);
  ASSERT_NEAR(interpolated_values[0], 199, kNearTolerance);
  ASSERT_NEAR(interpolated_values[1], 200, kNearTolerance);

  // South
  point = {100, -50};
  interpolated_values = (*interpolator)(point);
  ASSERT_NEAR(interpolated_values[0], 100, kNearTolerance);
  ASSERT_NEAR(interpolated_values[1], 100, kNearTolerance);

  // West
  point = {100, 400};
  interpolated_values = (*interpolator)(point);
  ASSERT_NEAR(interpolated_values[0], 100, kNearTolerance);
  ASSERT_NEAR(interpolated_values[1], 299, kNearTolerance);

  // South-West
  point = {-100, -100};
  interpolated_values = (*interpolator)(point);
  ASSERT_NEAR(interpolated_values[0], 0, kNearTolerance);
  ASSERT_NEAR(interpolated_values[1], 100, kNearTolerance);

  // North-East
  point = {250, 400};
  interpolated_values = (*interpolator)(point);
  ASSERT_NEAR(interpolated_values[0], 199, kNearTolerance);
  ASSERT_NEAR(interpolated_values[1], 299, kNearTolerance);
}
} // namespace
