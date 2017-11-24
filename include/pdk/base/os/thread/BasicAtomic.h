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

#ifndef PDK_M_BASE_OS_THREAD_BASIC_ATOMIC_H
#define PDK_M_BASE_OS_THREAD_BASIC_ATOMIC_H

#include "AtomicCxx11.h"

PDK_WARNING_PUSH
PDK_WARNING_DISABLE_MSVC(4522)

namespace pdk {
namespace os {
namespace thread {

template <typename T>
class BasicAtomicInteger
{
public:
   using Operations = AtomicOperations<T>;
   PDK_STATIC_ASSERT_X(std::is_integral<T>::value, "template parameter is not an integral type");
   PDK_STATIC_ASSERT_X(AtomicOpsSupport<sizeof(T)>::IsSupported, "template parameter is an integral of a size not supported on this platform");
   
public:
   T load() const noexcept
   {
      return Operations::load(m_atomic);
   }
   
   void store(T newValue) noexcept
   {
      Operations::store(m_atomic, newValue);
   }
   
   T loadAcquire() const noexcept
   {
      return Operations::loadAcquire(m_atomic);
   }
   
   void storeRelease(T newValue) noexcept
   {
      Operations::storeRelease(m_atomic, newValue);
   }
   
   operator T() const noexcept
   {
      return Operations::loadAcquire(m_atomic);
   }
   
   T operator =(T newValue) noexcept
   {
      Operations::storeRelease(m_atomic, newValue);
      return newValue;
   }

   static constexpr bool isRefCountingNative() noexcept
   {
      return Operations::isRefCountingNative();
   }
   
   static constexpr bool isRefCountingWaitFree() noexcept
   {
      return Operations::isRefCountingWaitFree();
   }
   
   bool ref() noexcept
   {
      return Operations::ref(m_atomic);
   }
   
   bool deref() noexcept
   {
      return Operations::deref(m_atomic);
   }
   
   static constexpr bool isTestAndSetNative() noexcept
   {
      return Operations::isTestAndSetNative();
   }
   
   static constexpr bool isTestAndSetWaitFree() noexcept
   {
      return Operations::isTestAndSetWaitFree();
   }
   
   bool testAndSetRelaxed(T expectedValue, T newValue) noexcept
   {
      return Operations::testAndSetRelaxed(m_atomic, expectedValue, newValue);
   }
   
   bool testAndSetAcquire(T expectedValue, T newValue) noexcept
   {
      return Operations::testAndSetAcquire(m_atomic, expectedValue, newValue);
   }
   
   bool testAndSetRelease(T expectedValue, T newValue) noexcept
   {
      return Operations::testAndSetRelease(m_atomic, expectedValue, newValue);
   }
   
   bool testAndSetOrdered(T expectedValue, T newValue) noexcept
   {
      return Operations::testAndSetOrdered(m_atomic, expectedValue, newValue);
   }
   
   bool testAndSetRelaxed(T expectedValue, T newValue, T &currentValue) noexcept
   {
      return Operations::testAndSetRelaxed(m_atomic, expectedValue, newValue, &currentValue);
   }
   
   bool testAndSetAcquire(T expectedValue, T newValue, T &currentValue) noexcept
   {
      return Operations::testAndSetAcquire(m_atomic, expectedValue, newValue, &currentValue);
   }
   
   bool testAndSetRelease(T expectedValue, T newValue, T &currentValue) noexcept
   {
      return Operations::testAndSetRelease(m_atomic, expectedValue, newValue, &currentValue);
   }
   
   bool testAndSetOrdered(T expectedValue, T newValue, T &currentValue) noexcept
   {
      return Operations::testAndSetOrdered(m_atomic, expectedValue, newValue, &currentValue);
   }
   
   static constexpr bool isFetchAndStoreNative() noexcept
   {
      return Operations::isFetchAndStoreNative();
   }
   
   static constexpr bool isFetchAndStoreWaitFree() noexcept
   {
      return Operations::isFetchAndStoreWaitFree();
   }
   
   T fetchAndStoreRelaxed(T newValue) noexcept
   {
      return Operations::fetchAndStoreRelaxed(m_atomic, newValue);
   }
   
   T fetchAndStoreAcquire(T newValue) noexcept
   {
      return Operations::fetchAndStoreAcquire(m_atomic, newValue);
   }
   
   T fetchAndStoreRelease(T newValue) noexcept
   {
      return Operations::fetchAndStoreRelease(m_atomic, newValue);
   }
   
   T fetchAndStoreOrdered(T newValue) noexcept
   {
      return Operations::fetchAndStoreOrdered(m_atomic, newValue);
   }
   
   static constexpr bool isFetchAndAddNative() noexcept
   {
      return Operations::isFetchAndAddNative();
   }
   
   static constexpr bool isFetchAndAddWaitFree() noexcept
   {
      return Operations::isFetchAndAddWaitFree();
   }
   
   T fetchAndAddRelaxed(T newValue) noexcept
   {
      return Operations::fetchAndAddRelaxed(m_atomic, newValue);
   }
   
   T fetchAndAddAcquire(T newValue) noexcept
   {
      return Operations::fetchAndAddAcquire(m_atomic, newValue);
   }
   
   T fetchAndAddRelease(T newValue) noexcept
   {
      return Operations::fetchAndAddRelease(m_atomic, newValue);
   }
   
   T fetchAndAddOrdered(T newValue) noexcept
   {
      return Operations::fetchAndAddOrdered(m_atomic, newValue);
   }
public:
   typename Operations::Type m_atomic;
};

} // thread
} // os
} // pdk

PDK_WARNING_POP

#endif // PDK_M_BASE_OS_THREAD_BASIC_ATOMIC_H
