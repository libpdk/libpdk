# This scripts collect libpdk implement library source files

pdk_collect_files(
   TYPE_SOURCE
   DIR pal
   OUTPUT_VAR PDK_BASE_SOURCES)

pdk_collect_files(
   TYPE_SOURCE
   DIR global
   OUTPUT_VAR PDK_BASE_SOURCES)

pdk_collect_files(
   TYPE_SOURCE
   DIR kernel
   OUTPUT_VAR PDK_BASE_SOURCES)

pdk_collect_files(
   TYPE_SOURCE
   DIR utils
   OUTPUT_VAR PDK_BASE_SOURCES)

pdk_collect_files(
   TYPE_SOURCE
   DIR stdext
   OUTPUT_VAR PDK_BASE_SOURCES)

pdk_collect_files(
   TYPE_SOURCE
   DIR stdext
   OUTPUT_VAR PDK_BASE_SOURCES)

pdk_collect_files(
   TYPE_SOURCE
   DIR modules/base
   OUTPUT_VAR PDK_BASE_MODULE_SOURCES)

set(MODULE_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modules/base)
set(KERNEL_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kernel)
if(UNIX)
   list(APPEND PDK_BASE_MODULE_SOURCES
      ${MODULE_BASE_DIR}/os/thread/_platform/ThreadUnix.cpp)
   list(APPEND PDK_BASE_SOURCES
      ${KERNEL_BASE_DIR}/_platform/CoreUnix.cpp
      ${KERNEL_BASE_DIR}/_platform/TimerInfoUnix.cpp
      ${KERNEL_BASE_DIR}/_platform/EventDispatcherUnix.cpp)
   if(APPLE)
      list(APPEND PDK_BASE_SOURCES
         ${KERNEL_BASE_DIR}/_platform/ElapsedTimerMac.cpp)
   else()
      list(APPEND PDK_BASE_SOURCES
         ${KERNEL_BASE_DIR}/_platform/ElapsedTimerUnix.cpp)
   endif()
   
elseif(WIN32)
   
else()
   list(APPEND PDK_BASE_SOURCES
      ${KERNEL_BASE_DIR}/_platform/ElapsedTimerGeneric.cpp)
endif()

list(REMOVE_ITEM PDK_BASE_MODULE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/modules/base/lang/UnicodeTables.cpp)

pdk_add_library(libpdk 
   ${PDK_HEADER_FILES} 
   ${PDK_BASE_SOURCES}
   ${PDK_BASE_MODULE_SOURCES})

