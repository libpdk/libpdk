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
// Created by softboy on 2017/11/16.

#ifndef PDK_UTILS_REFCOUNT_H
#define PDK_UTILS_REFCOUNT_H

#include "pdk/global/Global.h"
#include "pdk/base/os/thread/Atomic.h"

namespace pdk {
namespace utils {

class RefCount
{
public:
   inline bool ref() noexcept
   {
      int count = m_atomic.load();
#if !defined(PDK_NO_UNSHARED_CONTAINERS)
      if (0 == count) {
         return false;
      }
#endif
      if (-1 != count) {
         m_atomic.ref();
      }
      return true;
   }
   
   inline bool deref() noexcept
   {
      int count = m_atomic.load();
#if !defined(PDK_NO_UNSHARED_CONTAINERS)
      if (0 == count) {
         return false;
      }
#endif
      if (-1 == count) {
         return true;
      }
      return m_atomic.deref();
   }
   
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
   bool setSharable(bool sharable) noexcept
   {
      PDK_ASSERT(!isShared());
      if (sharable) {
         return m_atomic.testAndSetRelaxed(0, 1);
      } else {
         return m_atomic.testAndSetRelaxed(1, 0);
      }
   }
   
   bool isSharable() const noexcept
   {
      return m_atomic.load() != 0;
   }
#endif
   
   bool isStatic() const noexcept
   {
      return m_atomic.load() == -1;
   }
   
   bool isShared() const noexcept
   {
      int count = m_atomic.load();
      return (1 != count) && (0 != count);
   }
   
   void initializeOwned() noexcept
   {
      m_atomic.store(1);
   }
   
   void initializeUnshared() noexcept
   {
       m_atomic.store(0);
   }
private:
    pdk::os::thread::AtomicInt m_atomic;
};

#define PDK_REFCOUNT_INITIALIZE_STATIC { PDK_BASIC_ATOMIC_INITIALIZER(-1) }

} // utils
} // pdk

#endif // PDK_UTILS_REFCOUNT_H
