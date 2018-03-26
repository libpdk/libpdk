pdk_collect_files(
   TYPE_BOTH
   DIR ./
   OUTPUT_VAR PDK_TESTLIB_SOURCES)

pdk_add_library(pdktest
   ${PDK_HEADER_FILES} 
   ${PDK_TESTLIB_SOURCES}
   LINK_LIBS pdk
   )
