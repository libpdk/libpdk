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
// Created by softboy on 2017/11/24.

#ifndef PDK_M_BASE_OS_THREAD_ATOMIC_CXX11_H
#define PDK_M_BASE_OS_THREAD_ATOMIC_CXX11_H

#include "GenericAtomic.h"
#include <atomic>

namespace pdk {
namespace os {
namespace thread {

template <int N>
struct AtomicTraits
{
   static constexpr inline bool isLockFree();
};

#define PDK_ATOMIC_INT32_IS_SUPPORTED
#if ATOMIC_INT_LOCK_FREE == 2

#  define PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT32_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT32_TEST_AND_SET_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT32_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT32_FETCH_AND_ADD_IS_ALWAYS_NATIVE

template <>
struct AtomicTraits<4>
{
   static constexpr inline bool isLockFree()
   {
      return true;
   }
};

#elif ATOMIC_INT_LOCK_FREE == 1

#  define PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT_TEST_AND_SET_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT_FETCH_AND_ADD_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT32_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT32_TEST_AND_SET_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT32_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT32_FETCH_AND_ADD_IS_SOMETIMES_NATIVE

template <>
struct AtomicTraits<4>
{
   static constexpr inline bool isLockFree()
   {
      return false;
   }
};

#else

#  define PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT_TEST_AND_SET_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT_FETCH_AND_STORE_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT_FETCH_AND_ADD_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT32_REFERENCE_COUNTING_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT32_TEST_AND_SET_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT32_FETCH_AND_STORE_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT32_FETCH_AND_ADD_IS_NEVER_NATIVE

template <>
struct AtomicTraits<4>
{
   static constexpr inline bool isLockFree()
   {
      return false;
   }
};

#endif

#if ATOMIC_POINTER_LOCK_FREE == 2

#  define PDK_ATOMIC_POINTER_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_POINTER_FETCH_AND_ADD_IS_ALWAYS_NATIVE

#elif ATOMIC_POINTER_LOCK_FREE == 1

#  define PDK_ATOMIC_POINTER_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_POINTER_TEST_AND_SET_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_POINTER_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_POINTER_FETCH_AND_ADD_IS_SOMETIMES_NATIVE

#else

#  define PDK_ATOMIC_POINTER_REFERENCE_COUNTING_IS_NEVER_NATIVE
#  define PDK_ATOMIC_POINTER_TEST_AND_SET_IS_NEVER_NATIVE
#  define PDK_ATOMIC_POINTER_FETCH_AND_STORE_IS_NEVER_NATIVE
#  define PDK_ATOMIC_POINTER_FETCH_AND_ADD_IS_NEVER_NATIVE

#endif

template <>
struct AtomicOpsSupport<1>
{
   enum {
      IsSupported = 1
   };
};

#define PDK_ATOMIC_INT8_IS_SUPPORTED

#if ATOMIC_CHAR_LOCK_FREE == 2

#  define PDK_ATOMIC_INT8_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT8_TEST_AND_SET_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT8_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT8_FETCH_AND_ADD_IS_ALWAYS_NATIVE
template <>
struct AtomicTraits<1>
{
   static constexpr inline bool isLockFree()
   {
      return true;
   }
};
#elif ATOMIC_CHAR_LOCK_FREE == 1

#  define PDK_ATOMIC_INT8_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT8_TEST_AND_SET_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT8_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT8_FETCH_AND_ADD_IS_SOMETIMES_NATIVE

template <>
struct AtomicTraits<1>
{
   static constexpr inline bool isLockFree()
   {
      return false;
   }
};

#else

#  define PDK_ATOMIC_INT8_REFERENCE_COUNTING_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT8_TEST_AND_SET_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT8_FETCH_AND_STORE_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT8_FETCH_AND_ADD_IS_NEVER_NATIVE

template <>
struct AtomicTraits<1>
{
   static constexpr inline bool isLockFree()
   {
      return false;
   }
};

#endif

template <>
struct AtomicOpsSupport<2>
{
   enum {
      IsSupported = 1
   };
};

#define PDK_ATOMIC_INT16_IS_SUPPORTED

#if ATOMIC_SHORT_LOCK_FREE == 2

#  define PDK_ATOMIC_INT16_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT16_TEST_AND_SET_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT16_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT16_FETCH_AND_ADD_IS_ALWAYS_NATIVE
template <>
struct AtomicTraits<2>
{
   static constexpr inline bool isLockFree()
   {
      return true;
   }
};
#elif ATOMIC_SHORT_LOCK_FREE == 1

#  define PDK_ATOMIC_INT16_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT16_TEST_AND_SET_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT16_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT16_FETCH_AND_ADD_IS_SOMETIMES_NATIVE

template <>
struct AtomicTraits<2>
{
   static constexpr inline bool isLockFree()
   {
      return false;
   }
};

#else

#  define PDK_ATOMIC_INT16_REFERENCE_COUNTING_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT16_TEST_AND_SET_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT16_FETCH_AND_STORE_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT16_FETCH_AND_ADD_IS_NEVER_NATIVE

template <>
struct AtomicTraits<2>
{
   static constexpr inline bool isLockFree()
   {
      return false;
   }
};

#endif

#ifndef PDK_NO_STD_ATOMIC64

template <>
struct AtomicOpsSupport<8>
{
   enum {
      IsSupported = 1
   };
};

#  define PDK_ATOMIC_INT64_IS_SUPPORTED

#if ATOMIC_SHORT_LOCK_FREE == 2

#  define PDK_ATOMIC_INT64_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT64_TEST_AND_SET_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT64_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#  define PDK_ATOMIC_INT64_FETCH_AND_ADD_IS_ALWAYS_NATIVE
template <>
struct AtomicTraits<8>
{
   static constexpr inline bool isLockFree()
   {
      return true;
   }
};
#elif ATOMIC_SHORT_LOCK_FREE == 1

#  define PDK_ATOMIC_INT64_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT64_TEST_AND_SET_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT64_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
#  define PDK_ATOMIC_INT64_FETCH_AND_ADD_IS_SOMETIMES_NATIVE

template <>
struct AtomicTraits<8>
{
   static constexpr inline bool isLockFree()
   {
      return false;
   }
};

#else

#  define PDK_ATOMIC_INT64_REFERENCE_COUNTING_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT64_TEST_AND_SET_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT64_FETCH_AND_STORE_IS_NEVER_NATIVE
#  define PDK_ATOMIC_INT64_FETCH_AND_ADD_IS_NEVER_NATIVE

template <>
struct AtomicTraits<8>
{
   static constexpr inline bool isLockFree()
   {
      return false;
   }
};

#endif

#endif

template <typename AtomicType>
struct AtomicOperations
{
   using Type = std::atomic<AtomicType>;
   
