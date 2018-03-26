add_custom_target(UtilsUnittests)
set_target_properties(UtilsUnittests PROPERTIES FOLDER "GlobalUnittests")

set(PDK_UTILS_TEST_SRCS)
pdk_add_files(PDK_UTILS_TEST_SRCS
    MemoryHelperTest.cpp
    RefCountTest.cpp
    ScopedPointerTest.cpp
    sharedpointer/SharedPointerTest.cpp
    sharedpointer/Wrapper.h
    sharedpointer/Wrapper.cpp
    sharedpointer/ForwardDeclared.h
    sharedpointer/ForwardDeclared.cpp
    LockFreeListTest.cpp
    LocaleTest.cpp)

pdk_add_unittest(UtilsUnittests PdkUtilsTest ${PDK_UTILS_TEST_SRCS})
