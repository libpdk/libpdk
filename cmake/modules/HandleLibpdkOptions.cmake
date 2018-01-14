# This CMake module is responsible for interpreting the user defined PDK_
# options and executing the appropriate CMake commands to realize the users'
# selections.

# This is commonly needed so make sure it's defined before we include anything
# else.

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(Utils)

string(TOUPPER "${CMAKE_BUILD_TYPE}" UPPERCASE_CMAKE_BUILD_TYPE)

if(CMAKE_LINKER MATCHES "lld-link.exe" OR (WIN32 AND PDK_USE_LINKER STREQUAL "lld"))
    set(PDK_LINKER_IS_LLD_LINK TRUE)
else()
    set(PDK_LINKER_IS_LLD_LINK FALSE)
endif()

set(PDK_ENABLE_LTO OFF CACHE STRING 
    "Build libpdk with LTO. May be specified as Thin or Full to use a particular kind of LTO")

string(TOUPPER "${PDK_ENABLE_LTO}" UPPERCASE_LLVM_ENABLE_LTO)

# Ninja Job Pool support
# The following only works with the Ninja generator in CMake >= 3.0.
set(PDK_PARALLEL_COMPILE_JOBS "" CACHE STRING
    "Define the maximum number of concurrent compilation jobs.")

if(PDK_PARALLEL_COMPILE_JOBS)
    if(NOT CMAKE_MAKE_PROGRAM MATCHES "ninja")
        message(WARNING "Job pooling is only available with Ninja generators.")
    else()
        set_property(GLOBAL APPEND PROPERTY JOB_POOLS compile_job_pool=${PDK_PARALLEL_COMPILE_JOBS})
        set(CMAKE_JOB_POOL_COMPILE compile_job_pool)
    endif()
endif()

set(PDK_PARALLEL_LINK_JOBS "" CACHE STRING
    "Define the maximum number of concurrent link jobs.")
if(CMAKE_MAKE_PROGRAM MATCHES "ninja")
    if(NOT PDK_PARALLEL_LINK_JOBS AND UPPERCASE_PDK_ENABLE_LTO STREQUAL "THIN")
        message(STATUS "ThinLTO provides its own parallel linking - limiting parallel link jobs to 2.")
        set(PDK_PARALLEL_LINK_JOBS "2")
    endif()
    if(PDK_PARALLEL_LINK_JOBS)
        set_property(GLOBAL APPEND PROPERTY JOB_POOLS link_job_pool=${PDK_PARALLEL_LINK_JOBS})
        set(CMAKE_JOB_POOL_LINK link_job_pool)
    endif()
elseif(PDK_PARALLEL_LINK_JOBS)
    message(WARNING "Job pooling is only available with Ninja generators.")
endif()

