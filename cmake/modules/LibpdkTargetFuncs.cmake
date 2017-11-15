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
function(pdk_set_windows_version_resource_properties name resource_file)
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

# pdk_add_library(name sources...
#   SHARED;STATIC
#     STATIC by default w/o BUILD_SHARED_LIBS.
#     SHARED by default w/  BUILD_SHARED_LIBS.
#   OBJECT
#     Also create an OBJECT library target. Default if STATIC && SHARED.
#   MODULE
#     Target ${name} might not be created on unsupported platforms.
#     Check with "if(TARGET ${name})".
#   DISABLE_PDK_LINK_PDK_DYLIB
#     Do not link this library to libPDK, even if
#     PDK_LINK_PDK_DYLIB is enabled.
#   OUTPUT_NAME name
#     Corresponds to OUTPUT_NAME in target properties.
#   DEPENDS targets...
#     Same semantics as add_dependencies().
#   LINK_COMPONENTS components...
#     Same as the variable PDK_LINK_COMPONENTS.
#   LINK_LIBS lib_targets...
#     Same semantics as target_link_libraries().
#   ADDITIONAL_HEADERS
#     May specify header files for IDE generators.
#   SONAME
#     Should set SONAME link flags and create symlinks
#   PLUGIN_TOOL
#     The tool (i.e. cmake target) that this plugin will link against
#   )
function(pdk_add_library name)
    cmake_parse_arguments(ARG
        "MODULE;SHARED;STATIC;OBJECT;DISABLE_PDK_LINK_PDK_DYLIB;SONAME"
        "OUTPUT_NAME"
        "ADDITIONAL_HEADERS;DEPENDS;LINK_COMPONENTS;LINK_LIBS;OBJLIBS"
        ${ARGN})
    list(APPEND PDK_COMMON_DEPENDS ${ARG_DEPENDS})
    
    if(ARG_ADDITIONAL_HEADERS)
        # Pass through ADDITIONAL_HEADERS.
        # for argument keyword
        set(ARG_ADDITIONAL_HEADERS ADDITIONAL_HEADERS ${ARG_ADDITIONAL_HEADERS})
    endif()
    
    if(ARG_OBJLIBS)
        set(ALL_FILES ${ARG_OBJLIBS})
    else()
        pdk_process_sources(ALL_FILES ${ARG_UNPARSED_ARGUMENTS} ${ARG_ADDITIONAL_HEADERS})
    endif()
    
    if(ARG_MODULE)
        if(ARG_SHARED OR ARG_STATIC)
            message(WARNING "MODULE with SHARED|STATIC doesn't make sense.")
        endif()
    else()
        if(BUILD_SHARED_LIBS AND NOT ARG_STATIC)
            set(ARG_SHARED TRUE)
        endif()
        if(NOT ARG_SHARED)
            set(ARG_STATIC TRUE)
        endif()
    endif()
    
    # Generate objlib
    if((ARG_SHARED AND ARG_STATIC) OR ARG_OBJECT)
        # Generate an obj library for both targets.
        set(obj_name "obj.${name}")
        add_library(${obj_name} OBJECT EXCLUDE_FROM_ALL ${ALL_FILES})
        pek_update_compile_flags(${obj_name})
        set(ALL_FILES "$<TARGET_OBJECTS:${obj_name}>")
        # Do add_dependencies(obj) later due to CMake issue 14747.
        list(APPEND objlibs ${obj_name})
        set_target_properties(${obj_name} PROPERTIES FOLDER "Object Libraries")
    endif()
    
    if(ARG_SHARED AND ARG_STATIC)
        # static
        set(name_static "${name}_static")
        f(ARG_OUTPUT_NAME)
        set(output_name OUTPUT_NAME "${ARG_OUTPUT_NAME}")
        # DEPENDS has been appended to PDK_COMMON_LIBS.
        pdk_add_library(${name_static} STATIC
            ${output_name}
            OBJLIBS ${ALL_FILES} # objlib
            LINK_LIBS ${ARG_LINK_LIBS}
            LINK_COMPONENTS ${ARG_LINK_COMPONENTS})
        # FIXME: Add name_static to anywhere in TARGET ${name}'s PROPERTY.
        set(ARG_STATIC)
    endif()
    
    if(ARG_MODULE)
        add_library(${name} MODULE ${ALL_FILES})
        pdk_setup_rpath(${name})
    elseif(ARG_SHARED)
        pdk_add_windows_version_resource_file(ALL_FILES ${ALL_FILES})
        add_library(${name} SHARED ${ALL_FILES})
        pdk_setup_rpath(${name})
    else()
        add_library(${name} STATIC ${ALL_FILES})
    endif()
    
    if(DEFINED windows_resource_file)
        pdk_set_windows_version_resource_properties(${name} ${windows_resource_file})
        set(windows_resource_file ${windows_resource_file} PARENT_SCOPE)
    endif()
    
    pdk_set_output_directory(${name} 
        BINARY_DIR ${PDK_RUNTIME_OUTPUT_INTDIR} 
        LIBRARY_DIR ${PDK_LIBRARY_OUTPUT_INTDIR})
    
    if(NOT obj_name)
        pdk_update_compile_flags(${name})
    endif()
    pdk_add_link_opts(${name})
    if(ARG_OUTPUT_NAME)
        set_target_properties(${name}
            PROPERTIES
            OUTPUT_NAME ${ARG_OUTPUT_NAME})
    endif()
    
    if(ARG_MODULE)
        set_target_properties(${name} PROPERTIES
            PREFIX ""
            SUFFIX ${PDK_PLUGIN_EXT})
    endif()
    
    if(ARG_SHARED)
        if(WIN32)
            set_target_properties(${name} PROPERTIES
                PREFIX "")
        endif()
        # Set SOVERSION on shared libraries that lack explicit SONAME
        # specifier, on *nix systems that are not Darwin.
        if(UNIX AND NOT APPLE AND NOT ARG_SONAME)
            set_target_properties(${name}
                PROPERTIES
                # Since 4.0.0, the ABI version is indicated by the major version
                SOVERSION ${PDK_VERSION_MAJOR}
                VERSION ${PDK_VERSION_MAJOR}.${PDK_VERSION_MINOR}.${PDK_VERSION_PATCH}${PDK_VERSION_SUFFIX})
        endif()
    endif()
    
    if(ARG_SHARED OR ARG_MODULE)
        # Do not add -Dname_EXPORTS to the command-line when building files in this
        # target. Doing so is actively harmful for the modules build because it
        # creates extra module variants, and not useful because we don't use these
        # macros.
        set_target_properties(${name} PROPERTIES DEFINE_SYMBOL "")
        # TODO whether export symbols
    endif()
    
    if(ARG_SHAHRED AND UNIX)
        if(NOT APPLE AND ARG_SONAME)
            get_target_property(output_name ${name} OUTPUT_NAME)
            if(${output_name} STREQUAL "output_name-NOTFOUND")
                set(output_name ${name})
            endif()
            set(library_name ${output_name}-${PDK_VERSION_MAJOR}.${PDK_VERSION_MINOR}${PDK_VERSION_SUFFIX})
            set(api_name ${output_name}-${PDK_VERSION_MAJOR}.${PDK_VERSION_MINOR}.${PDK_VERSION_PATCH}${PDK_VERSION_SUFFIX})
            set_target_properties(${name} PROPERTIES OUTPUT_NAME ${library_name})
            pdk_install_library_symlink(${api_name} ${library_name} SHARED
                COMPONENT ${name}
                ALWAYS_GENERATE)
            pdk_install_library_symlink(${output_name} ${library_name} SHARED
                COMPONENT ${name}
                ALWAYS_GENERATE)
        endif()
    endif()
    
    if(ARG_STATIC)
        set(libtype INTERFACE)
    else()
        # We can use PRIVATE since SO knows its dependent libs.
        set(libtype PRIVATE)
    endif()
    
    target_link_libraries(${name} ${libtype}
        ${ARG_LINK_LIBS})
    
    if(PDK_COMMON_DEPENDS)
        add_dependencies(${name} ${PDK_COMMON_DEPENDS})
        # Add dependencies also to objlibs.
        # CMake issue 14747 --  add_dependencies() might be ignored to objlib's user.
        foreach(objlib ${objlibs})
            add_dependencies(${objlib} ${PDK_COMMON_DEPENDS})
        endforeach()
    endif()
    
    if(ARG_SHARED OR ARG_MODULE)
        pdk_externalize_debuginfo(${name})
    endif()
