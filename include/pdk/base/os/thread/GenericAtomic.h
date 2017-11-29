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

#ifndef PDK_M_BASE_OS_THREAD_GENERIC_ATOMIC_H
#define PDK_M_BASE_OS_THREAD_GENERIC_ATOMIC_H

#include "pdk/global/Global.h"

namespace pdk {
namespace os {
namespace thread {

template <int>
struct AtomicOpsSupport
{
   enum {
      IsSupported = 0
   };
};

template <>
struct AtomicOpsSupport<4>
{
   enum {
      IsSupported = 1
   };
};

template <typename T>
struct AtomicAdditiveType
{
   using AdditiveType = T;
   static const int AddScale = 1;
};

template <typename T>
struct AtomicAdditiveType<T *>
{
   using AdditiveType = T;
   static const int AddScale = sizeof(T);
};

template <typename BaseClass>
struct GenericAtomicOps
{
   template <typename T>
   struct AtomicUnderType 
   {
      using Type = T;
      using PointerType = T *;
   };
   
   template <typename AtomicType>
   static void acquireMemoryFence(const AtomicType &value) noexcept
   {
      BaseClass::acquireMemoryFence(value);
   }
   
   template <typename AtomicType>
   static void releaseMemoryFence(const AtomicType &value) noexcept
   {
      BaseClass::releaseMemoryFence(value);
   }
   
