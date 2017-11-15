add_custom_target(ModuleBaseUnittests)
set_target_properties(ModuleBaseUnittests PROPERTIES FOLDER "ModuleBaseUnittests")

set(PDK_LANG_TEST_SRCS)
pdk_add_files(PDK_LANG_TEST_SRCS
    lang/CharacterTest.cpp)

pdk_add_unittest(ModuleBaseUnittests LangTest ${PDK_LANG_TEST_SRCS})
