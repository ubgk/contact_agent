// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria

#include "observers/Replay.h"

#include <sys/mman.h>
#include <sys/types.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "mpack.h"
#include "mpacklog/Logger.h"
#include "observers/ContactFilter.h"
#include "observers/MeasurementModel.h"
#include "observers/TransitionModel.h"
#include "palimpsest/Dictionary.h"

MemoryMappedFile::MemoryMappedFile(const std::filesystem::path &path,
                                   bool read_only) {
  int flags = read_only ? O_RDONLY : O_RDWR;
  fildes = open(path.c_str(), flags);

  if (fildes == -1) {
    spdlog::error("Failed to open input file: {}", path.string());
    exit(1);
    return;
  }

  if (fstat(fildes, &sb) == -1) {
    spdlog::error("Failed to stat input file: {}", path.string());
    exit(1);
    return;
  }

  int prot = read_only ? PROT_READ : (PROT_READ | PROT_WRITE);
  int map = read_only ? MAP_PRIVATE : MAP_SHARED;
  mmap_addr = mmap(NULL, sb.st_size, prot, map, fildes, 0);

  if (mmap_addr == MAP_FAILED) {
    spdlog::error("Failed to mmap input file: {} error: {}", path.string(),
                  errno);
    exit(1);
    return;
  }
}

MemoryMappedFile::~MemoryMappedFile() {
  spdlog::info("Closing file descriptor and unmapping memory");
  munmap(mmap_addr, sb.st_size);
  close(fildes);
}

static bool validate_path(const std::filesystem::path &path,
                          bool check_exists = true) {
  if (!std::filesystem::exists(path) && check_exists) {
    spdlog::error("Path does not exist: '{}'", path.string());
    return false;
  }
  if (std::filesystem::is_directory(path)) {
    spdlog::error("Path is a directory: '{}'", path.string());
    return false;
  }
  return true;
}

//! Command-line arguments for the mock spine.
class CommandLineArguments {
public:
  /*! Read command line arguments.
   *
   * \param[in] args List of command-line arguments.
   */
  explicit CommandLineArguments(const std::vector<std::string> &args) {
    for (size_t i = 1; i < args.size(); i++) {
      const auto &arg = args[i];
      if (arg == "-h" || arg == "--help") {
        help = true;
      } else if (arg == "") {
        error = true;
        spdlog::error("Path cannot be empty!");
      } else if (i == 1) {
        input_path = args.at(i);
        spdlog::info("Command line: input_path = {}", args.at(i));
      } else if (i == 2) {
        output_path = args.at(i);
        spdlog::info("Command line: output_path = {}", args.at(i));
      } else {
        spdlog::error("Unknown argument: {}", arg);
        error = true;
      }
    }

    if (args.size() <= 1) {
      spdlog::error("No arguments were provided!");
      error = true;
    }

    if (input_path == output_path && !help) {
      spdlog::error("Input and output paths cannot be the same!");
      error = true;
    }

    if (!input_path.empty() && output_path.empty()) {
      output_path = input_path;
      output_path.replace_extension(".contact.mpack");
      spdlog::info("Output path not provided, writing to {}",
                   output_path.c_str());
    }

    if (help) {
      print_usage(args[0].c_str());
      exit(0);
    } else if (error) {
      print_usage(args[0].c_str());
      spdlog::error("Error parsing command line arguments!");
      exit(1);
    }
  }

  /*! Show help message
   *
   * \param[in] name Binary name from argv[0].
   */
  inline void print_usage(const char *name) noexcept {
    std::cout << "Usage: " << name << " <input-path> <output-path>"
              << " [options]\n\n";
    std::cout << "Required arguments:\n\n";
    std::cout << "<input-path>\n"
              << "    Path to the input file.\n";
    std::cout << "<output-path>\n"
              << "    Path to the output file.\n";
    std::cout << "\n";
    std::cout << "Optional arguments:\n\n";
    std::cout << "-h, --help\n"
              << "    Print this help and exit.\n";
    std::cout << "\n";
  }

public:
  //! Error flag
  bool error = false;

  //! Help flag
  bool help = false;

  //! Log path
  std::filesystem::path input_path;

  //! Output path
  std::filesystem::path output_path;

  //! Version flag
  bool version = false;
};

Replay::Replay(const Parameters &parameters) {
  //! Check if the paths are valid
  if (!validate_path(parameters.input_path))
    return;

  // Open the input file and mmap it
  input_file = std::make_shared<MemoryMappedFile>(parameters.input_path, true);

  // Create the output file
  logger = std::make_unique<mpacklog::Logger>(parameters.output_path, false);

  std::cout << "argv0: " << parameters.argv0 << std::endl;

  // Initialize observers
  TransitionModel::Parameters transition_model_params;
  transition_model_params.dt = 0.001;
  transition_model_params.window_size = 128;
  auto transition_model =
      std::make_shared<TransitionModel>(transition_model_params);
  observers.push_back(transition_model);

  // Observation: Measurement model
  MeasurementModel::Parameters measurement_model_params;
  measurement_model_params.argv0 = parameters.argv0;
  measurement_model_params.dt = 0.001;
  measurement_model_params.cutoff_periods = {0.025, 0.025};
  auto measurement_model =
      std::make_shared<MeasurementModel>(measurement_model_params);
  observers.push_back(measurement_model);

  // Observation: Contact filter
  ContactFilter contact_filter(0.5);
  observers.push_back(std::make_shared<ContactFilter>(contact_filter));
}

void Replay::process() {
  // Read the input file
  mpack_tree_t tree;
  mpack_tree_init_data(&tree, (const char *)input_file->mmap_addr,
                       input_file->sb.st_size);

  palimpsest::Dictionary dictionary;
  int i = 0;
  while (true) {
    mpack_tree_parse(&tree);

    if (mpack_tree_error(&tree) != mpack_ok) {
      spdlog::info("End of file reached, terminating...");
      break;
    }
    mpack_node_t root = mpack_tree_root(&tree);
    dictionary.update(root);

    // Update observers
    for (auto &observer : observers) {
      observer->read(dictionary("observation"));
      observer->write(dictionary("observation"));
    }

    if (i >= 1) {
      // Delete the config key
      dictionary.remove("config");
    }

    // Write the dictionary to the output file
    if (!logger->put(dictionary)) {
      spdlog::error("Failed to write dictionary to output file");
      exit(1);
      break;
    }
  }
  mpack_tree_destroy(&tree);
}

// Main function
int main(int argc, char **argv) {
  CommandLineArguments args({argv, argv + argc});
  Replay::Parameters parameters(args.input_path, args.output_path, argv[0]);
  Replay replay(parameters);
  replay.process();

  return 0;
}
