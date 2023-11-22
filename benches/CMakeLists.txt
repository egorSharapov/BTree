cmake_minimum_required(VERSION 3.22)
project(bench_tree)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

CPMAddPackage(
  NAME benchmark
  GITHUB_REPOSITORY google/benchmark
  VERSION 1.8.0
  OPTIONS "BENCHMARK_ENABLE_TESTING Off"
)

if(benchmark_ADDED)
  set_target_properties(benchmark PROPERTIES CXX_STANDARD 11)
endif()

add_executable(bench_tree bench_tree.cpp)
target_include_directories(bench_tree PRIVATE ../src)
target_link_libraries(bench_tree benchmark::benchmark)
# target_compile_options(bench_tree -DNDEBUG)
