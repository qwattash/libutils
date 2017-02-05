
file(GLOB utils_C_SRCS "*.c")

# full libutils
add_library(utils STATIC ${utils_C_SRCS})
add_library(utils-shared SHARED ${utils_C_SRCS})
set_target_properties(utils-shared PROPERTIES OUTPUT_NAME utils)

install(
  TARGETS utils utils-shared
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  COMPONENT libraries)
