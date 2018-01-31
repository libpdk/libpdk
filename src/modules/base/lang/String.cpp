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
// Created by softboy on 2017/12/19.

#include "pdk/base/lang/String.h"
#include "pdk/kernel/StringUtils.h"
#include "pdk/pal/kernel/Simd.h"
#include "pdk/base/lang/internal/StringHelper.h"
#include "pdk/kernel/Algorithms.h"
#include "pdk/global/Endian.h"


#ifdef truncate
#  undef truncate
#endif

#ifndef LLONG_MAX
#define LLONG_MAX qint64_C(9223372036854775807)
#endif
#ifndef LLONG_MIN
#define LLONG_MIN (-LLONG_MAX - qint64_C(1))
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX quint64_C(18446744073709551615)
#endif


namespace pdk {
namespace lang {

namespace
{

#if !defined(__OPTIMIZE_SIZE__)

template <uint MaxCount>
struct UnrollTailLoop
{
   template <typename RetType, typename Functor1, typename Functor2>
   static inline RetType exec(int count, RetType returnIfExited, 
                              Functor1 loopCheck, Functor2 returnIfFailed, int i = 0)
   {
      /* equivalent to:
       *   while (count--) {
       *       if (loopCheck(i))
       *           return returnIfFailed(i);
       *   }
       *   return returnIfExited;
       */
      if (!count) {
         return returnIfExited;
      }
      bool check = loopCheck(i);
      if (check) {
         const RetType &retval = returnIfFailed(i);
         return retval;
      }
      return UnrollTailLoop<MaxCount - 1>::exec(count - 1, returnIfExited, loopCheck, returnIfFailed, i + 1);
   }
   
