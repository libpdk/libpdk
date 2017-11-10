cmake_minimum_required(VERSION 3.4.3 FATAL_ERROR)

# register zapi cmake modules, you must modify this according to your self zapi install prefix 
#list(APPEND CMAKE_PREFIX_PATH "/usr/local/lib/cmake/zapi")
# here find zendAPI lib
#find_package(zendAPI 0.0.1 EXACT REQUIRED CONFIG)

if(NOT DEFINED PDK_VERSION_MAJOR)
    set(PDK_VERSION_MAJOR 0)
endif()

if(NOT DEFINED PDK_VERSION_MINOR)
    set(PDK_VERSION_MINOR 0)
endif()

if(NOT DEFINED PDK_VERSION_PATCH)
    set(PDK_VERSION_PATCH 1)
endif()

if(NOT DEFINED PDK_VERSION_SUFFIX)
    set(PDK_VERSION_SUFFIX git)
endif()

if(NOT DEFINED PDK_PACKAGE_VERSION)
    set(PDK_PACKAGE_VERSION 
        "${PDK_VERSION_MAJOR}.${PDK_VERSION_MINOR}.${PDK_VERSION_PATCH}${PDK_VERSION_SUFFIX}")
endif()

# prohibit compile libpdk on Windows platform
if(CMAKE_HOST_WIN32)
    message(FATAL_ERROR "sorry, at this moment libpdk does not support Windows platform.")
endif()

