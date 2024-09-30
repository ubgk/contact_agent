// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria
#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "mpacklog/Logger.h"
#include "palimpsest/Dictionary.h"
#include "upkie/cpp/observers/Observer.h"

using upkie::cpp::observers::Observer;

struct MemoryMappedFile {
  int fildes;
  void *mmap_addr;
  struct stat sb;

  explicit MemoryMappedFile(const std::filesystem::path &path, bool read_only);
  ~MemoryMappedFile();
};

class Replay {
public:
  struct Parameters {
    explicit Parameters(const std::string &input_path,
                        const std::string &output_path,
                        const std::string &argv0)
        : input_path(input_path), output_path(output_path), argv0(argv0) {}

    //! Path to the replay file.
    std::filesystem::path input_path;
    std::filesystem::path output_path;
    std::filesystem::path argv0;
  };

  palimpsest::Dictionary current_dictionary;

  explicit Replay(const Parameters &parameters);

  void process();

  std::filesystem::path input_path;
  std::filesystem::path output_path;

  //! Vector of observers, a polymorphic container.
  std::vector<std::shared_ptr<Observer>> observers;

  //! Input file
  std::shared_ptr<MemoryMappedFile> input_file;

  //! Output file
  std::unique_ptr<mpacklog::Logger> logger;
};