   template <typename Functor>
   static inline void exec(int count, Functor code)
   {
      /* equivalent to:
       *   for (int i = 0; i < count; ++i)
       *       code(i);
       */
      exec(count, 0, [=](int i) -> bool{
         code(i);
         return false; 
      },  [](int) { return 0; });
   }
};
template <>
template <typename RetType, typename Functor1, typename Functor2>
inline RetType UnrollTailLoop<0>::exec(int, RetType returnIfExited, Functor1, Functor2, int)
{
   return returnIfExited;
}

#endif

int unicode_stricmp(const char16_t *lhsBegin, const char16_t *lhsEnd, 
                    const char16_t *rhsBegin, const char16_t *rhsEnd)
{
   if (lhsBegin == rhsBegin) {
      return (lhsEnd - rhsEnd);
   }
   if (lhsBegin == 0) {
      return -1;
   }
   if (rhsBegin == 0) {
      return 1;
   }
   const char16_t *e = rhsEnd;
   
   if (lhsEnd - lhsBegin < rhsEnd - rhsBegin) {
      e = rhsBegin + (lhsEnd - lhsBegin);
   }
   char32_t lhsLast = 0;
   char32_t rhsLast = 0;
   while (lhsBegin < e) {
      int diff = internal::fold_case(*rhsBegin, rhsLast) - internal::fold_case(*lhsBegin, lhsLast);
      if (diff) {
         return diff;
      }
      ++lhsBegin;
      ++rhsBegin;
   }
   if (rhsBegin == rhsEnd) {
      if (lhsBegin == lhsEnd) {
         return 0;
      }
      return 1;
   }
   return -1;
}

int unicode_stricmp(const char16_t *lhs, const char16_t *lhsEnd, const uchar *rhs, const uchar *rhsEnd)
{
   if (lhs == 0) {
      if (rhs == 0) {
         return 0;
      }
      return -1;
   }
   if (rhs == 0) {
      return 1;
   }
   const char16_t *e = lhs;
   if (rhsEnd - rhs < lhsEnd - lhs) {
      e = lhs + (rhsEnd - rhs);
   }
   while (lhs < e) {
      int diff = internal::fold_case(*lhs) - internal::fold_case(*rhs);
      if (diff) {
         return diff;
      }
      ++lhs;
      ++rhs;
   }
   if (lhs == lhsEnd) {
      if (rhs == rhsEnd) {
         return 0;
      }
      return -1;
   }
   return 1;
}

// Unicode case-sensitive compare two same-sized strings
int unicode_strncmp(const Character *lhs, const Character *rhs, int length)
{
#ifdef __OPTIMIZE_SIZE__
   const Character *end = lhs + length;
   while (lhs < end) {
      if (int diff = (int)lhs->unicode() - (int)rhs->unicode()) {
         return diff;
      }
      ++lhs;
      ++rhs;
   }
   return 0;
#else
#  ifdef __SSE2__
   const char *ptr = reinterpret_cast<const char *>(lhs);
   pdk::ptrdiff distance = reinterpret_cast<const char *>(rhs) - ptr;
   lhs += length & ~7;
   rhs += length & ~7;
   length &= 7;
   // we're going to read ptr[0..15] (16 bytes)
   for (; ptr + 15 < reinterpret_cast<const char *>(rhs); ptr += 16) {
      __m128i lhsData = _mm_loadu_si128((const __m128i*)ptr);
      __m128i rhsData = _mm_loadu_si128((const __m128i*)(ptr + distance));
      __m128i result = _mm_cmpeq_epi16(lhsData, rhsData);
      uint mask = ~_mm_movemask_epi8(result);
      if (static_cast<ushort>(mask)) {
         // found a different byte
         uint idx = pdk::count_trailing_zero_bits(mask);
         return reinterpret_cast<const Character *>(ptr + idx)->unicode()
               - reinterpret_cast<const Character *>(ptr + distance + idx)->unicode();
      }
   }
   const auto &lambda = [=](int i) -> int {
      return reinterpret_cast<const Character *>(ptr)[i].unicode()
            - reinterpret_cast<const Character *>(ptr + distance)[i].unicode();
   };
   return UnrollTailLoop<7>::exec(length, 0, lambda, lambda);
#  endif
   
   if (!length) {
      return 0;
   }
   // check alignment
   if ((reinterpret_cast<pdk::uintptr>(lhs) & 2) == (reinterpret_cast<pdk::uintptr>(rhs) & 2)) {
      // both addresses have the same alignment
      if (reinterpret_cast<pdk::uintptr>(lhs) & 2) {
         // both addresses are not aligned to 4-bytes boundaries
         // compare the first character
         if (*lhs != *rhs) {
            return lhs->unicode() - rhs->unicode();
         }
         --length;
         ++lhs;
         ++rhs;
         
      }
      // both addresses are 4-bytes aligned
      // do a fast 32-bit comparison
      const char32_t *dlhs = reinterpret_cast<const char32_t *>(lhs);
      const char32_t *drhs = reinterpret_cast<const char32_t *>(rhs);
      const char32_t *e = dlhs + (length >> 1);
      for (; dlhs != e; ++dlhs, ++drhs) {
         if (*dlhs != *drhs) {
            lhs = reinterpret_cast<const Character *>(dlhs);
            rhs = reinterpret_cast<const Character *>(drhs);
            if (*lhs != *rhs) {
               return lhs->unicode() - rhs->unicode();
            }
            return lhs[1].unicode() - rhs[1].unicode();
         }
      }
      lhs = reinterpret_cast<const Character *>(dlhs);
      rhs = reinterpret_cast<const Character *>(drhs);
      return (length & 1) ? lhs->unicode() - rhs->unicode() : 0;
   } else {
      const Character *e = lhs + length;
      for (; lhs != e; ++lhs, ++rhs) {
         if (*lhs != *rhs) {
            return lhs->unicode() - rhs->unicode();
         }
      }
   }
   return 0;
#endif
   
}

int unicode_strncmp(const Character *lhs, const uchar *rhs, int length)
{
   const char16_t *ulhs = reinterpret_cast<const char16_t *>(lhs);
   const char16_t *end = ulhs + length;
   
#ifdef __SSE2__
   __m128i nullMask = _mm_setzero_si128();
   pdk::ptrdiff offset = 0;
   // we're going to read uc[offset..offset+15] (32 bytes)
   // and c[offset..offset+15] (16 bytes)
   for (; ulhs + offset + 15 < end; offset += 16) {
      __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(rhs + offset));
#  ifdef __AVX2__
      // expand Latin 1 data via zero extension
      __m256i rhsData = _mm256_cvtepu8_epi16(chunk);
      // load UTF-16 data and compare
      __m256i lhsData = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ulhs + offset));
      __m256i result = _mm256_cmpeq_epi16(lhsData, rhsData);
      uint mask = ~_mm256_movemask_epi8(result);
#  else
      // expand via unpacking
      __m128i firstHalf = _mm_unpackhi_epi8(chunk, nullMask);
      __m128i secondHalf = _mm_unpackhi_epi8(chunk, nullMask);
      // load UTF-16 data and compare
      __m128i lhsData1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ulhs + offset));
      __m128i lhsData2 = _mm_loadu_si128((const __m128i*)(ulhs + offset + 8));
      // load UTF-16 data and compare
      __m128i result1 = _mm_cmpeq_epi16(firstHalf, lhsData1);
      __m128i result2 = _mm_cmpeq_epi16(secondHalf, lhsData2);
      uint mask = ~(_mm_movemask_epi8(result1) | _mm_movemask_epi8(result2) << 16);