if (PDK_LINKER_IS_LLD_LINK)
    # Pass /MANIFEST:NO so that CMake doesn't run mt.exe on our binaries.  Adding
    # manifests with mt.exe breaks LLD's symbol tables and takes as much time as
    # the link. See PR24476.
    append("/MANIFEST:NO"
        CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
endif()

if(PDK_ENABLE_ASSERTIONS)
    # MSVC doesn't like _DEBUG on release builds. See PR 4379.
    if(NOT MSVC)
        add_definitions(-D_DEBUG)
    endif()
    # On non-Debug builds cmake automatically defines NDEBUG, so we
    # explicitly undefine it:
    if(NOT UPPERCASE_CMAKE_BUILD_TYPE STREQUAL "DEBUG")
        add_definitions(-UNDEBUG)
        # Also remove /D NDEBUG to avoid MSVC warnings about conflicting defines.
        foreach(flags_var_to_scrub 
                CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_RELWITHDEBINFO
                CMAKE_CXX_FLAGS_MINSIZEREL
                CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_RELWITHDEBINFO
                CMAKE_C_FLAGS_MINSIZEREL)
            string (REGEX REPLACE "(^| )[/-]D *NDEBUG($| )" " "
                "${flags_var_to_scrub}" "${${flags_var_to_scrub}}")
        endforeach()
    endif()
endif()

if(WIN32)
    set(PDK_HAVE_LINK_VERSION_SCRIPT 0)
    if(CYGWIN)
        set(PDK_ON_WIN32 0)
        set(PDK_ON_UNIX 1)
    else(CYGWIN)
        set(PDK_ON_WIN32 1)
        set(PDK_ON_UNIX 0)
    endif(CYGWIN)
else(WIN32)
    if(UNIX)
        set(PDK_ON_WIN32 0)
        set(PDK_ON_UNIX 1)
        if(APPLE OR ${CMAKE_SYSTEM_NAME} MATCHES "AIX")
            set(PDK_HAVE_LINK_VERSION_SCRIPT 0)
        else()
            set(PDK_HAVE_LINK_VERSION_SCRIPT 1)
        endif()
    else(UNIX)
        message(SEND_ERROR "Unable to determine platform")
    endif(UNIX)
endif(WIN32)

# We use *.dylib rather than *.so on darwin.
set(PDK_PLUGIN_EXT ${CMAKE_SHARED_LIBRARY_SUFFIX})

if(APPLE)
    if(PDK_ENABLE_LLD AND PDK_ENABLE_LTO)
        message(FATAL_ERROR "lld does not support LTO on Darwin")
    endif()
    # Darwin-specific linker flags for loadable modules.
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-flat_namespace -Wl,-undefined -Wl,suppress")
endif()

if(PDK_ENABLE_LLD)
    if(PDK_USE_LINKER)
        message(FATAL_ERROR "PDK_ENABLE_LLD and PDK_USE_LINKER can't be set at the same time")
    endif()
    set(PDK_USE_LINKER "lld")
endif()

if(PDK_USE_LINKER)
    set(PDK_TEMP_OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -fuse-ld=${PDK_USE_LINKER}")
    check_cxx_source_compiles("int main() { return 0; }" PDK_TEMP_CXX_SUPPORTS_CUSTOM_LINKER)
    if (NOT PDK_TEMP_CXX_SUPPORTS_CUSTOM_LINKER)
        message(FATAL_ERROR "Host compiler does not support '-fuse-ld=${PDK_USE_LINKER}'")
    endif()
    set(CMAKE_REQUIRED_FLAGS ${PDK_TEMP_OLD_CMAKE_REQUIRED_FLAGS})
    pdk_append("-fuse-ld=${PDK_USE_LINKER}"
        CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
endif()

if(PDK_ENABLE_PIC)
    if(XCODE)
        # Xcode has -mdynamic-no-pic on by default, which overrides -fPIC. I don't
        # know how to disable this, so just force ENABLE_PIC off for now.
        message(WARNING "-fPIC not supported with Xcode.")
    elseif(WIN32 OR CYGWIN)
        # On Windows all code is PIC. MinGW warns if -fPIC is used.
    else()
        pdk_add_flag_or_print_warning("-fPIC" FPIC)
    endif()
endif()

if(NOT WIN32 AND NOT CYGWIN)
    # MinGW warns if -fvisibility-inlines-hidden is used.
    check_cxx_compiler_flag("-fvisibility-inlines-hidden" PDK_TEMP_SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG)
    pdk_append_if(PDK_TEMP_SUPPORTS_FVISIBILITY_INLINES_HIDDEN_FLAG "-fvisibility-inlines-hidden" CMAKE_CXX_FLAGS)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT WIN32)
    # TODO: support other platforms and toolchains.
    if(PDK_BUILD_32_BITS)
        message(STATUS "Building 32 bits executables and libraries.")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -m32")
        # FIXME: CMAKE_SIZEOF_VOID_P is still 8
        add_definitions(-D_LARGEFILE_SOURCE)
        add_definitions(-D_FILE_OFFSET_BITS=64)
    endif(PDK_BUILD_32_BITS)
endif(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT WIN32)

# If building on a GNU specific 32-bit system, make sure off_t is 64 bits
# so that off_t can stored offset > 2GB.
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # FIXME: It isn't handled in PDK_BUILD_32_BITS.
    add_definitions(-D_LARGEFILE_SOURCE)
    add_definitions(-D_FILE_OFFSET_BITS=64)
endif()

if(XCODE)
    # For Xcode enable several build settings that correspond to
    # many warnings that are on by default in Clang but are
    # not enabled for historical reasons.  For versions of Xcode
    # that do not support these options they will simply
    # be ignored.
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_RETURN_TYPE "YES")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_NEWLINE "YES")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_VALUE "YES")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_VARIABLE "YES")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_SIGN_COMPARE "YES")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNUSED_FUNCTION "YES")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_INITIALIZER_NOT_FULLY_BRACKETED "YES")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_HIDDEN_VIRTUAL_FUNCTIONS "YES")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_UNINITIALIZED_AUTOS "YES")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_BOOL_CONVERSION "YES")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_EMPTY_BODY "YES")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_ENUM_CONVERSION "YES")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_INT_CONVERSION "YES")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_CONSTANT_CONVERSION "YES")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_NON_VIRTUAL_DESTRUCTOR "YES")
endif()

