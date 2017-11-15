# the libpdk headers

pdk_collect_files(
    DIR global
    OUTPUT_VAR PDK_HEADER_FILES)

pdk_collect_files(
    DIR pal
    OUTPUT_VAR PDK_HEADER_FILES)

pdk_collect_files(
    DIR base
    OUTPUT_VAR PDK_HEADER_FILES)

pdk_collect_files(
    DIR stdext
    OUTPUT_VAR PDK_HEADER_FILES)

pdk_collect_files(
    DIR utils
    OUTPUT_VAR PDK_HEADER_FILES)

# add the automatic generated header files
list(APPEND PDK_HEADER_FILES 
    ${PDK_INCLUDE_DIR}/pdk/Config.h
    ${PDK_INCLUDE_DIR}/pdk/Version.h)

set(PDK_HEADER_FILES ${PDK_HEADER_FILES} PARENT_SCOPE)
