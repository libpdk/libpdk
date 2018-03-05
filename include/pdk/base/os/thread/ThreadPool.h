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
// Created by softboy on 2018/03/02.

#ifndef PDK_M_BASE_OS_THREAD_THREAD_POOL_H
#define PDK_M_BASE_OS_THREAD_THREAD_POOL_H

#include "pdk/global/Global.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/Runnable.h"
#include "pdk/kernel/Object.h"

namespace pdk {
namespace os {
namespace thread {

namespace internal {
class ThreadPoolPrivate;
} // internal

using internal::ThreadPoolPrivate;
using pdk::kernel::Object;

class PDK_CORE_EXPORT ThreadPool : public Object
{
public:
   ThreadPool(Object *parent = nullptr);
   ~ThreadPool();
   static ThreadPool *getGlobalInstance();
   void start(Runnable *runnable, int priority = 0);
   bool tryStart(Runnable *runnable);
   int getExpiryTimeout() const;
   void setExpiryTimeout(int expiryTimeout);
   int getMaxThreadCount() const;
   void setMaxThreadCount(int maxThreadCount);
   int getActiveThreadCount() const;
   void setStackSize(uint stackSize);
   uint getStackSize() const;
   void reserveThread();
   void releaseThread();
   bool waitForDone(int msecs = -1);
   void clear();
   
   PDK_REQUIRED_RESULT bool tryTake(Runnable *runnable);
private:
   friend class FutureInterfaceBase;
   PDK_DECLARE_PRIVATE(ThreadPool);
};

} // thread
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_THREAD_POOL_H