project(libpdk 
    VERSION ${PDK_VERSION_MAJOR}.${PDK_VERSION_MINOR}.${PDK_VERSION_PATCH}
    LANGUAGES C CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected, default to Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (default Debug)." FORCE)
endif()

# This should only apply if you are both on an Apple host, and targeting Apple.
if(CMAKE_HOST_APPLE AND APPLE)
    # if CMAKE_LIBTOOL is not set, try and find it with xcrun or find_program
    if(NOT CMAKE_LIBTOOL)
        if(NOT CMAKE_XCRUN)
            find_program(CMAKE_XCRUN NAMES xcrun)
        endif()
        if(CMAKE_XCRUN)
            execute_process(COMMAND ${CMAKE_XCRUN} -find libtool
                OUTPUT_VARIABLE CMAKE_LIBTOOL
                OUTPUT_STRIP_TRAILING_WHITESPACE)
        endif()
        if(NOT CMAKE_LIBTOOL OR NOT EXISTS CMAKE_LIBTOOL)
            find_program(CMAKE_LIBTOOL NAMES libtool)
        endif()
    endif()
    get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    if(CMAKE_LIBTOOL)
        set(CMAKE_LIBTOOL ${CMAKE_LIBTOOL} CACHE PATH "libtool executable")
        message(STATUS "Found libtool - ${CMAKE_LIBTOOL}")
        execute_process(COMMAND ${CMAKE_LIBTOOL} -V
            OUTPUT_VARIABLE LIBTOOL_V_OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if("${LIBTOOL_V_OUTPUT}" MATCHES ".*cctools-([0-9.]+).*")
            string(REGEX REPLACE ".*cctools-([0-9.]+).*" "\\1" LIBTOOL_VERSION
                ${LIBTOOL_V_OUTPUT})
            if(NOT LIBTOOL_VERSION VERSION_LESS "862")
                set(LIBTOOL_NO_WARNING_FLAG "-no_warning_for_no_symbols")
            endif()
        endif()
        foreach(lang ${languages})
            set(CMAKE_${lang}_CREATE_STATIC_LIBRARY
                "${CMAKE_LIBTOOL} -static ${LIBTOOL_NO_WARNING_FLAG} -o <TARGET> \
                <LINK_FLAGS> <OBJECTS> ")
        endforeach()
    endif()
    
    # If DYLD_LIBRARY_PATH is set we need to set it on archiver commands
    if(DYLD_LIBRARY_PATH)
        set(dyld_envar "DYLD_LIBRARY_PATH=${DYLD_LIBRARY_PATH}")
        foreach(lang ${languages})
            foreach(cmd ${CMAKE_${lang}_CREATE_STATIC_LIBRARY})
                list(APPEND CMAKE_${lang}_CREATE_STATIC_LIBRARY_NEW
                    "${dyld_envar} ${cmd}")
            endforeach()
            set(CMAKE_${lang}_CREATE_STATIC_LIBRARY
                ${CMAKE_${lang}_CREATE_STATIC_LIBRARY_NEW})
        endforeach()
    endif()
endif()

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
    )

option(PDK_INSTALL_HEADERS "Install libpdk header files." ON)
option(PDK_INSTALL_BINUTILS_SYMLINKS
    "Install symlinks from the binutils tool names to the corresponding libpdk tools." OFF)
option(PDK_USE_FOLDERS "Enable solution folders in Visual Studio. Disable for Express versions." ON)

if(PDK_USE_FOLDERS)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

include(AddLibpdkDefinitions)
include(VersionFromGit)

set(PDK_PACKAGE_NAME libpdk)
set(PDK_PACKAGE_STRING "${PDK_PACKAGE_NAME} ${PDK_PACKAGE_VERSION}")
set(PDK_PACKAGE_BUGREPORT "http://libpdk.org/bugs/")

set(PDK_BUG_REPORT_URL "${PDK_PACKAGE_BUGREPORT}" CACHE STRING
    "Default URL where bug reports are to be submitted.")

# Sanity check our source directory to make sure that we are not trying to
# generate an in-source build (unless on MSVC_IDE, where it is ok), and to make
# sure that we don't have any stray generated files lying around in the tree
# (which would end up getting picked up by header search, instead of the correct
# versions).
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR AND NOT MSVC_IDE)
    message(FATAL_ERROR "In-source builds are not allowed.
        CMake would overwrite the makefiles distributed with libpdk.
        Please create a directory and run cmake from there, passing the path
        to this source directory as the last argument.
        This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
        Please delete them.")
endif()

string(TOUPPER "${CMAKE_BUILD_TYPE}" UPPERCASE_CMAKE_BUILD_TYPE)

if(CMAKE_BUILD_TYPE AND
        NOT UPPERCASE_CMAKE_BUILD_TYPE MATCHES "^(DEBUG|RELEASE|RELWITHDEBINFO|MINSIZEREL)$")
    message(FATAL_ERROR "Invalid value for CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

set(PDK_LIBDIR_SUFFIX "" CACHE STRING "Define suffix of library directory name (32/64).")

set(PDK_TOOLS_INSTALL_DIR "bin" CACHE STRING "Path for binary subdirectory (defaults to 'bin').")
mark_as_advanced(PDK_TOOLS_INSTALL_DIR)

set(PDK_RUNTIME_OUTPUT_INTDIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/bin)
set(PDK_LIBRARY_OUTPUT_INTDIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib${PDK_LIBDIR_SUFFIX})

if(WIN32 OR CYGWIN)
    # DLL platform -- put DLLs into bin.
    set(PDK_SHLIB_OUTPUT_INTDIR ${PDK_RUNTIME_OUTPUT_INTDIR})
else()
    set(PDK_SHLIB_OUTPUT_INTDIR ${PDK_LIBRARY_OUTPUT_INTDIR})
endif()

# Each of them corresponds to libpdk-config's.
set(PDK_TOOLS_BINARY_DIR ${PDK_RUNTIME_OUTPUT_INTDIR}) # --bindir
set(PDK_LIBRARY_DIR      ${PDK_LIBRARY_OUTPUT_INTDIR}) # --libdir
set(PDK_MAIN_SRC_DIR     ${CMAKE_CURRENT_SOURCE_DIR})  # --src-root
set(PDK_MAIN_INCLUDE_DIR ${PDK_MAIN_SRC_DIR}/include)  # --includedir
set(PDK_BINARY_DIR       ${CMAKE_CURRENT_BINARY_DIR})  # --prefix

# Note: PDK_CMAKE_PATH does not include generated files
set(PDK_CMAKE_PATH ${PDK_MAIN_SRC_DIR}/cmake/modules)
set(PDK_EXAMPLES_BINARY_DIR ${PDK_BINARY_DIR}/examples)
set(PDK_INCLUDE_DIR ${PDK_BINARY_DIR}/include)
set(PDK_THIRDPARTY_DIR ${PDK_MAIN_SRC_DIR}/thirdparty)

option(BUILD_SHARED_LIBS 
    "Build all libraries as shared libraries instead of static" ON)
option(PDK_PHPCFG_PATH "Specify the php-config path of host platform" "")
option(PDK_ENABLE_TERMINFO "Use terminfo database if available." ON)
option(PDK_ENABLE_LIBXML2 "Use libxml2 if available." ON)
option(PDK_ENABLE_THREADS "Use threads if available." ON)
option(PDK_ENABLE_ZLIB "Use zlib for compression/decompression if available." ON)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    option(PDK_ENABLE_MODULE_DEBUGGING "Compile with -gmodules." ON)
    option(PDK_ENABLE_LOCAL_SUBMODULE_VISIBILITY "Compile with -fmodules-local-submodule-visibility." OFF)
else()
    option(PDK_ENABLE_MODULE_DEBUGGING "Compile with -gmodules." OFF)
    option(PDK_ENABLE_LOCAL_SUBMODULE_VISIBILITY "Compile with -fmodules-local-submodule-visibility." ON)
endif()
option(PDK_ENABLE_PIC "Build Position-Independent Code" ON)
option(PDK_ENABLE_WARNINGS "Enable compiler warnings." ON)
option(PDK_ENABLE_LLD "Use lld as C and C++ linker." OFF)
option(PDK_ENABLE_MODULES "Compile with C++ modules enabled." OFF)
option(PDK_ENABLE_PEDANTIC "Compile with pedantic enabled." ON)
option(PDK_ENABLE_WERROR "Fail and stop if a warning is triggered." OFF)
option(PDK_EXTERNALIZE_DEBUGINFO
    "Generate dSYM files and strip executables and libraries (Darwin Only)" OFF)
option(PDK_ENABLE_RUNTIME_TEST "Whether enable runtime test" ON)
option(PDK_ENABLE_UNITTEST "Whether enable unit test" ON)

# Define an option controlling whether we should build for 32-bit on 64-bit
# platforms, where supported.
if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT WIN32)
    # TODO: support other platforms and toolchains.
    option(PDK_BUILD_32_BITS "Build 32 bits executables and libraries." OFF)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    option(PDK_ENABLE_MODULE_DEBUGGING "Compile with -gmodules." ON)
    option(PDK_ENABLE_LOCAL_SUBMODULE_VISIBILITY "Compile with -fmodules-local-submodule-visibility." OFF)
else()
    option(PDK_ENABLE_MODULE_DEBUGGING "Compile with -gmodules." OFF)
    option(PDK_ENABLE_LOCAL_SUBMODULE_VISIBILITY "Compile with -fmodules-local-submodule-visibility." ON)
endif()

option(PDK_ENABLE_PEDANTIC "Compile with pedantic enabled." ON)
option(PDK_ENABLE_WERROR "Fail and stop if a warning is triggered." OFF)

if(NOT UPPERCASE_CMAKE_BUILD_TYPE STREQUAL "DEBUG")
    option(PDK_ENABLE_ASSERTIONS "Enable assertions" OFF)
else()
    option(PDK_ENABLE_ASSERTIONS "Enable assertions" ON)
endif()

# Define an option controlling whether we should build for 32-bit on 64-bit
# platforms, where supported.
if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT WIN32)
    option(PDK_BUILD_32_BITS "Build 32 bits executables and libraries." OFF)
