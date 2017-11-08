# There is no clear way of keeping track of compiler command-line
# options chosen via `add_definitions'

# Beware that there is no implementation of pdk_remove_definitions.

macro(pdk_add_definitions)
    # We don't want no semicolons on LLVM_DEFINITIONS:
    foreach(arg ${ARGN})
        if(DEFINED PDK_DEFINITIONS)
            set(PDK_DEFINITIONS "${PDK_DEFINITIONS} ${arg}")
        else()
            set(PDK_DEFINITIONS ${arg})
        endif()
    endforeach()
    add_definitions(${ARGN})
endmacro()
