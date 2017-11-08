# We need to execute this script at installation time because the
# DESTDIR environment variable may be unset at configuration time.
# See PR8397.

function(pdk_install_symlink name target outdir)
    if(UNIX)
        set(PDK_LINK_OR_COPY create_symlink)
        set(PDK_DESTDIR $ENV{DESTDIR})
    else()
        set(PDK_LINK_OR_COPY copy)
    endif()
    set(bindir "${PDK_DESTDIR}${CMAKE_INSTALL_PREFIX}/${outdir}/")
    message("Creating ${name}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E ${PDK_LINK_OR_COPY} "${target}" "${name}"
        WORKING_DIRECTORY "${bindir}")
endfunction()
