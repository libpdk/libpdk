# This CMake module force clang use libcxx

if(NOT DEFINED PDK_STDLIB_HANDLED)
    set(PDK_STDLIB_HANDLED ON)
    function(append value)
        foreach(variable ${ARGN})
            set(${variable} "${${variable}} ${value}" PARENT_SCOPE)
        endforeach(variable)
    endfunction()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        check_cxx_compiler_flag("-stdlib=libc++" PDK_CXX_SUPPORTS_STDLIB)
        if(PDK_CXX_SUPPORTS_STDLIB)
            append("-stdlib=libc++"
                CMAKE_CXX_FLAGS CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS
                CMAKE_MODULE_LINKER_FLAGS)
        endif()
    endif()
endif()
