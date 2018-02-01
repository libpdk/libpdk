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
    
    function(pdk_canonicalize_name name output)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" nameStrip ${name})
        string(REPLACE "-" "_" nameUNDERSCORE ${nameStrip})
        string(TOUPPER ${nameUNDERSCORE} nameUPPER)
        set(${output} "${nameUPPER}" PARENT_SCOPE)
    endfunction(pdk_canonicalize_name)
    
    function(pdk_add_headers)
        foreach(file ${ARGV})
            list(APPEND PDK_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${file})
        endforeach()
        set(PDK_HEADER_FILES ${PDK_HEADER_FILES} PARENT_SCOPE)
    endfunction()
    
    function(pdk_add_base_sources)
        foreach(file ${ARGV})
            list(APPEND PDK_BASE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${file})
        endforeach()
        set(PDK_BASE_SOURCES ${PDK_BASE_SOURCES} PARENT_SCOPE)
    endfunction()
    
    function(pdk_add_module_sources module)
        list(REMOVE_AT ARGV 0)
        foreach(file ${ARGV})
            list(APPEND ${module} ${CMAKE_CURRENT_SOURCE_DIR}/${file})
        endforeach()
        set(${module} ${${module}} PARENT_SCOPE)
    endfunction()
    
    function(pdk_add_files target)
        list(REMOVE_AT ARGV 0)
        foreach(file ${ARGV})
            list(APPEND ${target} ${CMAKE_CURRENT_SOURCE_DIR}/${file})
        endforeach()
        set(${target} ${${target}} PARENT_SCOPE)
    endfunction()
    
    function(pdk_collect_files)
        cmake_parse_arguments(ARG "TYPE_HEADER;TYPE_SOURCE;TYPE_BOTH;OVERWRITE" "OUTPUT_VAR;DIR;SKIP_DIR" "" ${ARGN})
        set(GLOB ${ARG_DIR})
        if(ARG_TYPE_BOTH OR (ARG_TYPE_HEADER AND ARG_TYPE_SOURCE))
            string(APPEND GLOB "/*.[h|cpp]")
        elseif(ARG_TYPE_HEADER) 
            string(APPEND GLOB "/*.h")
        elseif(ARG_TYPE_SOURCE)
            string(APPEND GLOB "/*.cpp")
        else()
            string(APPEND GLOB "/*.h")
        endif()
        if(ARG_OVERWRITE)
            set(${ARG_OUTPUT_VAR} "")
        endif()
        file(GLOB_RECURSE TEMP_OUTPUTFILES
             LIST_DIRECTORIES false
             ${GLOB})
        list(FILTER TEMP_OUTPUTFILES EXCLUDE REGEX "_platform/")
        if(ARG_SKIP_DIR)
           list(FILTER TEMP_OUTPUTFILES EXCLUDE REGEX ${ARG_SKIP_DIR})
        endif()
        list(APPEND ${ARG_OUTPUT_VAR} ${TEMP_OUTPUTFILES})
        set(${ARG_OUTPUT_VAR} ${${ARG_OUTPUT_VAR}} PARENT_SCOPE)
    endfunction()
endif()
