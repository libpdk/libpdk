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

#include "pdk/base/io/fs/LockFile.h"
#include "pdk/base/io/fs/internal/LockFilePrivate.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/IoDevice.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/DeadlineTimer.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/global/SysInfo.h"
#include "pdk/base/ds/ByteArrayBuilder.h"

namespace pdk {
namespace io {
namespace fs {

using pdk::kernel::DeadlineTimer;
using pdk::os::thread::Thread;
using pdk::ds::ByteArray;
using pdk::kernel::CoreApplication;
using pdk::lang::String;

namespace {
struct LockFileInfo
{
   pdk::pint64 m_pid;
   String m_appname;
   String m_hostname;
};

bool get_lock_info_helper(const String &fileName, LockFileInfo *info)
{
   File reader(fileName);
   if (!reader.open(IoDevice::OpenMode::ReadOnly)) {
      return false;
   }
   ByteArray pidLine = reader.readLine();
   pidLine.chop(1);
   if (pidLine.isEmpty()) {
      return false;
   }
   ByteArray appNameLine = reader.readLine();
   appNameLine.chop(1);
   ByteArray hostNameLine = reader.readLine();
   hostNameLine.chop(1);
   bool ok;
   info->m_appname = String::fromUtf8(appNameLine);
   info->m_hostname = String::fromUtf8(hostNameLine);
   info->m_pid = pidLine.toLongLong(&ok);
   return ok && info->m_pid > 0;
}


String get_machine_name()
{
#ifdef PDK_OS_WIN
   // we don't use SysInfo because it tries to do name resolution
   return pdk::pdk_env_var("COMPUTERNAME");
#else
   return SysInfo::getMachineHostName();
#endif
}

} // anonymous namespace

LockFile::LockFile(const String &fileName)
   : m_implPtr(new LockFilePrivate(fileName))
{
}

LockFile::~LockFile()
{
   unlock();
}

void LockFile::setStaleLockTime(int staleLockTime)
{
   PDK_D(LockFile);
   implPtr->m_staleLockTime = staleLockTime;
}

int LockFile::getStaleLockTime() const
{
   PDK_D(const LockFile);
   return implPtr->m_staleLockTime;
}

bool LockFile::isLocked() const
{
   PDK_D(const LockFile);
   return implPtr->m_isLocked;
}

bool LockFile::lock()
{
   return tryLock(-1);
}

bool LockFile::tryLock(int timeout)
{
   PDK_D(LockFile);
   DeadlineTimer timer(std::max(timeout, -1));    // QDT only takes -1 as "forever"
   int sleepTime = 100;
   while (true) {
      implPtr->m_lockError = implPtr->tryLockSys();
      switch (implPtr->m_lockError) {
      case LockError::NoError:
         implPtr->m_isLocked = true;
         return true;
      case LockError::PermissionError:
      case LockError::UnknownError:
         return false;
      case LockError::LockFailedError:
         if (!implPtr->m_isLocked && implPtr->isApparentlyStale()) {
            if (PDK_UNLIKELY(FileInfo(implPtr->m_fileName).getLastModified() > DateTime::getCurrentDateTime()))
               info_stream("LockFile: Lock file '%ls' has a modification time in the future", pdk_utf16_printable(implPtr->m_fileName));
            // Stale lock from another thread/process
            // Ensure two processes don't remove it at the same time
            LockFile rmlock(implPtr->m_fileName + Latin1String(".rmlock"));
            if (rmlock.tryLock()) {
               if (implPtr->isApparentlyStale() && implPtr->removeStaleLock()) {
                  continue;
               }
            }
         }
         break;
      }
      
      int remainingTime = timer.getRemainingTime();
      if (remainingTime == 0) {
         return false;
      } else if (uint(sleepTime) > uint(remainingTime)) {
         sleepTime = remainingTime;
      }
      
      Thread::msleep(sleepTime);
      if (sleepTime < 5 * 1000) {
         sleepTime *= 2;
      }
   }
   // not reached
   return false;
}

bool LockFile::getLockInfo(pdk::pint64 *pid, String *hostname, String *appname) const
{
   PDK_D(const LockFile);
   LockFileInfo info;
   if (!get_lock_info_helper(implPtr->m_fileName, &info)) {
      return false;
   } 
   if (pid) {
      *pid = info.m_pid;
   }
   if (hostname) {
      *hostname = info.m_hostname;
   }
   if (appname) {
      *appname = info.m_appname;
   }
   return true;
}

ByteArray LockFilePrivate::lockFileContents() const
{
   // Use operator% from the fast builder to avoid multiple memory allocations.
   return ByteArray::number(CoreApplication::getAppPid()) % '\n'
         % processNameByPid(CoreApplication::getAppPid()).toUtf8() % '\n'
         % get_machine_name().toUtf8() % '\n';
}

bool LockFilePrivate::isApparentlyStale() const
{
   LockFileInfo info;
   if (get_lock_info_helper(m_fileName, &info)) {
      if (info.m_hostname.isEmpty() || info.m_hostname == get_machine_name()) {
         if (!isProcessRunning(info.m_pid, info.m_appname)) {
            return true;
         } 
      }
   }
   
   const pdk::pint64 age = FileInfo(m_fileName).getLastModified().msecsTo(DateTime::getCurrentDateTimeUtc());
   return m_staleLockTime > 0 && std::abs(age) > m_staleLockTime;
}

bool LockFile::removeStaleLockFile()
{
   PDK_D(LockFile);
   if (implPtr->m_isLocked) {
      warning_stream("removeStaleLockFile can only be called when not holding the lock");
      return false;
   }
   return implPtr->removeStaleLock();
}

LockFile::LockError LockFile::getError() const
{
   PDK_D(const LockFile);
   return implPtr->m_lockError;
}

} // fs
} // io
} // pdk