endfunction()

macro(pdk_add_executable name)
    cmake_parse_arguments(ARG "DISABLE_PDK_LINK_PDK_DYLIB;IGNORE_EXTERNALIZE_DEBUGINFO;NO_INSTALL_RPATH" "" "DEPENDS" ${ARGN})
    pdk_process_sources(ALL_FILES ${ARG_UNPARSED_ARGUMENTS})
    list(APPEND PDK_COMMON_DEPENDS ${ARG_DEPENDS})
    # Generate objlib
    if(PDK_ENABLE_OBJLIB)
        # Generate an obj library for both targets.
        set(obj_name "obj.${name}")
        add_library(${obj_name} OBJECT EXCLUDE_FROM_ALL ${ALL_FILES})
        pdk_update_compile_flags(${obj_name})
        set(ALL_FILES "$<TARGET_OBJECTS:${obj_name}>")
        set_target_properties(${obj_name} PROPERTIES FOLDER "Object Libraries")
    endif()
    pdk_add_windows_version_resource_file(ALL_FILES ${ALL_FILES})
    if(XCODE)
        # Note: the dummy.cpp source file provides no definitions. However,
        # it forces Xcode to properly link the static library.
        list(APPEND ALL_FILES "${PDK_MAIN_SRC_DIR}/cmake/dummy.cpp")
    endif()
    if(EXCLUDE_FROM_ALL)
        add_executable(${name} EXCLUDE_FROM_ALL ${ALL_FILES})
    else()
        add_executable(${name} ${ALL_FILES})
    endif()
    
    if(NOT ARG_NO_INSTALL_RPATH)
        pdk_setup_rpath(${name})
    endif()
    
    if(DEFINED windows_resource_file)
        pdk_set_windows_version_resource_properties(${name} ${windows_resource_file})
    endif()
    
    # $<TARGET_OBJECTS> doesn't require compile flags.
    if(NOT PDK_ENABLE_OBJLIB)
        pdk_update_compile_flags(${name})
    endif()
    pdk_add_link_opts(${name})
    
    # Do not add -Dname_EXPORTS to the command-line when building files in this
    # target. Doing so is actively harmful for the modules build because it
    # creates extra module variants, and not useful because we don't use these
    # macros.
    set_target_properties(${name} PROPERTIES DEFINE_SYMBOL "")
    if (PDK_LINK_PDK_DYLIB AND NOT ARG_DISABLE_PDK_LINK_PDK_DYLIB)
        set(USE_SHARED USE_SHARED)
    endif()
    set(EXCLUDE_FROM_ALL OFF)
    
    pdk_set_output_directory(${name} BINARY_DIR ${PDK_RUNTIME_OUTPUT_INTDIR} LIBRARY_DIR ${PDK_LIBRARY_OUTPUT_INTDIR})
    if(PDK_COMMON_DEPENDS)
        add_dependencies(${name} ${PDK_COMMON_DEPENDS})
    endif(PDK_COMMON_DEPENDS)
    
    if(NOT ARG_IGNORE_EXTERNALIZE_DEBUGINFO)
        pdk_externalize_debuginfo(${name})
    endif()
    
    if (PDK_PTHREAD_LIB)
        # libpthreads overrides some standard library symbols, so main
        # executable must be linked with it in order to provide consistent
        # API for all shared libaries loaded by this executable.
        target_link_libraries(${name} ${PDK_PTHREAD_LIB})
    endif()