# On Win32 using MS tools, provide an option to set the number of parallel jobs
# to use.
if(MSVC_IDE)
    set(PDK_COMPILER_JOBS "0" CACHE STRING
        "Number of parallel compiler jobs. 0 means use all processors. Default is 0.")
    if(NOT PDK_COMPILER_JOBS STREQUAL "1")
        if(PDK_COMPILER_JOBS STREQUAL "0")
            add_definitions( /MP )
        else()
            message(STATUS "Number of parallel compiler jobs set to " ${PDK_COMPILER_JOBS})
            add_definitions(/MP${PDK_COMPILER_JOBS})
        endif()
    else()
        message(STATUS "Parallel compilation disabled")
    endif()
endif()

# set stack reserved size to ~10MB

if(MSVC)
    # CMake previously automatically set this value for MSVC builds, but the
    # behavior was changed in CMake 2.8.11 (Issue 12437) to use the MSVC default
    # value (1 MB) which is not enough for us in tasks such as parsing recursive
    # C++ templates in Clang.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:10000000")
elseif(MINGW) # FIXME: Also cygwin?
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,16777216")
    # Pass -mbig-obj to mingw gas on Win64. COFF has a 2**16 section limit, and
    # on Win64, every COMDAT function creates at least 3 sections: .text, .pdata,
    # and .xdata.
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        pdk_append("-Wa,-mbig-obj" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    endif()
endif()

if(MSVC)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0)
        # For MSVC 2013, disable iterator null pointer checking in debug mode,
        # especially so std::equal(nullptr, nullptr, nullptr) will not assert.
        add_definitions("-D_DEBUG_POINTER_IMPL=")
    endif()
endif()

