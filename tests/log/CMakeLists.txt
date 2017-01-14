
file(GLOB log_base_SRCS
  "log_base.c"
  "${PROJECT_SOURCE_DIR}/src/log.c")

file(GLOB log_hierarchy_SRCS
  "log_hierarchy.c"
  "${PROJECT_SOURCE_DIR}/src/log.c")

add_executable(log_base ${log_base_SRCS})
add_executable(log_hierarchy ${log_hierarchy_SRCS})
add_test(log_base log_base)
add_test(log_hierarchy log_hierarchy)

set_target_properties(log_base PROPERTIES
  COMPILE_FLAGS "-Wno-unused-function"
  LINK_FLAGS "-Wl,--wrap=vfprintf -Wl,--wrap=fprintf -Wl,--wrap=fopen -Wl,--wrap=openlog -Wl,--wrap=vsyslog -Wl,--wrap=setvbuf")

set_target_properties(log_hierarchy PROPERTIES
  COMPILE_FLAGS "-Wno-unused-function"
  LINK_FLAGS "-Wl,--wrap=vfprintf -Wl,--wrap=fprintf")

target_link_libraries(log_base cmocka)
target_link_libraries(log_hierarchy cmocka)