endmacro()

function(pdk_setup_rpath name)
    if(CMAKE_INSTALL_RPATH)
        return()
    endif()
    if(PDK_INSTALL_PREFIX AND NOT (PDK_INSTALL_PREFIX STREQUAL CMAKE_INSTALL_PREFIX))
        set(extra_libdir ${PDK_LIBRARY_DIR})
    elseif(PDK_BUILD_LIBRARY_DIR)
        set(extra_libdir ${PDK_LIBRARY_DIR})
    endif()
    
    if(APPLE)
        set(_install_name_dir INSTALL_NAME_DIR "@rpath")
        set(_install_rpath "@loader_path/../lib" ${extra_libdir})
    elseif(UNIX)
        set(_install_rpath "\$ORIGIN/../lib${PDK_LIBDIR_SUFFIX}" ${extra_libdir})
        if(${CMAKE_SYSTEM_NAME} MATCHES "(FreeBSD|DragonFly)")
            set_property(TARGET ${name} APPEND_STRING PROPERTY
                LINK_FLAGS " -Wl,-z,origin ")
        elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND NOT PDK_LINKER_IS_GOLD)
            # $ORIGIN is not interpreted at link time by ld.bfd
            set_property(TARGET ${name} APPEND_STRING PROPERTY
                LINK_FLAGS " -Wl,-rpath-link,${PDK_LIBRARY_OUTPUT_INTDIR} ")
        endif()
    else()
        return()
    endif()
    set_target_properties(${name} PROPERTIES
        BUILD_WITH_INSTALL_RPATH On
        INSTALL_RPATH "${_install_rpath}"
        ${_install_name_dir})