if(PDK_ENABLE_WARNINGS AND PDK_COMPILER_IS_GCC_COMPATIBLE)
    pdk_append("-Wall -W -Wno-unused-parameter -Wwrite-strings" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    pdk_append("-Wcast-qual" CMAKE_CXX_FLAGS)
    # Turn off missing field initializer warnings for gcc to avoid noise from
    # false positives with empty {}. Turn them on otherwise (they're off by
    # default for clang).
    check_cxx_compiler_flag("-Wmissing-field-initializers" PDK_TEMP_CXX_SUPPORTS_MISSING_FIELD_INITIALIZERS_FLAG)
    if(PDK_TEMP_CXX_SUPPORTS_MISSING_FIELD_INITIALIZERS_FLAG)
        if(CMAKE_COMPILER_IS_GNUCXX)
            pdk_append("-Wno-missing-field-initializers" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
        else()
            pdk_append("-Wmissing-field-initializers" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
        endif()
    endif()
    
    if(PDK_ENABLE_PEDANTIC AND PDK_COMPILER_IS_GCC_COMPATIBLE)
        pdk_append("-pedantic" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
        pdk_append("-Wno-long-long" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    endif()
    pdk_add_flag_if_supported("-Wcovered-switch-default" COVERED_SWITCH_DEFAULT_FLAG)
    pdk_append_if(PDK_USE_NO_UNINITIALIZED "-Wno-uninitialized" CMAKE_CXX_FLAGS)
    pdk_append_if(PDK_USE_NO_MAYBE_UNINITIALIZED "-Wno-maybe-uninitialized" CMAKE_CXX_FLAGS)
    
    # Check if -Wnon-virtual-dtor warns even though the class is marked final.
    # If it does, don't add it. So it won't be added on clang 3.4 and older.
    # This also catches cases when -Wnon-virtual-dtor isn't supported by
    # the compiler at all.  This flag is not activated for gcc since it will
    # incorrectly identify a protected non-virtual base when there is a friend
    # declaration. Don't activate this in general on Windows as this warning has
    # too many false positives on COM-style classes, which are destroyed with
    # Release() (PR32286).
    if(NOT CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32)
        set(PDK_TEMP_OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
        set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -std=c++11 -Werror=non-virtual-dtor")
        check_cxx_source_compiles("class base {public: virtual void anchor();protected: ~base();};
            class derived final : public base { public: ~derived();};
            int main() { return 0; }"
            PDK_TEMP_CXX_WONT_WARN_ON_FINAL_NONVIRTUALDTOR)
        set(CMAKE_REQUIRED_FLAGS ${PDK_TEMP_OLD_CMAKE_REQUIRED_FLAGS})
        pdk_append_if(PDK_TEMP_CXX_WONT_WARN_ON_FINAL_NONVIRTUALDTOR
            "-Wnon-virtual-dtor" CMAKE_CXX_FLAGS)
    endif()
    # Enable -Wdelete-non-virtual-dtor if available.
    pdk_add_flag_if_supported("-Wdelete-non-virtual-dtor" DELETE_NON_VIRTUAL_DTOR_FLAG)
    # Check if -Wcomment is OK with an // comment ending with '\' if the next
    # line is also a // comment.
    set(PDK_TEMP_OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -Werror -Wcomment")
    check_c_source_compiles("// \\\\\\n//\\nint main() {return 0;}"
        PDK_TEMP_C_WCOMMENT_ALLOWS_LINE_WRAP)
    set(CMAKE_REQUIRED_FLAGS ${PDK_TEMP_OLD_CMAKE_REQUIRED_FLAGS})
    if (NOT PDK_TEMP_C_WCOMMENT_ALLOWS_LINE_WRAP)
        pdk_append("-Wno-comment" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    endif()
    # Enable -Wstring-conversion to catch misuse of string literals.
    pdk_add_flag_if_supported("-Wstring-conversion" STRING_CONVERSION_FLAG)
    
endif(PDK_ENABLE_WARNINGS AND PDK_COMPILER_IS_GCC_COMPATIBLE)

if(PDK_COMPILER_IS_GCC_COMPATIBLE)
    pdk_append_if(PDK_ENABLE_WERROR "-Werror" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    pdk_append_if(PDK_ENABLE_WERROR "-Wno-error" CMAKE_REQUIRED_FLAGS)
    pdk_add_flag_if_supported("-Werror=date-time" WERROR_DATE_TIME)
    pdk_add_flag_if_supported("-Wno-unused-function" WERROR_UNUSED_FUNC)
    pdk_add_flag_if_supported("-Wno-unneeded-internal-declaration" WERROR_NO_INTERNAL_DECLEARATION)
    pdk_add_flag_if_supported("-Werror=unguarded-availability-new" WERROR_UNGUARDED_AVAILABILITY_NEW)
    check_cxx_compiler_flag("-std=c++17" PDK_TEMP_CXX_SUPPORTS_CXX17)
    if(PDK_TEMP_CXX_SUPPORTS_CXX17)
        if (CYGWIN OR MINGW)
            # MinGW and Cygwin are a bit stricter and lack things like
            # 'strdup', 'stricmp', etc in c++11 mode.
            pdk_append("-std=gnu++17" CMAKE_CXX_FLAGS)
        else()
            pdk_append("-std=c++17" CMAKE_CXX_FLAGS)
        endif()
    else()
        message(FATAL_ERROR "libpdk requires C++17 support but the '-std=c++17' flag isn't supported.")
    endif()
    
    if(PDK_ENABLE_MODULES)
        set(PKD_TEMP_OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
        set(module_flags "-fmodules -fmodules-cache-path=${PROJECT_BINARY_DIR}/module.cache")
        if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            # On Darwin -fmodules does not imply -fcxx-modules.
            set(module_flags "${module_flags} -fcxx-modules")
        endif()
        if(PDK_ENABLE_LOCAL_SUBMODULE_VISIBILITY)
            set(module_flags "${module_flags} -Xclang -fmodules-local-submodule-visibility")
        endif()
        if (PDK_ENABLE_MODULE_DEBUGGING AND ((UPPERCASE_CMAKE_BUILD_TYPE STREQUAL "DEBUG") OR
                (UPPERCASE_CMAKE_BUILD_TYPE STREQUAL "RELWITHDEBINFO")))
            set(module_flags "${module_flags} -gmodules")
        endif()
        set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${module_flags}")
        
        # Check that we can build code with modules enabled, and that repeatedly
        # including <cassert> still manages to respect NDEBUG properly.
        check_cxx_source_compiles("#undef NDEBUG
            #include <cassert>
            #define NDEBUG
            #include <cassert>
            int main() { assert(this code is not compiled); }"
            PDK_TEMP_CXX_SUPPORTS_MODULES)
        set(CMAKE_REQUIRED_FLAGS ${PKD_TEMP_OLD_CMAKE_REQUIRED_FLAGS})
        if(PDK_TEMP_CXX_SUPPORTS_MODULES)
            pdk_append("${module_flags}" CMAKE_CXX_FLAGS)
        else()
            message(FATAL_ERROR "PDK_ENABLE_MODULES is not supported by this compiler")
        endif()
    endif(PDK_ENABLE_MODULES)
endif(PDK_COMPILER_IS_GCC_COMPATIBLE)

if (PDK_COMPILER_IS_GCC_COMPATIBLE AND NOT PDK_ENABLE_WARNINGS)
    pdk_append("-w" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
endif()

add_definitions(-D__STDC_CONSTANT_MACROS)
add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-D__STDC_LIMIT_MACROS)

# clang doesn't print colored diagnostics when invoked from Ninja
if (UNIX AND
        CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND
        CMAKE_GENERATOR STREQUAL "Ninja")
    pdk_append("-fcolor-diagnostics" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
endif()

# lld doesn't print colored diagnostics when invoked from Ninja
if(UNIX AND CMAKE_GENERATOR STREQUAL "Ninja")
    pdk_check_linker_flag("-Wl,--color-diagnostics" PDK_TEMP_LINKER_SUPPORTS_COLOR_DIAGNOSTICS)
    append_if(PDK_TEMP_LINKER_SUPPORTS_COLOR_DIAGNOSTICS "-Wl,--color-diagnostics"
        CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
endif()

# Add flags for add_dead_strip().
# FIXME: With MSVS, consider compiling with /Gy and linking with /OPT:REF?
# But MinSizeRel seems to add that automatically, so maybe disable these
# flags instead if LLVM_NO_DEAD_STRIP is set.
if(NOT CYGWIN AND NOT WIN32)
    if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND
            NOT UPPERCASE_CMAKE_BUILD_TYPE STREQUAL "DEBUG")
        check_c_compiler_flag("-Werror -fno-function-sections" PDK_TEMP_C_SUPPORTS_FNO_FUNCTION_SECTIONS)
        if (PDK_TEMP_C_SUPPORTS_FNO_FUNCTION_SECTIONS)
            # Don't add -ffunction-section if it can be disabled with -fno-function-sections.
            # Doing so will break sanitizers.
            pdk_add_flag_if_supported("-ffunction-sections" FFUNCTION_SECTIONS)
        endif()
        pdk_add_flag_if_supported("-fdata-sections" FDATA_SECTIONS)
    endif()
endif()

if(MSVC)
    # Remove flags here, for exceptions and RTTI.
    # Each target property or source property should be responsible to control
    # them.
    # CL.EXE complains to override flags like "/GR /GR-".
    string(REGEX REPLACE "(^| ) */EH[-cs]+ *( |$)" "\\1 \\2" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REGEX REPLACE "(^| ) */GR-? *( |$)" "\\1 \\2" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

if(PDK_ENABLE_LTO AND PDK_ON_WIN32 AND NOT PDK_LINKER_IS_LLD_LINK)
    message(FATAL_ERROR "When compiling for Windows, PDK_ENABLE_LTO requires using lld as the linker (point CMAKE_LINKER at lld-link.exe)")
endif()

if(UPPERCASE_PDK_ENABLE_LTO STREQUAL "THIN")
    pdk_append("-flto=thin" CMAKE_CXX_FLAGS CMAKE_C_FLAGS)
    if(NOT PDK_LINKER_IS_LLD_LINK)
        pdk_append("-flto=thin" CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
    endif()
    # If the linker supports it, enable the lto cache. This improves initial build
    # time a little since we re-link a lot of the same objects, and significantly
    # improves incremental build time.
    # FIXME: We should move all this logic into the clang driver.
    if(APPLE)
        pdk_append("-Wl,-cache_path_lto,${PROJECT_BINARY_DIR}/lto.cache"
            CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
    elseif(UNIX AND PDK_USE_LINKER STREQUAL "lld")
        pdk_append("-Wl,--thinlto-cache-dir=${PROJECT_BINARY_DIR}/lto.cache"
            CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
    elseif(PDK_USE_LINKER STREQUAL "gold")
        pdk_append("-Wl,--plugin-opt,cache-dir=${PROJECT_BINARY_DIR}/lto.cache"
            CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
    endif()
elseif(UPPERCASE_LLVM_ENABLE_LTO STREQUAL "FULL")
    pdk_append("-flto=full" CMAKE_CXX_FLAGS CMAKE_C_FLAGS)
    if(NOT PDK_LINKER_IS_LLD_LINK)
        pdk_append("-flto=full" CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
    endif()
elseif(PDK_ENABLE_LTO)
    pdk_append("-flto" CMAKE_CXX_FLAGS CMAKE_C_FLAGS)
    if(NOT PDK_LINKER_IS_LLD_LINK)
        pdk_append("-flto" CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
    endif()
endif()

# Provide public options to globally control RTTI and EH
option(PDK_ENABLE_EH "Enable Exception handling" OFF)
option(PDK_ENABLE_RTTI "Enable run time type information" OFF)
if(PDK_ENABLE_EH AND NOT PDK_ENABLE_RTTI)
    message(FATAL_ERROR "Exception handling requires RTTI. You must set PDK_ENABLE_RTTI to ON")
endif()

pdk_get_compile_definitions()

