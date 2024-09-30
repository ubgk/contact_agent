# -*- python -*-
#
# SPDX-License-Identifier: Apache-2.0

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def upkie_repository():
    """
    Clone repository from GitHub and make its targets available for binding.
    """
    git_repository(
        name = "upkie",
        remote = "https://github.com/upkie/upkie.git",
        commit = "6a909321f16331e1d0a1576c2fd6f226da53bbc8",
        shallow_since = "1722520468 +0200",
    )
