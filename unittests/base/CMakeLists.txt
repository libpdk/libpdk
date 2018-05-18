add_custom_target(ModuleBaseUnittests)
set_target_properties(ModuleBaseUnittests PROPERTIES FOLDER "ModuleBaseUnittests")

set(PDK_LANG_TEST_SRCS)
pdk_add_files(PDK_LANG_TEST_SRCS
   lang/CharacterTest.cpp
   lang/string/DoubleData.h
   lang/string/StringTest.cpp)

pdk_add_unittest(ModuleBaseUnittests PdkLangTest ${PDK_LANG_TEST_SRCS})

set(PDK_OS_THREAD_TEST_SRCS)
pdk_add_files(PDK_OS_THREAD_TEST_SRCS
   os/thread/AtomicIntTest.cpp
   os/thread/AtomicIntegerTest.cpp
   os/thread/AtomicPointerTest.cpp
   os/thread/ReadWriteLockTest.cpp
   os/thread/SemaphoreTest.cpp
   os/thread/ThreadTest.cpp
   os/thread/ThreadOnce.h
   os/thread/ThreadOnce.cpp
   os/thread/ThreadOnceTest.cpp
   os/thread/ThreadStorageTest.cpp
   os/thread/ThreadPoolTest.cpp
   os/thread/ReadLockerTest.cpp
   os/thread/WriteLockerTest.cpp)

pdk_add_unittest(ModuleBaseUnittests OsThreadTest ${PDK_OS_THREAD_TEST_SRCS})

set(PDK_DS_TEST_SRCS)
pdk_add_files(PDK_DS_TEST_SRCS
   ds/arraydata/SimpleVector.h
   ds/arraydata/ArrayDataTest.cpp
   ds/ByteArrayTest.cpp
   ds/ByteArrayMatcherTest.cpp
   ds/VarLengthArrayTest.cpp
   ds/BitArrayTest.cpp
   ds/RingBufferTest.cpp)

pdk_add_unittest(ModuleBaseUnittests PdkDsTest ${PDK_DS_TEST_SRCS})

set(PDK_TEXT_TEST_SRCS)
pdk_add_files(PDK_TEXT_TEST_SRCS
   text/codecs/TextCodecTest.cpp
   text/RegularExpressionTest.cpp)

pdk_add_unittest(ModuleBaseUnittests TextTest ${PDK_TEXT_TEST_SRCS})

set(PDK_IO_TEST_SRCS)
pdk_add_files(PDK_IO_TEST_SRCS
   io/BufferTest.cpp
   io/DirTest.cpp
   io/DirIteratorTest.cpp
   io/FileSystemEntryTest.cpp
   io/FileTest.cpp
   io/FileInfoTest.cpp
   io/StorageInfoTest.cpp
   io/TemporaryDirTest.cpp
   io/TemporaryFileTest.cpp)

list(APPEND PDK_IO_TEST_SRCS ${SHARED_HEADERS})

pdk_add_unittest(ModuleBaseUnittests IoTest ${PDK_IO_TEST_SRCS})
target_compile_definitions(IoTest PUBLIC PDKTEST_CURRENT_TEST_DIR="${CMAKE_CURRENT_BINARY_DIR}"
   PUBLIC PDKTEST_CURRENT_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/io")

add_subdirectory(os/process)
add_subdirectory(io/textstream)

