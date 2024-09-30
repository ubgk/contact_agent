# -*- python -*-
#
# SPDX-License-Identifier: Apache-2.0

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def btwxt_repository():
    """
    Clone repository from GitHub and make its targets available for binding.
    """
    git_repository(
        name = "btwxt",
        remote = "https://github.com/bigladder/btwxt.git",
        commit = "b952f2dfe86929bbc7bee0ed086cb0af9ba14331",
        build_file="//tools/workspace/btwxt:btwxt.BUILD"
    )
