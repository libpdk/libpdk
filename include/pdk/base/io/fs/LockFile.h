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
// Created by softboy on 2018/02/06.

#ifndef PDK_M_BASE_IO_FS_FILE_LOCK_H
#define PDK_M_BASE_IO_FS_FILE_LOCK_H

#include "pdk/utils/ScopedPointer.h"

namespace pdk {

// forward declare class with namespace
namespace lang {
class String;
} // lang

namespace io {
namespace fs {

// forward declare class with namespace
namespace internal {
class LockFilePrivate;
} // internal

using internal::LockFilePrivate;
using pdk::lang::String;

class PDK_CORE_EXPORT LockFile
{
public:
   LockFile(const String &fileName);
   ~LockFile();
   
   bool lock();
   bool tryLock(int timeout = 0);
   void unlock();
   
   void setStaleLockTime(int);
   int staleLockTime() const;
   
   bool isLocked() const;
   bool getLockInfo(pdk::pint64 *pid, String *hostname, String *appname) const;
   bool removeStaleLockFile();
   
   enum class LockError : uint
   {
      NoError = 0,
      LockFailedError = 1,
      PermissionError = 2,
      UnknownError = 3
   };
   LockError getError() const;
   
protected:
   pdk::utils::ScopedPointer<LockFilePrivate> m_implPtr;
   
private:
   PDK_DECLARE_PRIVATE(LockFile);
   PDK_DISABLE_COPY(LockFile);
};

} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_FILE_LOCK_H
