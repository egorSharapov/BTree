cmake_minimum_required(VERSION 3.22)
project(tree)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/CPM.cmake)

file(GLOB SRCS
        "${PROJECT_SOURCE_DIR}/src/main.cpp"
)

add_subdirectory(benches)
add_subdirectory(tests)
add_executable(tree ${SRCS})
