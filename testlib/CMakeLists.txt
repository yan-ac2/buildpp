cmake_minimum_required(VERSION 3.10)

project(bench CXX)
set(BENCHMARK_ENABLE_GTEST_TEST OFF)
set(BENCHMARK_ENABLE_TESTING OFF)
add_subdirectory(benchmark)
add_executable(bench bench.cc)
target_link_libraries(bench benchmark::benchmark)