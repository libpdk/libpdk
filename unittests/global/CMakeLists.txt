add_custom_target(GlobalUnittests)
set_target_properties(GlobalUnittests PROPERTIES FOLDER "GlobalUnittests")

set(PDK_GLOBAL_TEST_SRCS)
pdk_add_files(PDK_GLOBAL_TEST_SRCS
    FlagsTest.cpp
    NumericTest.cpp
    GlobalStaticTest.cpp)

pdk_add_unittest(GlobalUnittests GlobalTest ${PDK_GLOBAL_TEST_SRCS})
