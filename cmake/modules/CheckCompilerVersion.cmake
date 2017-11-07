# Check if the host compiler is new enough. libpdk requires at least GCC 7.1,
# MSVC 2017 (Update 3), or Clang 5.0.

if(NOT DEFINED PDK_COMPILER_CHECKED)
    set(PDK_COMPILER_CHECKED ON)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.1)
            message(FATAL_ERROR "Host GCC version must be at least 7.1!")
        endif()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
            message(FATAL_ERROR "Host Clang version must be at least 5.0!")
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 25.0)
            message(FATAL_ERROR "Host Visual Studio version must be at least 25.0!")
        endif()
    endif()
endif()
