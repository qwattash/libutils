
file(GLOB list_TEST_SRCS "*.c")

foreach (TEST_SRC ${list_TEST_SRCS})
  get_filename_component(TEST ${TEST_SRC} NAME_WE)
  add_executable(${TEST} ${TEST_SRC})
  add_test(${TEST} ${TEST})
  target_include_directories(${TEST} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/include")
  set_target_properties(${TEST} PROPERTIES
    COMPILE_FLAGS "-Wno-unused-function")
  target_link_libraries(${TEST} utils cmocka)
endforeach ()
