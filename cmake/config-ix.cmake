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

# include checks
check_include_file(dirent.h PDK_HAVE_DIRENT_H)
check_include_file(pthread.h PDK_HAVE_PTHREAD_H)
check_include_file(signal.h PDK_HAVE_SIGNAL_H)
check_include_file(stdint.h PDK_HAVE_STDINT_H)
check_include_file(sys/dir.h PDK_HAVE_SYS_DIR_H)
check_include_file(sys/ioctl.h PDK_HAVE_SYS_IOCTL_H)
check_include_file(sys/mman.h PDK_HAVE_SYS_MMAN_H)
check_include_file(sys/ndir.h PDK_HAVE_SYS_NDIR_H)
check_include_file(sys/param.h PDK_HAVE_SYS_PARAM_H)
check_include_file(sys/resource.h PDK_HAVE_SYS_RESOURCE_H)
check_include_file(sys/stat.h PDK_HAVE_SYS_STAT_H)
check_include_file(sys/time.h PDK_HAVE_SYS_TIME_H)
check_include_file(sys/types.h PDK_HAVE_SYS_TYPES_H)
check_include_file(sys/uio.h PDK_HAVE_SYS_UIO_H)
check_include_file(termios.h PDK_HAVE_TERMIOS_H)
check_include_file(unistd.h PDK_HAVE_UNISTD_H)
check_include_file(valgrind/valgrind.h PDK_HAVE_VALGRIND_VALGRIND_H)
check_include_file(zlib.h PDK_HAVE_ZLIB_H)
check_include_file(fenv.h PDK_HAVE_FENV_H)

# library checks
check_library_exists(pthread pthread_create "" PDK_HAVE_LIBPTHREAD)
check_library_exists(pthread pthread_getspecific "" PDK_HAVE_PTHREAD_GETSPECIFIC)
check_library_exists(pthread pthread_rwlock_init "" PDK_HAVE_PTHREAD_RWLOCK_INIT)
check_library_exists(pthread pthread_mutex_lock "" PDK_HAVE_PTHREAD_MUTEX_LOCK)
check_library_exists(dl dlopen "" PDK_HAVE_LIBDL)
check_library_exists(rt clock_gettime "" PDK_HAVE_LIBRT)

# function checks
check_symbol_exists(getpagesize unistd.h HAVE_GETPAGESIZE)
check_symbol_exists(sysconf unistd.h HAVE_SYSCONF)
check_symbol_exists(getrusage sys/resource.h HAVE_GETRUSAGE)
check_symbol_exists(setrlimit sys/resource.h HAVE_SETRLIMIT)
check_symbol_exists(isatty unistd.h HAVE_ISATTY)
check_symbol_exists(futimens sys/stat.h HAVE_FUTIMENS)
check_symbol_exists(futimes sys/time.h HAVE_FUTIMES)
check_symbol_exists(posix_fallocate fcntl.h HAVE_POSIX_FALLOCATE)

if(PDK_HAVE_SYS_UIO_H)
    check_symbol_exists(writev sys/uio.h PDK_HAVE_WRITEV)
endif()
set(CMAKE_REQUIRED_DEFINITIONS "-D_LARGEFILE64_SOURCE")
check_symbol_exists(lseek64 "sys/types.h;unistd.h" PDK_HAVE_LSEEK64)
set(CMAKE_REQUIRED_DEFINITIONS "")

check_symbol_exists(mallctl malloc_np.h PDK_HAVE_MALLCTL)
check_symbol_exists(mallinfo malloc.h PDK_HAVE_MALLINFO)
check_symbol_exists(malloc_zone_statistics malloc/malloc.h
    PDK_HAVE_MALLOC_ZONE_STATISTICS)
