package(default_visibility = ["//visibility:public"])

cc_test(
    name = "npz_interpolator",
    srcs = ["NpzInterpolatorTest.cpp"],
    deps = [
        "@googletest//:main",
        "//observers:utils",
        "//observers:npz_interpolator",
    ] + select({
        "//:pi64_config": [
            "@org_llvm_libcxx//:libcxx",
        ],
        "//conditions:default": [],
    }),
    data = [
        "//observers/tests/data:grid_data"
    ]
)

cc_test(
    name = "measurement_model",
    srcs = ["MeasurementModelTest.cpp"],
    deps = [
        "@googletest//:main",
        # "@org_llvm_libcxx//:libcxx",
        "//observers:utils",
        "//observers:npz_interpolator",
        "//observers:measurement_model",
    ] + select({
        "//:pi64_config": [
            "@org_llvm_libcxx//:libcxx",
        ],
        "//conditions:default": [],
    }),
    data = [
        "//observers/data:contact_models"
    ]
)

cc_test(
    name = "transition_model",
    srcs = ["TransitionModelTest.cpp"],
    deps = [
        "@googletest//:main",
        "//observers:transition_model",
    ] + select({
        "//:pi64_config": [
            "@org_llvm_libcxx//:libcxx",
        ],
        "//conditions:default": [],
    }),
)