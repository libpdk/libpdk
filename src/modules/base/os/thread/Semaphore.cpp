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
// Created by softboy on 2018/01/08.

#include "pdk/base/os/thread/Semaphore.h"
#include <condition_variable>
#include <mutex>
#include <chrono>

namespace pdk {
namespace os {
namespace thread {

namespace internal {

class SemaphorePrivate
{
public:
   inline SemaphorePrivate(int num)
      : m_num(num)
   {}
   
   std::mutex m_mutex;
   std::condition_variable m_condVar;
   int m_num;
};

} // internal

using internal::SemaphorePrivate;

Semaphore::Semaphore(int num)
{
   PDK_ASSERT_X(num >= 0, "Semaphore", "parameter 'num' must be non-negative");
   m_implPtr = new SemaphorePrivate(num);
}

Semaphore::~Semaphore()
{
   delete m_implPtr;
}

void Semaphore::acquire(int num)
{
   PDK_ASSERT_X(num >= 0, "Semaphore::acquire", "parameter 'num' must be non-negative");
   std::unique_lock<std::mutex> locker(m_implPtr->m_mutex);
   while (num > m_implPtr->m_num) {
      m_implPtr->m_condVar.wait(locker);
   }
   m_implPtr->m_num -= num;
}

void Semaphore::release(int num)
{
   PDK_ASSERT_X(num >= 0, "Semaphore", "parameter 'num' must be non-negative");
   std::unique_lock<std::mutex> locker(m_implPtr->m_mutex);
   m_implPtr->m_num += num;
   m_implPtr->m_condVar.notify_all();
}

int Semaphore::available() const
{
   std::unique_lock<std::mutex> locker(m_implPtr->m_mutex);
   return m_implPtr->m_num;
}

bool Semaphore::tryAcquire(int num)
{
   PDK_ASSERT_X(num >= 0, "Semaphore", "parameter 'num' must be non-negative");
   std::unique_lock<std::mutex> locker(m_implPtr->m_mutex);
   if (num > m_implPtr->m_num) {
      return false;
   }
   m_implPtr->m_num -= num;
   return true;
}

bool Semaphore::tryAcquire(int num, int timeout)
{
   PDK_ASSERT_X(num >= 0, "Semaphore", "parameter 'num' must be non-negative");
   std::unique_lock<std::mutex> locker(m_implPtr->m_mutex);
   if (timeout < 0) {
      while (num > m_implPtr->m_num) {
         m_implPtr->m_condVar.wait(locker);
      }
   } else {
      std::chrono::system_clock::time_point start;
      while (num > m_implPtr->m_num) {
         int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
         if (timeout - elapsed <= 0 ||
             std::cv_status::timeout == m_implPtr->m_condVar.wait_for(locker, std::chrono::milliseconds(timeout - elapsed))) {
            return false;
         }
      }
   }
   m_implPtr->m_num -= num;
   return true;
}

} // thread
} // os
} // pdk

