
add_subdirectory(compress)
add_subdirectory(concurrent)
add_subdirectory(ds)
add_subdirectory(io)
add_subdirectory(math)
add_subdirectory(net)
add_subdirectory(os)
add_subdirectory(security)
add_subdirectory(text)
add_subdirectory(time)
add_subdirectory(utils)

pdk_add_headers(
    lang/Boolean.h
    lang/Byte.h
    lang/Character.h
    lang/Double.h
    lang/Float.h
    lang/Integer.h
    lang/Long.h
    lang/Number.h
    lang/Object.h
    lang/Short.h
    lang/String.h
    lang/internal/UnicodeTables.h
    lang/internal/UnicodeTools.h)

set(PDK_HEADER_FILES ${PDK_HEADER_FILES} PARENT_SCOPE)
