# -*- python -*-

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "bullet_spine",
    srcs = [
        "bullet_spine.cpp",
    ],
    data = [
        "@upkie_description",
    	"//assets:urdf_files"
    ],
    deps = [
        "@upkie//upkie/cpp/actuation:bullet_interface",
        "@upkie//upkie/cpp/model",
        "@upkie//upkie/cpp/observers",
        "@upkie//upkie/cpp/sensors",
        "@upkie//upkie/cpp/spine",
        "@upkie//upkie/cpp/utils:datetime_now_string",
        "@upkie//upkie/cpp/utils:get_log_path",
        "@upkie//upkie/cpp:version",
        "//observers:measurement_model",
        "//observers:transition_model",
        "//observers:contact_filter"
    ],
)

cc_binary(
    name = "pi3hat_spine",
    srcs = [
        "pi3hat_spine.cpp",
    ],
    data = [
    	"//assets:urdf_files"
    ],
    deps = [
        "@upkie//upkie/cpp/model",
        "@upkie//upkie/cpp/observers",
        "@upkie//upkie/cpp/sensors",
        "@upkie//upkie/cpp/spine",
        "@upkie//upkie/cpp/utils:datetime_now_string",
        "@upkie//upkie/cpp/utils:get_log_path",
        "@upkie//upkie/cpp:version",
        "//observers:measurement_model",
        "//observers:transition_model",
        "//observers:contact_filter"
    ] + select({
        "//:pi64_config": ["@upkie//upkie/cpp/actuation:pi3hat_interface"],
        "//conditions:default": [],
    }),
    target_compatible_with = select({
        "//:pi64_config": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
)
