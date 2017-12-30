add_custom_target(KerneUnittests)
set_target_properties(KerneUnittests PROPERTIES FOLDER "GlobalUnittests")

set(PDK_KERNEL_TEST_SRCS)
pdk_add_files(PDK_KERNEL_TEST_SRCS
    AlgorithmsTest.cpp
    MathTest.cpp
    StringUtilsTest.cpp
    HashFuncsTest.cpp)

pdk_add_unittest(KerneUnittests KernelTest ${PDK_KERNEL_TEST_SRCS})
