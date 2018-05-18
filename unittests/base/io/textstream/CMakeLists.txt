set(PDK_IO_TEXTSTREAM_TEST_SRCS)
pdk_add_files(PDK_IO_TEXTSTREAM_TEST_SRCS
   TextStreamTest.cpp)
pdk_add_unittest(ModuleBaseUnittests TextStreamTest ${PDK_IO_TEXTSTREAM_TEST_SRCS})

macro(add_process_testapp name)
   cmake_parse_arguments(ARG "" "OUTPUT_NAME" "" ${ARGN})
   add_executable(${name} EXCLUDE_FROM_ALL ${ARG_UNPARSED_ARGUMENTS})
   target_link_libraries(${name} pdk)
   add_dependencies(ProcessTest ${name})
   if (ARG_OUTPUT_NAME)
      set_target_properties(${name} PROPERTIES
         OUTPUT_NAME ${ARG_OUTPUT_NAME})
   endif()
   set_target_properties(${name} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/apps)
endmacro()

add_process_testapp(ReadAllStdinProcessApp apps/ReadAllStdinProcess.cpp)
add_process_testapp(ReadLineStdinProcessApp apps/ReadLineStdinProcess.cpp)
add_process_testapp(StdinProcessApp apps/StdinProcess.cpp)

target_compile_definitions(TextStreamTest PUBLIC PDKTEST_CURRENT_DIR="${CMAKE_CURRENT_BINARY_DIR}"
   PUBLIC PDKTEST_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
