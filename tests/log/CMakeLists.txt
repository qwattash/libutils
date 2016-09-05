
file(GLOB log_TEST_SRCS "log_base.c" "${CMAKE_SOURCE_DIR}/src/log.c")

add_executable(log_base ${log_TEST_SRCS})
add_test(log_base log_base)

target_include_directories(log_base PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/include")

set_target_properties(log_base PROPERTIES
  COMPILE_FLAGS "-Wno-unused-function"
  LINK_FLAGS "-Wl,--wrap=vfprintf -Wl,--wrap=fprintf")

target_link_libraries(log_base cmocka)
