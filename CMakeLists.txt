cmake_minimum_required(VERSION 3.10)

project(ThreadPool)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SOURCES "src/*.cc")

# add_library(ThreadPool ${SOURCES})

# target_include_directories(ThreadPool PUBLIC include)
