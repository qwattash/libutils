cmake_minimum_required(VERSION 3.2)

project(utils)

set(utils_VERSION 1.0)

option(ENABLE_TEST "Enable testing" ON)

if (ENABLE_TEST)
  enable_testing()
  add_subdirectory("tests")
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall -DENABLE_OUTPUT")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -DENABLE_OUTPUT")

add_subdirectory("src")
