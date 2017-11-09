# define utils functions and macros
include(CheckCXXCompilerFlag)
if(NOT DEFINED PDK_CMAKE_MODULE_UTILS)
    set(PDK_CMAKE_MODULE_UTILS ON)
    function(pdk_append value)
        foreach(variable ${ARGN})
            set(${variable} "${${variable}} ${value}" PARENT_SCOPE)
        endforeach(variable)
    endfunction()
    
    function(pdk_append_if condition value)
        if (${condition})
            foreach(variable ${ARGN})
                set(${variable} "${${variable}} ${value}" PARENT_SCOPE)
            endforeach(variable)
        endif()
    endfunction()
    
    macro(pdk_add_flag_if_supported flag name)
        check_c_compiler_flag("-Werror ${flag}" "C_SUPPORTS_${name}")
        pdk_append_if("C_SUPPORTS_${name}" "${flag}" CMAKE_C_FLAGS)
        check_cxx_compiler_flag("-Werror ${flag}" "CXX_SUPPORTS_${name}")
        pdk_append_if("CXX_SUPPORTS_${name}" "${flag}" CMAKE_CXX_FLAGS)
    endmacro()
    
    function(pdk_add_flag_or_print_warning flag name)
        check_c_compiler_flag("-Werror ${flag}" "C_SUPPORTS_${name}")
        check_cxx_compiler_flag("-Werror ${flag}" "CXX_SUPPORTS_${name}")
        if (C_SUPPORTS_${name} AND CXX_SUPPORTS_${name})
            message(STATUS "Building with ${flag}")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
            set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${flag}" PARENT_SCOPE)
        else()
            message(WARNING "${flag} is not supported.")
        endif()
    endfunction()
    
    function(pdk_check_linker_flag flag out_var)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${flag}")
        check_cxx_compiler_flag("" ${out_var})
    endfunction()
    
    function(pdk_get_compile_definitions)
        get_directory_property(top_dir_definitions DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS)
        foreach(definition ${top_dir_definitions})
            if(DEFINED result)
                string(APPEND result " -D${definition}")
            else()
                set(result "-D${definition}")
            endif()
        endforeach()
        set(PDK_DEFINITIONS "${result}" PARENT_SCOPE)
    endfunction()
    
    # for config-ix.cmake
    macro(pdk_add_cxx_include result files)
        set(${result} "")
        foreach (file_name ${files})
            set(${result} "${${result}} #include<${file_name}>\n")
        endforeach()
    endmacro()
    
    function(pdk_check_type_exists type files variable)
        pdk_add_cxx_include(includes "${files}")
        CHECK_CXX_SOURCE_COMPILES("
            ${includes} ${type} typeVar;
            int main() {
            return 0;
            }
            " ${variable})
    endfunction()
endif()
