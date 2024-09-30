# -*- python -*-
#
# SPDX-License-Identifier: Apache-2.0

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def cnpy_repository():
    """
    Clone repository from GitHub and make its targets available for binding.
    """
    git_repository(
        name = "cnpy",
        remote = "https://github.com/rogersce/cnpy.git",
        commit = "4e8810b1a8637695171ed346ce68f6984e585ef4",
        build_file="//tools/workspace/cnpy:cnpy.BUILD"
    )
