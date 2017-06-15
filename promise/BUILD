cc_library(
  name = "promise",
  hdrs = ["promise.hpp"],
  includes = ["."],
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "main",
  deps = ["promise"],
  srcs = ["main.cpp", "async.h", "async.cpp"],
  copts = ["-std=c++14"],
)
