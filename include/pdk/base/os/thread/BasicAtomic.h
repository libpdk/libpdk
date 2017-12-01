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

//PDK_WARNING_PUSH
//PDK_WARNING_DISABLE_MSVC(4522)

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
   
   T fetchAndAddRelaxed(T value) noexcept
   {
      return Operations::fetchAndAddRelaxed(m_atomic, value);
   }
   
   T fetchAndAddAcquire(T value) noexcept
   {
      return Operations::fetchAndAddAcquire(m_atomic, value);
   }
   
   T fetchAndAddRelease(T value) noexcept
   {
      return Operations::fetchAndAddRelease(m_atomic, value);
   }
   
   T fetchAndAddOrdered(T value) noexcept
   {
      return Operations::fetchAndAddOrdered(m_atomic, value);
   }
   
   T fetchAndSubRelaxed(T value) noexcept
   {
      return Operations::fetchAndSubRelaxed(m_atomic, value);
   }
   
   T fetchAndSubAcquire(T value) noexcept
   {
      return Operations::fetchAndSubAcquire(m_atomic, value);
   }
   
   T fetchAndSubRelease(T value) noexcept
   {
      return Operations::fetchAndSubRelease(m_atomic, value);
   }
   
   T fetchAndSubOrdered(T value) noexcept
   {
      return Operations::fetchAndSubOrdered(m_atomic, value);
   }
   
   T fetchAndAndRelaxed(T value) noexcept
   {
      return Operations::fetchAndAndRelaxed(m_atomic, value);
   }
   
   T fetchAndAndAcquire(T value) noexcept
   {
      return Operations::fetchAndAndAcquire(m_atomic, value);
   }
   
   T fetchAndAndRelease(T value) noexcept
   {
      return Operations::fetchAndAndRelease(m_atomic, value);
   }
   
   T fetchAndAndOrdered(T value) noexcept
   {
      return Operations::fetchAndAndOrdered(m_atomic, value);
   }
   
   T fetchAndOrRelaxed(T value) noexcept
   {
      return Operations::fetchAndOrRelaxed(m_atomic, value);
   }
   
   T fetchAndOrAcquire(T value) noexcept
   {
      return Operations::fetchAndOrAcquire(m_atomic, value);
   }
   
   T fetchAndOrRelease(T value) noexcept
   {
      return Operations::fetchAndOrRelease(m_atomic, value);
   }
   
   T fetchAndOrOrdered(T value) noexcept
   {
      return Operations::fetchAndOrOrdered(m_atomic, value);
   }
   
   T fetchAndXorRelaxed(T value) noexcept
   {
      return Operations::fetchAndXorRelaxed(m_atomic, value);
   }
   
   T fetchAndXorAcquire(T value) noexcept
   {
      return Operations::fetchAndXorAcquire(m_atomic, value);
   }
   
   T fetchAndXorRelease(T value) noexcept
   {
      return Operations::fetchAndXorRelease(m_atomic, value);
   }
   
   T fetchAndXorOrdered(T value) noexcept
   {
      return Operations::fetchAndXorOrdered(m_atomic, value);
   }
   
   T operator++() noexcept
   {
      return fetchAndAddOrdered(1) + 1;
   }
   
   T operator++(int) noexcept
   {
      return fetchAndAddOrdered(1);
   }
   
   T operator--() noexcept
   {
      return fetchAndSubOrdered(1) - 1;
   }
   
   T operator--(int) noexcept
   {
      return fetchAndSubOrdered(1);
   }
   
   T operator+=(T value) noexcept
   {
      return fetchAndAddOrdered(value) + value;
   }
   
   T operator-=(T value) noexcept
   {
      return fetchAndSubOrdered(value) - value;
   }
   
   T operator&=(T value) noexcept
   {
      return fetchAndAndOrdered(value) & value;
   }
   
   T operator|=(T value) noexcept
   {
      return fetchAndOrOrdered(value) | value;
   }
   
   T operator^=(T value) noexcept
   {
      return fetchAndXorOrdered(value) ^ value;
   }
   
   BasicAtomicInteger() = default;
   constexpr BasicAtomicInteger(T value) noexcept
      : m_atomic(value)
   {}
   
   BasicAtomicInteger(const BasicAtomicInteger &other) = delete;
   BasicAtomicInteger &operator=(const BasicAtomicInteger &other) = delete;
   BasicAtomicInteger &operator=(const BasicAtomicInteger &other) volatile = delete;
   
public:
   typename Operations::Type m_atomic;
};

using BasicAtomicInt = BasicAtomicInteger<int>;

template <typename T>
class BasicAtomicPointer
{
public:
   using Type = T *;
   using Operations = AtomicOperations<Type>;
   using AtomicType = typename Operations::Type;
   
   
   BasicAtomicPointer() = default;
   constexpr BasicAtomicPointer(Type value)
      : m_atomic(value)
   {}
   BasicAtomicPointer(BasicAtomicPointer &other) = delete;
   BasicAtomicPointer &operator=(BasicAtomicPointer &other) = delete;
   BasicAtomicPointer &operator=(BasicAtomicPointer &other) volatile = delete;
   
