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
// Created by softboy on 2018/05/10.

#include "pdk/base/io/Debug.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/kernel/StringUtils.h"

#include "pdk/global/PlatformDefs.h"
#include "pdk/base/os/process/Process.h"
#include "pdk/base/os/process/internal/ProcessPrivate.h"
#include "pdk/base/io/fs/StandardPaths.h"
#include "pdk/kernel/internal/CoreUnixPrivate.h"

#ifdef PDK_OS_MAC
#include "pdk/kernel/internal/CoreMacPrivate.h"
#endif

#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/Dir.h"
#include <list>
#include <mutex>
#include "pdk/base/os/thread/Semaphore.h"
#include "pdk/kernel/SocketNotifier.h"
#include "pdk/kernel/ElapsedTimer.h"
#include "pdk/base/os/thread/Thread.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if defined(PDK_PROCESS_DEBUG)
#include <ctype.h>
#endif

#include "forkfd/forkfd.h"

namespace pdk {
namespace os {
namespace process {

using pdk::ds::ByteArray;

#if defined(PDK_PROCESS_DEBUG)
/*
    Returns a human readable representation of the first \a len
    characters in \a data.
*/
ByteArray pretty_debug(const char *data, int len, int maxSize)
{
   if (!data) return "(null)";
   ByteArray out;
   for (int i = 0; i < len; ++i) {
      char c = data[i];
      if (isprint(c)) {
         out += c;
      } else switch (c) {
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: {
         const char buf[] =  {
            '\\',
            pdk::to_oct(uchar(c) / 64),
            pdk::to_oct(uchar(c) % 64 / 8),
            pdk::to_oct(uchar(c) % 8),
            0
         };
         out += buf;
      }
      }
   }
   
   if (len < maxSize) {
       out += "...";
   }
   return out;
}
#endif

} // process
} // os
} // pdk