endif()

# Define the default arguments to use with 'lit', and an option for the user to
# override.
set(PDK_LIT_ARGS_DEFAULT "-sv")
if(MSVC_IDE OR XCODE)
    set(PDK_LIT_ARGS_DEFAULT "${PDK_LIT_ARGS_DEFAULT} --no-progress-bar")
endif()
set(PDK_LIT_ARGS "${PDK_LIT_ARGS_DEFAULT}" CACHE STRING "Default options for lit")

# On Win32 hosts, provide an option to specify the path to the GnuWin32 tools.
if(WIN32 AND NOT CYGWIN)
    set(PDK_LIT_TOOLS_DIR "" CACHE PATH "Path to GnuWin32 tools")
endif()

option(PDK_BUILD_TOOLS
    "Build the libpdk tools. If OFF, just generate build targets." ON)

option(PDK_BUILD_UNITTESTS
    "Build libpdk unit tests. If OFF, just generate build targets." OFF)

if(PDK_BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

# All options referred to from HandleLibpdkOptions have to be specified
# BEFORE this include, otherwise options will not be correctly set on
# first cmake run
include(config-ix)

# They are not referenced. See set_output_directory().
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PDK_BINARY_DIR}/bin )
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PDK_BINARY_DIR}/lib${PDK_LIBDIR_SUFFIX})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PDK_BINARY_DIR}/lib${PDK_LIBDIR_SUFFIX})