   Type load() const noexcept
   {
      return Operations::load(m_atomic);
   }
   
   void store(Type newValue) noexcept
   {
      Operations::store(m_atomic, newValue);
   }
   
   Type loadAcquire() const noexcept
   {
      return Operations::loadAcquire(m_atomic);
   }
   
   void storeRelease(Type newValue) noexcept
   {
      Operations::storeRelease(m_atomic, newValue);
   }
   
   static constexpr bool isTestAndSetNative() noexcept
   {
      return Operations::isTestAndSetNative();
   }
   
   static constexpr bool isTestAndSetWaitFree() noexcept
   {
      return Operations::isTestAndSetWaitFree();
   }
   
   bool testAndSetRelaxed(Type expectedValue, Type newValue) noexcept
   {
      return Operations::testAndSetRelaxed(m_atomic, expectedValue, newValue);
   }
   
   bool testAndSetAcquire(Type expectedValue, Type newValue) noexcept
   {
      return Operations::testAndSetAcquire(m_atomic, expectedValue, newValue);
   }
   
   bool testAndSetRelease(Type expectedValue, Type newValue) noexcept
   {
      return Operations::testAndSetRelease(m_atomic, expectedValue, newValue);
   }
   
   bool testAndSetOrdered(Type expectedValue, Type newValue) noexcept
   {
      return Operations::testAndSetOrdered(m_atomic, expectedValue, newValue);
   }
   
   static constexpr bool isFetchAndStoreNative() noexcept
   {
      return Operations::isFetchAndStoreNative();
   }
   
   static constexpr bool isFetchAndStoreWaitFree() noexcept
   {
      return Operations::isFetchAndStoreWaitFree();
   }
   
   Type fetchAndStoreRelaxed(Type newValue) noexcept
   {
      return Operations::fetchAndStoreRelaxed(m_atomic, newValue);
   }
   
   Type fetchAndStoreAcquire(Type newValue) noexcept
   {
      return Operations::fetchAndStoreAcquire(m_atomic, newValue);
   }
   
   Type fetchAndStoreRelease(Type newValue) noexcept
   {
      return Operations::fetchAndStoreRelease(m_atomic, newValue);
   }
   
   Type fetchAndStoreOrdered(Type newValue) noexcept
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
   
   Type fetchAndAddRelaxed(pdk::ptrdiff value) noexcept
   {
      return Operations::fetchAndAddRelaxed(m_atomic, value);
   }
   
   Type fetchAndAddAcquire(pdk::ptrdiff value) noexcept
   {
      return Operations::fetchAndAddAcquire(m_atomic, value);
   }
   
   Type fetchAndAddRelease(pdk::ptrdiff value) noexcept
   {
      return Operations::fetchAndAddRelease(m_atomic, value);
   }
   
   Type fetchAndAddOrdered(pdk::ptrdiff value) noexcept
   {
      return Operations::fetchAndAddOrdered(m_atomic, value);
   }
   
   Type fetchAndSubRelaxed(pdk::ptrdiff value) noexcept
   {
      return Operations::fetchAndSubRelaxed(m_atomic, value);
   }
   
   Type fetchAndSubAcquire(pdk::ptrdiff value) noexcept
   {
      return Operations::fetchAndSubAcquire(m_atomic, value);
   }
   
   Type fetchAndSubRelease(pdk::ptrdiff value) noexcept
   {
      return Operations::fetchAndSubRelease(m_atomic, value);
   }
   
   Type fetchAndSubOrdered(pdk::ptrdiff value) noexcept
   {
      return Operations::fetchAndSubOrdered(m_atomic, value);
   }
   
   Type operator++() noexcept
   {
      return Operations::fetchAndAddOrdered(m_atomic, 1) + 1;
   }
   
   Type operator++(int) noexcept
   {
      return Operations::fetchAndAddOrdered(m_atomic, 1);
   }
   
   Type operator--() noexcept
   {
      return Operations::fetchAndSubOrdered(m_atomic, 1) - 1;
   }
   
   Type operator--(int) noexcept
   {
      return Operations::fetchAndSubOrdered(m_atomic, 1);
   }
   
   Type operator+=(pdk::ptrdiff value) noexcept
   {
      return Operations::fetchAndAddOrdered(m_atomic, value) + value;
   }
   
   Type operator-=(pdk::ptrdiff value) noexcept
   {
      return Operations::fetchAndSubOrdered(m_atomic, value) - value;
   }
   
   operator Type() const noexcept 
   { 
      return loadAcquire(); 
   }
   
   Type operator=(Type value) noexcept {
      storeRelease(value); 
      return value;
   }
public:
   AtomicType m_atomic;
};

#ifndef PDK_BASIC_ATOMIC_INITIALIZER
#  define PDK_BASIC_ATOMIC_INITIALIZER(a) { (a) }
#endif

} // thread
} // os
} // pdk

//PDK_WARNING_POP

#endif // PDK_M_BASE_OS_THREAD_BASIC_ATOMIC_H
