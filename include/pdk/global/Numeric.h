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
// Created by softboy on 2017/12/04.

#ifndef PDK_GLOBAL_NUMERIC_H
#define PDK_GLOBAL_NUMERIC_H

#include "Global.h"
#include "pdk/global/internal/NumericPrivate.h"

namespace pdk {

template <typename T>
inline typename std::enable_if<std::is_unsigned<T>::value, bool>::type
add_overflow(T value1, T value2, T *result)
{
   // unsigned additions are well-defined
   *result = value1 + value2;
   return value1 > static_cast<T>(value1 + value2);
}

template <typename T>
inline typename std::enable_if<std::is_unsigned<T>::value, bool>::type
mul_overflow(T value1, T value2, T *result)
{
   // use the next biggest type
   // Note: for 64-bit systems where __int128 isn't supported, this will cause an error.
   // A fallback is present below.
   using Larger = typename pdk::IntegerForSize<sizeof(T) * 2>::Unsigned;
   Larger lr = static_cast<Larger>(value1) * static_cast<Larger>(value2);
   *result = static_cast<T>(lr);
   return lr > std::numeric_limits<T>::max();
}

#if defined(__SIZEOF_INT128__)
#  define HAVE_MUL64_OVERFLOW
#endif

// GCC 5 and Clang have builtins to detect overflows
#if (defined(PDK_CC_GNU) && !defined(PDK_CC_INTEL) && PDK_CC_GNU >= 500) || PDK_HAS_BUILTIN(__builtin_uadd_overflow)
template <>
inline bool add_overflow(unsigned int value1, unsigned int value2, unsigned int *result)
{
   return __builtin_uadd_overflow(value1, value2, result);
}
#endif

#if (defined(PDK_CC_GNU) && !defined(PDK_CC_INTEL) && PDK_CC_GNU >= 500) || PDK_HAS_BUILTIN(__builtin_uaddl_overflow)
template <>
inline bool add_overflow(unsigned long value1, unsigned long value2, unsigned long *result)
{
   return __builtin_uaddl_overflow(value1, value2, result);
}
#endif

#if (defined(PDK_CC_GNU) && !defined(PDK_CC_INTEL) && PDK_CC_GNU >= 500) || PDK_HAS_BUILTIN(__builtin_uaddll_overflow)
template <>
inline bool add_overflow(unsigned long long value1, unsigned long long value2, unsigned long long *result)
{
   return __builtin_uaddll_overflow(value1, value2, result);
}
#endif

#if (defined(PDK_CC_GNU) && !defined(PDK_CC_INTEL) && PDK_CC_GNU >= 500) || PDK_HAS_BUILTIN(__builtin_umul_overflow)
template <>
inline bool mul_overflow(unsigned int value1, unsigned int value2, unsigned int *result)
{
   return __builtin_umul_overflow(value1, value2, result);
}
#endif

#if (defined(PDK_CC_GNU) && !defined(PDK_CC_INTEL) && PDK_CC_GNU >= 500) || PDK_HAS_BUILTIN(__builtin_umull_overflow)
template <>
inline bool mul_overflow(unsigned long value1, unsigned long value2, unsigned long *result)
{
   return __builtin_umull_overflow(value1, value2, result);
}
#endif

#if (defined(PDK_CC_GNU) && !defined(PDK_CC_INTEL) && PDK_CC_GNU >= 500) || PDK_HAS_BUILTIN(__builtin_umulll_overflow)
template <>
inline bool mul_overflow(unsigned long long value1, unsigned long long value2, unsigned long long *result)
{
   return __builtin_umulll_overflow(value1, value2, result);
}
#  define HAVE_MUL64_OVERFLOW
#endif

#if ((defined(PDK_CC_MSVC) && _MSC_VER >= 1800) || defined(PDK_CC_INTEL)) && defined(PDK_PROCESSOR_X86) && !PDK_HAS_BUILTIN(__builtin_uadd_overflow)
template <> 
inline bool add_overflow(unsigned value1, unsigned value2, unsigned *result)
{
   return _addcarry_u32(0, value1, value2, result);
}
#  ifdef PDK_CC_MSVC // longs are 32-bit
inline bool add_overflow(unsigned long value1, unsigned long value2, unsigned long *result)
{
   return _addcarry_u32(0, value1, value2, reinterpret_cast<unsigned *>(result));
}
#  endif
#endif

#if ((defined(PDK_CC_MSVC) && _MSC_VER >= 1800) || defined(PDK_CC_INTEL)) && defined(PDK_PROCESSOR_X86_64) && !PDK_HAS_BUILTIN(__builtin_uadd_overflow)
template <> 
inline bool add_overflow(puint64 value1, puint64 value2, puint64 *result)
{
   return _addcarry_u64(0, value1, value2, reinterpret_cast<unsigned __int64 *>result);
}
#  ifdef PDK_CC_MSVC  // longs are 64-bit
inline bool add_overflow(unsigned long value1, unsigned long value2, unsigned long *result)
{
   return _addcarry_u64(0, value1, value2, reinterpret_cast<unsigned __int64 *>(result));
}
#  endif
#endif

#if defined(PDK_CC_MSVC) && (defined(PDK_PROCESSOR_X86_64) || defined(PDK_PROCESSOR_IA64)) && !PDK_HAS_BUILTIN(__builtin_uadd_overflow)
#  pragma intrinsic(_umul128)
template <> inline bool mul_overflow(puint64 value1, puint64 value2, puint64 *result)
{
   // use 128-bit multiplication with the _umul128 intrinsic
   // https://msdn.microsoft.com/en-us/library/3dayytw9.aspx
   puint64 high;
   *result = _umul128(value1, value2, &high);
   return high;
}
#  define HAVE_MUL64_OVERFLOW
#endif

#if !defined(HAVE_MUL64_OVERFLOW) && defined(__LP64__)
// no 128-bit multiplication, we need to figure out with a slow division
template <>
inline bool mul_overflow(puint64 value1, puint64 value2, puint64 *result)
{
   if (value2 && value1 > std::numeric_limits<puint64>::max() / value2) {
      return true;
   }
   *result = value1 * value2;
   return false;
}

template <>
inline bool mul_overflow(unsigned long value1, unsigned long value2, unsigned long *result)
{
   return mul_overflow<puint64>(value1, value2, reinterpret_cast<puint64 *>(result));
}
#else
#  undef HAVE_MUL64_OVERFLOW
#endif

//
// Signed overflow math
//
// In C++, signed overflow math is Undefined Behavior. However, many CPUs do implement some way to
// check for overflow. Some compilers expose intrinsics to use this functionality. If the no
// intrinsic is exposed, overflow checking can be done by widening the result type and "manually"
// checking for overflow. Or, alternatively, by using inline assembly to use the CPU features.
//
// Only int overflow checking is implemented, because it's the only one used.
#if (defined(PDK_CC_GNU) && !defined(PDK_CC_INTEL) && PDK_CC_GNU >= 500) || PDK_HAS_BUILTIN(__builtin_sadd_overflow)
inline bool add_overflow(int value1, int value2, int *result)
{
   return __builtin_sadd_overflow(value1, value2, result);
}
#elif defined(PDK_CC_GNU) && defined(PDK_PROCESSOR_X86)

inline bool add_overflow(int value1, int value2, int *result)
{
   puint8 overflow = 0;
   int res = value1;
   asm ("addl %2, %1\n"
   "seto %0"
   : "=q" (overflow), "=r" (res)
      : "r" (value2), "1" (res)
      : "cc"
      );
   *result = res;
   return overflow;
}
#else
inline bool add_overflow(int value1, int value2, int *result)
{
   pint64 t = static_cast<pint64>(value1) + value2;
   *result = static_cast<int>(t);
   return t > std::numeric_limits<int>::max() || t < std::numeric_limits<int>::min();
}
#endif

#if (defined(PDK_CC_GNU) && !defined(PDK_CC_INTEL) && PDK_CC_GNU >= 500) || PDK_HAS_BUILTIN(__builtin_ssub_overflow)
inline bool sub_overflow(int value1, int value2, int *result)
{
   return __builtin_ssub_overflow(value1, value2, result);
}
#elif defined(PDK_CC_GNU) && defined(PDK_PROCESSOR_X86)
inline bool sub_overflow(int value1, int value2, int *result)
{
   puint8 overflow = 0;
   int res = value1;
   asm ("subl %2, %1\n"
   "seto %0"
   : "=q" (overflow), "=r" (res)
      : "r" (value2), "1" (res)
      : "cc"
      );
   *result = res;
   return overflow;
}
#else
inline bool sub_overflow(int value1, int value2, int *result)
{
   pint64 t = static_cast<pint64>(value1) - value2;
   *result = static_cast<int>(t);
   return t > std::numeric_limits<int>::max() || t < std::numeric_limits<int>::min();
}
#endif

#if (defined(PDK_CC_GNU) && !defined(PDK_CC_INTEL) && PDK_CC_GNU >= 500) || PDK_HAS_BUILTIN(__builtin_smul_overflow)
inline bool mul_overflow(int value1, int value2, int *result)
{
   return __builtin_smul_overflow(value1, value2, result);
}
#elif defined(PDK_CC_GNU) && defined(PDK_PROCESSOR_X86)
inline bool mul_overflow(int value1, int value2, int *result)
{
   puint8 overflow = 0;
   int res = value1;
   
   asm ("imul %2, %1\n"
   "seto %0"
   : "=q" (overflow), "=r" (res)
      : "r" (value2), "1" (res)
      : "cc"
      );
   *r = res;
   return overflow;
}
#else
inline bool mul_overflow(int value1, int value2, int *result)
{
   pint64 t = static_cast<pint64>(value1) * value2;
   *result = static_cast<int>(t);
   return t > std::numeric_limits<int>::max() || t < std::numeric_limits<int>::min();
}
#endif

} // pdk

#endif // PDK_GLOBAL_NUMERIC_H
