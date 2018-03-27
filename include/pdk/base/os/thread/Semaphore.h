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

#ifndef PDK_M_BASE_OS_THREAD_SEMAPHORE_H
#define PDK_M_BASE_OS_THREAD_SEMAPHORE_H

#include "pdk/global/Global.h"

namespace pdk {
namespace os {
namespace thread {

// forward declare class with namespace
namespace internal
{
class SemaphorePrivate;
} // internal

class PDK_CORE_EXPORT Semaphore
{
public:
   explicit Semaphore(int num = 0);
   ~Semaphore();
   
   void acquire(int num = 1);
   bool tryAcquire(int num = 1);
   bool tryAcquire(int num, int timeout);
   void release(int num = 1);
   int available() const;
private:
   PDK_DISABLE_COPY(Semaphore);
   internal::SemaphorePrivate *m_implPtr;
};

class SemaphoreReleaser
{
public:
   SemaphoreReleaser() = default;
   explicit SemaphoreReleaser(Semaphore &sem, int n = 1) noexcept
      : m_sem(&sem),
        m_n(n)
   {}
   
   explicit SemaphoreReleaser(Semaphore *sem, int n = 1) noexcept
      : m_sem(sem),
        m_n(n)
   {}
   
   SemaphoreReleaser(SemaphoreReleaser &&other) noexcept
      : m_sem(other.m_sem),
        m_n(other.m_n)
   {
      other.m_sem = nullptr;
   }
   
   SemaphoreReleaser &operator=(SemaphoreReleaser &&other) noexcept
   {
      SemaphoreReleaser moved(std::move(other));
      swap(moved);
      return *this;
   }
   
   ~SemaphoreReleaser()
   {
      if (m_sem) {
         m_sem->release(m_n);
      }
      
   }
   
   void swap(SemaphoreReleaser &other) noexcept
   {
      std::swap(m_sem, other.m_sem);
      std::swap(m_n, other.m_n);
   }
   
   Semaphore *getSemaphore() const noexcept
   {
      return m_sem;
   }
   
   Semaphore *cancel() noexcept
   {
      Semaphore *old = m_sem;
      m_sem = nullptr;
      return old;
   }
   
private:
   Semaphore *m_sem = nullptr;
   int m_n;
};

} // thread
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_SEMAPHORE_H

