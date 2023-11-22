cmake_minimum_required(VERSION 3.22)
project(test_tree)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

CPMAddPackage(
  NAME googletest
  GITHUB_REPOSITORY google/googletest
  VERSION 1.14.0
  # OPTIONS "BENCHMARK_ENABLE_TESTING Off"
)

enable_testing()

add_executable(
    test_tree
  test_tree.cpp
)

target_include_directories(test_tree PRIVATE ../src)

target_link_libraries(test_tree gtest)

include(GoogleTest)
gtest_discover_tests(test_tree)
