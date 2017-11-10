# This script collect libpdk extension sources
# and build the extension pdk

set(PDK_EXT_SOURCES "main.cpp")

add_subdirectory(common)
add_subdirectory(consts)
add_subdirectory(funcs)
add_subdirectory(interfaces)
add_subdirectory(kernel)
add_subdirectory(ns)

