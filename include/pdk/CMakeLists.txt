# the libpdk headers

add_subdirectory(global)
add_subdirectory(pal)
add_subdirectory(base)
add_subdirectory(stdext)
add_subdirectory(utils)

# add the automatic generated header files
list(APPEND PDK_HEADER_FILES 
    ${PDK_INCLUDE_DIR}/pdk/Config.h
    ${PDK_INCLUDE_DIR}/pdk/Version.h)

set(PDK_HEADER_FILES ${PDK_HEADER_FILES} PARENT_SCOPE)
