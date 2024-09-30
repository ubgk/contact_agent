# -*- python -*-
#
# SPDX-License-Identifier: Apache-2.0

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def courier_repository():
    """
    Clone repository from GitHub and make its targets available for binding.
    """
    git_repository(
        name = "courier",
        remote = "https://github.com/bigladder/courier.git",
        commit = "5e9d1314cf9ff5a88eae21997d9034ea5fd03ab3",
        build_file="//tools/workspace/courier:courier.BUILD"
    )