   template <typename T> static inline 
   T load(const std::atomic<T> &atomicValue) noexcept
   {
      return atomicValue.load(std::memory_order_relaxed);
   }
   
   template <typename T> static inline 
   T load(const volatile std::atomic<T> &atomicValue) noexcept
   {
      return atomicValue.load(std::memory_order_relaxed);
   }
   
   template <typename T> static inline
   T loadAcquire(const std::atomic<T> &atomicValue) noexcept
   {
      return atomicValue.load(std::memory_order_acquire);
   }
   
   template <typename T> static inline
   T loadAcquire(const volatile std::atomic<T> &atomicValue) noexcept
   {
      return atomicValue.load(std::memory_order_acquire);
   }
   
   template <typename T> static inline
   void store(std::atomic<T> &atomicValue, T newValue) noexcept
   {
      atomicValue.store(newValue, std::memory_order_relaxed);
   }
   
   template <typename T> static inline
   void storeRelease(std::atomic<T> &atomicValue, T newValue) noexcept
   {
      atomicValue.store(newValue, std::memory_order_release);
   }
   
   static inline constexpr bool isRefCountingNative() noexcept
   {
      return isTestAndSetNative();
   }
   
   static inline constexpr bool isRefCountingWaitFree() noexcept
   {
      return false;
   }
   
   template <typename T>
   static inline bool ref(std::atomic<T> &atomicValue) noexcept
   {
      return ++atomicValue != 0;
   }
   
   template <typename T>
   static inline bool deref(std::atomic<T> &atomicValue) noexcept
   {
      return --atomicValue != 0;
   }
   
   static inline constexpr bool isTestAndSetNative() noexcept
   {
      AtomicTraits<sizeof(AtomicType)>::isLockFree();
   }
   
   static inline constexpr bool isTestAndSetWaitFree() noexcept
   {
      return false;
   }
   
   template <typename T>
   static bool testAndSetRelaxed(std::atomic<T> &atomicValue, T expectedValue,
                                 T newValue, T *currentValue = nullptr) noexcept
   {
      bool temp = atomicValue.compare_exchange_strong(expectedValue, newValue, std::memory_order_relaxed);
      if (currentValue) {
         *currentValue = expectedValue;
      }
      return temp;
   }
   
   template <typename T>
   static bool testAndSetAcquire(std::atomic<T> &atomicValue, T expectedValue,
                                 T newValue, T *currentValue = nullptr) noexcept
   {
      bool temp = atomicValue.compare_exchange_strong(expectedValue, newValue, std::memory_order_acquire);
      if (currentValue) {
         *currentValue = expectedValue;
      }
      return temp;
   }
   
   template <typename T>
   static bool testAndSetRelease(std::atomic<T> &atomicValue, T expectedValue,
                                 T newValue, T *currentValue = nullptr) noexcept
   {
      bool temp = atomicValue.compare_exchange_strong(expectedValue, newValue, std::memory_order_release);
      if (currentValue) {
         *currentValue = expectedValue;
      }
      return temp;
   }
   
