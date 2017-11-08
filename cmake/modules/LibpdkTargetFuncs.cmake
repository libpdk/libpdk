include(LibpdkProcessSources)
include(DetermineGCCCompatible)

function(pdk_update_compile_flags name)
    get_property(sources TARGET ${name} PROPERTY SOURCES)
    if("${sources}" MATCHES "\\.c(;|$)")
        set(update_src_props ON)
    endif()
    # PDK_REQUIRES_EH is an internal flag that individual targets can use to
    # force EH
    if(PDK_REQUIRES_EH OR PDK_ENABLE_EH)
        if(NOT (PDK_REQUIRES_RTTI OR PDK_ENABLE_RTTI))
            message(AUTHOR_WARNING "Exception handling requires RTTI. Enabling RTTI for ${name}")
            set(PDK_REQUIRES_RTTI ON)
        endif()
        if(MSVC)
            list(APPEND PDK_COMPILE_FLAGS "/EHsc")
        endif()
    else()
        if(PDK_COMPILER_IS_GCC_COMPATIBLE)
            list(APPEND PDK_COMPILE_FLAGS "-fno-exceptions")
        elseif(MSVC)
            list(APPEND PDK_COMPILE_DEFINITIONS _HAS_EXCEPTIONS=0)
            list(APPEND PDK_COMPILE_FLAGS "/EHs-c-")
        endif()
    endif()
    # PDK_REQUIRES_RTTI is an internal flag that individual
    # targets can use to force RTTI
    set(PDK_CONFIG_HAS_RTTI YES CACHE INTERNAL "")
    if(NOT (PDK_REQUIRES_RTTI OR PDK_ENABLE_RTTI))
        set(PDK_CONFIG_HAS_RTTI NO CACHE INTERNAL "")
        list(APPEND PDK_COMPILE_DEFINITIONS GTEST_HAS_RTTI=0)
        if(PDK_COMPILER_IS_GCC_COMPATIBLE)
            list(APPEND PDK_COMPILE_FLAGS "-fno-rtti")
        elseif (MSVC)
            list(APPEND PDK_COMPILE_FLAGS "/GR-")
        endif()
    endif()
    
    # Assume that;
    #   - PDK_COMPILE_FLAGS is list.
    #   - PROPERTY COMPILE_FLAGS is string.
    string(REPLACE ";" " " target_compile_flags " ${PDK_COMPILE_FLAGS}")
    if(update_src_props)
        foreach(fn ${sources})
            get_filename_component(suf ${fn} EXT)
            if("${suf}" STREQUAL ".cpp")
                set_property(SOURCE ${fn} APPEND_STRING PROPERTY
                    COMPILE_FLAGS "${target_compile_flags}")
            endif()
        endforeach()
    else()
        # Update target props, since all sources are C++.
        set_property(TARGET ${name} APPEND_STRING PROPERTY
            COMPILE_FLAGS "${target_compile_flags}")
    endif()
    set_property(TARGET ${name} APPEND PROPERTY COMPILE_DEFINITIONS ${PDK_COMPILE_DEFINITIONS})
endfunction()

if(NOT WIN32 AND NOT APPLE)
    # Detect what linker we have here
    if(PDK_USE_LINKER)
        set(command ${CMAKE_C_COMPILER} -fuse-ld=${PDK_USE_LINKER} -Wl,--version)
    else()
        set(command ${CMAKE_C_COMPILER} -Wl,--version)
    endif()
    execute_process(
        COMMAND ${command}
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr)
    set(PDK_LINKER_DETECTED ON)
    if("${stdout}" MATCHES "GNU gold")
        set(PDK_LINKER_IS_GOLD ON)
        message(STATUS "Linker detection: GNU Gold")
    elseif("${stdout}" MATCHES "^LLD")
        set(PDK_LINKER_IS_LLD ON)
        message(STATUS "Linker detection: LLD")
    elseif("${stdout}" MATCHES "GNU ld")
        set(PDK_LINKER_IS_GNULD ON)
        message(STATUS "Linker detection: GNU ld")
    elseif("${stderr}" MATCHES "Solaris Link Editors")
        set(PDK_LINKER_IS_SOLARISLD ON)
        message(STATUS "Linker detection: Solaris ld")
    else()
        set(PDK_LINKER_DETECTED OFF)
        message(STATUS "Linker detection: unknown")
    endif()
