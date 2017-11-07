if( WIN32 AND NOT CYGWIN )
    # We consider Cygwin as another Unix
    set(PURE_WINDOWS 1)
endif()

include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckCCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)
include(TestBigEndian)

include(CheckCompilerVersion)
include(HandleLibpdkStdlib)

# Helper macros and functions

macro(pdk_add_cxx_include result files)
    set(${result} "")
    foreach (file_name ${files})
        set(${result} "${${result}} #include<${file_name}>\n")
    endforeach()
endmacro()

function(pdk_check_type_exists type files variable)
    pdk_add_cxx_include(includes "${files}")
    CHECK_CXX_SOURCE_COMPILES("
        ${includes} ${type} typeVar;
        int main() {
        return 0;
        }
        " ${variable})
endfunction()

# include checks
check_include_file(dirent.h HAVE_DIRENT_H)
check_include_file(pthread.h HAVE_PTHREAD_H)
check_include_file(signal.h HAVE_SIGNAL_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(sys/dir.h HAVE_SYS_DIR_H)
check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/ndir.h HAVE_SYS_NDIR_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/uio.h HAVE_SYS_UIO_H)
check_include_file(termios.h HAVE_TERMIOS_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(valgrind/valgrind.h HAVE_VALGRIND_VALGRIND_H)
check_include_file(zlib.h HAVE_ZLIB_H)
check_include_file(fenv.h HAVE_FENV_H)

# library checks
check_library_exists(pthread pthread_create "" HAVE_LIBPTHREAD)
check_library_exists(pthread pthread_getspecific "" HAVE_PTHREAD_GETSPECIFIC)
check_library_exists(pthread pthread_rwlock_init "" HAVE_PTHREAD_RWLOCK_INIT)
check_library_exists(pthread pthread_mutex_lock "" HAVE_PTHREAD_MUTEX_LOCK)
check_library_exists(dl dlopen "" HAVE_LIBDL)
check_library_exists(rt clock_gettime "" HAVE_LIBRT)

