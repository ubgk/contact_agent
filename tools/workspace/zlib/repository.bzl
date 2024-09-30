# -*- python -*-
#
# SPDX-License-Identifier: Apache-2.0

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def zlib_repository():
    """
    Clone repository from GitHub and make its targets available for binding.
    """
    git_repository(
        name = "zlib",
        remote = "https://github.com/madler/zlib.git",
        commit = "ceadaf28dfa48dbf238a0ddb884d4c543b4170e8",
        build_file="//tools/workspace/zlib:zlib.BUILD"
    )