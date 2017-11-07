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
include(TestBigEndian)

include(CheckCompilerVersion)
include(HandleLibpdkStdlib)

