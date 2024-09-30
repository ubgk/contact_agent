srcs = glob(["include/btwxt/*.cpp", "src/*.cpp", "src/*.h"])
hdrs = glob(["include/btwxt/*.h", "include/btwxt/*.hpp", "src/*.hpp"])

cc_library(
    name = "btwxt",
    srcs = srcs,
    hdrs = hdrs,
    visibility = ["//visibility:public"],
    deps = [
        "@courier",
        "@fmt//:fmt",
    ],
    includes = ["include/btwxt", "include", "src"],
    strip_include_prefix = "include",
    )