check_symbol_exists(mkdtemp "stdlib.h;unistd.h" PDK_HAVE_MKDTEMP)
check_symbol_exists(mkstemp "stdlib.h;unistd.h" PDK_HAVE_MKSTEMP)
check_symbol_exists(mktemp "stdlib.h;unistd.h" PDK_HAVE_MKTEMP)
check_symbol_exists(getcwd unistd.h PDK_HAVE_GETCWD)
check_symbol_exists(gettimeofday sys/time.h PDK_HAVE_GETTIMEOFDAY)
check_symbol_exists(getrlimit "sys/types.h;sys/time.h;sys/resource.h" PDK_HAVE_GETRLIMIT)
check_symbol_exists(posix_spawn spawn.h PDK_HAVE_POSIX_SPAWN)
check_symbol_exists(pread unistd.h PDK_HAVE_PREAD)
check_symbol_exists(realpath stdlib.h PDK_HAVE_REALPATH)
check_symbol_exists(sbrk unistd.h PDK_HAVE_SBRK)
check_symbol_exists(strtoll stdlib.h PDK_HAVE_STRTOLL)
check_symbol_exists(strerror string.h PDK_HAVE_STRERROR)
check_symbol_exists(strerror_r string.h PDK_HAVE_STRERROR_R)
check_symbol_exists(strerror_s string.h PDK_HAVE_DECL_STRERROR_S)
check_symbol_exists(setenv stdlib.h PDK_HAVE_SETENV)

if(PDK_HAVE_DLFCN_H)
    if(PDK_HAVE_LIBDL)
        list(APPEND CMAKE_REQUIRED_LIBRARIES dl)
    endif()
    check_symbol_exists(dlopen dlfcn.h PDK_HAVE_DLOPEN)
    check_symbol_exists(dladdr dlfcn.h PDK_HAVE_DLADDR)
    if( HAVE_LIBDL )
        list(REMOVE_ITEM CMAKE_REQUIRED_LIBRARIES dl)
    endif()
endif()

check_symbol_exists(__GLIBC__ stdio.h PDK_USING_GLIBC)

if(PDK_USING_GLIBC)
    add_definitions(-D_GNU_SOURCE)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
endif()

# This check requires _GNU_SOURCE
check_symbol_exists(sched_getaffinity sched.h PDK_HAVE_SCHED_GETAFFINITY)
check_symbol_exists(CPU_COUNT sched.h PDK_HAVE_CPU_COUNT)
if(PDK_HAVE_LIBPTHREAD)
    check_library_exists(pthread pthread_getname_np "" PDK_HAVE_PTHREAD_GETNAME_NP)
    check_library_exists(pthread pthread_setname_np "" PDK_HAVE_PTHREAD_SETNAME_NP)
endif()

set(headers "sys/types.h")
if (PDK_HAVE_INTTYPES_H)
    set(headers ${headers} "inttypes.h")
endif()

if (PDK_HAVE_STDINT_H)
    set(headers ${headers} "stdint.h")
endif()

pdk_check_type_exists(int64_t "${headers}" HAVE_INT64_T)
pdk_check_type_exists(uint64_t "${headers}" HAVE_UINT64_T)
pdk_check_type_exists(u_int64_t "${headers}" HAVE_U_INT64_T)

check_cxx_compiler_flag("-Wvariadic-macros" PDK_SUPPORTS_VARIADIC_MACROS_FLAG)
check_cxx_compiler_flag("-Wgnu-zero-variadic-macro-arguments"
    PDK_SUPPORTS_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS_FLAG)

# Disable gcc's potentially uninitialized use analysis as it presents lots of
# false positives.
if (CMAKE_COMPILER_IS_GNUCXX)
  check_cxx_compiler_flag("-Wmaybe-uninitialized" PDK_HAS_MAYBE_UNINITIALIZED)
  if (PDK_HAS_MAYBE_UNINITIALIZED)
    set(PDK_USE_NO_MAYBE_UNINITIALIZED 1)
  else()
    # Only recent versions of gcc make the distinction between -Wuninitialized
    # and -Wmaybe-uninitialized. If -Wmaybe-uninitialized isn't supported, just
    # turn off all uninitialized use warnings.
    check_cxx_compiler_flag("-Wuninitialized" PDK_HAS_UNINITIALIZED)
    set(PDK_USE_NO_UNINITIALIZED ${PDK_HAS_UNINITIALIZED})
  endif()
endif()

include(CheckThirdPartyLibraries)
