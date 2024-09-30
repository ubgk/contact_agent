# -*- python -*-
#
# SPDX-License-Identifier: Apache-2.0

package(default_visibility = ["//visibility:public"])

py_binary(
    name = "agent",
    srcs = ["run.py"],
    main = "run.py",

    # Enable `from X import y` rather than `from agents.agent_name.X import y`
    # so that the agent can be run indifferently via Python or Bazel.
    imports = ["."],

    data = [
    ] + select({
        "//:pi64_config": ["//spines:pi3hat_spine"],
        "//conditions:default": [],
    }),
    deps = [
        "@upkie//upkie/utils:exceptions",
        "@upkie//upkie/utils:raspi",
        "@upkie//upkie/utils:spdlog",
        "@upkie//upkie:python",
    ],
)
