# in this file we check all dependent libraries
include (FindPcre2)

if (NOT PCRE2_FOUND)
   message(FATAL_ERROR "pcre2 library not exist, please install it")
endif()
