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
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Created by softboy on 2018/02/07.

#include "pdk/global/PlatformDefs.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/kernel/CoreUnix.h"
#include "pdk/base/ds/VarLengthArray.h"

#include <pwd.h>
#include <cstdlib> // for realpath()
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>

#if PDK_HAS_INCLUDE(<paths.h>)
# include <paths.h>
#endif
#ifndef _PATH_TMP           // from <paths.h>
# define _PATH_TMP          "/tmp"
#endif

#if defined(PDK_OS_MAC)
# include "pdk/kernel/internal/CoreMacPrivate.h"
# include <CoreFoundation/CFBundle.h>
#endif

#ifdef PDK_OS_OSX
#include <CoreServices/CoreServices.h>
#endif

#if defined(PDK_OS_DARWIN)
# include <copyfile.h>
// We cannot include <Foundation/Foundation.h> (it's an Objective-C header), but
// we need these declarations:
PDK_FORWARD_DECLARE_OBJC_CLASS(NSString);
extern "C" NSString *NSTemporaryDirectory();
#endif

#if defined(PDK_OS_LINUX)
#  include <sys/ioctl.h>
#  include <sys/syscall.h>
#  include <sys/sendfile.h>
#  include <linux/fs.h>

// in case linux/fs.h is too old and doesn't define it:
#ifndef FICLONE
#  define FICLONE       _IOW(0x94, 9, int)
#endif
#  if !PDK_CONFIG(renameat2) && defined(SYS_renameat2)
namespace {
int renameat2(int oldfd, const char *oldpath, int newfd, const char *newpath, unsigned flags)
{ 
   return syscall(SYS_renameat2, oldfd, oldpath, newfd, newpath, flags);
}
}
#  endif

#  if !PDK_CONFIG(statx) && defined(SYS_statx) && PDK_HAS_INCLUDE(<linux/stat.h>)
#    include <linux/stat.h>
namespace 
{
int statx(int dirfd, const char *pathname, int flag, unsigned mask, struct statx *statxbuf)
{
   return syscall(SYS_statx, dirfd, pathname, flag, mask, statxbuf); 
}
}
#  endif
#endif

namespace pdk {
namespace io {
namespace fs {
namespace internal {



} // internal
} // fs
} // io
} // pdk