endif()

function(pdk_add_link_opts target_name)
    # Don't use linker optimizations in debug builds since it slows down the
    # linker in a context where the optimizations are not important.
    if(NOT UPPERCASE_CMAKE_BUILD_TYPE STREQUAL "DEBUG")
        # Pass -O3 to the linker. This enabled different optimizations on different
        # linkers.
        if(NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin|SunOS|AIX" OR WIN32))
            set_property(TARGET ${target_name} APPEND_STRING PROPERTY
                LINK_FLAGS " -Wl,-O3")
        endif()
        if(PDK_LINKER_IS_GOLD)
            # With gold gc-sections is always safe.
            set_property(TARGET ${target_name} APPEND_STRING PROPERTY
                LINK_FLAGS " -Wl,--gc-sections")
            # Note that there is a bug with -Wl,--icf=safe so it is not safe
            # to enable. See https://sourceware.org/bugzilla/show_bug.cgi?id=17704.
        endif()
        
        if(NOT PDK_NO_DEAD_STRIP)
            if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
                # ld64's implementation of -dead_strip breaks tools that use plugins.
                set_property(TARGET ${target_name} APPEND_STRING PROPERTY
                    LINK_FLAGS " -Wl,-dead_strip")
            elseif(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
                set_property(TARGET ${target_name} APPEND_STRING PROPERTY
                    LINK_FLAGS " -Wl,-z -Wl,discard-unused=sections")
            elseif(NOT WIN32 AND NOT PDK_LINKER_IS_GOLD)
                # Object files are compiled with -ffunction-data-sections.
                # Versions of bfd ld < 2.23.1 have a bug in --gc-sections that breaks
                # tools that use plugins. Always pass --gc-sections once we require
                # a newer linker.
                set_property(TARGET ${target_name} APPEND_STRING PROPERTY
                    LINK_FLAGS " -Wl,--gc-sections")
            endif()
        endif()
    endif()
endfunction(pdk_add_link_opts)

# Set each output directory according to ${CMAKE_CONFIGURATION_TYPES}.
# Note: Don't set variables CMAKE_*_OUTPUT_DIRECTORY any more,
# or a certain builder, for eaxample, msbuild.exe, would be confused.
function(pdk_set_output_directory target)
    cmake_parse_arguments(ARG "" "BINARY_DIR;LIBRARY_DIR" "" ${ARGN})
    # module_dir -- corresponding to LIBRARY_OUTPUT_DIRECTORY.
    # It affects output of add_library(MODULE).
    if(WIN32 OR CYGWIN)
        # DLL platform
        set(module_dir ${ARG_BINARY_DIR})
    else()
        set(module_dir ${ARG_LIBRARY_DIR})
    endif()
    if(NOT "${CMAKE_CFG_INTDIR}" STREQUAL ".")
        foreach(build_mode ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER "${build_mode}" PDK_TEMP_CONFIG_SUFFIX)
            if(ARG_BINARY_DIR)
                string(REPLACE ${CMAKE_CFG_INTDIR} ${build_mode} bi ${ARG_BINARY_DIR})
                set_target_properties(${target} PROPERTIES "RUNTIME_OUTPUT_DIRECTORY_${PDK_TEMP_CONFIG_SUFFIX}" ${bi})
            endif()
            if(ARG_LIBRARY_DIR)
                string(REPLACE ${CMAKE_CFG_INTDIR} ${build_mode} li ${ARG_LIBRARY_DIR})
                set_target_properties(${target} PROPERTIES "ARCHIVE_OUTPUT_DIRECTORY_${PDK_TEMP_CONFIG_SUFFIX}" ${li})
            endif()
            if(module_dir)
                string(REPLACE ${CMAKE_CFG_INTDIR} ${build_mode} mi ${module_dir})
                set_target_properties(${target} PROPERTIES "LIBRARY_OUTPUT_DIRECTORY_${PDK_TEMP_CONFIG_SUFFIX}" ${mi})
            endif()
        endforeach()
    else()
        if(ARG_BINARY_DIR)
            set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ARG_BINARY_DIR})
        endif()
        if(ARG_LIBRARY_DIR)
            set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${ARG_LIBRARY_DIR})
        endif()
        if(module_dir)
            set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${module_dir})
        endif()
    endif()
