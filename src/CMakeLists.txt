# This scripts collect libpdk implement library source files

pdk_collect_files(
    TYPE_SOURCE
    DIR pal
    OUTPUT_VAR PDK_BASE_SOURCES)

pdk_collect_files(
    TYPE_SOURCE
    DIR global
    OUTPUT_VAR PDK_BASE_SOURCES)

pdk_collect_files(
    TYPE_SOURCE
    DIR kernel
    OUTPUT_VAR PDK_BASE_SOURCES)

pdk_collect_files(
    TYPE_SOURCE
    DIR utils
    OUTPUT_VAR PDK_BASE_SOURCES)

pdk_collect_files(
    TYPE_SOURCE
    DIR stdext
    OUTPUT_VAR PDK_BASE_SOURCES)

pdk_collect_files(
    TYPE_SOURCE
    DIR stdext
    OUTPUT_VAR PDK_BASE_SOURCES)

pdk_collect_files(
    TYPE_SOURCE
    DIR modules/base
    OUTPUT_VAR PDK_BASE_MODULE_SOURCES)

list(REMOVE_ITEM PDK_BASE_MODULE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/modules/base/lang/UnicodeTables.cpp)

pdk_add_library(libpdk 
    ${PDK_HEADER_FILES} 
    ${PDK_BASE_SOURCES}
    ${PDK_BASE_MODULE_SOURCES})

