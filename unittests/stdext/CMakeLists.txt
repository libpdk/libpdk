add_custom_target(StdextUnittests)
set_target_properties(StdextUnittests PROPERTIES FOLDER "StdExtUnittests")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/preprocessor)
set(PDK_STDEXT_TEST_SRCS)
pdk_add_files(PDK_STDEXT_TEST_SRCS
    preprocessor/ArithmeticTest.cpp
    preprocessor/ArrayTest.cpp
    preprocessor/ComparisonTest.cpp
    preprocessor/ControlTest.cpp
    preprocessor/IterationTest.h
    preprocessor/IterationTest.cpp
    preprocessor/RepetitionTest.cpp)

pdk_add_unittest(StdextUnittests StdExtTest ${PDK_STDEXT_TEST_SRCS})
