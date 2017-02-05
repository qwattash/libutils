
file(GLOB libutils_HDRS "libutils/*.h")

install(
  FILES ${libutils_HDRS}
  DESTINATION include/libutils
  COMPONENT headers)
