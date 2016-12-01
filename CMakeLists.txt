cmake_minimum_required(VERSION 3.2)

project(utils)

set(utils_VERSION 1.1)

# options
option(ENABLE_TEST "Enable testing" ON)
option(ENABLE_LOGGING "Enable logging" ON)

# check for optional required features
include(CheckIncludeFiles)
check_include_files(syslog.h HAVE_SYSLOG_H)
configure_file("include/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/utils_config.h")

# first update the flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall")
if (ENABLE_LOGGING)
  set(CMAKE_C_FLAGS_RELEASE "-DENABLE_LOGGING -DLOG_NODEBUG")
endif ()
set(CMAKE_C_FLAGS_DEBUG "-DENABLE_LOGGING -DDEBUG")

# main include directory
include_directories("include")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")

if (ENABLE_TEST)
  enable_testing()
  add_subdirectory("tests")
endif ()

add_subdirectory("src")
