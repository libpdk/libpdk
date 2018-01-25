// @copyright 2017-2018 zzu_softboy <zzu_softboy@163.com>
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) AR       ISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Created by softboy on 2018/01/25.

#ifndef PDK_GLOBAL_INTERNAL_COMMON_POSIX_PLATFORM_DEFS_H
#define PDK_GLOBAL_INTERNAL_COMMON_POSIX_PLATFORM_DEFS_H

#include <signal.h>
#include <sys/types.h>
#ifndef PDK_NO_SOCKET_H
#  include <sys/socket.h>
#endif
#include <sys/stat.h>

#if defined(PDK_USE_XOPEN_LFS_EXTENSIONS) && defined(PDK_LARGEFILE_SUPPORT)
#define PDK_STATBUF              struct stat64
#define PDK_FPOS_T               fpos64_t
#define PDK_OFF_T                off64_t

#define PDK_STAT                 ::stat64
#define PDK_LSTAT                ::lstat64
#define PDK_TRUNCATE             ::truncate64

// File I/O
#define PDK_OPEN                 ::open64
#define PDK_LSEEK                ::lseek64
#define PDK_FSTAT                ::fstat64
#define PDK_FTRUNCATE            ::ftruncate64

// Standard C89
#define PDK_FOPEN                ::fopen64
#define PDK_FSEEK                ::fseeko64
#define PDK_FTELL                ::ftello64
#define PDK_FGETPOS              ::fgetpos64
#define PDK_FSETPOS              ::fsetpos64

#define PDK_MMAP                 ::mmap64
#else // !defined (PDK_USE_XOPEN_LFS_EXTENSIONS) || !defined(PDK_LARGEFILE_SUPPORT)
#include "../c89/PlatformDefs.h"

#define PDK_STATBUF              struct stat

#define PDK_STAT                 ::stat
#define PDK_LSTAT                ::lstat
#define PDK_TRUNCATE             ::truncate

// File I/O
#define PDK_OPEN                 ::open
#define PDK_LSEEK                ::lseek
#define PDK_FSTAT                ::fstat
#define PDK_FTRUNCATE            ::ftruncate

// Posix extensions to C89
#if !defined(PDK_USE_XOPEN_LFS_EXTENSIONS) && !defined(PDK_NO_USE_FSEEKO)
#undef PDK_OFF_T
#undef PDK_FSEEK
#undef PDK_FTELL

#define PDK_OFF_T                off_t

#define PDK_FSEEK                ::fseeko
#define PDK_FTELL                ::ftello
#endif

#define PDK_MMAP    

#endif // !defined (PDK_USE_XOPEN_LFS_EXTENSIONS) || !defined(PDK_LARGEFILE_SUPPORT)

#define PDK_STAT_MASK            S_IFMT
#define PDK_STAT_REG             S_IFREG
#define PDK_STAT_DIR             S_IFDIR
#define PDK_STAT_LNK             S_IFLNK

#define PDK_ACCESS               ::access
#define PDK_GETCWD               ::getcwd
#define PDK_CHDIR                ::chdir
#define PDK_MKDIR                ::mkdir
#define PDK_RMDIR                ::rmdir

// File I/O
#define PDK_CLOSE                ::close
#define PDK_READ                 ::read
#define PDK_WRITE                ::write

#define PDK_OPEN_LARGEFILE       O_LARGEFILE
#define PDK_OPEN_RDONLY          O_RDONLY
#define PDK_OPEN_WRONLY          O_WRONLY
#define PDK_OPEN_RDWR            O_RDWR
#define PDK_OPEN_CREAT           O_CREAT
#define PDK_OPEN_TRUNC           O_TRUNC
#define PDK_OPEN_APPEND          O_APPEND

// Posix extensions to C89
#define PDK_FILENO               fileno

// Directory iteration
#define PDK_DIR                  DIR

#define PDK_OPENDIR              ::opendir
#define PDK_CLOSEDIR             ::closedir

#if defined(PDK_LARGEFILE_SUPPORT) \
        && defined(PDK_USE_XOPEN_LFS_EXTENSIONS) \
        && !defined(PDK_NO_READDIR64)
#define PDK_DIRENT               struct dirent64
#define PDK_READDIR              ::readdir64
#define PDK_READDIR_R            ::readdir64_r
#else
#define PDK_DIRENT               struct dirent
#define PDK_READDIR              ::readdir
#define PDK_READDIR_R            ::readdir_r
#endif

#define PDK_SOCKLEN_T            socklen_t

#define PDK_SOCKET_CONNECT       ::connect
#define PDK_SOCKET_BIND          ::bind

#define PDK_SIGNAL_RETTYPE       void
#define PDK_SIGNAL_ARGS          int
#define PDK_SIGNAL_IGNORE        SIG_IGN

#endif // PDK_GLOBAL_INTERNAL_COMMON_POSIX_PLATFORM_DEFS_H
