set(PDK_PROCESS_TEST_SRCS)
pdk_add_files(PDK_PROCESS_TEST_SRCS
   ProcessTest.cpp)

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

pdk_add_unittest(ModuleBaseUnittests ProcessTest ${PDK_PROCESS_TEST_SRCS})
add_process_testapp(FileWriterProcessApp apps/testFileWriterProcess.cpp)
#add_process_testapp(DetachedApp apps/testDetached.cpp)
add_process_testapp(ExitCodesApp apps/testExitCodes.cpp)
add_process_testapp(ForwardingApp apps/testForwarding.cpp)
add_process_testapp(ProcessCrashApp apps/testProcessCrash.cpp)
add_process_testapp(ProcessDeadWhileReadingApp apps/testProcessDeadWhileReading.cpp)
add_process_testapp(ProcessEOFApp apps/testProcessEOF.cpp)
add_process_testapp(ProcessEchoApp apps/testProcessEcho.cpp)
add_process_testapp(ProcessEcho2App apps/testProcessEcho2.cpp)
add_process_testapp(ProcessEcho3App apps/testProcessEcho3.cpp)
#add_process_testapp(ProcessEnvironmentApp apps/testProcessEnvironment.cpp)
add_process_testapp(ProcessHangApp apps/testProcessHang.cpp)
add_process_testapp(ProcessNormalApp apps/testProcessNormal.cpp)
add_process_testapp(ProcessOutputApp apps/testProcessOutput.cpp)
#add_process_testapp(ProcessSpacesArgsApp apps/testProcessSpacesArgs.cpp)
#add_process_testapp(ProcessSpacesArgsOneSpaceApp OUTPUT_NAME "one space" apps/testProcessSpacesArgs.cpp)
#add_process_testapp(ProcessSpacesArgsOneSpacesApp OUTPUT_NAME "two space s" apps/testProcessSpacesArgs.cpp)
#add_process_testapp(SetNamedPipeHandleStateApp apps/testSetNamedPipeHandleState.cpp)
#add_process_testapp(SetWorkingDirectoryApp apps/testSetWorkingDirectory.cpp)
add_process_testapp(SoftExitApp apps/testSoftExit.cpp)
#add_process_testapp(SpaceInNameApp apps/testSpaceInName.cpp)

target_compile_definitions(ProcessTest PUBLIC PDKTEST_CURRENT_TEST_DIR="${CMAKE_CURRENT_BINARY_DIR}"
   PUBLIC PDKTEST_CURRENT_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/io"
   PUBLIC PDKTEST_PROCESS_APPS_DIR="${CMAKE_CURRENT_BINARY_DIR}/apps")

target_compile_definitions(ForwardingApp PUBLIC PDKTEST_CURRENT_TEST_DIR="${CMAKE_CURRENT_BINARY_DIR}"
   PUBLIC PDKTEST_CURRENT_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/io"
   PUBLIC PDKTEST_PROCESS_APPS_DIR="${CMAKE_CURRENT_BINARY_DIR}/apps")
