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
// Created by softboy on 2017/12/05.

#ifndef PDK_KERNEL_ALGORITHMS_H
#define PDK_KERNEL_ALGORITHMS_H

#include "pdk/global/Global.h"

namespace pdk {
namespace kernel {
namespace internal {

#ifdef PDK_CC_CLANG
// Clang had a bug where __builtin_ctz/clz/popcount were not marked as constexpr.
#  if (defined(__apple_build_version__) && __clang_major__ >= 7) || (PDK_CC_CLANG >= 307)
#     define PDK_HAS_CONSTEXPR_BUILTINS
#  endif
#elif defined(PDK_CC_MSVC) && !defined(PDK_CC_INTEL) && !defined(PDK_OS_WINCE) && !defined(PDK_PROCESSOR_ARM)
#  define PDK_HAS_CONSTEXPR_BUILTINS
#elif defined(PDK_CC_GNU)
#  define PDK_HAS_CONSTEXPR_BUILTINS
#endif

#if defined(PDK_HAS_CONSTEXPR_BUILTINS)
#  if defined(PDK_CC_GNU) || defined(PDK_CC_CLANG)
#     define PDK_HAS_BUILTIN_CTZS

constexpr PDK_ALWAYS_INLINE uint pdk_builtin_ctzs(pdk::puint16 value) noexcept
{
#     if PDK_HAS_BUILTIN(__builtin_ctzs)
   return __builtin_ctzs(value);
#     endif
   return __builtin_ctz(value);
}

#     define PDK_HAS_BUILTIN_CLZS
constexpr PDK_ALWAYS_INLINE uint pdk_builtin_clzs(pdk::puint16 value) noexcept
{
#     if PDK_HAS_BUILTIN(__builtin_ctzs)
   return __builtin_clzs(value);
#     endif
   return __builtin_clz(value) - 16U;
}

#     define PDK_HAS_BUILTIN_CTZ
constexpr PDK_ALWAYS_INLINE uint pdk_builtin_ctz(pdk::puint32 value) noexcept
{
   return __builtin_ctz(value);
}

#     define PDK_HAS_BUILTIN_CLZ
constexpr PDK_ALWAYS_INLINE uint pdk_builtin_clz(pdk::puint32 value) noexcept
{
   return __builtin_clz(value);
}

#     define PDK_HAS_BUILTIN_CTZLL
constexpr PDK_ALWAYS_INLINE uint pdk_builtin_ctzll(pdk::puint64 value) noexcept
{
   return __builtin_ctzll(value);
}

#     define PDK_HAS_BUILTIN_CLZLL
constexpr PDK_ALWAYS_INLINE uint pdk_builtin_clzll(pdk::puint64 value) noexcept
{
   return __builtin_clzll(value);
}
#     define PDK_ALGORITHMS_USE_BUILTIN_POPCOUNT
constexpr PDK_ALWAYS_INLINE uint pdk_builtin_popcount(pdk::puint8 value)
{
   return __builtin_popcount(value);
}

constexpr PDK_ALWAYS_INLINE uint pdk_builtin_popcount(pdk::puint16 value)
{
   return __builtin_popcount(value);
}

constexpr PDK_ALWAYS_INLINE uint pdk_builtin_popcount(pdk::puint32 value)
{
   return __builtin_popcount(value);
}

#     define PDK_ALGORITHMS_USE_BUILTIN_POPCOUNTLL
constexpr PDK_ALWAYS_INLINE uint pdk_builtin_popcountll(pdk::puint64 value)
{
   return __builtin_popcountll(value);
}

#  elif defined(PDK_CC_MSVC) && !defined(PDK_OS_WINCE) && !defined(PDK_PROCESSOR_ARM)
#     define PDK_HAS_BUILTIN_CTZ
PDK_ALWAYS_INLINE unsigned long pdk_builtin_ctz(pdk::puint32 value)
{
   unsigned long result;
   _BitScanForward(&result, value);
   return result;
}
#     define PDK_HAS_BUILTIN_CLZ
PDK_ALWAYS_INLINE unsigned long pdk_builtin_clz(pdk::puint32 value)
{
   unsigned long result;
   _BitScanReverse(&result, value);
   // Now Invert the result: clz will count *down* from the msb to the lsb, so the msb index is 31
   // and the lsb index is 0. The result for the index when counting up: msb index is 0 (because it
   // starts there), and the lsb index is 31.
   result ^= sizeof(pdk::puint32) * 8 - 1;
   return result;
}
#     if PDK_PROCESSOR_WORDSIZE == 8
// These are only defined for 64bit builds.
#        define QT_HAS_BUILTIN_CTZLL
PDK_ALWAYS_INLINE unsigned long pdk_builtin_ctzll(pdk::puint64 value)
{
   unsigned long result;
   _BitScanForward64(&result, value);
   return result;
}
// MSVC calls it _BitScanReverse and returns the carry flag, which we don't need
#        define QT_HAS_BUILTIN_CLZLL
PDK_ALWAYS_INLINE unsigned long pdk_builtin_clzll(pdk::puint64 value)
{
   unsigned long result;
   _BitScanReverse64(&result, value);
   // see pdk_builtin_clz
   result ^= sizeof(pdk::puint64) * 8 - 1;
   return result;
}
#     endif //  PDK_PROCESSOR_WORDSIZE == 8
#     define PDK_HAS_BUILTIN_CTZS
PDK_ALWAYS_INLINE uint pdk_builtin_ctzs(pdk::puint16 value) noexcept
{
   return pdk_builtin_ctz(value);
}

#     define PDK_HAS_BUILTIN_CLZS
PDK_ALWAYS_INLINE uint pdk_builtin_clzs(pdk::puint16 value) noexcept
{
   return pdk_builtin_clz(value) - 16U;
}

#     define QALGORITHMS_USE_BUILTIN_POPCOUNT
PDK_ALWAYS_INLINE uint pdk_builtin_popcount(pdk::puint8 value) noexcept
{
   return __popcnt16(value);
}

PDK_ALWAYS_INLINE uint pdk_builtin_popcount(pdk::puint16 value) noexcept
{
   return __popcnt16(value);
}

PDK_ALWAYS_INLINE uint pdk_builtin_popcount(pdk::puint32 value) noexcept
{
   return __popcnt(value);
}
#     if PDK_PROCESSOR_WORDSIZE == 8
#        define PDK_ALGORITHMS_USE_BUILTIN_POPCOUNTLL
PDK_ALWAYS_INLINE uint pdk_builtin_popcountll(pdk::puint64 value) noexcept
{
   return __popcnt64(value);
}
#     endif // PDK_PROCESSOR_WORDSIZE == 8
#  endif
#endif // defined(PDK_HAS_CONSTEXPR_BUILTINS)

} // internal

PDK_DECL_CONST_FUNCTION constexpr inline uint population_count(pdk::puint8 value) noexcept
{
#ifdef PDK_ALGORITHMS_USE_BUILTIN_POPCOUNT
   return internal::pdk_builtin_popcount(value);
#else
   return
         (((value      ) & 0xfff)    * PDK_UINT64_C(0x1001001001001) & PDK_UINT64_C(0x84210842108421)) % 0x1f;
#endif
}

PDK_DECL_CONST_FUNCTION constexpr inline uint population_count(pdk::puint16 value) noexcept
{
#ifdef PDK_ALGORITHMS_USE_BUILTIN_POPCOUNT
   return internal::pdk_builtin_popcount(value);
#else
   return
         (((value      ) & 0xfff)    * PDK_UINT64_C(0x1001001001001) & PDK_UINT64_C(0x84210842108421)) % 0x1f +
         (((value >> 12) & 0xfff)    * PDK_UINT64_C(0x1001001001001) & PDK_UINT64_C(0x84210842108421)) % 0x1f;
#endif
}

PDK_DECL_CONST_FUNCTION constexpr inline uint population_count(pdk::puint32 value) noexcept
{
#ifdef PDK_ALGORITHMS_USE_BUILTIN_POPCOUNT
   return internal::pdk_builtin_popcount(value);
#else
   // See http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
   return 
         (((value      ) & 0xfff)    * PDK_UINT64_C(0x1001001001001) & PDK_UINT64_C(0x84210842108421)) % 0x1f +
         (((value >> 12) & 0xfff)    * PDK_UINT64_C(0x1001001001001) & PDK_UINT64_C(0x84210842108421)) % 0x1f +
         (((value >> 24) & 0xfff)    * PDK_UINT64_C(0x1001001001001) & PDK_UINT64_C(0x84210842108421)) % 0x1f;
#endif
}

PDK_DECL_CONST_FUNCTION constexpr inline uint population_count(pdk::puint64 value) noexcept
{
#ifdef PDK_ALGORITHMS_USE_BUILTIN_POPCOUNTLL
   return internal::pdk_builtin_popcountll(value);
#else
   return
         (((value      ) & 0xfff)    * PDK_UINT64_C(0x1001001001001) & PDK_UINT64_C(0x84210842108421)) % 0x1f +
         (((value >> 12) & 0xfff)    * PDK_UINT64_C(0x1001001001001) & PDK_UINT64_C(0x84210842108421)) % 0x1f +
         (((value >> 24) & 0xfff)    * PDK_UINT64_C(0x1001001001001) & PDK_UINT64_C(0x84210842108421)) % 0x1f +
         (((value >> 36) & 0xfff)    * PDK_UINT64_C(0x1001001001001) & PDK_UINT64_C(0x84210842108421)) % 0x1f +
         (((value >> 48) & 0xfff)    * PDK_UINT64_C(0x1001001001001) & PDK_UINT64_C(0x84210842108421)) % 0x1f +
         (((value >> 60) & 0xfff)    * PDK_UINT64_C(0x1001001001001) & PDK_UINT64_C(0x84210842108421)) % 0x1f;
#endif
}

PDK_DECL_CONST_FUNCTION constexpr inline uint population_count(unsigned long int value) noexcept
{
   return population_count(static_cast<pdk::puint64>(value));
}

#if defined(PDK_CC_GNU) && !defined(PDK_CC_CLANG)
#  undef PDK_ALGORITHMS_USE_BUILTIN_POPCOUNT
#endif

PDK_DECL_RELAXED_CONSTEXPR inline uint count_trailing_zero_bits(pdk::puint8 value) noexcept
{
#if defined(PDK_HAS_BUILTIN_CTZ)
   return value ? internal::pdk_builtin_ctz(value) :  8U;
#else
   unsigned int c = 8; // c will be the number of zero bits on the right
   value &= -static_cast<signed int>(value);
   if (value) c--;
   if (value & 0x0000000F) c -= 4;
   if (value & 0x00000033) c -= 2;
   if (value & 0x00000055) c -= 1;
   return c;
#endif
}

PDK_DECL_RELAXED_CONSTEXPR inline uint count_trailing_zero_bits(pdk::puint16 value) noexcept
{
#if defined(PDK_HAS_BUILTIN_CTZ)
   return value ? internal::pdk_builtin_ctz(value) :  16U;
#else
   unsigned int c = 16; // c will be the number of zero bits on the right
   value &= -static_cast<signed int>(value);
   if (value) c--;
   if (value & 0x000000FF) c -= 8;
   if (value & 0x00000F0F) c -= 4;
   if (value & 0x00003333) c -= 2;
   if (value & 0x00005555) c -= 1;
   return c;
#endif
}

PDK_DECL_RELAXED_CONSTEXPR inline uint count_trailing_zero_bits(pdk::puint32 value) noexcept
{
#if defined(PDK_HAS_BUILTIN_CTZ)
   return value ? internal::pdk_builtin_ctz(value) :  32U;
#else
   // see http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightParallel
   unsigned int c = 32; // c will be the number of zero bits on the right
   value &= -static_cast<signed int>(value);
   if (value) c--;
   if (value & 0x0000FFFF) c -= 16;
   if (value & 0x00FF00FF) c -= 8;
   if (value & 0x0F0F0F0F) c -= 4;
   if (value & 0x33333333) c -= 2;
   if (value & 0x55555555) c -= 1;
   return c;
#endif
}

PDK_DECL_RELAXED_CONSTEXPR inline uint count_trailing_zero_bits(pdk::puint64 value) noexcept
{
#if defined(PDK_HAS_BUILTIN_CTZLL)
   return value ? internal::pdk_builtin_ctzll(value) :  64U;
#else
   pdk::puint32 x = static_cast<pdk::puint32>(value);
   return x ? count_trailing_zero_bits(x)
            : 32 + count_trailing_zero_bits(static_cast<pdk::puint32>(value >> 32));
#endif
}

PDK_DECL_RELAXED_CONSTEXPR inline uint count_trailing_zero_bits(unsigned long value) noexcept
{
   return count_trailing_zero_bits(static_cast<pdk::IntegerForSizeof<long>::Unsigned>(value));
}

PDK_DECL_RELAXED_CONSTEXPR inline uint count_leading_zero_bits(pdk::puint8 value) noexcept
{
#if defined(PDK_HAS_BUILTIN_CLZ)
   return value ? internal::pdk_builtin_clz(value) - 24U : 8U;
#else
   value = value | (value >> 1);
   value = value | (value >> 2);
   value = value | (value >> 4);
   return population_count(static_cast<pdk::puint8>(~value));
#endif
}

PDK_DECL_RELAXED_CONSTEXPR inline uint count_leading_zero_bits(pdk::puint16 value) noexcept
{
#if defined(PDK_HAS_BUILTIN_CLZ)
   return value ? internal::pdk_builtin_clzs(value) : 16U;
#else
   value = value | (value >> 1);
   value = value | (value >> 2);
   value = value | (value >> 4);
   value = value | (value >> 8);
   return population_count(static_cast<pdk::puint16>(~value));
#endif
}

PDK_DECL_RELAXED_CONSTEXPR inline uint count_leading_zero_bits(pdk::puint32 value) noexcept
{
#if defined(PDK_HAS_BUILTIN_CLZ)
   return value ? internal::pdk_builtin_clz(value) : 32U;
#else
   value = value | (value >> 1);
   value = value | (value >> 2);
   value = value | (value >> 4);
   value = value | (value >> 8);
   value = value | (value >> 16);
   return population_count(~value);
#endif
}

PDK_DECL_RELAXED_CONSTEXPR inline uint count_leading_zero_bits(pdk::puint64 value) noexcept
{
#if defined(PDK_HAS_BUILTIN_CLZLL)
   return value ? internal::pdk_builtin_clzll(value) : 64U;
#else
   value = value | (value >> 1);
   value = value | (value >> 2);
   value = value | (value >> 4);
   value = value | (value >> 8);
   value = value | (value >> 16);
   value = value | (value >> 32);
   return population_count(~value);
#endif
}

PDK_DECL_RELAXED_CONSTEXPR inline uint count_leading_zero_bits(unsigned long int value) noexcept
{
   return count_leading_zero_bits(static_cast<pdk::IntegerForSizeof<long>::Unsigned>(value));
}

} // kernel
} // pdk


#endif // PDK_KERNEL_ALGORITHMS_H
