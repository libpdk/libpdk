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
// Created by softboy on 2017/11/23.

#ifndef PDK_M_BASE_OS_THREAD_ATOMIC_H
#define PDK_M_BASE_OS_THREAD_ATOMIC_H

#include "BasicAtomic.h"

namespace pdk {
namespace os {
namespace thread {

//PDK_WARNING_PUSH
//PDK_WARNING_DISABLE_GCC("-Wextra")

template <typename T>
class AtomicInteger : public BasicAtomicInteger<T>
{
public:
   constexpr AtomicInteger(T value = 0) noexcept
      : BasicAtomicInteger<T>(value)
   {}
   
   inline AtomicInteger(const AtomicInteger &other) noexcept
      : BasicAtomicInteger<T>()
   {
      this->storeRelease(other.loadAcquire());
   }
   
   AtomicInteger &operator=(const AtomicInteger &other) noexcept
   {
      this->storeRelease(other.loadAcquire());
      return *this;
   }
};

class AtomicInt : public AtomicInteger<int>
{
public:
   AtomicInt(int value = 0) noexcept
      : AtomicInteger<int>(value)
   {}
};

template <typename T>
class AtomicPointer : public BasicAtomicPointer<T>
{
public:
   constexpr AtomicPointer(T *value = 0) noexcept
      : BasicAtomicPointer<T>(value)
   {}
   AtomicPointer(const AtomicPointer &other) noexcept
      : BasicAtomicPointer<T>()
   {
      this->storeRelease(other.loadAcquire());
   }
   
   AtomicPointer &operator=(const AtomicPointer &other) noexcept
   {
      this->storeRelease(other.loadAcquire());
      return *this;
   }
};

PDK_WARNING_POP

template <typename T>
inline void atomic_assign(T *&dest, T *source)
{
   if (dest == source) {
      return;
   }
   source->ref.ref();
   if (!dest->ref.deref()) {
      delete dest;
   }
   dest = source;
}

template <typename T>
inline void atomic_detach(T *&dest)
{
   if (dest->ref.load() == 1) {
      return;
   }
   T *orig = dest;
   dest = new T(*dest);
   if (!orig->ref.deref()) {
      delete orig;
   }
}

} // thread
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_ATOMIC_H
