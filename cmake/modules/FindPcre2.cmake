# find pcre2 library module

find_library(PCRE2_8_LIBRARY NAMES pcre2-8)
find_library(PCRE2_16_LIBRARY NAMES pcre2-16)
find_path(PCRE2_INCLUDE_DIRS pcre2.h)
mark_as_advanced(PCRE2_8_LIBRARY)
mark_as_advanced(PCRE2_16_LIBRARY)
mark_as_advanced(PCRE2_INCLUDE_DIRS)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Pcre2 DEFAULT_MSG PCRE2_8_LIBRARY PCRE2_16_LIBRARY PCRE2_INCLUDE_DIRS)

if(PCRE2_FOUND)
   if((NOT TARGET Pcre2::8Bit) AND (NOT TARGET Pcre2::16Bit))
      set(PCRE2_LIBRARIES ${PCRE2_8_LIBRARY} ${PCRE2_16_LIBRARY})
      mark_as_advanced(PCRE2_LIBRARIES)
      add_library(Pcre2::8Bit UNKNOWN IMPORTED)
      add_library(Pcre2::16Bit UNKNOWN IMPORTED)
      if (PCRE2_INCLUDE_DIRS)
         set_target_properties(Pcre2::8Bit PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PCRE2_INCLUDE_DIRS}")
         set_target_properties(Pcre2::16Bit PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PCRE2_INCLUDE_DIRS}")
      endif()
      if (EXISTS "${PCRE2_8_LIBRARY}")
         set_target_properties(Pcre2::8Bit PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
            IMPORTED_LOCATION "${PCRE2_8_LIBRARY}")
      endif()
      if (EXISTS "${PCRE2_16_LIBRARY}")
         set_target_properties(Pcre2::16Bit PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
            IMPORTED_LOCATION "${PCRE2_16_LIBRARY}")
      endif()
   endif()
endif()