if(APPLE AND DARWIN_LTO_LIBRARY)
    set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -Wl,-lto_library -Wl,${DARWIN_LTO_LIBRARY}")
    set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-lto_library -Wl,${DARWIN_LTO_LIBRARY}")
    set(CMAKE_MODULE_LINKER_FLAGS
        "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-lto_library -Wl,${DARWIN_LTO_LIBRARY}")
endif()

# Work around a broken bfd ld behavior. When linking a binary with a
# foo.so library, it will try to find any library that foo.so uses and
# check its symbols. This is wasteful (the check was done when foo.so
# was created) and can fail since it is not the dynamic linker and
# doesn't know how to handle search paths correctly.
if (UNIX AND NOT APPLE AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "SunOS|AIX")
    set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -Wl,-allow-shlib-undefined")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(BEFORE ${PDK_INCLUDE_DIR} ${PDK_MAIN_INCLUDE_DIR} ${PDK_EXTRA_INCLUDE_DIR})

if(${CMAKE_SYSTEM_NAME} MATCHES "(FreeBSD|DragonFly)")
    include_directories(SYSTEM "/usr/local/include")
    link_directories("/usr/local/lib")
endif()

add_subdirectory(cmake/modules)

include(HandleLibpdkOptions)
include(LibpdkTargetFuncs)

# force setup some zapi variable
set(BUILD_SHARED_LIBS OFF)
set(ZAPI_OPT_ENABLE_UNIT_TESTS OFF)
set(ZAPI_OPT_ENABLE_RUNTIME_TESTS OFF)
add_subdirectory(thirdparty/zendapi)
set(BUILD_SHARED_LIBS ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND PDK_ENABLE_UNITTEST)
    set(PDK_FOUND_NATIVE_GTEST OFF)
    find_package(GTest)
    if(GTEST_FOUND)
        set(PDK_FOUND_NATIVE_GTEST ON)
    endif()
    if(NOT PDK_FOUND_NATIVE_GTEST)
        add_subdirectory(thirdparty/unittest)
    endif()
    add_subdirectory(unittests)
endif()

# configure head files
configure_file(
    ${PDK_MAIN_INCLUDE_DIR}/pdk/Config.h.in
    ${PDK_INCLUDE_DIR}/pdk/Config.h
    NEWLINE_STYLE UNIX)

configure_file(
    ${PDK_MAIN_INCLUDE_DIR}/pdk/Version.h.in
    ${PDK_INCLUDE_DIR}/pdk/Version.h
    NEWLINE_STYLE UNIX)

set(PDK_HEADERS "")
set(PDK_EXT_HEADERS "")

# we include our project source directories
add_subdirectory(include)
add_subdirectory(pdkext)

add_subdirectory(src)
add_subdirectory(benchmarks)
add_subdirectory(examples)
