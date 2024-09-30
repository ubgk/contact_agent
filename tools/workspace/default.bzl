# -*- python -*-
#
# SPDX-License-Identifier: Apache-2.0

load("//tools/workspace/upkie:repository.bzl", "upkie_repository")
load("//tools/workspace/cnpy:repository.bzl", "cnpy_repository")
load("//tools/workspace/btwxt:repository.bzl", "btwxt_repository")
load("//tools/workspace/courier:repository.bzl", "courier_repository")
load("//tools/workspace/zlib:repository.bzl", "zlib_repository")
load("//tools/workspace/kissfft:repository.bzl", "kissfft_repository")

def add_default_repositories():
    """
    Declare workspace repositories for all dependencies.

    This function intended to be loaded and called from a WORKSPACE file.
    """
    upkie_repository()
    cnpy_repository()
    btwxt_repository()
    courier_repository()
    zlib_repository()
    kissfft_repository()
