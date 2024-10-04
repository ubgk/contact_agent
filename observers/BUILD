# -*- python -*-
load("//tools/lint:lint.bzl", "add_lint_tests")

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "replay",
    deps = ["//observers:contact_filter",
            "//observers:transition_model",
            "//observers:measurement_model",
            "@mpacklog"],
    srcs = ["Replay.cpp", "Replay.h"]
)

cc_library(
    name = "contact_filter",
    srcs = ["ContactFilter.cpp"],
    hdrs = ["ContactFilter.h"],
    deps = ["@upkie//upkie/cpp/observers",],
)

cc_library(
    name = "transition_model",
    srcs = ["TransitionModel.cpp"],
    hdrs = ["TransitionModel.h"],
    deps = ["@upkie//upkie/cpp/observers",
            "@eigen", 
            "@palimpsest", 
            "@kissfft"],
)

cc_library(
    name = "measurement_model",
    srcs = ["MeasurementModel.cpp"],
    hdrs = ["MeasurementModel.h"],
    deps = [
        "@upkie//upkie/cpp/observers",
        "@upkie//upkie/cpp/utils:low_pass_filter",
        ":npz_interpolator",
        ":utils",
        "@kissfft"
    ],
    data = [
        "//observers/data:contact_models"
    ],
)

cc_library(
    name = "npz_interpolator",
    srcs = ["NpzInterpolator.cpp"],
    hdrs = ["NpzInterpolator.h"],
    deps = [
        "@spdlog",
        "@cnpy",
        "@btwxt//:btwxt",
    ],
)

cc_library(
    name = "utils",
    srcs = ["utils.cpp"],
    hdrs = ["utils.h"],
    deps = [
        "@spdlog",
        "@bazel_tools//tools/cpp/runfiles:runfiles",
    ]
)

add_lint_tests()