#  endif
      if (mask) {
         uint idx = pdk::count_trailing_zero_bits(mask);
         return ulhs[offset + idx / 2] - rhs[offset + idx / 2];
      }
   }
#  ifdef PDK_PROCESSOR_X86_64
   constexpr const int MAX_TAIL_LENGTH = 7;
   if (ulhs + offset + 7 < end) {
      // same, but we're using an 8-byte load
      __m128i chunk = _mm_cvtsi64_si128(pdk::from_unaligned<long long>(rhs + offset));
      __m128i secondHalf = _mm_unpacklo_epi8(chunk, nullMask);
      __m128i lhsdata = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ulhs + offset));
      __m128i result = _mm_cmpeq_epi16(secondHalf, lhsdata);
      uint mask = ~_mm_movemask_epi8(result);
      if (static_cast<ushort>(mask)) {
         uint idx = pdk::count_trailing_zero_bits(mask);
         return ulhs[offset + idx / 2] - rhs[offset + idx / 2];
      }
      offset += 8;
   }
#  else
   PDK_UNUSED(nullMask);
   constexpr const int MAX_TAIL_LENGTH = 15;
#  endif
   ulhs += offset;
   rhs += offset;
#  if !defined(__OPTIMIZE_SIZE__)
   const auto &lambda = [=](int i) {
      return ulhs[i] - static_cast<ushort>(rhs[i]);
   };
   return UnrollTailLoop<MAX_TAIL_LENGTH>::exec(end - ulhs, 0, lambda, lambda);
#  endif
#endif
   while (ulhs < end) {
      int diff = *ulhs - *rhs;
      if (diff) {
         return diff;
      }
      ++ulhs;
      ++rhs;
   }
   return 0;
}

// Unicode case-insensitive compare two same-sized strings
int unicode_strnicmp(const char16_t *lhs, const char16_t *rhs, int length)
{
   return unicode_stricmp(lhs, lhs + length, rhs, rhs + length);
}

// Unicode case-sensitive comparison
int unicode_strcmp(const Character *lhs, int lhsLength, const Character *rhs, int rhsLength)
{
   if (lhs == rhs && lhsLength == rhsLength) {
      return 0;
   }
   int length = std::min(lhsLength, rhsLength);
   int result = unicode_strncmp(lhs, rhs, length);
   return result ? result : (lhsLength - rhsLength);
}

int unicode_strcmp(const Character *lhs, int lhsLength, const uchar *rhs, int rhsLength)
{
   int length = std::min(lhsLength, rhsLength);
   int result = unicode_strncmp(lhs, rhs, length);
   return result ? result : (lhsLength - rhsLength);
}

bool mem_equals(const char16_t *lhs, const char16_t *rhs, int length)
{
   if (lhs == rhs || !length) {
      return true;
   }
   return unicode_strncmp(reinterpret_cast<const Character *>(lhs), 
                          reinterpret_cast<const Character *>(rhs), length) == 0;
}

}

