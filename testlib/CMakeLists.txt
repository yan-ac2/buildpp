cmake_minimum_required(VERSION 3.10)

project(bench CXX)

add_subdirectory(picobench)
add_executable(bench bench.cc)
target_link_libraries(bench picobench::picobench)