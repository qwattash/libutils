
file(GLOB log_TEST_SRCS "log_base.c" "${PROJECT_SOURCE_DIR}/src/log.c")

add_executable(log_base ${log_TEST_SRCS})
add_test(log_base log_base)

target_include_directories(log_base PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/include")

set_target_properties(log_base PROPERTIES
  COMPILE_FLAGS "-Wno-unused-function"
  LINK_FLAGS "-Wl,--wrap=vfprintf -Wl,--wrap=fprintf -Wl,--wrap=fopen -Wl,--wrap=openlog -Wl,--wrap=vsyslog")

target_link_libraries(log_base cmocka)
