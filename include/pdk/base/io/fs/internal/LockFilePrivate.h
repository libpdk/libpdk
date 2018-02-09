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
// Created by softboy on 2018/02/09.

#ifndef PDK_M_BASE_IO_FS_INTERNAL_LOCK_FILE_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_LOCK_FILE_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/io/fs/LockFile.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/ds/ByteArray.h"

#ifdef PDK_OS_WIN
#include "pdk/global/Windows.h"
#endif

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::ds::ByteArray;
using pdk::io::fs::LockFile;

class LockFilePrivate
{
public:
   LockFilePrivate(const String &fn)
      : m_fileName(fn),
     #ifdef PDK_OS_WIN
        m_fileHandle(INVALID_HANDLE_VALUE),
     #else
        m_fileHandle(-1),
     #endif
        m_staleLockTime(30 * 1000), // 30 seconds
        m_lockError(LockFile::LockError::NoError),
        m_isLocked(false)
   {
   }
   LockFile::LockError tryLockSys();
   bool removeStaleLock();
   ByteArray lockFileContents() const;
   // Returns \c true if the lock belongs to dead PID, or is old.
   // The attempt to delete it will tell us if it was really stale or not, though.
   bool isApparentlyStale() const;
   
   // used in dbusmenu
   PDK_CORE_EXPORT static String processNameByPid(pdk::pint64 pid);
   static bool isProcessRunning(pdk::pint64 pid, const String &appname);
   
   String m_fileName;
#ifdef PDK_OS_WIN
   pdk::HANDLE m_fileHandle;
#else
   int m_fileHandle;
#endif
   int m_staleLockTime; // "int milliseconds" is big enough for 24 days
   LockFile::LockError m_lockError;
   bool m_isLocked;
};

} // internal
} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_INTERNAL_LOCK_FILE_PRIVATE_H