endfunction()

function(pdk_install_library_symlink name dest type)
    cmake_parse_arguments(ARG "ALWAYS_GENERATE" "COMPONENT" "" ${ARGN})
    foreach(path ${CMAKE_MODULE_PATH})
        if(EXISTS ${path}/LibpdkInstallSymlink.cmake)
            set(INSTALL_SYMLINK ${path}/LibpdkInstallSymlink.cmake)
            break()
        endif()
    endforeach()
    set(component ${ARG_COMPONENT})
    if(NOT component)
        set(component ${name})
    endif()
    set(full_name ${CMAKE_${type}_LIBRARY_PREFIX}${name}${CMAKE_${type}_LIBRARY_SUFFIX})
    set(full_dest ${CMAKE_${type}_LIBRARY_PREFIX}${dest}${CMAKE_${type}_LIBRARY_SUFFIX})
    
    set(output_dir lib${PDK_LIBDIR_SUFFIX})
    if(WIN32 AND "${type}" STREQUAL "SHARED")
        set(output_dir bin)
    endif()
    
    install(SCRIPT ${INSTALL_SYMLINK}
        CODE "pdk_install_symlink(${full_name} ${full_dest} ${output_dir})"
        COMPONENT ${component})
    
    if (NOT CMAKE_CONFIGURATION_TYPES AND NOT ARG_ALWAYS_GENERATE)
        add_custom_target(install-${name}
            DEPENDS ${name} ${dest} install-${dest}
            COMMAND "${CMAKE_COMMAND}"
            -DCMAKE_INSTALL_COMPONENT=${name}
            -P "${CMAKE_BINARY_DIR}/cmake_install.cmake")
    endif()
endfunction()