   template <typename AtomicType>
   static void orderedMemoryFence(const AtomicType &value) noexcept
   {
      BaseClass::orderedMemoryFence(value);
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE AtomicType load(const AtomicType &value) noexcept
   {
      return value;
   }
   
   template <typename AtomicType, typename RawType>
   static PDK_ALWAYS_INLINE void store(AtomicType &atomicValue, RawType newValue) noexcept
   {
      atomicValue = newValue;
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE AtomicType loadAcquire(const AtomicType &atomicValue) noexcept
   {
      AtomicType temp = *static_cast<const volatile AtomicType *>(&atomicValue);
      BaseClass::acquireMemoryFence(atomicValue);
      return temp;
   }
   
   template <typename AtomicType, typename RawType>
   static PDK_ALWAYS_INLINE void storeRelease(AtomicType &atomicValue, 
                                              RawType newValue)
   {
      BaseClass::releaseMemoryFence(atomicValue);
      *static_cast<volatile AtomicType *>(&atomicValue) = newValue;
   }
   
   static inline constexpr bool isRefCountingNative() noexcept
   {
      return BaseClass::isFetchAndAddNative();
   }
   
   static inline constexpr bool isRefCountingWaitFree() noexcept
   {
      return BaseClass::isFetchAndAddWaitFree();
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE bool ref(AtomicType &atomicValue) noexcept
   {
      return BaseClass::fetchAndAddRelaxed(atomicValue, 1) != AtomicType(-1);
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE bool deref(AtomicType &atomicValue) noexcept
   {
      return BaseClass::fetchAndAddRelaxed(atomicValue, -1) != 1;
   }
   
#if 0
   static inline constexpr bool isTestAndSetNative() noexcept;
   static inline constexpr bool isTestAndSetWaitFree() noexcept;
   template <typename AtomicType, typename RawType> 
   static inline bool testAndSetRelaxed(AtomicType &atomicValue, RawType expectedValue, 
                                        RawType newValue) noexcept;
   template <typename AtomicType, typename RawType> 
   static inline bool testAndSetRelaxed(AtomicType &atomicValue, RawType expectedValue, 
                                        RawType newValue, RawType *currentValue) noexcept;
#endif
   template <typename AtomicType, typename RawType>
   static PDK_ALWAYS_INLINE bool testAndSetAcquire(AtomicType &atomicValue, RawType expectedValue, 
                                                   RawType newValue) noexcept
   {
      bool status = BaseClass::testAndSetRelaxed(atomicValue, expectedValue, newValue);
      BaseClass::acquireMemoryFence(atomicValue);
      return status;
   }
   
   template <typename AtomicType, typename RawType>
   static PDK_ALWAYS_INLINE bool testAndSetRelease(AtomicType &atomicValue, RawType expectedValue, 
                                                   RawType newValue) noexcept
   {
      BaseClass::releaseMemoryFence(atomicValue);
      return BaseClass::testAndSetRelaxed(atomicValue, expectedValue, newValue);
   }
   
   template <typename AtomicType, typename RawType>
   static PDK_ALWAYS_INLINE bool testAndSetOrdered(AtomicType &atomicValue, RawType expectedValue, 
                                                   RawType newValue) noexcept
   {
      BaseClass::orderedMemoryFence(atomicValue);
      return BaseClass::testAndSetRelaxed(atomicValue, expectedValue, newValue);
   }
   
   template <typename AtomicType, typename RawType>
   static PDK_ALWAYS_INLINE bool testAndSetAcquire(AtomicType &atomicValue, RawType expectedValue, 
                                                   RawType newValue, RawType *currentValue) noexcept
   {
      bool status = BaseClass::testAndSetRelaxed(atomicValue, expectedValue, newValue, currentValue);
      BaseClass::acquireMemoryFence(atomicValue);
      return status;
   }
   
   template <typename AtomicType, typename RawType>
   static PDK_ALWAYS_INLINE bool testAndSetRelease(AtomicType &atomicValue, RawType expectedValue, 
                                                   RawType newValue, RawType *currentValue) noexcept
   {
      BaseClass::releaseMemoryFence(atomicValue);
      return BaseClass::testAndSetRelaxed(atomicValue, expectedValue, newValue, currentValue);
   }
   
   template <typename AtomicType, typename RawType>
   static PDK_ALWAYS_INLINE bool testAndSetOrdered(AtomicType &atomicValue, RawType expectedValue, 
                                                   RawType newValue, RawType *currentValue) noexcept
   {
      BaseClass::orderedMemoryFence(atomicValue);
      return BaseClass::testAndSetRelaxed(atomicValue, expectedValue, newValue, currentValue);
   }
   
   static inline constexpr bool isFetchAndStoreNative() noexcept
   {
      return false;
   }
   
   static inline constexpr bool isFetchAndStoreWaitFree() noexcept
   {
      return false;
   }
   
   template <typename AtomicType, typename RawType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndStoreRelaxed(AtomicType &atomicValue, RawType newValue) noexcept
   {
      while(true) {
         AtomicType temp = load(atomicValue);
         if (BaseClass::testAndSetRelaxed(atomicValue, temp, newValue)) {
            return temp;
         }
      }
   }
   
   template <typename AtomicType, typename RawType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndStoreAcquire(AtomicType &atomicValue, RawType newValue) noexcept
   {
      AtomicType temp = BaseClass::fetchAndStoreRelaxed(atomicValue, newValue);
      BaseClass::acquireMemoryFence(atomicValue);
      return temp;
   }
   
   template <typename AtomicType, typename RawType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndStoreRelease(AtomicType &atomicValue, RawType newValue) noexcept
   {
      BaseClass::releaseMemoryFence(atomicValue);
      return BaseClass::fetchAndStoreRelaxed(atomicValue, newValue);
   }
   
   template <typename AtomicType, typename RawType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndStoreOrdered(AtomicType &atomicValue, RawType newValue) noexcept
   {
      BaseClass::orderedMemoryFence(atomicValue);
      return BaseClass::fetchAndStoreRelaxed(atomicValue, newValue);
   }
   
   static inline constexpr bool isFetchAndAddNative() noexcept
   {
      return false;
   }
   
   static inline constexpr bool isFetchAndAddWaitFree() noexcept
   {
      return false;
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndAddRelaxed(AtomicType &atomicValue, 
                                 typename AtomicAdditiveType<AtomicType>::AdditiveType valueToAdd) noexcept
   {
      while(true) {
         AtomicType temp = load(atomicValue);
         if (BaseClass::testAndSetRelaxed(atomicValue, temp, static_cast<AtomicType>(temp + valueToAdd))) {
            return temp;
         }
      }
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndAddAcquire(AtomicType &atomicValue,
                                 typename AtomicAdditiveType<AtomicType>::AdditiveType valueToAdd) noexcept
   {
      AtomicType temp = BaseClass::fetchAndAddRelaxed(atomicValue, valueToAdd);
      BaseClass::acquireMemoryFence(atomicValue);
      return temp;
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndAddRelease(AtomicType &atomicValue,
                                 typename AtomicAdditiveType<AtomicType>::AdditiveType valueToAdd) noexcept
   {
      BaseClass::releaseMemoryFence(atomicValue);
      return BaseClass::fetchAndAddRelaxed(atomicValue, valueToAdd);
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndAddOrdered(AtomicType &atomicValue,
                                 typename AtomicAdditiveType<AtomicType>::AdditiveType valueToAdd) noexcept
   {
      BaseClass::orderedMemoryFence(atomicValue);
      return BaseClass::fetchAndAddRelaxed(atomicValue, valueToAdd);
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndSubRelaxed(AtomicType &atomicValue, 
                                 typename AtomicAdditiveType<AtomicType>::AdditiveType valueToSub) noexcept
   {
      fetchAndAddRelaxed(atomicValue, valueToSub);
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndSubAcquire(AtomicType &atomicValue,
                                 typename AtomicAdditiveType<AtomicType>::AdditiveType valueToSub) noexcept
   {
      AtomicType temp = BaseClass::fetchAndSubRelaxed(atomicValue, valueToSub);
      BaseClass::acquireMemoryFence(atomicValue);
      return temp;
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndSubRelease(AtomicType &atomicValue,
                                 typename AtomicAdditiveType<AtomicType>::AdditiveType valueToSub) noexcept
   {
      BaseClass::releaseMemoryFence(atomicValue);
      return BaseClass::fetchAndSubRelaxed(atomicValue, valueToSub);
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndSubOrdered(AtomicType &atomicValue,
                                 typename AtomicAdditiveType<AtomicType>::AdditiveType valueToSub) noexcept
   {
      BaseClass::orderedMemoryFence(atomicValue);
      return BaseClass::fetchAndSubRelaxed(atomicValue, valueToSub);
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndAndRelaxed(AtomicType &atomicValue,
                                 typename std::enable_if<std::is_integral<AtomicType>::value, AtomicType>::type operand) noexcept
   {
      AtomicType temp = load(atomicValue);
      while(true) {
         if (BaseClass::testAndSetRelaxed(atomicValue, temp, AtomicType(temp & operand), &temp)) {
            return temp;
         }       
      }
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndAndAcquire(AtomicType &atomicValue,
                                 typename std::enable_if<std::is_integral<AtomicType>::value, AtomicType>::type operand) noexcept
   {
      AtomicType temp = BaseClass::fetchAndAndRelaxed(atomicValue, operand);
      BaseClass::acquireMemoryFence(atomicValue);
      return temp;
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndAndRelease(AtomicType &atomicValue,
                                 typename std::enable_if<std::is_integral<AtomicType>::value, AtomicType>::type operand) noexcept
   {
      BaseClass::releaseMemoryFence(atomicValue);
      return BaseClass::fetchAndAndRelaxed(atomicValue, operand);
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndAndOrdered(AtomicType &atomicValue,
                                 typename std::enable_if<std::is_integral<AtomicType>::value, AtomicType>::type operand) noexcept
   {
      BaseClass::orderedMemoryFence(atomicValue);
      return BaseClass::fetchAndAndRelaxed(atomicValue, operand);
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndOrRelaxed(AtomicType &atomicValue,
                                 typename std::enable_if<std::is_integral<AtomicType>::value, AtomicType>::type operand) noexcept
   {
      AtomicType temp = load(atomicValue);
      while(true) {
         if (BaseClass::testAndSetRelaxed(atomicValue, temp, AtomicType(temp | operand), &temp)) {
            return temp;
         }       
      }
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndOrAcquire(AtomicType &atomicValue,
                                 typename std::enable_if<std::is_integral<AtomicType>::value, AtomicType>::type operand) noexcept
   {
      AtomicType temp = BaseClass::fetchAndOrRelaxed(atomicValue, operand);
      BaseClass::acquireMemoryFence(atomicValue);
      return temp;
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndOrRelease(AtomicType &atomicValue,
                                 typename std::enable_if<std::is_integral<AtomicType>::value, AtomicType>::type operand) noexcept
   {
      BaseClass::releaseMemoryFence(atomicValue);
      return BaseClass::fetchAndOrRelaxed(atomicValue, operand);
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndOrOrdered(AtomicType &atomicValue,
                                 typename std::enable_if<std::is_integral<AtomicType>::value, AtomicType>::type operand) noexcept
   {
      BaseClass::orderedMemoryFence(atomicValue);
      return BaseClass::fetchAndOrRelaxed(atomicValue, operand);
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndXorRelaxed(AtomicType &atomicValue,
                                 typename std::enable_if<std::is_integral<AtomicType>::value, AtomicType>::type operand) noexcept
   {
      AtomicType temp = load(atomicValue);
      while(true) {
         if (BaseClass::testAndSetRelaxed(atomicValue, temp, AtomicType(temp ^ operand), &temp)) {
            return temp;
         }       
      }
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndXorAcquire(AtomicType &atomicValue,
                                 typename std::enable_if<std::is_integral<AtomicType>::value, AtomicType>::type operand) noexcept
   {
      AtomicType temp = BaseClass::fetchAndXorRelaxed(atomicValue, operand);
      BaseClass::acquireMemoryFence(atomicValue);
      return temp;
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndXorRelease(AtomicType &atomicValue,
                                 typename std::enable_if<std::is_integral<AtomicType>::value, AtomicType>::type operand) noexcept
   {
      BaseClass::releaseMemoryFence(atomicValue);
      return BaseClass::fetchAndXorRelaxed(atomicValue, operand);
   }
   
   template <typename AtomicType>
   static PDK_ALWAYS_INLINE
   AtomicType fetchAndXorOrdered(AtomicType &atomicValue,
                                 typename std::enable_if<std::is_integral<AtomicType>::value, AtomicType>::type operand) noexcept
   {
      BaseClass::orderedMemoryFence(atomicValue);
      return BaseClass::fetchAndXorRelaxed(atomicValue, operand);
   }
};

} // thread
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_GENERIC_ATOMIC_H
