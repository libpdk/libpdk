
pdk_add_headers(
    Global.h
    )
message("${PDK_HEADERS}")
set(PDK_HEADERS ${PDK_HEADERS} PARENT_SCOPE)
