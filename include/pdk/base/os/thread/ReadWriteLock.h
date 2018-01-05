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
// Created by softboy on 2018/01/05.

#ifndef PDK_M_BASE_OS_THREAD_READWRITE_LOCK_H
#define PDK_M_BASE_OS_THREAD_READWRITE_LOCK_H

#include "pdk/global/Global.h"
#include "pdk/base/os/thread/Atomic.h"

namespace pdk {
namespace os {
namespace thread {

namespace internal {
class ReadWriteLockPrivate;
} // internal

class PDK_CORE_EXPORT ReadWriteLock
{
public:
   enum class RecursionMode
   {
      NonRecursion,
      Recursion
   };
   
   explicit ReadWriteLock(RecursionMode recursionMode = RecursionMode::NonRecursion);
   ~ReadWriteLock();
   
   void lockForRead();
   bool tryLockForRead();
   bool tryLockForRead(int timeout);
   
   void lockForWrite();
   bool tryLockForWrite();
   bool tryLockForWrite(int timeout);
   
   void unlock();
   
private:
   PDK_DISABLE_COPY(ReadWriteLock);
};

} // pdk
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_READWRITE_LOCK_H
