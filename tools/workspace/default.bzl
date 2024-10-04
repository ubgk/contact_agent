# -*- python -*-
#
# SPDX-License-Identifier: Apache-2.0

load("//tools/workspace/btwxt:repository.bzl", "btwxt_repository")
load("//tools/workspace/cnpy:repository.bzl", "cnpy_repository")
load("//tools/workspace/courier:repository.bzl", "courier_repository")
load("//tools/workspace/kissfft:repository.bzl", "kissfft_repository")
load("//tools/workspace/mpack:repository.bzl", "mpack_repository")
load("//tools/workspace/upkie:repository.bzl", "upkie_repository")
load("//tools/workspace/zlib:repository.bzl", "zlib_repository")

def add_default_repositories():
    """
    Declare workspace repositories for all dependencies.

    This function intended to be loaded and called from a WORKSPACE file.
    """
    btwxt_repository()
    cnpy_repository()
    courier_repository()
    kissfft_repository()
    mpack_repository()
    upkie_repository()
    zlib_repository()