endfunction()

# If on Windows and building with MSVC, add the resource script containing the
# VERSIONINFO data to the project.  This embeds version resource information
# into the output .exe or .dll.
# TODO: Enable for MinGW Windows builds too.
#
function(pdk_add_windows_version_resource_file OUT_VAR)
    set(sources ${ARGN})
    if (MSVC AND CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        set(resource_file ${PDK_SOURCE_DIR}/resources/windows_version_resource.rc)
        if(EXISTS ${resource_file})
            set(sources ${sources} ${resource_file})
            source_group("Resource Files" ${resource_file})
            set(windows_resource_file ${resource_file} PARENT_SCOPE)
        endif()
    endif()
    set(${OUT_VAR} ${sources} PARENT_SCOPE)
endfunction()

# pdk_set_windows_version_resource_properties(name resource_file...
#   VERSION_MAJOR int
#     Optional major version number (defaults to PDK_VERSION_MAJOR)
#   VERSION_MINOR int
#     Optional minor version number (defaults to PDK_VERSION_MINOR)
#   VERSION_PATCHLEVEL int
#     Optional patchlevel version number (defaults to PDK_VERSION_PATCH)
#   VERSION_STRING
#     Optional version string (defaults to PDK_PACKAGE_VERSION)
#   PRODUCT_NAME
#     Optional product name string (defaults to "libpdk")
#   )
function(set_windows_version_resource_properties name resource_file)
    cmake_parse_arguments(ARG
        ""
        "VERSION_MAJOR;VERSION_MINOR;VERSION_PATCHLEVEL;VERSION_STRING;PRODUCT_NAME"
        ""
        ${ARGN})
    
    if(NOT DEFINED ARG_VERSION_MAJOR)
        set(ARG_VERSION_MAJOR ${PDK_VERSION_MAJOR})
    endif()
    if(NOT DEFINED ARG_VERSION_MINOR)
        set(ARG_VERSION_MINOR ${PDK_VERSION_MINOR})
    endif()
    if (NOT DEFINED ARG_VERSION_PATCHLEVEL)
        set(ARG_VERSION_PATCHLEVEL ${PDK_VERSION_PATCH})
    endif()
    if (NOT DEFINED ARG_VERSION_STRING)
        set(ARG_VERSION_STRING ${PDK_PACKAGE_VERSION})
    endif()
    if (NOT DEFINED ARG_PRODUCT_NAME)
        set(ARG_PRODUCT_NAME "libpdk")
    endif()
    set_property(SOURCE ${resource_file} PROPERTY COMPILE_FLAGS /nologo)
    set_property(SOURCE ${resource_file}
        PROPERTY COMPILE_DEFINITIONS
        "RC_VERSION_FIELD_1=${ARG_VERSION_MAJOR}"
        "RC_VERSION_FIELD_2=${ARG_VERSION_MINOR}"
        "RC_VERSION_FIELD_3=${ARG_VERSION_PATCHLEVEL}"
        "RC_VERSION_FIELD_4=0"
        "RC_FILE_VERSION=\"${ARG_VERSION_STRING}\""
        "RC_INTERNAL_NAME=\"${name}\""
        "RC_PRODUCT_NAME=\"${ARG_PRODUCT_NAME}\""
        "RC_PRODUCT_VERSION=\"${ARG_VERSION_STRING}\"")
endfunction()
