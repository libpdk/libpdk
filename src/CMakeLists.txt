# This scripts collect libpdk implement library source files

set(PDK_BASE_SOURCES "")

add_subdirectory(pal)
add_subdirectory(global)
add_subdirectory(kernel)

add_subdirectory(modules/base)