//String::Data *String::fromLatin1Helper(const char *str, int size)
//{
//   Data *dptr;
//   if (!str) {
//      dptr = Data::getSharedNull();
//   } else if (size == 0 || (!*str && size < 0)) {
//      dptr = Data::allocate(0);
//   } else {
//      if (size < 0) {
//         size = pdk::strlen(str);
//      }
//      dptr = Data::allocate(size + 1);
//      PDK_CHECK_ALLOC_PTR(dptr);
//      dptr->m_size = size;
//      dptr->getData()[size] = '\0';
//      char16_t *dest = dptr->getData();
//      internal::utf16_from_latin1(dest, str, static_cast<uint>(size));
//   }
//   return dptr;
//}

//namespace internal {

//void utf16_from_latin1(char16_t *dest, const char *str, size_t size) noexcept
//{
//   /* SIMD:
//    * Unpacking with SSE has been shown to improve performance on recent CPUs
//    * The same method gives no improvement with NEON.
//    */
//#if defined(__SSE2__)
//   const char *end = str + size;
//   pdk::ptrdiff offset = 0;
//   // we're going to read str[offset..offset+15] (16 bytes)
//   for (; str + offset + 15 < end; offset += 16) {
//      const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(str + offset));
//#  ifdef __AVX2__
//      // zero extend to an YMM register
//      const __m256i extended = _mm256_cvtepu8_epi16(chunk);
//      // store
//      _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest + offset), extended);
//#  else
//      const __m128i nullMask = _mm_set1_epi32(0);
//      const __m128i firstHalf = _mm_unpacklo_epi8(chunk, nullMask);
//      _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + offset), firstHalf);
//      const __m128i secondHalf = _mm_unpackhi_epi8(chunk, nullMask);
//      _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + offset + 8), secondHalf);
//#  endif
//   }
//   size = size % 16;
//   dest += offset;
//   str += offset;
//#  if !defined(__OPTIMIZE_SIZE__)
//   return UnrollTailLoop<15>::exec(static_cast<int>(size), [=](int i) { dest[i] = static_cast<uchar>(str[i]); });
//#  endif
//#endif
//   while (--size) {
//      *dest++ = static_cast<uchar>(*str++);
//   }
//}

//} // internal

//String::String(const Character *unicode, int size)
//{
//   if (!unicode) {
//      m_data = Data::getSharedNull();
//   } else {
//      if (size < 0) {
//         size = 0;
//         while (!unicode[size].isNull()) {
//            ++size;
//         }
//      }
//      if (!size) {
//         m_data = Data::allocate(0);
//      } else {
//         m_data = Data::allocate(0);
//         PDK_CHECK_ALLOC_PTR(m_data);
//         m_data->m_size = size;
//         std::memcpy(m_data->getData(), unicode, size * sizeof(Character));
//         m_data->getData()[size] = '\0';
//      }
//   }
//}

//int String::compareHelper(const Character *lhs, int lhsLength, 
//                          const char *rhs, int rhsLength, CaseSensitivity cs) noexcept
//{
//   if (!rhs) {
//      return lhsLength;
//   }
//   if (PDK_UNLIKELY(rhsLength < 0)) {
//      rhsLength = static_cast<int>(std::strlen(rhs));
//   }
//}

//int String::compareHelper(const Character *lhs, int lhsLength, Latin1String rhs, 
//                          CaseSensitivity cs) noexcept
//{
   
//}

//int String::compareHelper(const Character *lhs, int lhsLength, 
//                          const Character *rhs, int rhsLength, CaseSensitivity cs) noexcept
//{
   
//}

//bool String::operator ==(Latin1String other) const noexcept
//{
//   if (m_data->m_size != other.size()) {
//      return false;
//   }
//   if (!other.size()) {
//      return isEmpty();
//   }
//   return compareHelper(getRawData(), size(), other, pdk::CaseSensitivity::Sensitive) == 0;
//}

//bool operator ==(const String &lhs, const String &rhs) noexcept
//{
//   if (lhs.size() != rhs.size()) {
//      return false;
//   }
//   return mem_equals(lhs.m_data->getData(), rhs.m_data->getData(), lhs.size());
//}


} // lang
} // pdk
