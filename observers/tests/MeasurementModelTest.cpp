// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria
#include "gtest/gtest.h"
#include "observers/MeasurementModel.h"
#include "observers/utils.h"
#include "spdlog/spdlog.h"
#include "tools/cpp/runfiles/runfiles.h"

using bazel::tools::cpp::runfiles::Runfiles;

constexpr double kNearTolerance = 1e-8;

namespace {
class MeasurementModelTest : public testing::Test {
 protected:
  std::unique_ptr<MeasurementModel> measurement_model;

  MeasurementModelTest() {
    MeasurementModel::Parameters params;
    params.argv0 = "observers/tests/MeasurementModelTest";
    params.model_path = "contact_agent/observers/data/measurement_model.npz";
    measurement_model = std::make_unique<MeasurementModel>(params);
  }
};

TEST_F(MeasurementModelTest, TestPrefix) {
  // Check the axis and value keys
  ASSERT_EQ(measurement_model->prefix(), "measurement_model");
}

TEST_F(MeasurementModelTest, TestLikelihoods) {
  auto likelihoods = measurement_model->query_likelihoods({0.0, 0.0});
  ASSERT_NEAR(likelihoods.contact, 1.14514531, kNearTolerance);
  ASSERT_NEAR(likelihoods.no_contact, 120.58327121, kNearTolerance);

  likelihoods = measurement_model->query_likelihoods({0.025, 0.0});
  ASSERT_NEAR(likelihoods.contact, 1.33038333, kNearTolerance);
  ASSERT_NEAR(likelihoods.no_contact, 50.46375233, kNearTolerance);

  likelihoods = measurement_model->query_likelihoods({0.0, 0.025});
  ASSERT_NEAR(likelihoods.contact, 1.887405973, kNearTolerance);
  ASSERT_NEAR(likelihoods.no_contact, 253.86017085, kNearTolerance);

  likelihoods = measurement_model->query_likelihoods({0.3, 0.2});
  ASSERT_NEAR(likelihoods.contact, 0.20144421, kNearTolerance);
  ASSERT_NEAR(likelihoods.no_contact, 0., kNearTolerance);
}
}  // namespace
