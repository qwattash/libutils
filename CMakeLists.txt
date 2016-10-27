cmake_minimum_required(VERSION 3.2)

project(utils)

set(utils_VERSION 1.1)

# options
option(ENABLE_TEST "Enable testing" ON)
option(ENABLE_LOGGING "Enable logging" ON)

# first update the flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall -DLINUX")
if (ENABLE_LOGGING)
  set(CMAKE_C_FLAGS_RELEASE "-DENABLE_LOGGING -DLOG_NODEBUG")
endif ()
set(CMAKE_C_FLAGS_DEBUG "-DENABLE_LOGGING -DDEBUG")

if (ENABLE_TEST)
  enable_testing()
  add_subdirectory("tests")
endif ()

# main include directory
include_directories("include")

add_subdirectory("src")
