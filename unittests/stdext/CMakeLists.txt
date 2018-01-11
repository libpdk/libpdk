add_custom_target(StdextUnittests)
set_target_properties(StdextUnittests PROPERTIES FOLDER "StdExtUnittests")

set(PDK_STDEXT_TEST_SRCS)
pdk_add_files(PDK_STDEXT_TEST_SRCS
    optional/OptionalTest.cpp
    optional/OptionalValueAccessTest.cpp
    optional/OptionalFailureTest.cpp)

pdk_add_unittest(StdextUnittests StdExtTest ${PDK_STDEXT_TEST_SRCS})
