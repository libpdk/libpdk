# This CMake module retrieve version info from Git repo

function(pdk_get_version_info_from_vcs)
    set(SOURCE_DIR ${ARGV1})
    if("${SOURCE_DIR}" STREQUAL "")
        set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    set(result "${${VERS}}")
    if(EXISTS "${SOURCE_DIR}/.git")
        find_program(git_executable NAMES git git.exe git.cmd)
        if(git_executable)
            # Run from a subdirectory to force git to print an absoute path.
            execute_process(COMMAND ${git_executable} rev-parse --git-dir
                WORKING_DIRECTORY ${SOURCE_DIR}/cmake
                RESULT_VARIABLE git_result
                OUTPUT_VARIABLE git_dir
                ERROR_QUIET)
            if (git_result EQUAL 0)
                # Try to get a ref-id
                string(STRIP "${git_dir}" git_dir)
                set(result "${result}git")
            endif()
            
            # Get the git ref id
            execute_process(COMMAND
                ${git_executable} rev-parse --short HEAD
                WORKING_DIRECTORY ${SOURCE_DIR}
                TIMEOUT 5
                RESULT_VARIABLE git_result
                OUTPUT_VARIABLE git_ref_id
                OUTPUT_STRIP_TRAILING_WHITESPACE)
            
            if(git_result EQUAL 0)
                set(GIT_COMMIT ${git_ref_id} PARENT_SCOPE)
                set(result "${result}-${git_ref_id}")
            else()
                set(result "${result}")
            endif()
        endif()
    endif()
    set(${VERS} ${result} PARENT_SCOPE)
endfunction()
