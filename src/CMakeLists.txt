
set(list_C_SRCS "list.c")
set(argparse_C_SRCS "argparse.c")

include_directories("../include")

# static targets
add_library(list STATIC ${list_C_SRCS})
add_library(argparse STATIC ${argparse_C_SRCS})
target_link_libraries(argparse list)

# dynamic targets
add_library(list-shared SHARED ${list_C_SRCS})
add_library(argparse-shared SHARED ${argparse_C_SRCS})
set_target_properties(list-shared PROPERTIES OUTPUT_NAME list)
set_target_properties(argparse-shared PROPERTIES OUTPUT_NAME argparse)
target_link_libraries(argparse-shared list-shared)