   template <typename T>
   static bool testAndSetOrdered(std::atomic<T> &atomicValue, T expectedValue,
                                 T newValue, T *currentValue = nullptr) noexcept
   {
      bool temp = atomicValue.compare_exchange_strong(expectedValue, newValue, std::memory_order_seq_cst);
      if (currentValue) {
         *currentValue = expectedValue;
      }
      return temp;
   }
   
   static inline constexpr bool isFetchAndStoreNative() noexcept
   {
      return isTestAndSetNative();
   }
   
   static inline constexpr bool isFetchAndStoreWaitFree() noexcept
   {
      return false;
   }
   
   template <typename T>
   static T fetchAndStoreRelaxed(std::atomic<T> &atomicValue, T newValue) noexcept
   {
      return atomicValue.exchange(newValue, std::memory_order_relaxed);
   }
   
   template <typename T>
   static T fetchAndStoreAcquire(std::atomic<T> &atomicValue, T newValue) noexcept
   {
      return atomicValue.exchange(newValue, std::memory_order_acquire);
   }
   
   template <typename T>
   static T fetchAndStoreRelease(std::atomic<T> &atomicValue, T newValue) noexcept
   {
      return atomicValue.exchange(newValue, std::memory_order_release);
   }
   
   template <typename T>
   static T fetchAndStoreOrdered(std::atomic<T> &atomicValue, T newValue) noexcept
   {
      return atomicValue.exchange(newValue, std::memory_order_seq_cst);
   }
   
   static inline constexpr bool isFetchAndAddNative() noexcept
   {
      return isTestAndSetNative();
   }
   
   static inline constexpr bool isFetchAndAddWaitFree() noexcept
   {
      return false;
   }
   
   template <typename T>
   static inline T fetchAndAddRelaxed(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_add(value, std::memory_order_relaxed);
   }
   
   template <typename T>
   static inline T fetchAndAddAcquire(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_add(value, std::memory_order_acquire);
   }
   
   template <typename T>
   static inline T fetchAndAddRelease(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_add(value, std::memory_order_release);
   }
   
   template <typename T>
   static inline T fetchAndAddOrdered(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_add(value, std::memory_order_seq_cst);
   }
   
   template <typename T>
   static inline T fetchAndSubRelaxed(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_sub(value, std::memory_order_relaxed);
   }
   
   template <typename T>
   static inline T fetchAndSubAcquire(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_sub(value, std::memory_order_acquire);
   }
   
   template <typename T>
   static inline T fetchAndSubRelease(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_sub(value, std::memory_order_release);
   }
   
   template <typename T>
   static inline T fetchAndSubOrdered(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_sub(value, std::memory_order_seq_cst);
   }
   
   template <typename T>
   static inline T fetchAndAndRelaxed(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_and(value, std::memory_order_relaxed);
   }
   
   template <typename T>
   static inline T fetchAndAndAcquire(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_and(value, std::memory_order_acquire);
   }
   
   template <typename T>
   static inline T fetchAndAndRelease(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_and(value, std::memory_order_release);
   }
   
   template <typename T>
   static inline T fetchAndAndOrdered(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_and(value, std::memory_order_seq_cst);
   }
   
   template <typename T>
   static inline T fetchAndOrRelaxed(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_or(value, std::memory_order_relaxed);
   }
   
   template <typename T>
   static inline T fetchAndOrAcquire(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_or(value, std::memory_order_acquire);
   }
   
   template <typename T>
   static inline T fetchAndOrRelease(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_or(value, std::memory_order_release);
   }
   
   template <typename T>
   static inline T fetchAndOrOrdered(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_or(value, std::memory_order_seq_cst);
   }
   
   template <typename T>
   static inline T fetchAndXorRelaxed(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_xor(value, std::memory_order_relaxed);
   }
   
   template <typename T>
   static inline T fetchAndXorAcquire(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_xor(value, std::memory_order_acquire);
   }
   
   template <typename T>
   static inline T fetchAndXorRelease(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_xor(value, std::memory_order_release);
   }
   
   template <typename T>
   static inline T fetchAndXorOrdered(std::atomic<T> &atomicValue,
                                      typename AtomicAdditiveType<T>::AdditiveType value) noexcept
   {
      return atomicValue.fetch_xor(value, std::memory_order_seq_cst);
   }
   
};

#if defined(PDK_COMPILER_CONSTEXPR) && defined(PDK_COMPILER_DEFAULT_MEMBERS) && defined(PDK_COMPILER_DELETE_MEMBERS)
#  define PDK_BASIC_ATOMIC_INITIALIZER(atomic) { atomic }
#else
#  define PDK_BASIC_ATOMIC_INITIALIZER(atomic) { ATOMIC_VAR_INIT(atomic) }
#endif

} // thread
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_ATOMIC_CXX11_H
