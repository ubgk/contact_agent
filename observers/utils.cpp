// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria

#include "observers/utils.h"

#include <memory>

#include "spdlog/spdlog.h"
#include "tools/cpp/runfiles/runfiles.h"

using bazel::tools::cpp::runfiles::Runfiles;

std::string find_model_path(const std::string &argv0,
                            const std::string &model_name) {
  std::string error;
  std::unique_ptr<Runfiles> runfiles(Runfiles::Create(argv0, &error));

  if (runfiles == nullptr) {
    spdlog::error("Failed to load runfiles: {}", error);
    throw std::runtime_error("Failed to load runfiles: " + error);
  }

  return runfiles->Rlocation(model_name);
}