function(pdk_externalize_debuginfo name)
    if(NOT PDK_EXTERNALIZE_DEBUGINFO)
        return()
    endif()
    
    if(NOT PDK_EXTERNALIZE_DEBUGINFO_SKIP_STRIP)
        if(APPLE)
            set(strip_command COMMAND xcrun strip -Sxl $<TARGET_FILE:${name}>)
        else()
            set(strip_command COMMAND strip -gx $<TARGET_FILE:${name}>)
        endif()
    endif()
    
    if(APPLE)
        if(CMAKE_CXX_FLAGS MATCHES "-flto"
                OR CMAKE_CXX_FLAGS_${UPPERCASE_CMAKE_BUILD_TYPE} MATCHES "-flto")
            set(lto_object ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${name}-lto.o)
            set_property(TARGET ${name} APPEND_STRING PROPERTY
                LINK_FLAGS " -Wl,-object_path_lto,${lto_object}")
        endif()
        add_custom_command(TARGET ${name} POST_BUILD
            COMMAND xcrun dsymutil $<TARGET_FILE:${name}>
            ${strip_command})
    else()
        add_custom_command(TARGET ${name} POST_BUILD
            COMMAND objcopy --only-keep-debug $<TARGET_FILE:${name}> $<TARGET_FILE:${name}>.debug
            ${strip_command} -R .gnu_debuglink
            COMMAND objcopy --add-gnu-debuglink=$<TARGET_FILE:${name}>.debug $<TARGET_FILE:${name}>)
    endif()
endfunction()

# Generic support for adding a unittest.
function(pdk_add_unittest test_suite test_name)
    if(NOT PDK_ENABLE_UNITTEST)
        set(EXCLUDE_FROM_ALL ON)
    endif()
    # Our current version of gtest does not properly recognize C++11 support
    # with MSVC, so it falls back to tr1 / experimental classes.  Since Libpdk
    # itself requires C++11, we can safely force it on unconditionally so that
    # we don't have to fight with the buggy gtest check.
    add_definitions(-DGTEST_LANG_CXX11=1)
    add_definitions(-DGTEST_HAS_TR1_TUPLE=0)
    
    if(PDK_FOUND_NATIVE_GTEST)
        include_directories(${GTEST_INCLUDE_DIRS})
        set(PDK_TEMP_GTEST_LIBS ${GTEST_BOTH_LIBRARIES})
    else()
        include_directories(${PDK_THIRDPARTY_DIR}/unittest/googletest/include)
        include_directories(${PDK_THIRDPARTY_DIR}/unittest/googlemock/include)
        set(PDK_TEMP_GTEST_LIBS gtest_main gtest)
    endif()
    
    include_directories(${PDK_THIRDPARTY_DIR}/unittest/googletest/include)
    include_directories(${PDK_THIRDPARTY_DIR}/unittest/googlemock/include)
    
    if (NOT PDK_ENABLE_THREADS)
        list(APPEND PDK_COMPILE_DEFINITIONS GTEST_HAS_PTHREAD=0)
    endif ()
    
    if (PDK_SUPPORTS_VARIADIC_MACROS_FLAG)
        list(APPEND PDK_COMPILE_FLAGS "-Wno-variadic-macros")
    endif ()
    # Some parts of gtest rely on this GNU extension, don't warn on it.
    if(PDK_SUPPORTS_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS_FLAG)
        list(APPEND PDK_COMPILE_FLAGS "-Wno-gnu-zero-variadic-macro-arguments")
    endif()
    
    pdk_add_executable(${test_name} IGNORE_EXTERNALIZE_DEBUGINFO NO_INSTALL_RPATH ${ARGN})
    set(outdir ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
    pdk_set_output_directory(${test_name} BINARY_DIR ${outdir} LIBRARY_DIR ${outdir})
    # libpthreads overrides some standard library symbols, so main
    # executable must be linked with it in order to provide consistent
    # API for all shared libaries loaded by this executable.
    target_link_libraries(${test_name} gtest_main gtest ${PDK_PTHREAD_LIB} libpdk)
    add_dependencies(${test_suite} ${test_name})
    get_target_property(test_suite_folder ${test_suite} FOLDER)
    if (NOT ${test_suite_folder} STREQUAL "NOTFOUND")
        set_property(TARGET ${test_name} PROPERTY FOLDER "${test_suite_folder}")
    endif ()
endfunction()
