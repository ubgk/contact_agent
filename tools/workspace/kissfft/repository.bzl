# -*- python -*-
#
# SPDX-License-Identifier: Apache-2.0

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def kissfft_repository():
    """
    Clone repository from GitHub and make its targets available for binding.
    """
    git_repository(
        name = "kissfft",
        remote = "https://github.com/mborgerding/kissfft.git",
        commit = "f5f2a3b2f2cd02bf80639adb12cbeed125bdf420",
        build_file = "//tools/workspace/kissfft:kissfft.BUILD",
    )
