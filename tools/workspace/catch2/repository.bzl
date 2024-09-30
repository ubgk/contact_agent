# -*- python -*-
#
# SPDX-License-Identifier: Apache-2.0

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def catch_repository():
    """
    Clone repository from GitHub and make its targets available for binding.
    """
    git_repository(
        name = "catch2",
        remote = "https://github.com/catchorg/Catch2.git",
        commit = "4e8d92bf02f7d1c8006a0e7a5ecabd8e62d98502",
        build_file="//tools/workspace/catch2:catch2.BUILD"
    )
