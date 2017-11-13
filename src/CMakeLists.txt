# This scripts collect libpdk implement library source files

set(PDK_BASE_SOURCES "")

add_subdirectory(pal)
add_subdirectory(global)
add_subdirectory(kernel)
add_subdirectory(utils)

message("${PDK_BASE_SOURCES}")

set(PDK_BASE_MODULE_SOURCES "")
add_subdirectory(modules/base)

pdk_add_library(libpdk 
    ${PDK_HEADER_FILES} 
    ${PDK_BASE_SOURCES}
    ${PDK_BASE_MODULE_SOURCES})
