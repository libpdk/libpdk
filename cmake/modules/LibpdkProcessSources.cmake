include(AddFileDependencies)
include(CMakeParseArguments)

function(pkd_replace_compiler_option var old new)
    # Replaces a compiler option or switch `old' in `var' by `new'.
    # If `old' is not in `var', appends `new' to `var'.
    # Example: pdk_replace_compiler_option(CMAKE_CXX_FLAGS_RELEASE "-O3" "-O2")
    # If the option already is on the variable, don't add it:
    if("${${var}}" MATCHES "(^| )${new}($| )")
        set(n "")
    else()
        set(n "${new}")
    endif()
    if("${${var}}" MATCHES "(^| )${old}($| )" )
        string(REGEX REPLACE "(^| )${old}($| )" " ${n} " ${var} "${${var}}")
    else()
        set(${var} "${${var}} ${n}")
    endif()
    set(${var} "${${var}}" PARENT_SCOPE)
endfunction()

function(add_header_files_for_glob headers_out glob)
    file(GLOB hds ${glob})
    set(${headers_out} ${hds} PARENT_SCOPE)
endfunction()

function(pdk_all_header_files headers_out additional_headerdirs)
    add_header_files_for_glob(hds *.h)
    list(APPEND all_headers ${hds})
    foreach(additional_dir ${additional_headerdirs})
        add_header_files_for_glob(hds "${additional_dir}/*.h")
        list(APPEND all_headers ${hds})
        add_header_files_for_glob(hds "${additional_dir}/*.inc")
        list(APPEND all_headers ${hds})
    endforeach()
    set(${hdrs_out} ${all_headers} PARENT_SCOPE)
endfunction()

function(pdk_check_source_file_list)
    cmake_parse_arguments(ARG "" "SOURCE_DIR" "" ${ARGN})
    set(listed ${ARG_UNPARSED_ARGUMENTS})
    if(ARG_SOURCE_DIR)
        file(GLOB globbed
            RELATIVE "${CMAKE_CURRENT_LIST_DIR}"
            "${ARG_SOURCE_DIR}/*.c" "${ARG_SOURCE_DIR}/*.cpp")
    else()
        file(GLOB globbed *.c *.cpp)
    endif()
    foreach(g ${globbed})
        get_filename_component(fn ${g} NAME)
        if(ARG_SOURCE_DIR)
            set(entry "${g}")
        else()
            set(entry "${fn}")
        endif()
        # Don't reject hidden files. Some editors create backups in the
        # same directory as the file.
        if (NOT "${fn}" MATCHES "^\\.")
            list(FIND PDK_OPTIONAL_SOURCES ${entry} idx)
            if(idx LESS 0)
                message(SEND_ERROR "Found unknown source file ${g}
                    Please update ${CMAKE_CURRENT_LIST_FILE}\n")
            endif()
        endif()
    endforeach()
endfunction()

