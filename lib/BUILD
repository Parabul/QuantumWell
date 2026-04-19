load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")
load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")



cc_library(
    name = "quantum_well",
    hdrs = ["quantum_well.h"],
    srcs = ["quantum_well.cc"],
    copts = ["-std=c++20"],
    visibility = ["//main:__pkg__"],
)

cc_binary(
    name = "main",
    srcs = ["main.cc"],
    copts = ["-std=c++20"],
    deps = [
        ":quantum_well",
    ],
)

cc_test(
    name = "quantum_well_benchmark",
    srcs = ["quantum_well_benchmark.cc"],
    deps = [
        "@google_benchmark//:benchmark_main",
        ":quantum_well",
    ],
)

wasm_cc_binary(
    name = "main-wasm",
    cc_target = ":main",
)
