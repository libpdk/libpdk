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
#include "pdk/base/lang/StringAlgorithms.h"
#include "pdk/base/lang/internal/StringAlgorithmsPrivate.h"
#include "pdk/base/lang/internal/StringHelper.h"
#include "pdk/base/lang/internal/UnicodeTablesPrivate.h"
#include "pdk/base/lang/StringIterator.h"
#include "pdk/base/lang/StringBuilder.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/kernel/StringUtils.h"
#include "pdk/kernel/Algorithms.h"
#include "pdk/pal/kernel/Simd.h"
#include "pdk/global/Endian.h"
#include "pdk/base/text/codecs/TextCodec.h"
#include "pdk/base/text/codecs/internal/UtfCodecPrivate.h"
#include "pdk/base/io/DataStream.h"
#include "pdk/utils/Locale.h"
#include "pdk/utils/internal/LocalePrivate.h"
#include "pdk/global/Logging.h"
#include "pdk/base/io/Debug.h"
#include <cstring>

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

#ifdef PDK_OS_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef truncate
#  undef truncate
#endif

#ifndef LLONG_MAX
#define LLONG_MAX pdk::pint64_C(9223372036854775807)
#endif
#ifndef LLONG_MIN
#define LLONG_MIN (-LLONG_MAX - pdk::pint64_C(1))
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX pdk::puint64_C(18446744073709551615)
#endif

#define IS_RAW_DATA(d) ((d)->m_offset != sizeof(StringData))

#define REHASH(a) \
   if (slminus1 < sizeof(uint) * CHAR_BIT)  \
   hashHaystack -= uint(a) << slminus1; \
   hashHaystack <<= 1

namespace pdk {
namespace lang {

using pdk::text::codecs::internal::Utf8;
using pdk::text::codecs::internal::Utf16;
using pdk::text::codecs::internal::Utf32;
using pdk::ds::VarLengthArray;
using pdk::lang::StringIterator;
using pdk::text::codecs::TextCodec;
using pdk::io::DataStream;
using pdk::utils::Locale;

// forward declare with namespace
namespace internal {
void utf16_from_latin1(char16_t *dest, const char *str, size_t size) noexcept;
void utf16_to_latin1(uchar *dest, const char16_t *src, int length);
} // internal

namespace
{

inline int last_index_of(const Character *haystack, int haystackLen, Character needle,
                         int from, pdk::CaseSensitivity cs);
inline int string_count(const Character *haystack, int haystackLen,
                        const Character *needle, int needleLen,
                        pdk::CaseSensitivity cs);
inline int string_count(const Character *haystack, int haystackLen,
                        Character needle, pdk::CaseSensitivity cs);
int find_latin1_string(const Character *hay, int size, Latin1String needle,
                       int from, pdk::CaseSensitivity cs);
inline bool starts_with(StringView haystack, StringView needle, pdk::CaseSensitivity cs);
inline bool starts_with(StringView haystack, Latin1String needle, pdk::CaseSensitivity cs);
inline bool starts_with(StringView haystack, Character needle, pdk::CaseSensitivity cs);
static inline bool ends_with(StringView haystack, StringView needle, pdk::CaseSensitivity cs);
static inline bool ends_with(StringView haystack, Latin1String needle, pdk::CaseSensitivity cs);
static inline bool ends_with(StringView haystack, Character needle, pdk::CaseSensitivity cs);

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

// Unicode case-insensitive comparison
int unicode_stricmp(const Character *lhsBegin, const Character *lhsEnd, 
                    const Character *rhsBegin, const Character *rhsEnd)
{
   if (lhsBegin == rhsBegin) {
      return (rhsEnd - lhsEnd);
   }
   if (!rhsBegin) {
      return lhsEnd - lhsBegin;
   }
   if (!lhsBegin) {
      return rhsBegin - rhsEnd;
   }
   const Character *end = rhsEnd;
   
   if (lhsEnd - lhsBegin < rhsEnd - rhsBegin) {
      end = rhsBegin + (lhsEnd - lhsBegin);
   }
   char32_t lhsLast = 0;
   char32_t rhsLast = 0;
   while (rhsBegin < end) {
      int diff = internal::fold_case(lhsBegin->unicode(), lhsLast) - internal::fold_case(rhsBegin->unicode(), rhsLast);
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

// Case-insensitive comparison between a Unicode string and a Latin1String
int unicode_stricmp(const Character *lhsBegin, const Character *lhsEnd, const char *rhsBegin, const char *rhsEnd)
{
   if (!rhsBegin) {
      return lhsEnd - lhsBegin;
   }
   if (!lhsBegin) {
      return rhsBegin - rhsEnd;
   }
   const char *end = rhsEnd;
   if (lhsEnd - lhsBegin < rhsEnd - rhsBegin) {
      end = rhsBegin + (lhsEnd - lhsBegin);
   }
   while (rhsBegin < end) {
      int diff = internal::fold_case(lhsBegin->unicode()) - internal::fold_case(static_cast<char16_t>(*rhsBegin));
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

// Unicode case-sensitive compare two same-sized strings
int unicode_strncmp(const Character *lhs, const Character *rhs, int length)
{
#ifdef __OPTIMIZE_SIZE__
   const Character *end = rhs + length;
   while (rhs < end) {
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

template <typename Number>
constexpr int lencmp(Number lhs, Number rhs) noexcept
{
   return lhs == rhs ? 0 :
                       lhs >  rhs ? 1 :
                                    -1 ;
}

// Unicode case-sensitive comparison
int unicode_strcmp(const Character *lhs, int lhsLength, const Character *rhs, int rhsLength)
{
   if (lhs == rhs && lhsLength == rhsLength) {
      return 0;
   }
   int length = std::min(lhsLength, rhsLength);
   int result = unicode_strncmp(lhs, rhs, length);
   return result ? result : lencmp(lhsLength, rhsLength);
}

int unicode_strcmp(const Character *lhs, int lhsLength, const char *rhs, int rhsLength)
{
   int length = std::min(lhsLength, rhsLength);
   int result = unicode_strncmp(lhs, reinterpret_cast<const uchar *>(rhs), length);
   return result ? result : lencmp(lhsLength, rhsLength);
}

int pdk_compare_strings(StringView lhs, StringView rhs, pdk::CaseSensitivity cs) noexcept
{
   if (cs == pdk::CaseSensitivity::Sensitive) {
      return unicode_strcmp(lhs.begin(), lhs.size(), rhs.begin(), rhs.size());
   }
   return unicode_stricmp(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}


int pdk_compare_strings(StringView lhs, Latin1String rhs, pdk::CaseSensitivity cs) noexcept
{
   if (cs == pdk::CaseSensitivity::Sensitive) {
      return unicode_strcmp(lhs.begin(), lhs.size(), rhs.begin(), rhs.size());
   }
   return unicode_stricmp(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

int pdk_compare_strings(Latin1String lhs, StringView rhs, pdk::CaseSensitivity cs) noexcept
{
   return -pdk_compare_strings(rhs, lhs, cs);
}

int pdk_compare_strings(Latin1String lhs, Latin1String rhs, pdk::CaseSensitivity cs) noexcept
{
   if (lhs.isEmpty()) {
      return lencmp(0, rhs.size());
   }
   const auto length = std::min(lhs.size(), rhs.size());
   int result;
   if (cs == pdk::CaseSensitivity::Sensitive) {
      result = pdk::strncmp(lhs.getRawData(), rhs.getRawData(), length);
   } else {
      result = pdk::strnicmp(lhs.getRawData(), rhs.getRawData(), length);
   }
   return result ? result : lencmp(lhs.size(), rhs.size());
}

int find_char(const Character *str, int len, Character ch, int from,
              pdk::CaseSensitivity cs)
{
   const char16_t *s = (const char16_t *)str;
   char16_t c = ch.unicode();
   if (from < 0) {
      from = std::max(from + len, 0);
   }
   if (from < len) {
      const char16_t *n = s + from;
      const char16_t *e = s + len;
      if (cs == pdk::CaseSensitivity::Sensitive) {
#ifdef __SSE2__
         __m128i mch = _mm_set1_epi32(c | (c << 16));
         
         // we're going to read n[0..7] (16 bytes)
         for (const char16_t *next = n + 8; next <= e; n = next, next += 8) {
            __m128i data = _mm_loadu_si128((const __m128i*)n);
            __m128i result = _mm_cmpeq_epi16(data, mch);
            uint mask = _mm_movemask_epi8(result);
            if (ushort(mask)) {
               // found a match
               // same as: return n - s + _bit_scan_forward(mask) / 2
               return (reinterpret_cast<const char *>(n) - reinterpret_cast<const char *>(s)
                       + pdk::count_trailing_zero_bits(mask)) >> 1;
            }
         }
         
#  if !defined(__OPTIMIZE_SIZE__)
         return UnrollTailLoop<7>::exec(e - n, -1,
                                        [=](int i) { return n[i] == c; },
         [=](int i) { return n - s + i; });
#  endif
#endif
         --n;
         while (++n != e) {
            if (*n == c) {
               return  n - s;
            }
         }
      } else {
         c = internal::fold_case(c);
         --n;
         while (++n != e) {
            if (internal::fold_case(*n) == c) {
               return  n - s;
            }
         }    
      }
   }
   return -1;
}

ByteArray pdk_convert_to_latin1(StringView string)
{
   if (PDK_UNLIKELY(string.isNull())) {
      return ByteArray();
   }
   ByteArray ba(string.length(), pdk::Uninitialized);
   // since we own the only copy, we're going to const_cast the constData;
   // that avoids an unnecessary call to detach() and expansion code that will never get used
   internal::utf16_to_latin1(reinterpret_cast<uchar *>(const_cast<char *>(ba.getConstRawData())),
                             reinterpret_cast<const char16_t *>(string.data()), string.length());
   return ba;
}

#if defined(__SSE2__)
inline __m128i merge_question_marks(__m128i chunk)
{
   const __m128i questionMark = _mm_set1_epi16('?');
   
# ifdef __SSE4_2__
   // compare the unsigned shorts for the range 0x0100-0xFFFF
   // note on the use of _mm_cmpestrm:
   //  The MSDN documentation online (http://technet.microsoft.com/en-us/library/bb514080.aspx)
   //  says for range search the following:
   //    For each character c in a, determine whether b0 <= c <= b1 or b2 <= c <= b3
   //
   //  However, all examples on the Internet, including from Intel
   //  (see http://software.intel.com/en-us/articles/xml-parsing-accelerator-with-intel-streaming-simd-extensions-4-intel-sse4/)
   //  put the range to be searched first
   //
   //  Disassembly and instruction-level debugging with GCC and ICC show
   //  that they are doing the right thing. Inverting the arguments in the
   //  instruction does cause a bunch of test failures.
   
   const __m128i rangeMatch = _mm_cvtsi32_si128(0xffff0100);
   const __m128i offLimitMask = _mm_cmpestrm(rangeMatch, 2, chunk, 8,
                                             _SIDD_UWORD_OPS | _SIDD_CMP_RANGES | _SIDD_UNIT_MASK);
   
   // replace the non-Latin 1 characters in the chunk with question marks
   chunk = _mm_blendv_epi8(chunk, questionMark, offLimitMask);
# else
   // SSE has no compare instruction for unsigned comparison.
   // The variables must be shiffted + 0x8000 to be compared
   const __m128i signedBitOffset = _mm_set1_epi16(short(0x8000));
   const __m128i thresholdMask = _mm_set1_epi16(short(0xff + 0x8000));
   
   const __m128i signedChunk = _mm_add_epi16(chunk, signedBitOffset);
   const __m128i offLimitMask = _mm_cmpgt_epi16(signedChunk, thresholdMask);
   
#  ifdef __SSE4_1__
   // replace the non-Latin 1 characters in the chunk with question marks
   chunk = _mm_blendv_epi8(chunk, questionMark, offLimitMask);
#  else
   // offLimitQuestionMark contains '?' for each 16 bits that was off-limit
   // the 16 bits that were correct contains zeros
   const __m128i offLimitQuestionMark = _mm_and_si128(offLimitMask, questionMark);
   
   // correctBytes contains the bytes that were in limit
   // the 16 bits that were off limits contains zeros
   const __m128i correctBytes = _mm_andnot_si128(offLimitMask, chunk);
   
   // merge offLimitQuestionMark and correctBytes to have the result
   chunk = _mm_or_si128(correctBytes, offLimitQuestionMark);
#  endif
# endif
   return chunk;
}
#endif

ByteArray pdk_convert_to_local_8bit(StringView string)
{
   if (string.isNull()) {
      return ByteArray();
   }
   
#ifndef PDK_NO_TEXTCODEC
   TextCodec *localeCodec = TextCodec::getCodecForLocale();
   if (localeCodec)
      return localeCodec->fromUnicode(string);
#endif // PDK_NO_TEXTCODEC
   return pdk_convert_to_latin1(string);
}

ByteArray pdk_convert_to_utf8(StringView str)
{
   if (str.isNull()) {
      return ByteArray();
   }
   return Utf8::convertFromUnicode(str.data(), str.length());
}

std::vector<char32_t> pdk_convert_to_ucs4(StringView string)
{
   std::vector<char32_t> v(string.length());
   char32_t *a = v.data();
   StringIterator iter(string);
   while (iter.hasNext()) {
      *a++ = iter.next();
   }
   v.resize(a - v.data());
   return v;
}

template <typename StringView>
StringView pdk_trimmed(StringView str) noexcept;

} // anonymous namespace

namespace internal {

void utf16_from_latin1(char16_t *dest, const char *str, size_t size) noexcept
{
   /* SIMD:
    * Unpacking with SSE has been shown to improve performance on recent CPUs
    * The same method gives no improvement with NEON.
    */
#if defined(__SSE2__)
   const char *end = str + size;
   pdk::ptrdiff offset = 0;
   // we're going to read str[offset..offset+15] (16 bytes)
   for (; str + offset + 15 < end; offset += 16) {
      const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(str + offset));
#  ifdef __AVX2__
      // zero extend to an YMM register
      const __m256i extended = _mm256_cvtepu8_epi16(chunk);
      // store
      _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest + offset), extended);
#  else
      const __m128i nullMask = _mm_set1_epi32(0);
      const __m128i firstHalf = _mm_unpacklo_epi8(chunk, nullMask);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + offset), firstHalf);
      const __m128i secondHalf = _mm_unpackhi_epi8(chunk, nullMask);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + offset + 8), secondHalf);
#  endif
   }
   size = size % 16;
   dest += offset;
   str += offset;
#  if !defined(__OPTIMIZE_SIZE__)
   return UnrollTailLoop<15>::exec(static_cast<int>(size), [=](int i) { dest[i] = static_cast<uchar>(str[i]); });
#  endif
#endif
   // @TODO optimized for __mips_dsp
   while (--size) {
      *dest++ = static_cast<uchar>(*str++);
   }
}

void utf16_to_latin1(uchar *dest, const char16_t *src, int length)
{
#if defined(__SSE2__)
   uchar *e = dest + length;
   pdk::ptrdiff offset = 0;
   
   // we're going to write to dest[offset..offset+15] (16 bytes)
   for ( ; dest + offset + 15 < e; offset += 16) {
      __m128i chunk1 = _mm_loadu_si128((const __m128i*)(src + offset)); // load
      chunk1 = merge_question_marks(chunk1);
      
      __m128i chunk2 = _mm_loadu_si128((const __m128i*)(src + offset + 8)); // load
      chunk2 = merge_question_marks(chunk2);
      
      // pack the two vector to 16 x 8bits elements
      const __m128i result = _mm_packus_epi16(chunk1, chunk2);
      _mm_storeu_si128((__m128i*)(dest + offset), result); // store
   }
   
   length = length % 16;
   dest += offset;
   src += offset;
   
#  if !defined(__OPTIMIZE_SIZE__)
   return UnrollTailLoop<15>::exec(length, [=](int i) { dest[i] = (src[i]>0xff) ? '?' : (uchar) src[i]; });
#  endif
#endif
   while (length--) {
      *dest++ = (*src > 0xff) ? '?' : (uchar) *src;
      ++src;
   }
}

inline bool is_upper(char ch)
{
   return ch >= 'A' && ch <= 'Z';
}

inline bool is_digit(char ch)
{
   return ch >= '0' && ch <= '9';
}

inline char to_lower(char ch)
{
   if (ch >= 'A' && ch <= 'Z') {
      return ch - 'A' + 'a';
   } else {
      return ch;
   }
}

int find_string(const Character *haystack, int haystackLen, int from,
                const Character *needle, int needleLen, pdk::CaseSensitivity cs);

int find_string_boyer_moore(const Character *haystack, int haystackLen, int from,
                            const Character *needle, int needleLen, pdk::CaseSensitivity cs);

} // internal


pdk::sizetype stringprivate::ustrlen(const char16_t *str) noexcept
{
   pdk::sizetype result = 0;
#ifdef __SSE2__
   // find the 16-byte alignment immediately prior or equal to str
   pdk::uintptr misalignment = reinterpret_cast<pdk::uintptr>(str) & 0xf;
   PDK_ASSERT((misalignment & 1) == 0);
   const char16_t *ptr = str - (misalignment / 2);
   // load 16 bytes and see if we have a null
   // (aligned loads can never segfault)
   const __m128i zeroes = _mm_setzero_si128();
   __m128i data = _mm_load_si128(reinterpret_cast<const __m128i *>(ptr));
   __m128i comparison = _mm_cmpeq_epi16(data, zeroes);
   pdk::puint32 mask = _mm_movemask_epi8(comparison);
   // ignore the result prior to the beginning of str
   mask >>= misalignment;
   // Have we found something in the first block? Need to handle it now
   // because of the left shift above.
   if (mask) {
      return pdk::count_trailing_zero_bits(static_cast<pdk::puint32>(mask)) / 2;
   }
   do {
      ptr += 8;
      data = _mm_load_si128(reinterpret_cast<const __m128i *>(ptr));
      
      comparison = _mm_cmpeq_epi16(data, zeroes);
      mask = _mm_movemask_epi8(comparison);
   } while (mask == 0);
   // found a null
   uint idx = pdk::count_trailing_zero_bits(static_cast<pdk::puint32>(mask));
   return ptr - str + idx / 2;
#endif
   if (sizeof(wchar_t) == sizeof(char16_t))
      return wcslen(reinterpret_cast<const wchar_t *>(str));
   while (*str++)
      ++result;
   return result;
}

int stringprivate::compare_strings(StringView lhs, StringView rhs, CaseSensitivity cs) noexcept
{
   return pdk_compare_strings(lhs, rhs, cs);
}

int stringprivate::compare_strings(StringView lhs, Latin1String rhs, CaseSensitivity cs) noexcept
{
   return pdk_compare_strings(lhs, rhs, cs);
}

int stringprivate::compare_strings(Latin1String lhs, StringView rhs, CaseSensitivity cs) noexcept
{
   return pdk_compare_strings(lhs, rhs, cs);
}

int stringprivate::compare_strings(Latin1String lhs, Latin1String rhs, CaseSensitivity cs) noexcept
{
   return pdk_compare_strings(lhs, rhs, cs);
}

ByteArray stringprivate::convert_to_latin1(StringView str)
{
   return pdk_convert_to_latin1(str);
}

ByteArray stringprivate::convert_to_local_8bit(StringView str)
{
   return pdk_convert_to_local_8bit(str);
}

ByteArray stringprivate::convert_to_utf8(StringView str)
{
   return pdk_convert_to_utf8(str);  
}

std::vector<char32_t> stringprivate::convert_to_ucs4(StringView str)
{
   return pdk_convert_to_ucs4(str);
}

Latin1String stringprivate::trimmed(Latin1String str) noexcept
{
   return pdk_trimmed(str);
}

StringView stringprivate::trimmed(StringView str) noexcept
{
   return pdk_trimmed(str);
}

template <typename Haystack, typename Needle>
bool starts_with_impl(Haystack haystack, Needle needle, pdk::CaseSensitivity cs) noexcept
{
   if (haystack.isNull()) {
      return needle.isNull(); // historical behavior, consider changing in ### @todo.
   }
   const auto haystackLen = haystack.size();
   const auto needleLen = needle.size();
   if (haystackLen == 0) {
      return needleLen == 0;
   }
   if ((pdk::pulonglong)needleLen > (pdk::pulonglong)haystackLen) {
      return false;
   }
   return pdk_compare_strings(haystack.left(needleLen), needle, cs) == 0;
}

namespace {

inline bool starts_with(StringView haystack, StringView needle, pdk::CaseSensitivity cs)
{
   return starts_with_impl(haystack, needle, cs);
}

inline bool starts_with(StringView haystack, Latin1String needle, pdk::CaseSensitivity cs)
{
   return starts_with_impl(haystack, needle, cs);
}

inline bool starts_with(StringView haystack, Character needle, pdk::CaseSensitivity cs)
{
   return haystack.size()
         && (cs == pdk::CaseSensitivity::Sensitive ? haystack.front() == needle
                                                   : internal::fold_case(haystack.front()) == internal::fold_case(needle));
}

template <typename Haystack, typename Needle>
bool ends_with_impl(Haystack haystack, Needle needle, pdk::CaseSensitivity cs) noexcept
{
   if (haystack.isNull()) {
      return needle.isNull(); // historical behavior, consider changing in ###  @todo.
   }
   const auto haystackLen = haystack.size();
   const auto needleLen = needle.size();
   if (haystackLen == 0) {
      return needleLen == 0;
   }
   if ((pdk::pulonglong)haystackLen < (pdk::pulonglong)needleLen) {
      return false;
   }
   return pdk_compare_strings(haystack.right(needleLen), needle, cs) == 0;
}

inline bool ends_with(StringView haystack, StringView needle, pdk::CaseSensitivity cs)
{
   return ends_with_impl(haystack, needle, cs);
}

inline bool ends_with(StringView haystack, Latin1String needle, pdk::CaseSensitivity cs)
{
   return ends_with_impl(haystack, needle, cs);
}

inline bool ends_with(StringView haystack, Character needle, pdk::CaseSensitivity cs)
{
   return haystack.size()
         && (cs == pdk::CaseSensitivity::Sensitive ? haystack.back() == needle
                                                   : internal::fold_case(haystack.back()) == internal::fold_case(needle));
}

} // anonymous namespace

bool stringprivate::starts_with(Latin1String haystack, Latin1String needle, CaseSensitivity cs) noexcept
{
   return starts_with_impl(haystack, needle, cs);
}

bool stringprivate::starts_with(Latin1String haystack, StringView needle, CaseSensitivity cs) noexcept
{
   return starts_with_impl(haystack, needle, cs);
}

bool stringprivate::starts_with(StringView haystack, Latin1String needle, CaseSensitivity cs) noexcept
{
   return starts_with_impl(haystack, needle, cs);
}

bool stringprivate::starts_with(StringView haystack, StringView needle, CaseSensitivity cs) noexcept
{
   return starts_with_impl(haystack, needle, cs);
}

bool stringprivate::ends_with(Latin1String haystack, Latin1String needle, CaseSensitivity cs) noexcept
{
   return ends_with_impl(haystack, needle, cs);
}

bool stringprivate::ends_with(Latin1String haystack, StringView needle, CaseSensitivity cs) noexcept
{
   return ends_with_impl(haystack, needle, cs);
}

bool stringprivate::ends_with(StringView haystack, Latin1String needle, CaseSensitivity cs) noexcept
{
   return ends_with_impl(haystack, needle, cs);
}

bool stringprivate::ends_with(StringView haystack, StringView needle, CaseSensitivity cs) noexcept
{
   return ends_with_impl(haystack, needle, cs);
}

int String::toUcs4Helper(const char16_t *str, int length, char32_t *out)
{
   int count = 0;
   StringIterator i(StringView(str, length));
   while (i.hasNext()) {
      out[count++] = i.next();
   }
   return count;
}

String::String(const Character *unicode, int size)
{
   if (!unicode) {
      m_data = Data::getSharedNull();
   } else {
      if (size < 0) {
         size = 0;
         while (!unicode[size].isNull()) {
            ++size;
         }
      }
      if (!size) {
         m_data = Data::allocate(0);
      } else {
         m_data = Data::allocate(size + 1);
         PDK_CHECK_ALLOC_PTR(m_data);
         m_data->m_size = size;
         std::memcpy(m_data->getData(), unicode, size * sizeof(Character));
         m_data->getData()[size] = '\0';
      }
   }
}

String::String(int size, Character c)
{
   m_data = Data::allocate(size + 1);
   PDK_CHECK_ALLOC_PTR(m_data);
   m_data->m_size = size;
   m_data->getData()[size] = '\0';
}

String::String(int size, pdk::Initialization)
{
   m_data = Data::allocate(size + 1);
   PDK_CHECK_ALLOC_PTR(m_data);
   m_data->m_size = size;
   m_data->getData()[size] = '\0';
}

String::String(Character c)
{
   m_data = Data::allocate(2);
   PDK_CHECK_ALLOC_PTR(m_data);
   m_data->m_size = 1;
   m_data->getData()[0] = c.unicode();
   m_data->getData()[1] = '\0';
}

void String::resize(int size)
{
   if (size < 0) {
      size = 0;
   }
   if (IS_RAW_DATA(m_data) && !m_data->m_ref.isShared() && size < m_data->m_size) {
      m_data->m_size = size;
      return;
   }
   if (m_data->m_ref.isShared() || static_cast<uint>(size) + 1u > m_data->m_alloc) {
      reallocData(static_cast<uint>(size) + 1u, true);
   }
   if (m_data->m_alloc) {
      m_data->m_size = size;
      m_data->getData()[size] = '\0';
   }
}

void String::resize(int size, Character fillChar)
{
   const int oldSize = length();
   resize(size);
   const int difference = length() - oldSize;
   if (difference > 0) {
      std::fill_n(m_data->begin() + oldSize, difference, fillChar.unicode());
   }
}

void String::reallocData(uint alloc, bool grow)
{
   auto allocOptions = m_data->detachFlags();
   if (grow) {
      allocOptions |= ArrayData::Grow;
   }
   if (m_data->m_ref.isShared() || IS_RAW_DATA(m_data)) {
      Data *newDataPtr = Data::allocate(alloc, allocOptions);
      PDK_CHECK_ALLOC_PTR(newDataPtr);
      newDataPtr->m_size = std::min(static_cast<int>(alloc - 1), m_data->m_size);
      std::memcpy(newDataPtr->getData(), m_data->getData(), newDataPtr->m_size * sizeof(Character));
      newDataPtr->getData()[newDataPtr->m_size] = '\0';
      if (!m_data->m_ref.deref()) {
         Data::deallocate(m_data);
      }
      m_data = newDataPtr;
   } else {
      Data *dptr = Data::reallocateUnaligned(m_data, alloc, allocOptions);
      PDK_CHECK_ALLOC_PTR(dptr);
      m_data = dptr;
   }
}

String &String::operator =(const String &other) noexcept
{
   if (this != &other) {
      other.m_data->m_ref.ref();
      if (!m_data->m_ref.deref()) {
         Data::deallocate(m_data);
      }
      m_data = other.m_data;
   }
   return *this;
}

String &String::operator =(Latin1String other)
{
   if (isDetached() && other.size() < capacity()) {
      // assumes m_data->m_alloc == 0 -> !isDetached() (sharedNull)
      m_data->m_size = other.size();
      m_data->getData()[other.size()] = 0;
      internal::utf16_from_latin1(m_data->getData(), other.latin1(), other.size());
   } else {
      *this = fromLatin1(other.latin1(), other.size());
   }
   return *this;
}

String &String::operator =(Character ch)
{
   if (isDetached() && capacity() >= 1) {
      // assumes m_data->m_alloc == 0 -> !isDetached() (sharedNull)
      char16_t *data = m_data->getData();
      data[0] = ch.unicode();
      data[1] = '\0';
      m_data->m_size = 1;
   } else {
      operator =(String(ch));
   }
   return *this;
}

String &String::insert(int i, Latin1String str)
{
   const char *rawStr = str.latin1();
   if (i < 0 || !rawStr || !(*rawStr)) {
      return *this;
   }
   int len = str.size();
   if (PDK_UNLIKELY(i > m_data->m_size)) {
      resize(i + len, Latin1Character(' '));
   } else {
      resize(m_data->m_size + len);
   }
   std::memmove(m_data->getData() + i + len, m_data->getData() + i, (m_data->m_size - i - len) * sizeof(Character));
   internal::utf16_from_latin1(m_data->getData() + i, rawStr, static_cast<uint>(len));
   return *this;
}

String &String::insert(int i, const Character *str, int length)
{
   if (i < 0 || length <= 0) {
      return *this;
   }
   const char16_t *rawStr = reinterpret_cast<const char16_t *>(str);
   if (rawStr >= m_data->getData() && rawStr < m_data->getData() + m_data->m_alloc) {
      // Part of me - take a copy
      char16_t *temp = static_cast<char16_t *>(std::malloc(length * sizeof(Character)));
      PDK_CHECK_ALLOC_PTR(temp);
      std::memcpy(temp, rawStr, length * sizeof(Character));
      insert(i, reinterpret_cast<const Character *>(temp), length);
      std::free(temp);
      return *this;
   }
   if (PDK_UNLIKELY(i > m_data->m_size)) {
      resize(i + length, Latin1Character(' '));
   } else {
      resize(m_data->m_size + length);
   }
   std::memmove(m_data->getData() + i + length, m_data->getData() + i, (m_data->m_size - i - length) * sizeof(Character));
   std::memcpy(m_data->getData() + i, rawStr, length * sizeof(Character));
   return *this;
}

String &String::insert(int i, Character c)
{
   if (i < 0) {
      i += m_data->m_size;
   }
   if (i < 0) {
      return *this;
   }
   if (PDK_UNLIKELY(i > m_data->m_size)) {
      resize(i + 1, Latin1Character(' '));
   } else {
      resize(m_data->m_size + 1);
   }
   std::memmove(m_data->getData() + i + 1, m_data->getData() + i, (m_data->m_size - i - 1) * sizeof(Character));
   m_data->getData()[i] = c.unicode();
   return *this;
}

String &String::append(const String &str)
{
   if (str.m_data != Data::getSharedNull()) {
      if (m_data == Data::getSharedNull()) {
         operator =(str);
      } else {
         if (m_data->m_ref.isShared() || static_cast<uint>(m_data->m_size + str.m_data->m_size) + 1u > m_data->m_alloc) {
            reallocData(static_cast<uint>(m_data->m_size + str.m_data->m_size) + 1u, true);
         }
         std::memcpy(m_data->getData() + m_data->m_size, str.m_data->getData(), str.m_data->m_size * sizeof(Character));
         m_data->m_size += str.m_data->m_size;
         m_data->getData()[m_data->m_size] = '\0';
      }
   }
   return *this;
}

String &String::append(const Character *str, int length)
{
   if (str && length > 0) {
      if (m_data->m_ref.isShared() || static_cast<uint>(m_data->m_size + length) + 1u > m_data->m_alloc) {
         reallocData(static_cast<uint>(m_data->m_size + length) + 1u, true);
      }
      std::memcpy(m_data->getData() + m_data->m_size, str, length * sizeof(Character));
      m_data->m_size += length;
      m_data->getData()[m_data->m_size] = '\0';
   }
   return *this;
}

String &String::append(Latin1String str)
{
   const char *rawStr = str.latin1();
   if (rawStr) {
      int len = str.size();
      if (m_data->m_ref.isShared() || static_cast<uint>(m_data->m_size + len) + 1u > m_data->m_alloc) {
         reallocData(static_cast<uint>(m_data->m_size + len) + 1u, true);
      }
      char16_t *target = m_data->getData() + m_data->m_size;
      internal::utf16_from_latin1(target, rawStr, static_cast<uint>(len));
      target[len] = '\0';
      m_data->m_size += len;
   }
   return *this;
}

String &String::append(Character ch)
{
   if (m_data->m_ref.isShared() || static_cast<uint>(m_data->m_size) + 2u > m_data->m_alloc) {
      reallocData(static_cast<uint>(m_data->m_size) + 2u, true);
   }
   m_data->getData()[m_data->m_size++] = ch.unicode();
   m_data->getData()[m_data->m_size] = '\0';
   return *this;
}

String &String::remove(int pos, int length)
{
   if (pos < 0) {
      pos += m_data->m_size;
   }
   if (static_cast<uint>(pos) >= static_cast<uint>(m_data->m_size)) {
      // range problems
      // do nothing
   } else if (length >= m_data->m_size - pos) {
      // truncate
      resize(pos);
   } else if (length > 0) {
      detach();
      std::memmove(m_data->getData() + pos, m_data->getData() + pos + length,
                   (m_data->m_size - pos - length + 1) * sizeof(Character));
      m_data->m_size -= length;
   }
   return *this;
}

String &String::remove(const String &str, pdk::CaseSensitivity cs)
{
   if (str.m_data->m_size) {
      int i = 0;
      while ((i = indexOf(str, i, cs))) {
         remove(i, str.m_data->m_size);
      }
   }
   return *this;
}

String &String::remove(Character ch, pdk::CaseSensitivity cs)
{
   const int idx = indexOf(ch, 0, cs);
   if (idx != -1) {
      const auto first = begin(); // implicit detach()
      auto last = end();
      if (cs == pdk::CaseSensitivity::Sensitive) {
         last = std::remove(first + idx, last, ch);
      } else {
         const Character c = ch.toCaseFolded();
         auto caseInsensEqual = [c](Character x) {
            return c == x.toCaseFolded();
         };
         last = std::remove_if(first + idx, last, caseInsensEqual);
      }
      resize(last - first);
   }
   return *this;
}

String &String::replace(int pos, int length, const String &after)
{
   return replace(pos, length, after.getRawData(), after.length());
}

String &String::replace(int pos, int length, const Character *after, int alength)
{
   if (static_cast<uint>(pos) > static_cast<uint>(m_data->m_size)) {
      return *this;
   }
   if (length > m_data->m_size - pos) {
      length = m_data->m_size - pos;
   }
   uint index = pos;
   replaceHelper(&index, 1, length, after, alength);
   return *this;
}

String &String::replace(int pos, int length, Character after)
{
   return replace(pos, length, &after, 1);
}

String &String::replace(const String &before, const String &after, pdk::CaseSensitivity cs)
{
   return replace(before.getConstRawData(), before.size(), after.getConstRawData(), after.size(), cs);
}

namespace { // helpers for replace and its helper:
Character *text_copy(const Character *start, int len)
{
   const size_t size = len * sizeof(Character);
   Character *const copy = static_cast<Character *>(std::malloc(size));
   PDK_CHECK_ALLOC_PTR(copy);
   std::memcpy(copy, start, size);
   return copy;
}

bool points_into_range(const Character *ptr, const char16_t *base, int len)
{
   const Character *const start = reinterpret_cast<const Character *>(base);
   return start <= ptr && ptr < start + len;
}
} // end namespace

void String::replaceHelper(uint *indices, int nIndices, int blength, 
                           const Character *after, int alength)
{
   // Copy after if it lies inside our own m_data->getData() area (which we could
   // possibly invalidate via a realloc or modify by replacement).
   Character *afterBuffer = 0;
   if (points_into_range(after, m_data->getData(), m_data->m_size)) {// Use copy in place of vulnerable original:
      after = afterBuffer = text_copy(after, alength);
   }
   try {
      if (blength == alength) {
         // replace in place
         detach();
         for (int i = 0; i < nIndices; ++i) {
            std::memcpy(m_data->getData() + indices[i], after, alength * sizeof(Character));
         }
      } else if (alength < blength) {
         // replace from front
         detach();
         uint to = indices[0];
         if (alength)
            memcpy(m_data->getData()+to, after, alength*sizeof(Character));
         to += alength;
         uint movestart = indices[0] + blength;
         for (int i = 1; i < nIndices; ++i) {
            int msize = indices[i] - movestart;
            if (msize > 0) {
               memmove(m_data->getData() + to, m_data->getData() + movestart, msize * sizeof(Character));
               to += msize;
            }
            if (alength) {
               memcpy(m_data->getData() + to, after, alength * sizeof(Character));
               to += alength;
            }
            movestart = indices[i] + blength;
         }
         int msize = m_data->m_size - movestart;
         if (msize > 0) {
            std::memmove(m_data->getData() + to, m_data->getData() + movestart, msize * sizeof(Character));
         }
         resize(m_data->m_size - nIndices*(blength-alength));
      } else {
         // replace from back
         int adjust = nIndices*(alength - blength);
         int newLen = m_data->m_size + adjust;
         int moveend = m_data->m_size;
         resize(newLen);
         while (nIndices) {
            --nIndices;
            int movestart = indices[nIndices] + blength;
            int insertstart = indices[nIndices] + nIndices*(alength-blength);
            int moveto = insertstart + alength;
            std::memmove(m_data->getData() + moveto, m_data->getData() + movestart,
                         (moveend - movestart)*sizeof(Character));
            std::memcpy(m_data->getData() + insertstart, after, alength * sizeof(Character));
            moveend = movestart - blength;
         }
      }
   } catch(const std::bad_alloc &) {
      std::free(afterBuffer);
      throw;
   }
   std::free(afterBuffer);
}

String &String::replace(const Character *before, int blength, 
                        const Character *after, int alength, pdk::CaseSensitivity cs)
{
   if (m_data->m_size == 0) {
      if (blength) {
         return *this;
      }
   } else {
      if (cs == pdk::CaseSensitivity::Sensitive && before == after && blength == alength) {
         return *this;
      }
   }
   if (alength == 0 && blength == 0)
      return *this;
   
   StringMatcher matcher(before, blength, cs);
   Character *beforeBuffer = 0, *afterBuffer = 0;
   
   int index = 0;
   while (1) {
      uint indices[1024];
      uint pos = 0;
      while (pos < 1024) {
         index = matcher.indexIn(*this, index);
         if (index == -1) {
            break;
         }
         indices[pos++] = index;
         if (blength) {// Step over before:
            index += blength;
         } else {// Only count one instance of empty between any two characters:
            index++;
         }
      }
      if (!pos) { // Nothing to replace
         break;
      }
      
      if (PDK_UNLIKELY(index != -1)) {
         // We're about to change data, that before and after might point
         // into, and we'll need that data for our next batch of indices.
         if (!afterBuffer && points_into_range(after, m_data->getData(), m_data->m_size))
            after = afterBuffer = text_copy(after, alength);
         
         if (!beforeBuffer && points_into_range(before, m_data->getData(), m_data->m_size)) {
            beforeBuffer = text_copy(before, blength);
            matcher = StringMatcher(beforeBuffer, blength, cs);
         }
      }
      replaceHelper(indices, pos, blength, after, alength);
      if (PDK_LIKELY(index == -1)) {// Nothing left to replace
         break;  
      }
      // The call to replace_helper just moved what index points at:
      index += pos * (alength - blength);
   }
   std::free(afterBuffer);
   std::free(beforeBuffer);
   return *this;
}

String &String::replace(Character ch, const String &after, pdk::CaseSensitivity cs)
{
   if (after.m_data->m_size == 0) {
      return remove(ch, cs);
   }
   if (after.m_data->m_size == 1) {
      return replace(ch, after.m_data->getData()[0], cs);
   }
   if (m_data->m_size == 0) {
      return *this;
   }
   char16_t cc = (cs == pdk::CaseSensitivity::Sensitive ? ch.unicode() : ch.toCaseFolded().unicode());
   int index = 0;
   while (1) {
      uint indices[1024];
      uint pos = 0;
      if (cs == pdk::CaseSensitivity::Sensitive) {
         while (pos < 1024 && index < m_data->m_size) {
            if (m_data->getData()[index] == cc) {
               indices[pos++] = index;
            }
            index++;
         }
      } else {
         while (pos < 1024 && index < m_data->m_size) {
            if (Character::toCaseFolded(m_data->getData()[index]) == cc) {
               indices[pos++] = index;
            }
            index++;
         }
      }
      if (!pos) {// Nothing to replace
         break;
      }
      replaceHelper(indices, pos, 1, after.getConstRawData(), after.m_data->m_size);
      if (PDK_LIKELY(index == -1)) {
         // Nothing left to replace
         break;
      }
      // The call to replace_helper just moved what index points at:
      index += pos * (after.m_data->m_size - 1);
   }
   return *this;
}

String &String::replace(Character before, Character after, pdk::CaseSensitivity cs)
{
   if (m_data->m_size) {
      const int idx = indexOf(before, 0, cs);
      if (idx != -1) {
         detach();
         const ushort a = after.unicode();
         char16_t *i = m_data->getData();
         const char16_t *e = i + m_data->m_size;
         i += idx;
         *i = a;
         if (cs == pdk::CaseSensitivity::Sensitive) {
            const char16_t b = before.unicode();
            while (++i != e) {
               if (*i == b) {
                  *i = a;
               }
            }
         } else {
            const char16_t b = internal::fold_case(before.unicode());
            while (++i != e) {
               if (internal::fold_case(*i) == b) {
                  *i = a;
               }
            }
         }
      }
   }
   return *this;
}

String &String::replace(Latin1String before, Latin1String after, pdk::CaseSensitivity cs)
{
   int alen = after.size();
   int blen = before.size();
   VarLengthArray<char16_t> a(alen);
   VarLengthArray<char16_t> b(blen);
   internal::utf16_from_latin1(a.getRawData(), after.latin1(), alen);
   internal::utf16_from_latin1(b.getRawData(), before.latin1(), blen);
   return replace(reinterpret_cast<const Character *>(b.getRawData()), blen, 
                  reinterpret_cast<const Character *>(a.getRawData()), alen, cs);
}

String &String::replace(Latin1String before, const String &after, pdk::CaseSensitivity cs)
{
   int blen = before.size();
   VarLengthArray<char16_t> b(blen);
   internal::utf16_from_latin1(b.getRawData(), before.latin1(), blen);
   return replace(reinterpret_cast<const Character *>(b.getRawData()), blen, 
                  after.getConstRawData(), after.m_data->m_size, cs);
}

String &String::replace(const String &before, Latin1String after, pdk::CaseSensitivity cs)
{
   int alen = after.size();
   VarLengthArray<char16_t> a(alen);
   internal::utf16_from_latin1(a.getRawData(), after.latin1(), alen);
   return replace(before.getConstRawData(), before.m_data->m_size, 
                  reinterpret_cast<const Character *>(a.getRawData()), alen, cs);
}

String &String::replace(Character c, Latin1String after, pdk::CaseSensitivity cs)
{
   int alen = after.size();
   VarLengthArray<char16_t> a(alen);
   internal::utf16_from_latin1(a.getRawData(), after.latin1(), alen);
   return replace(&c, 1, reinterpret_cast<const Character *>(a.getRawData()), alen, cs);
}

bool operator ==(const String &lhs, const String &rhs) noexcept
{
   if (lhs.m_data->m_size != rhs.m_data->m_size) {
      return false;
   }
   return pdk_compare_strings(lhs, rhs, pdk::CaseSensitivity::Sensitive) == 0;
}

bool String::operator ==(Latin1String other) const noexcept
{
   if (m_data->m_size != other.size()) {
      return false;
   }
   return pdk_compare_strings(*this, other, pdk::CaseSensitivity::Sensitive) == 0;
}

bool operator<(const String &lhs, const String &rhs) noexcept
{
   return pdk_compare_strings(lhs, rhs, pdk::CaseSensitivity::Sensitive) < 0;
}

bool String::operator <(Latin1String other) const noexcept
{
   return pdk_compare_strings(*this, other, pdk::CaseSensitivity::Sensitive) < 0;
}

bool String::operator >(Latin1String other) const noexcept
{
   return pdk_compare_strings(*this, other, pdk::CaseSensitivity::Sensitive) > 0;
}

int String::indexOf(const String &needle, int from, CaseSensitivity cs) const
{
   return internal::find_string(unicode(), length(), from, needle.unicode(), needle.length(), cs);
}

int String::indexOf(Latin1String needle, int from, CaseSensitivity cs) const
{
   return find_latin1_string(unicode(), size(), needle, from, cs);
}

int internal::find_string(
      const Character *haystack0, int haystackLen, int from,
      const Character *needle0, int needleLen, pdk::CaseSensitivity cs)
{
   const int l = haystackLen;
   const int sl = needleLen;
   if (from < 0)
      from += l;
   if (uint(sl + from) > (uint)l) {
      return -1;
   }
   if (!sl) {
      return from;
   }
   if (!l) {
      return -1;
   }
   if (sl == 1) {
      return find_char(haystack0, haystackLen, needle0[0], from, cs);
   }
   /*
        We use the Boyer-Moore algorithm in cases where the overhead
        for the skip table should pay off, otherwise we use a simple
        hash function.
    */
   if (l > 500 && sl > 5) {
      return internal::find_string_boyer_moore(haystack0, haystackLen, from,
                                               needle0, needleLen, cs);
   }
   
   
   auto sv = [sl](const char16_t *v) { return StringView(v, sl); };
   /*
        We use some hashing for efficiency's sake. Instead of
        comparing strings, we compare the hash value of str with that
        of a part of this String. Only if that matches, we call
        pdk_string_compare().
    */
   const char16_t *needle = (const char16_t *)needle0;
   const char16_t *haystack = (const char16_t *)haystack0 + from;
   const char16_t *end = (const char16_t *)haystack0 + (l-sl);
   const uint slminus1 = sl - 1;
   uint hashNeedle = 0, hashHaystack = 0;
   int idx;
   
   if (cs == pdk::CaseSensitivity::Sensitive) {
      for (idx = 0; idx < sl; ++idx) {
         hashNeedle = ((hashNeedle<<1) + needle[idx]);
         hashHaystack = ((hashHaystack<<1) + haystack[idx]);
      }
      hashHaystack -= haystack[slminus1];
      
      while (haystack <= end) {
         hashHaystack += haystack[slminus1];
         if (hashHaystack == hashNeedle
             && pdk_compare_strings(sv(needle), sv(haystack), pdk::CaseSensitivity::Sensitive) == 0) {
            return haystack - (const char16_t *)haystack0;
         }
         REHASH(*haystack);
         ++haystack;
      }
   } else {
      const char16_t *haystackStart = (const char16_t *)haystack0;
      for (idx = 0; idx < sl; ++idx) {
         hashNeedle = (hashNeedle<<1) + internal::fold_case(needle + idx, needle);
         hashHaystack = (hashHaystack<<1) + internal::fold_case(haystack + idx, haystackStart);
      }
      hashHaystack -= internal::fold_case(haystack + slminus1, haystackStart);
      
      while (haystack <= end) {
         hashHaystack += internal::fold_case(haystack + slminus1, haystackStart);
         if (hashHaystack == hashNeedle
             && pdk_compare_strings(sv(needle), sv(haystack), pdk::CaseSensitivity::Insensitive) == 0)
            return haystack - (const char16_t *)haystack0;
         
         REHASH(internal::fold_case(haystack, haystackStart));
         ++haystack;
      }
   }
   return -1;
}

int String::indexOf(Character needle, int from, CaseSensitivity cs) const
{
   return find_char(unicode(), length(), needle, from, cs);
}

int String::indexOf(const StringRef &needle, int from, CaseSensitivity cs) const
{
   return internal::find_string(unicode(), length(), from, needle.unicode(), needle.length(), cs);
}

namespace {

int last_index_of_helper(const char16_t *haystack, int from, const char16_t *needle, int sl, pdk::CaseSensitivity cs)
{
   /*
        See indexOf() for explanations.
    */
   
   auto sv = [sl](const char16_t *v) { return StringView(v, sl); };
   
   const char16_t *end = haystack;
   haystack += from;
   const uint slminus1 = sl - 1;
   const char16_t *n = needle+slminus1;
   const char16_t *h = haystack+slminus1;
   uint hashNeedle = 0, hashHaystack = 0;
   int idx;
   
   if (cs == pdk::CaseSensitivity::Sensitive) {
      for (idx = 0; idx < sl; ++idx) {
         hashNeedle = ((hashNeedle<<1) + *(n-idx));
         hashHaystack = ((hashHaystack<<1) + *(h-idx));
      }
      hashHaystack -= *haystack;
      while (haystack >= end) {
         hashHaystack += *haystack;
         if (hashHaystack == hashNeedle
             && pdk_compare_strings(sv(needle), sv(haystack), pdk::CaseSensitivity::Sensitive) == 0) {
            return haystack - end;
         }
         --haystack;
         REHASH(haystack[sl]);
      }
   } else {
      for (idx = 0; idx < sl; ++idx) {
         hashNeedle = ((hashNeedle<<1) + internal::fold_case(n-idx, needle));
         hashHaystack = ((hashHaystack<<1) + internal::fold_case(h-idx, end));
      }
      hashHaystack -= internal::fold_case(haystack, end);
      
      while (haystack >= end) {
         hashHaystack += internal::fold_case(haystack, end);
         if (hashHaystack == hashNeedle
             && pdk_compare_strings(sv(haystack), sv(needle), pdk::CaseSensitivity::Insensitive) == 0) {
            return haystack - end;
         }
         --haystack;
         REHASH(internal::fold_case(haystack + sl, end));
      }
   }
   return -1;
}

inline int last_index_of_helper(
      const StringRef &haystack, int from, const StringRef &needle, pdk::CaseSensitivity cs)
{
   return last_index_of_helper(reinterpret_cast<const char16_t*>(haystack.unicode()), from,
                               reinterpret_cast<const char16_t*>(needle.unicode()), needle.size(), cs);
}

inline int last_index_of_helper(
      const StringRef &haystack, int from, Latin1String needle, pdk::CaseSensitivity cs)
{
   const int size = needle.size();
   VarLengthArray<char16_t> s(size);
   internal::utf16_from_latin1(s.getRawData(), needle.latin1(), size);
   return last_index_of_helper(reinterpret_cast<const char16_t*>(haystack.unicode()), from,
                               s.getRawData(), size, cs);
}

inline int find_latin1_string(const Character *haystack, int size,
                              Latin1String needle,
                              int from, pdk::CaseSensitivity cs)
{
   if (size < needle.size()) {
      return -1;
   }
   const char *latin1 = needle.latin1();
   int len = needle.size();
   VarLengthArray<char16_t> s(len);
   internal::utf16_from_latin1(s.getRawData(), latin1, len);
   return internal::find_string(haystack, size, from,
                                reinterpret_cast<const Character*>(s.getConstRawData()), len, cs);
}

} // anonymous namespace

int String::lastIndexOf(const String &needle, int from, CaseSensitivity cs) const
{
   return StringRef(this).lastIndexOf(StringRef(&needle), from, cs);
}

int String::lastIndexOf(Latin1String needle, int from, CaseSensitivity cs) const
{
   return StringRef(this).lastIndexOf(needle, from, cs);
}

int String::lastIndexOf(Character needle, int from, CaseSensitivity cs) const
{
   return last_index_of(unicode(), size(), needle, from, cs);
}

int String::lastIndexOf(const StringRef &needle, int from, CaseSensitivity cs) const
{
   return StringRef(this).lastIndexOf(needle, from, cs);
}

namespace {

inline int last_index_of(const Character *haystack, int haystackLen, Character needle,
                         int from, pdk::CaseSensitivity cs)
{
   ushort c = needle.unicode();
   if (from < 0) {
      from += haystackLen;
   }
   if (uint(from) >= uint(haystackLen)) {
      return -1;
   }
   if (from >= 0) {
      const char16_t *b = reinterpret_cast<const char16_t*>(haystack);
      const char16_t *n = b + from;
      if (cs == pdk::CaseSensitivity::Sensitive) {
         for (; n >= b; --n) {
            if (*n == c) {
               return n - b;
            }
         } 
      } else {
         c = internal::fold_case(c);
         for (; n >= b; --n) {
            if (internal::fold_case(*n) == c) {
               return n - b;
            }
         }   
      }
   }
   return -1;
}



inline int string_count(const Character *haystack, int haystackLen,
                        const Character *needle, int needleLen,
                        pdk::CaseSensitivity cs)
{
   int num = 0;
   int i = -1;
   if (haystackLen > 500 && needleLen > 5) {
      StringMatcher matcher(needle, needleLen, cs);
      while ((i = matcher.indexIn(haystack, haystackLen, i + 1)) != -1) {
         ++num;
      }
   } else {
      while ((i = internal::find_string(haystack, haystackLen, i + 1, needle, needleLen, cs)) != -1) {
         ++num;
      }
   }
   return num;
}

inline int string_count(const Character *unicode, int size, Character ch,
                        pdk::CaseSensitivity cs)
{
   ushort c = ch.unicode();
   int num = 0;
   const ushort *b = reinterpret_cast<const ushort*>(unicode);
   const ushort *i = b + size;
   if (cs == pdk::CaseSensitivity::Sensitive) {
      while (i != b)
         if (*--i == c) {
            ++num;
         }
   } else {
      c = internal::fold_case(c);
      while (i != b)
         if (internal::fold_case(*(--i)) == c) {
            ++num;
         }
   }
   return num;
}

} // anonymous namespace

int String::count(const String &needle, CaseSensitivity cs) const
{
   return string_count(unicode(), size(), needle.unicode(), needle.size(), cs);
}

int String::count(Character needle, CaseSensitivity cs) const
{
   return string_count(unicode(), size(), needle, cs);
}

int String::count(const StringRef &needle, CaseSensitivity cs) const
{
   return string_count(unicode(), size(), needle.unicode(), needle.size(), cs);
}

String String::section(const String &separator, int start, int end, SectionFlags flags) const
{
   const std::vector<StringRef> sections = splitRef(separator, SplitBehavior::KeepEmptyParts,
                                                    (flags & SectionFlag::CaseInsensitiveSeps) 
                                                    ? pdk::CaseSensitivity::Insensitive 
                                                    : pdk::CaseSensitivity::Sensitive);
   const int sectionsSize = sections.size();
   if (!(flags & SectionFlag::SkipEmpty)) {
      if (start < 0) {
         start += sectionsSize;
      } 
      if (end < 0) {
         end += sectionsSize;
      }
   } else {
      int skip = 0;
      for (int k = 0; k < sectionsSize; ++k) {
         if (sections.at(k).isEmpty()) {
            skip++;
         }
      }
      if (start < 0) {
         start += sectionsSize - skip;
      }
      if (end < 0) {
         end += sectionsSize - skip;
      }
   }
   if (start >= sectionsSize || end < 0 || start > end)
      return String();
   
   String ret;
   int firstIndex = start, lastIndex = end;
   for (int x = 0, i = 0; x <= end && i < sectionsSize; ++i) {
      const StringRef &section = sections.at(i);
      const bool empty = section.isEmpty();
      if (x >= start) {
         if(x == start) {
            firstIndex = i;
         }
         if(x == end) {
            lastIndex = i;
         }
         if (x > start && i > 0) {
            ret += separator;
         }
         ret += section;
      }
      if (!empty || !(flags & SectionFlag::SkipEmpty))
         x++;
   }
   if ((flags & SectionFlag::IncludeLeadingSep) && firstIndex > 0) {
      ret.prepend(separator);
   }
   if ((flags & SectionFlag::IncludeTrailingSep) && lastIndex < sectionsSize - 1) {
      ret += separator;
   }
   return ret;
}

String String::left(int n) const
{
   if (static_cast<uint>(n) >= static_cast<uint>(m_data->m_size)) {
      return *this;
   }  
   return String(reinterpret_cast<const Character*>(m_data->getData()), n);
}

String String::right(int n) const
{
   if (static_cast<uint>(n) >= static_cast<uint>(m_data->m_size))
      return *this;
   return String(reinterpret_cast<const Character*>(m_data->getData() + m_data->m_size - n), n);
}

String String::substring(int pos, int n) const
{
   using pdk::ds::internal::ContainerImplHelper;
   switch (ContainerImplHelper::mid(m_data->m_size, &pos, &n)) {
   case ContainerImplHelper::CutResult::Null:
      return String();
   case ContainerImplHelper::CutResult::Empty:
   {
      StringDataPtr empty = { Data::allocate(0) };
      return String(empty);
   }
   case ContainerImplHelper::CutResult::Full:
      return *this;
   case ContainerImplHelper::CutResult::Subset:
      return String((const Character*)m_data->getData() + pos, n);
   }
   PDK_UNREACHABLE();
   return String();
}

#if PDK_STRINGVIEW_LEVEL < 2

bool String::startsWith(const String &needle, CaseSensitivity cs) const
{
   return starts_with(*this, needle, cs);
}

#endif

bool String::startsWith(Latin1String needle, CaseSensitivity cs) const
{
   return starts_with(*this, needle, cs);  
}

bool String::startsWith(Character needle, CaseSensitivity cs) const
{
   return starts_with(*this, needle, cs);  
}

#if PDK_STRINGVIEW_LEVEL < 2

bool String::startsWith(const StringRef &needle, CaseSensitivity cs) const
{
   return starts_with(*this, needle, cs);  
}

#endif

#if PDK_STRINGVIEW_LEVEL < 2

bool String::endsWith(const String &needle, CaseSensitivity cs) const
{
   return ends_with(*this, needle, cs);  
}

bool String::endsWith(const StringRef &needle, CaseSensitivity cs) const
{
   return ends_with(*this, needle, cs);
}

#endif

bool String::endsWith(Latin1String needle, CaseSensitivity cs) const
{
   return ends_with(*this, needle, cs);
}

bool String::endsWith(Character needle, CaseSensitivity cs) const
{
   return ends_with(*this, needle, cs);
}

ByteArray String::toLatin1Helper(const String &str)
{
   return pdk_convert_to_latin1(str);
}

ByteArray String::toLatin1Helper(const Character *str, int size)
{
   return pdk_convert_to_latin1(StringView(str, size));
}

using pdk::ds::ByteArrayDataPtr;

ByteArray String::toLatin1HelperInplace(String &str)
{
   if (!str.isDetached()) {
      return pdk_convert_to_latin1(str);
   }
   // We can return our own buffer to the caller.
   // Conversion to Latin-1 always shrinks the buffer by half.
   const char16_t *data = reinterpret_cast<const char16_t *>(str.getConstRawData());
   uint length = str.size();
   
   // Swap the d pointers.
   // Kids, avert your eyes. Don't try this at home.
   ArrayData *byteArrayData = str.m_data;
   
   // multiply the allocated capacity by sizeof(ushort)
   byteArrayData->m_alloc *= sizeof(ushort);
   
   // reset ourselves to tring()
   str.m_data = String().m_data;
   
   // do the in-place conversion
   uchar *dst = reinterpret_cast<uchar *>(byteArrayData->getData());
   internal::utf16_to_latin1(dst, data, length);
   dst[length] = '\0';
   
   ByteArrayDataPtr badptr = { byteArrayData };
   return ByteArray(badptr);
}

ByteArray String::toLocal8BitHelper(const Character *str, int size)
{
   return pdk_convert_to_local_8bit(StringView(str, size));
}

ByteArray String::toUtf8Helper(const String &str)
{
   return pdk_convert_to_utf8(str);
}

std::vector<char32_t> String::toUcs4() const
{
   return pdk_convert_to_ucs4(*this);
}

String::Data *String::fromLatin1Helper(const char *str, int size)
{
   Data *data;
   if (!str) {
      data = Data::getSharedNull();
   } else if (size == 0 || (!*str && size < 0)) {
      data = Data::allocate(0);
   } else {
      if (size < 0) {
         size = pdk::strlen(str);
      }
      data = Data::allocate(size + 1);
      PDK_CHECK_ALLOC_PTR(data);
      data->m_size = size;
      data->getData()[size] = '\0';
      char16_t *dest = data->getData();
      internal::utf16_from_latin1(dest, str, static_cast<uint>(size));
   }
   return data;
}

String::Data *String::fromAsciiHelper(const char *str, int size)
{
   String s = fromUtf8(str, size);
   s.m_data->m_ref.ref();
   return s.m_data;
}

String String::fromLocal8BitHelper(const char *str, int size)
{
   if (!str) {
      return String();
   }
   if (size == 0 || (!*str && size < 0)) {
      StringDataPtr empty = { Data::allocate(0) };
      return String(empty);
   }
#if !defined(PDK_NO_TEXTCODEC)
   if (size < 0) {
      size = pdk::strlen(str);
   }
   TextCodec *codec = TextCodec::getCodecForLocale();
   if (codec) {
      return codec->toUnicode(str, size);
   }
#endif // !PDK_NO_TEXTCODEC
   return fromLatin1(str, size);
}

String String::fromUtf8Helper(const char *str, int size)
{
   if (!str) {
      return String();
   }
   PDK_ASSERT(size != -1);
   return Utf8::convertToUnicode(str, size);
}

String String::fromUtf16(const char16_t *str, int size)
{
   if (!str) {
      return String();
   }
   if (size < 0) {
      size = 0;
      while (str[size] != 0) {
         ++size;
      }
   }
   return Utf16::convertToUnicode(reinterpret_cast<const char *>(str), size * 2, 0);
}

String String::fromUcs4(const char32_t *str, int size)
{
   if (!str)
      return String();
   if (size < 0) {
      size = 0;
      while (str[size] != 0) {
         ++size;
      }
   }
   return Utf32::convertToUnicode(reinterpret_cast<const char *>(str), size * 4, 0);
}

String &String::setUnicode(const Character *unicode, int size)
{
   resize(size);
   if (unicode && size) {
      std::memcpy(m_data->getData(), unicode, size * sizeof(Character));
   }
   return *this;
}

String String::simplifiedHelper(const String &str)
{
   return internal::StringAlgorithms<const String>::simplifiedHelper(str);
}

String String::simplifiedHelper(String &str)
{
   return internal::StringAlgorithms<String>::simplifiedHelper(str);
}

namespace {

template <typename StringView>
StringView pdk_trimmed(StringView str) noexcept
{
   auto begin = str.begin();
   auto end = str.end();
   internal::StringAlgorithms<const StringView>::trimmedHelperPositions(begin, end);
   return StringView{begin, end};
}

} // anonymous namespace

String String::trimmedHelper(const String &str)
{
   return internal::StringAlgorithms<const String>::trimmedHelper(str);
}

String String::trimmedHelper(String &str)
{
   return internal::StringAlgorithms<String>::trimmedHelper(str);
}

void String::truncate(int pos)
{
   if (pos < m_data->m_size) {
      resize(pos);
   }
}

void String::chop(int n)
{
   if (n > 0) {
      resize(m_data->m_size - n);
   }
}

String &String::fill(Character ch, int size)
{
   resize(size < 0 ? m_data->m_size : size);
   if (m_data->m_size) {
      Character *i = reinterpret_cast<Character *>(m_data->getData()) + m_data->m_size;
      Character *b = reinterpret_cast<Character *>(m_data->getData());
      while (i != b) {
         *--i = ch;
      }
   }
   return *this;
}

int String::compare(const String &str, CaseSensitivity cs) const noexcept
{
   return pdk_compare_strings(*this, str, cs);
}

int String::compareHelper(const Character *lhs, int lhsLength, 
                          const Character *rhs, int rhsLength, CaseSensitivity cs) noexcept
{
   PDK_ASSERT(lhsLength >= 0);
   PDK_ASSERT(rhsLength >= 0);
   PDK_ASSERT(lhs || lhsLength == 0);
   PDK_ASSERT(rhs || rhsLength == 0);
   return pdk_compare_strings(StringView(lhs, lhsLength),
                              StringView(rhs, rhsLength), cs);
}

int String::compare(Latin1String str, CaseSensitivity cs) const noexcept
{
   return pdk_compare_strings(*this, str, cs);
}

int String::compareHelper(const Character *lhs, int lhsLength, 
                          const char *rhs, int rhsLength, CaseSensitivity cs) noexcept
{
   PDK_ASSERT(lhsLength >= 0);
   PDK_ASSERT(lhs || lhsLength == 0);
   if (!rhs) {
      return lhsLength;
   }  
   if (PDK_UNLIKELY(rhsLength < 0)) {
      rhsLength = int(pdk::strlen(rhs));
   }
   // ### make me nothrow in all cases
   VarLengthArray<char16_t> s2(rhsLength);
   const auto beg = reinterpret_cast<Character *>(s2.getRawData());
   const auto end = Utf8::convertToUnicode(beg, rhs, rhsLength);
   return pdk_compare_strings(StringView(lhs, lhsLength), StringView(beg, end - beg), cs);
}

int String::compareHelper(const Character *lhs, int lhsLength, Latin1String rhs, CaseSensitivity cs) noexcept
{
   PDK_ASSERT(lhsLength >= 0);
   PDK_ASSERT(lhs || lhsLength == 0);
   return pdk_compare_strings(StringView(lhs, lhsLength), rhs, cs);
}

#if !defined(CSTR_LESS_THAN)
#define CSTR_LESS_THAN    1
#define CSTR_EQUAL        2
#define CSTR_GREATER_THAN 3
#endif

int String::localeAwareCompare(const String &other) const
{
   return localeAwareCompareHelper(getConstRawData(), length(), other.getConstRawData(), other.length());  
}

int String::localeAwareCompareHelper(const Character *lhs, int lhsLength, const Character *rhs, int rhsLength)
{
   PDK_ASSERT(lhsLength >= 0);
   PDK_ASSERT(lhs || lhsLength == 0);
   PDK_ASSERT(rhsLength >= 0);
   PDK_ASSERT(rhs || rhsLength == 0);
   
   // do the right thing for null and empty
   if (lhsLength == 0 || rhsLength == 0) {
      return pdk_compare_strings(StringView(lhs, lhsLength), StringView(rhs, rhsLength),
                                 pdk::CaseSensitivity::Sensitive);
   }
   
#if defined(PDK_OS_WIN)
   int res = CompareStringEx(LOCALE_NAME_USER_DEFAULT, 0, (LPCWSTR)lhs, lhsLength, (LPCWSTR)rhs, rhsLength, NULL, NULL, 0);
   switch (res) {
   case CSTR_LESS_THAN:
      return -1;
   case CSTR_GREATER_THAN:
      return 1;
   default:
      return 0;
   }
#elif defined (PDK_OS_MAC)
   // Use CFStringCompare for comparing strings on Mac. This makes pdk order
   // strings the same way as native applications do, and also respects
   // the "Order for sorted lists" setting in the International preferences
   // panel.
   const CFStringRef thisString =
         CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault,
                                            reinterpret_cast<const UniChar *>(lhs), lhsLength, kCFAllocatorNull);
   const CFStringRef otherString =
         CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault,
                                            reinterpret_cast<const UniChar *>(rhs), rhsLength, kCFAllocatorNull);
   
   const int result = CFStringCompare(thisString, otherString, kCFCompareLocalized);
   CFRelease(thisString);
   CFRelease(otherString);
   return result;
#elif PDK_CONFIG(icu)
   if (!defaultCollator()->hasLocalData())
      defaultCollator()->setLocalData(QCollator());
   return defaultCollator()->localData().compare(data1, length1, data2, length2);
#elif defined(PDK_OS_UNIX)
   // declared in <string.h>
   int delta = strcoll(toLocal8BitHelper(lhs, lhsLength).getConstRawData(), 
                       toLocal8BitHelper(rhs, rhsLength).getConstRawData());
   if (delta == 0) {
      delta = pdk_compare_strings(StringView(lhs, lhsLength), StringView(rhs, rhsLength),
                                  pdk::CaseSensitivity::Sensitive);
   }
   return delta;
#else
   return pdk_compare_strings(StringView(lhs, lhsLength), StringView(rhs, rhsLength),
                              pdk::CaseSensitivity::Sensitive);
#endif
}

const char16_t *String::utf16() const
{
   if (IS_RAW_DATA(m_data)) {
      // ensure '\0'-termination for ::fromRawData strings
      const_cast<String *>(this)->reallocData(static_cast<uint>(m_data->m_size) + 1u);
   }
   return m_data->getData();
}

String String::leftJustified(int width, Character fill, bool truncate) const
{
   String result;
   int len = length();
   int padlen = width - len;
   if (padlen > 0) {
      result.resize(len + padlen);
      if (len) {
         std::memcpy(result.m_data->getData(), m_data->getData(), sizeof(Character)*len);
      }
      Character *uc = reinterpret_cast<Character *>(result.m_data->getData()) + len;
      while (padlen--) {
         * uc++ = fill;
      }
   } else {
      if (truncate) {
         result = left(width);
      } else {
         result = *this;
      }
   }
   return result;
}

String String::rightJustified(int width, Character fill, bool truncate) const
{
   String result;
   int len = length();
   int padlen = width - len;
   if (padlen > 0) {
      result.resize(len+padlen);
      Character *uc = reinterpret_cast<Character *>(result.m_data->getData());
      while (padlen--) {
         * uc++ = fill;
      }
      if (len) {
         std::memcpy(uc, m_data->getData(), sizeof(Character) * len);
      }
   } else {
      if (truncate) {
         result = left(width);
      } else {
         result = *this;
      }  
   }
   return result;
}

namespace internal {
namespace unicodetables {
namespace {

template <typename Traits, typename T>
PDK_NEVER_INLINE
String detach_and_convert_case(T &str, StringIterator it)
{
   PDK_ASSERT(!str.isEmpty());
   String s = std::move(str);             // will copy if T is const String
   Character *pp = s.begin() + it.index(); // will detach if necessary
   
   do {
      char32_t uc = it.nextUnchecked();
      const Properties *prop = get_unicode_properties(uc);
      signed short caseDiff = Traits::caseDiff(prop);
      
      if (PDK_UNLIKELY(Traits::caseSpecial(prop))) {
         const char16_t *specialCase = special_case_map + caseDiff;
         ushort length = *specialCase++;
         
         if (PDK_LIKELY(length == 1)) {
            *pp++ = Character(*specialCase);
         } else {
            // slow path: the string is growing
            int inpos = it.index() - 1;
            int outpos = pp - s.constBegin();
            s.replace(outpos, 1, reinterpret_cast<const Character *>(specialCase), length);
            pp = const_cast<Character *>(s.constBegin()) + outpos + length;
            
            // do we need to adjust the input iterator too?
            // if it is pointing to s's data, str is empty
            if (str.isEmpty()) {
               it = StringIterator(s.constBegin(), inpos + length, s.constEnd());
            } 
         }
      } else if (PDK_UNLIKELY(Character::requiresSurrogates(uc))) {
         // so far, case convertion never changes planes (guaranteed by the qunicodetables generator)
         pp++;
         *pp++ = Character::getLowSurrogate(uc + caseDiff);
      } else {
         *pp++ = Character(uc + caseDiff);
      }
   } while (it.hasNext());
   
   return s;
}

template <typename Traits, typename T>
String convert_case(T &str)
{
   const Character *p = str.constBegin();
   const Character *e = p + str.size();
   
   // this avoids out of bounds check in the loop
   while (e != p && e[-1].isHighSurrogate()) {
      --e;
   }
   StringIterator it(p, e);
   while (it.hasNext()) {
      char32_t uc = it.nextUnchecked();
      if (Traits::caseDiff(get_unicode_properties(uc))) {
         it.recedeUnchecked();
         return detach_and_convert_case<Traits>(str, it);
      }
   }
   return std::move(str);
}

} // anonymous namespace
} // unicodetables
} // internal

String String::toLowerHelper(const String &str)
{
   return internal::unicodetables::convert_case<internal::unicodetables::LowercaseTraits>(str);
}

String String::toLowerHelper(String &str)
{
   return internal::unicodetables::convert_case<internal::unicodetables::LowercaseTraits>(str);
}

String String::toCaseFoldedHelper(const String &str)
{
   return internal::unicodetables::convert_case<internal::unicodetables::CasefoldTraits>(str);
}

String String::toCaseFoldedHelper(String &str)
{
   return internal::unicodetables::convert_case<internal::unicodetables::CasefoldTraits>(str);
}

String String::toUpperHelper(const String &str)
{
   return internal::unicodetables::convert_case<internal::unicodetables::UppercaseTraits>(str);
}

String String::toUpperHelper(String &str)
{
   return internal::unicodetables::convert_case<internal::unicodetables::UppercaseTraits>(str);
}

String String::asprintf(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   const String s = vasprintf(format, ap);
   va_end(ap);
   return s;
}

namespace {

void append_utf8(String &qs, const char *cs, int len)
{
   const int oldSize = qs.size();
   qs.resize(oldSize + len);
   const Character *newEnd = Utf8::convertToUnicode(qs.getRawData() + oldSize, cs, len);
   qs.resize(newEnd - qs.getConstRawData());
}

using pdk::utils::internal::LocaleData;

LocaleData::Flags parse_flag_characters(const char * &c) noexcept
{
   LocaleData::Flags flags = LocaleData::Flag::ZeroPadExponent;
   while (true) {
      switch (*c) {
      case '#':
         flags |= LocaleData::Flag::ShowBase;
         flags |= LocaleData::Flag::AddTrailingZeroes;
         flags|= LocaleData::Flag::ForcePoint;
         break;
      case '0': flags |= LocaleData::Flag::ZeroPadded; break;
      case '-': flags |= LocaleData::Flag::LeftAdjusted; break;
      case ' ': flags |= LocaleData::Flag::BlankBeforePositive; break;
      case '+': flags |= LocaleData::Flag::AlwaysShowSign; break;
      case '\'': flags |= LocaleData::Flag::ThousandsGroup; break;
      default: return flags;
      }
      ++c;
   }
}

int parse_field_width(const char * &c)
{
   PDK_ASSERT(internal::is_digit(*c));
   
   // can't be negative - started with a digit
   // contains at least one digit
   char *endp;
   bool ok = true;
   const pdk::pulonglong result = std::strtoull(c, &endp, 10);
   if (errno == ERANGE){
      errno = 0;
      ok = false;
   }
   c = endp;
   while (internal::is_digit(*c)) {// preserve the behavior of consuming all digits, no matter how many
      ++c;
   }
   return ok && result < pdk::pulonglong(std::numeric_limits<int>::max()) ? int(result) : 0;
}

enum class LengthMod { lm_none, lm_hh, lm_h, lm_l, lm_ll, lm_L, lm_j, lm_z, lm_t };

inline bool can_consume(const char * &c, char ch) noexcept
{
   if (*c == ch) {
      ++c;
      return true;
   }
   return false;
}

LengthMod parse_length_modifier(const char * &c) noexcept
{
   switch (*c++) {
   case 'h': return can_consume(c, 'h') ? LengthMod::lm_hh : LengthMod::lm_h;
   case 'l': return can_consume(c, 'l') ? LengthMod::lm_ll : LengthMod::lm_l;
   case 'L': return LengthMod::lm_L;
   case 'j': return LengthMod::lm_j;
   case 'z':
   case 'Z': return LengthMod::lm_z;
   case 't': return LengthMod::lm_t;
   }
   --c; // don't consume *c - it wasn't a flag
   return LengthMod::lm_none;
}

} // anonymous namespace

String String::vasprintf(const char *format, va_list ap)
{
   if (!format || !*format) {
      return fromLatin1("");
   }
   
   // Parse format
   
   String result;
   const char *c = format;
   for (;;) {
      // Copy non-escape chars to result
      const char *cb = c;
      while (*c != '\0' && *c != '%') {
         c++;
      }
      append_utf8(result, cb, int(c - cb));
      if (*c == '\0') {
         break;
      }
      // Found '%'
      const char *escapeStart = c;
      ++c;
      
      if (*c == '\0') {
         result.append(Latin1Character('%')); // a % at the end of the string - treat as non-escape text
         break;
      }
      if (*c == '%') {
         result.append(Latin1Character('%')); // %%
         ++c;
         continue;
      }
      
      LocaleData::Flags flags = parse_flag_characters(c);
      
      if (*c == '\0') {
         result.append(Latin1String(escapeStart)); // incomplete escape, treat as non-escape text
         break;
      }
      
      // Parse field width
      int width = -1; // -1 means unspecified
      if (internal::is_digit(*c)) {
         width = parse_field_width(c);
      } else if (*c == '*') { // can't parse this in another function, not portably, at least
         width = va_arg(ap, int);
         if (width < 0) {
            width = -1; // treat all negative numbers as unspecified
         }
         ++c;
      }
      
      if (*c == '\0') {
         result.append(Latin1String(escapeStart)); // incomplete escape, treat as non-escape text
         break;
      }
      
      // Parse precision
      int precision = -1; // -1 means unspecified
      if (*c == '.') {
         ++c;
         if (internal::is_digit(*c)) {
            precision = parse_field_width(c);
         } else if (*c == '*') { // can't parse this in another function, not portably, at least
            precision = va_arg(ap, int);
            if (precision < 0) {
               precision = -1; // treat all negative numbers as unspecified
            }
            ++c;
         }
      }
      if (*c == '\0') {
         result.append(Latin1String(escapeStart)); // incomplete escape, treat as non-escape text
         break;
      }
      const LengthMod length_mod = parse_length_modifier(c);
      if (*c == '\0') {
         result.append(Latin1String(escapeStart)); // incomplete escape, treat as non-escape text
         break;
      }
      
      // Parse the conversion specifier and do the conversion
      String subst;
      switch (*c) {
      case 'd':
      case 'i': {
         pdk::pint64 i;
         switch (length_mod) {
         case LengthMod::lm_none: i = va_arg(ap, int); break;
         case LengthMod::lm_hh: i = va_arg(ap, int); break;
         case LengthMod::lm_h: i = va_arg(ap, int); break;
         case LengthMod::lm_l: i = va_arg(ap, long int); break;
         case LengthMod::lm_ll: i = va_arg(ap, pdk::pint64); break;
         case LengthMod::lm_j: i = va_arg(ap, long int); break;
         case LengthMod::lm_z: i = va_arg(ap, size_t); break;
         case LengthMod::lm_t: i = va_arg(ap, int); break;
         default: i = 0; break;
         }
         subst = LocaleData::c()->longLongToString(i, precision, 10, width, flags);
         ++c;
         break;
      }
      case 'o':
      case 'u':
      case 'x':
      case 'X': {
         pdk::puint64 u;
         switch (length_mod) {
         case LengthMod::lm_none: u = va_arg(ap, uint); break;
         case LengthMod::lm_hh: u = va_arg(ap, uint); break;
         case LengthMod::lm_h: u = va_arg(ap, uint); break;
         case LengthMod::lm_l: u = va_arg(ap, ulong); break;
         case LengthMod::lm_ll: u = va_arg(ap, pdk::puint64); break;
         case LengthMod::lm_z: u = va_arg(ap, size_t); break;
         default: u = 0; break;
         }
         
         if (internal::is_upper(*c)) {
            flags |= LocaleData::Flag::CapitalEorX;
         } 
         
         int base = 10;
         switch (internal::to_lower(*c)) {
         case 'o':
            base = 8; break;
         case 'u':
            base = 10; break;
         case 'x':
            base = 16; break;
         default: break;
         }
         subst = LocaleData::c()->unsLongLongToString(u, precision, base, width, flags);
         ++c;
         break;
      }
      case 'E':
      case 'e':
      case 'F':
      case 'f':
      case 'G':
      case 'g':
      case 'A':
      case 'a': {
         double d;
         if (length_mod == LengthMod::lm_L) {
            d = va_arg(ap, long double); // not supported - converted to a double
         } else {
            d = va_arg(ap, double);
         }
         if (internal::is_upper(*c)) {
            flags |= LocaleData::Flag::CapitalEorX;
         }
         LocaleData::DoubleForm form = LocaleData::DoubleForm::DFDecimal;
         switch (internal::to_lower(*c)) {
         case 'e': form = LocaleData::DoubleForm::DFExponent; break;
         case 'a':                             // not supported - decimal form used instead
         case 'f': form = LocaleData::DoubleForm::DFDecimal; break;
         case 'g': form = LocaleData::DoubleForm::DFSignificantDigits; break;
         default: break;
         }
         subst = LocaleData::c()->doubleToString(d, precision, form, width, flags);
         ++c;
         break;
      }
      case 'c': {
         if (length_mod == LengthMod::lm_l)
            subst = Character((ushort) va_arg(ap, int));
         else
            subst = Latin1Character((uchar) va_arg(ap, int));
         ++c;
         break;
      }
      case 's': {
         if (length_mod == LengthMod::lm_l) {
            const char16_t *buff = va_arg(ap, const char16_t*);
            const char16_t *ch = buff;
            while (*ch != 0)
               ++ch;
            subst.setUtf16(buff, ch - buff);
         } else
            subst = String::fromUtf8(va_arg(ap, const char*));
         if (precision != -1)
            subst.truncate(precision);
         ++c;
         break;
      }
      case 'p': {
         void *arg = va_arg(ap, void*);
         const pdk::puint64 i = reinterpret_cast<pdk::uintptr>(arg);
         flags |= LocaleData::Flag::ShowBase;
         subst = LocaleData::c()->unsLongLongToString(i, precision, 16, width, flags);
         ++c;
         break;
      }
      case 'n':
         switch (length_mod) {
         case LengthMod::lm_hh: {
            signed char *n = va_arg(ap, signed char*);
            *n = result.length();
            break;
         }
         case LengthMod::lm_h: {
            short int *n = va_arg(ap, short int*);
            *n = result.length();
            break;
         }
         case LengthMod::lm_l: {
            long int *n = va_arg(ap, long int*);
            *n = result.length();
            break;
         }
         case LengthMod::lm_ll: {
            pdk::pint64 *n = va_arg(ap, pdk::pint64*);
            *n = result.length();
            break;
         }
         default: {
            int *n = va_arg(ap, int*);
            *n = result.length();
            break;
         }
         }
         ++c;
         break;
         
      default: // bad escape, treat as non-escape text
         for (const char *cc = escapeStart; cc != c; ++cc)
            result.append(Latin1Character(*cc));
         continue;
      }
      
      if (flags & LocaleData::Flag::LeftAdjusted)
         result.append(subst.leftJustified(width));
      else
         result.append(subst.rightJustified(width));
   }
   
   return result;
}

pdk::pint64 String::toLongLong(bool *ok, int base) const
{
   return toIntegralHelper<pdk::pint64>(getConstRawData(), size(), ok, base);
}

pdk::puint64 String::toULongLong(bool *ok, int base) const
{
   return toIntegralHelper<pdk::puint64>(getConstRawData(), size(), ok, base);
}

pdk::plonglong String::toIntegralHelper(const Character *data, int len, bool *ok, int base)
{
#if defined(PDK_CHECK_RANGE)
   if (base != 0 && (base < 2 || base > 36)) {
      warning_stream("String::toULongLong: Invalid base (%d)", base);
      base = 10;
   }
#endif
   return LocaleData::c()->stringToLongLong(StringView(data, len), base, ok, Locale::NumberOption::RejectGroupSeparator);
}

pdk::pulonglong String::toIntegralHelper(const Character *data, uint len, bool *ok, int base)
{
#if defined(PDK_CHECK_RANGE)
   if (base != 0 && (base < 2 || base > 36)) {
      warning_stream("String::toULongLong: Invalid base (%d)", base);
      base = 10;
   }
#endif
   return LocaleData::c()->stringToUnsLongLong(StringView(data, len), base, ok,
                                               Locale::NumberOption::RejectGroupSeparator);
}

long String::toLong(bool *ok, int base) const
{
   return toIntegralHelper<long>(getConstRawData(), size(), ok, base);
}

ulong String::toULong(bool *ok, int base) const
{
   return toIntegralHelper<ulong>(getConstRawData(), size(), ok, base);
}

int String::toInt(bool *ok, int base) const
{
   return toIntegralHelper<int>(getConstRawData(), size(), ok, base);
}

uint String::toUInt(bool *ok, int base) const
{
   return toIntegralHelper<uint>(getConstRawData(), size(), ok, base);
}

short String::toShort(bool *ok, int base) const
{
   return toIntegralHelper<short>(getConstRawData(), size(), ok, base);
}

ushort String::toUShort(bool *ok, int base) const
{
   return toIntegralHelper<ushort>(getConstRawData(), size(), ok, base);
}

double String::toDouble(bool *ok) const
{
   return LocaleData::c()->stringToDouble(*this, ok, Locale::NumberOption::RejectGroupSeparator);
}

float String::toFloat(bool *ok) const
{
   return LocaleData::convertDoubleToFloat(toDouble(ok), ok);
}

String &String::setNum(pdk::plonglong n, int base)
{
   return *this = number(n, base);
}

String &String::setNum(pdk::pulonglong n, int base)
{
   return *this = number(n, base);
}

String &String::setNum(double n, char f, int prec)
{
   return *this = number(n, f, prec);
}

String String::number(long n, int base)
{
   return number(pdk::plonglong(n), base);
}

String String::number(ulong n, int base)
{
   return number(pdk::pulonglong(n), base);
}

String String::number(int n, int base)
{
   return number(pdk::plonglong(n), base);
}

String String::number(uint n, int base)
{
   return number(pdk::pulonglong(n), base);
}

String String::number(pdk::plonglong n, int base)
{
#if defined(PDK_CHECK_RANGE)
   if (base < 2 || base > 36) {
      warning_stream("String::setNum: Invalid base (%d)", base);
      base = 10;
   }
#endif
   return LocaleData::c()->longLongToString(n, -1, base);
}

String String::number(pdk::pulonglong n, int base)
{
#if defined(PDK_CHECK_RANGE)
   if (base < 2 || base > 36) {
      warning_stream("String::setNum: Invalid base (%d)", base);
      base = 10;
   }
#endif
   return LocaleData::c()->unsLongLongToString(n, -1, base);
}

String String::number(double n, char f, int prec)
{
   LocaleData::DoubleForm form = LocaleData::DoubleForm::DFDecimal;
   LocaleData::Flags flags = 0;
   if (internal::is_upper(f)) {
      flags = LocaleData::Flag::CapitalEorX;
   }   
   f = internal::to_lower(f);
   switch (f) {
   case 'f':
      form = LocaleData::DoubleForm::DFDecimal;
      break;
   case 'e':
      form = LocaleData::DoubleForm::DFExponent;
      break;
   case 'g':
      form = LocaleData::DoubleForm::DFSignificantDigits;
      break;
   default:
#if defined(PDK_CHECK_RANGE)
      warning_stream("String::setNum: Invalid format char '%c'", f);
#endif
      break;
   }
   
   return LocaleData::c()->doubleToString(n, prec, form, -1, flags);
}

namespace {
template<class ResultList, class StringSource>
static ResultList split_string(const StringSource &source, const Character *sep,
                               String::SplitBehavior behavior, pdk::CaseSensitivity cs, const int separatorSize)
{
   ResultList list;
   int start = 0;
   int end;
   int extra = 0;
   while ((end = internal::find_string(source.getConstRawData(), source.size(), start + extra, sep, separatorSize, cs)) != -1) {
      if (start != end || behavior == String::SplitBehavior::KeepEmptyParts) {
         list.push_back(source.substring(start, end - start));
      }
      start = end + separatorSize;
      extra = (separatorSize == 0 ? 1 : 0);
   }
   if (start != source.size() || behavior == String::SplitBehavior::KeepEmptyParts) {
      list.push_back(source.substring(start, -1));
   }
   return list;
}

} // namespace

StringList String::split(const String &separator, SplitBehavior behavior, 
                         CaseSensitivity cs) const
{
   return split_string<StringList>(*this, separator.getConstRawData(), behavior, cs, separator.size());
}

std::vector<StringRef> String::splitRef(const String &separator, SplitBehavior behavior,
                                        CaseSensitivity cs) const
{
   return split_string<std::vector<StringRef> >(StringRef(this), separator.getConstRawData(), behavior, cs, separator.size());
}

StringList String::split(Character separator, SplitBehavior behavior, CaseSensitivity cs) const
{
   return split_string<StringList>(*this, &separator, behavior, cs, 1);
}

std::vector<StringRef> String::splitRef(Character separator, SplitBehavior behavior, CaseSensitivity cs) const
{
   return split_string<std::vector<StringRef>>(StringRef(this), &separator, behavior, cs, 1);
}

std::vector<StringRef> StringRef::split(const String &separator, String::SplitBehavior behavior, 
                                        CaseSensitivity cs) const
{
   return split_string<std::vector<StringRef>>(*this, separator.getConstRawData(), behavior, cs, separator.size());
}

std::vector<StringRef> StringRef::split(Character separator, String::SplitBehavior behavior, 
                                        CaseSensitivity cs) const
{
   return split_string<std::vector<StringRef>>(*this, &separator, behavior, cs, 1);
}

String String::repeated(int times) const
{
   if (m_data->m_size == 0) {
      return *this;
   }
   if (times <= 1) {
      if (times == 1) {
         return *this;
      }
      return String();
   }
   const int resultSize = times * m_data->m_size;
   String result;
   result.reserve(resultSize);
   if (result.m_data->m_alloc != uint(resultSize) + 1u) {
      return String(); // not enough memory
   }
   memcpy(result.m_data->getData(), m_data->getData(), m_data->m_size * sizeof(char16_t));
   int sizeSoFar = m_data->m_size;
   char16_t *end = result.m_data->getData() + sizeSoFar;
   const int halfResultSize = resultSize >> 1;
   while (sizeSoFar <= halfResultSize) {
      memcpy(end, result.m_data->getData(), sizeSoFar * sizeof(char16_t));
      end += sizeSoFar;
      sizeSoFar <<= 1;
   }
   memcpy(end, result.m_data->getData(), (resultSize - sizeSoFar) * sizeof(char16_t));
   result.m_data->getData()[resultSize] = '\0';
   result.m_data->m_size = resultSize;
   return result;
}

namespace internal {
bool normalization_quick_check_helper(String *str, String::NormalizationForm mode, int from, int *lastStable);
void compose_helper(String *str, Character::UnicodeVersion version, int from);
void canonical_order_helper(String *str, Character::UnicodeVersion version, int from);
void decompose_helper(String *str, bool canonical, Character::UnicodeVersion version, int from);

} // internal

namespace {

using internal::normalization_quick_check_helper;
using internal::compose_helper;
using internal::canonical_order_helper;
using internal::decompose_helper;

void string_normalize(String *data, String::NormalizationForm mode, Character::UnicodeVersion version, int from)
{
   using namespace pdk::lang::internal::unicodetables;
   bool simple = true;
   const Character *p = data->getConstRawData();
   int len = data->length();
   for (int i = from; i < len; ++i) {
      if (p[i].unicode() >= 0x80) {
         simple = false;
         if (i > from) {
            from = i - 1;
         }
         break;
      }
   }
   if (simple) {
      return;
   }
   if (version == Character::UnicodeVersion::Unicode_Unassigned) {
      version = Character::getCurrentUnicodeVersion();
   } else if (int(version) <= NormalizationCorrectionsVersionMax) {
      const String &s = *data;
      Character *d = 0;
      for (int i = 0; i < NumNormalizationCorrections; ++i) {
         const NormalizationCorrection &n = uc_normalization_corrections[i];
         if (n.m_version > pdk::as_integer<pdk::lang::Character::UnicodeVersion>(version)) {
            int pos = from;
            if (Character::requiresSurrogates(n.m_ucs4)) {
               ushort ucs4High = Character::getHighSurrogate(n.m_ucs4);
               ushort ucs4Low = Character::getLowSurrogate(n.m_ucs4);
               ushort oldHigh = Character::getHighSurrogate(n.m_oldMapping);
               ushort oldLow = Character::getLowSurrogate(n.m_oldMapping);
               while (pos < s.length() - 1) {
                  if (s.at(pos).unicode() == ucs4High && s.at(pos + 1).unicode() == ucs4Low) {
                     if (!d) {
                        d = data->getRawData();
                     }
                     d[pos] = Character(oldHigh);
                     d[++pos] = Character(oldLow);
                  }
                  ++pos;
               }
            } else {
               while (pos < s.length()) {
                  if (s.at(pos).unicode() == n.m_ucs4) {
                     if (!d) {
                        d = data->getRawData();
                     }
                     d[pos] = Character(n.m_oldMapping);
                  }
                  ++pos;
               }
            }
         }
      }
   }
   
   if (normalization_quick_check_helper(data, mode, from, &from)) {
      return;
   }
   decompose_helper(data, mode < String::NormalizationForm::Form_KD, version, from);
   canonical_order_helper(data, version, from);
   if (mode == String::NormalizationForm::Form_D || mode == String::NormalizationForm::Form_KD) {
      return;
   }
   compose_helper(data, version, from);
}

} // anonymous namespace

String String::normalized(NormalizationForm mode, Character::UnicodeVersion version) const
{
   String copy = *this;
   string_normalize(&copy, mode, version, 0);
   return copy;
}

namespace {

struct ArgEscapeData
{
   int m_minEscape;            // lowest escape sequence number
   int m_occurrences;           // number of m_occurrences of the lowest escape sequence number
   int m_localeOccurrences;    // number of m_occurrences of the lowest escape sequence number that
   // contain 'L'
   int m_escapeLen;            // total length of escape sequences which will be replaced
};

ArgEscapeData find_arg_escapes(StringView s)
{
   const Character *ucBegin = s.begin();
   const Character *ucEnd = s.end();
   
   ArgEscapeData d;
   
   d.m_minEscape = INT_MAX;
   d.m_occurrences = 0;
   d.m_escapeLen = 0;
   d.m_localeOccurrences = 0;
   
   const Character *c = ucBegin;
   while (c != ucEnd) {
      while (c != ucEnd && c->unicode() != '%') {
         ++c;
      }
      if (c == ucEnd) {
         break;
      }
      const Character *escapeStart = c;
      if (++c == ucEnd) {
         break;
      }
      bool localeArg = false;
      if (c->unicode() == 'L') {
         localeArg = true;
         if (++c == ucEnd) {
            break;
         }
      }
      
      int escape = c->getDigitValue();
      if (escape == -1) {
         continue;
      }
      ++c;
      
      if (c != ucEnd) {
         int nextEscape = c->getDigitValue();
         if (nextEscape != -1) {
            escape = (10 * escape) + nextEscape;
            ++c;
         }
      }
      if (escape > d.m_minEscape)
         continue;
      
      if (escape < d.m_minEscape) {
         d.m_minEscape = escape;
         d.m_occurrences = 0;
         d.m_escapeLen = 0;
         d.m_localeOccurrences = 0;
      }
      
      ++d.m_occurrences;
      if (localeArg) {
         ++d.m_localeOccurrences;
      } 
      d.m_escapeLen += c - escapeStart;
   }
   return d;
}

String replace_arg_escapes(StringView s, const ArgEscapeData &d, int field_width,
                           StringView arg, StringView larg, Character fillChar)
{
   const Character *ucBegin = s.begin();
   const Character *ucEnd = s.end();
   
   int absfieldWidth = std::abs(field_width);
   int resultLen = s.length()
         - d.m_escapeLen
         + (d.m_occurrences - d.m_localeOccurrences)
         *std::max(absfieldWidth, arg.length())
         + d.m_localeOccurrences
         *std::max(absfieldWidth, larg.length());
   
   String result(resultLen, pdk::Uninitialized);
   Character *resultBuff = const_cast<Character *>(result.unicode());
   
   Character *rc = resultBuff;
   const Character *c = ucBegin;
   int replCnt = 0;
   while (c != ucEnd) {
      /* We don't have to check if we run off the end of the string with c,
           because as long as d.m_occurrences > 0 we KNOW there are valid escape
           sequences. */
      
      const Character *textStart = c;
      while (c->unicode() != '%') {
         ++c;
      }
      const Character *escapeStart = c++;
      bool localeArg = false;
      if (c->unicode() == 'L') {
         localeArg = true;
         ++c;
      }
      
      int escape = c->getDigitValue();
      if (escape != -1) {
         if (c + 1 != ucEnd && (c + 1)->getDigitValue() != -1) {
            escape = (10 * escape) + (c + 1)->getDigitValue();
            ++c;
         }
      }
      
      if (escape != d.m_minEscape) {
         memcpy(rc, textStart, (c - textStart)*sizeof(Character));
         rc += c - textStart;
      }
      else {
         ++c;
         
         memcpy(rc, textStart, (escapeStart - textStart)*sizeof(Character));
         rc += escapeStart - textStart;
         
         uint pad_chars;
         if (localeArg)
            pad_chars = std::max(absfieldWidth, larg.length()) - larg.length();
         else
            pad_chars = std::max(absfieldWidth, arg.length()) - arg.length();
         
         if (field_width > 0) { // left padded
            for (uint i = 0; i < pad_chars; ++i) {
               (rc++)->unicode() = fillChar.unicode();
            }
         }
         
         if (localeArg) {
            memcpy(rc, larg.data(), larg.length()*sizeof(Character));
            rc += larg.length();
         }
         else {
            memcpy(rc, arg.data(), arg.length()*sizeof(Character));
            rc += arg.length();
         }
         
         if (field_width < 0) { // right padded
            for (uint i = 0; i < pad_chars; ++i) {
               (rc++)->unicode() = fillChar.unicode();
            }
         }
         
         if (++replCnt == d.m_occurrences) {
            memcpy(rc, c, (ucEnd - c)*sizeof(Character));
            rc += ucEnd - c;
            PDK_ASSERT(rc - resultBuff == resultLen);
            c = ucEnd;
         }
      }
   }
   PDK_ASSERT(rc == resultBuff + resultLen);
   return result;
}


} // anonymous namespace

#if PDK_STRINGVIEW_LEVEL < 2
String String::arg(const String &a, int fieldWidth, Character fillChar) const
{
   return arg(to_string_view_ignoring_null(a), fieldWidth, fillChar);
}
#endif

String String::arg(StringView a, int fieldWidth, Character fillChar) const
{
   ArgEscapeData d = find_arg_escapes(*this);
   if (PDK_UNLIKELY(d.m_occurrences == 0)) {
      warning_stream("String::arg: Argument missing: %ls, %ls", pdk_utf16_printable(*this),
                     pdk_utf16_printable(a.toString()));
      return *this;
   }
   return replace_arg_escapes(*this, d, fieldWidth, a, a, fillChar);
}

String String::arg(Latin1String a, int fieldWidth, Character fillChar) const
{
   VarLengthArray<char16_t> utf16(a.size());
   internal::utf16_from_latin1(utf16.getRawData(), a.getRawData(), a.size());
   return arg(StringView(utf16.getRawData(), utf16.size()), fieldWidth, fillChar);
}

String String::arg(pdk::plonglong a, int fieldWidth, int base, Character fillChar) const
{
   ArgEscapeData d = find_arg_escapes(*this);
   if (d.m_occurrences == 0) {
      warning_stream() << "String::arg: Argument missing:" << *this << ',' << a;
      return *this;
   }
   LocaleData::Flags flags = LocaleData::Flag::NoFlags;
   if (fillChar == Latin1Character('0')) {
      flags = LocaleData::Flag::ZeroPadded;
   }
   String arg;
   if (d.m_occurrences > d.m_localeOccurrences) {
      arg = LocaleData::c()->longLongToString(a, -1, base, fieldWidth, flags);
   }
   String localeArg;
   if (d.m_localeOccurrences > 0) {
      Locale locale;
      if (!(locale.numberOptions() & Locale::NumberOption::OmitGroupSeparator)) {
         flags |= LocaleData::Flag::ThousandsGroup;
      }
      
      localeArg = locale.m_implPtr->m_data->longLongToString(a, -1, base, fieldWidth, flags);
   }
   
   return replace_arg_escapes(*this, d, fieldWidth, arg, localeArg, fillChar);
}

String String::arg(pdk::pulonglong a, int fieldWidth, int base, Character fillChar) const
{
   ArgEscapeData d = find_arg_escapes(*this);
   if (d.m_occurrences == 0) {
      warning_stream() << "String::arg: Argument missing:" << *this << ',' << a;
      return *this;
   }
   LocaleData::Flags flags = LocaleData::Flag::NoFlags;
   if (fillChar == Latin1Character('0')) {
      flags = LocaleData::Flag::ZeroPadded;
   }
   
   String arg;
   if (d.m_occurrences > d.m_localeOccurrences) {
      arg = LocaleData::c()->unsLongLongToString(a, -1, base, fieldWidth, flags);
   }
   String localeArg;
   if (d.m_localeOccurrences > 0) {
      Locale locale;
      if (!(locale.numberOptions() & Locale::NumberOption::OmitGroupSeparator)) {
         flags |= LocaleData::Flag::ThousandsGroup;
      }
      localeArg = locale.m_implPtr->m_data->unsLongLongToString(a, -1, base, fieldWidth, flags);
   }
   
   return replace_arg_escapes(*this, d, fieldWidth, arg, localeArg, fillChar);
}

String String::arg(Character a, int fieldWidth, Character fillChar) const
{
   String c;
   c += a;
   return arg(c, fieldWidth, fillChar);
}

String String::arg(char a, int fieldWidth, Character fillChar) const
{
   String c;
   c += Latin1Character(a);
   return arg(c, fieldWidth, fillChar);
}

String String::arg(double a, int fieldWidth, char fmt, int prec, Character fillChar) const
{
   ArgEscapeData d = find_arg_escapes(*this);
   
   if (d.m_occurrences == 0) {
      warning_stream("String::arg: Argument missing: %s, %g", toLocal8Bit().getRawData(), a);
      return *this;
   }
   
   LocaleData::Flags flags = LocaleData::Flag::NoFlags;
   if (fillChar == Latin1Character('0')) {
      flags = LocaleData::Flag::ZeroPadded;
   }
   if (internal::is_upper(fmt)) {
      flags |= LocaleData::Flag::CapitalEorX;
   }
   fmt = internal::to_lower(fmt);   
   LocaleData::DoubleForm form = LocaleData::DoubleForm::DFDecimal;
   switch (fmt) {
   case 'f':
      form = LocaleData::DoubleForm::DFDecimal;
      break;
   case 'e':
      form = LocaleData::DoubleForm::DFExponent;
      break;
   case 'g':
      form = LocaleData::DoubleForm::DFSignificantDigits;
      break;
   default:
#if defined(PDK_CHECK_RANGE)
      warning_stream("String::arg: Invalid format char '%c'", fmt);
#endif
      break;
   }
   String arg;
   if (d.m_occurrences > d.m_localeOccurrences) {
      arg = LocaleData::c()->doubleToString(a, prec, form, fieldWidth, flags);
   }
   String localeArg;
   if (d.m_localeOccurrences > 0) {
      Locale locale;
      const Locale::NumberOptions numberOptions = locale.numberOptions();
      if (!(numberOptions & Locale::NumberOption::OmitGroupSeparator)) {
         flags |= LocaleData::Flag::ThousandsGroup;
      }
      if (!(numberOptions & Locale::NumberOption::OmitLeadingZeroInExponent)) {
         flags |= LocaleData::Flag::ZeroPadExponent;
      }
      if (numberOptions & Locale::NumberOption::IncludeTrailingZeroesAfterDot)
         flags |= LocaleData::Flag::AddTrailingZeroes;
      localeArg = locale.m_implPtr->m_data->doubleToString(a, prec, form, fieldWidth, flags);
   }
   
   return replace_arg_escapes(*this, d, fieldWidth, arg, localeArg, fillChar);
}

namespace {

int get_escape(const Character *uc, int *pos, int len, int maxNumber = 999)
{
   int i = *pos;
   ++i;
   if (i < len && uc[i] == Latin1Character('L')) {
      ++i;
   }
   if (i < len) {
      int escape = uc[i].unicode() - '0';
      if (uint(escape) >= 10U) {
         return -1;
      }
      ++i;
      while (i < len) {
         int digit = uc[i].unicode() - '0';
         if (uint(digit) >= 10U) {
            break;
         }
         escape = (escape * 10) + digit;
         ++i;
      }
      if (escape <= maxNumber) {
         *pos = i;
         return escape;
      }
   }
   return -1;
}

/*
    Algorithm for multiArg:
    
    1. Parse the string as a sequence of verbatim text and placeholders (%L?\d{,3}).
       The L is parsed and accepted for compatibility with non-multi-arg, but since
       multiArg only accepts strings as replacements, the localization request can
       be safely ignored.
    2. The result of step (1) is a list of (string-ref,int)-tuples. The string-ref
       either points at text to be copied verbatim (in which case the int is -1),
       or, initially, at the textual representation of the placeholder. In that case,
       the int contains the numerical number as parsed from the placeholder.
    3. Next, collect all the non-negative ints found, sort them in ascending order and
       remove duplicates.
       3a. If the result has more entires than multiArg() was given replacement strings,
           we have found placeholders we can't satisfy with replacement strings. That is
           fine (there could be another .arg() call coming after this one), so just
           truncate the result to the number of actual multiArg() replacement strings.
       3b. If the result has less entries than multiArg() was given replacement strings,
           the string is missing placeholders. This is an error that the user should be
           warned about.
    4. The result of step (3) is a mapping from the index of any replacement string to
       placeholder number. This is the wrong way around, but since placeholder
       numbers could get as large as 999, while we typically don't have more than 9
       replacement strings, we trade 4K of sparsely-used memory for doing a reverse lookup
       each time we need to map a placeholder number to a replacement string index
       (that's a linear search; but still *much* faster than using an associative container).
    5. Next, for each of the tuples found in step (1), do the following:
       5a. If the int is negative, do nothing.
       5b. Otherwise, if the int is found in the result of step (3) at index I, replace
           the string-ref with a string-ref for the (complete) I'th replacement string.
       5c. Otherwise, do nothing.
    6. Concatenate all string refs into a single result string.
*/

struct Part
{
   Part() 
      : m_stringRef(),
        m_number(0)
   {}
   
   Part(const String &s, int pos, int len, int num = -1) noexcept
      : m_stringRef(&s, pos, len),
        m_number(num)
   {}
   
   StringRef m_stringRef;
   int m_number;
};

} // anonymous namespace

} // lang

template <>
class TypeInfo<lang::Part> : public TypeInfoMerger<lang::Part, lang::StringRef, int>
{};

namespace lang {

namespace {

enum { ExpectedParts = 32 };

typedef VarLengthArray<Part, ExpectedParts> ParseResult;
typedef VarLengthArray<int, ExpectedParts/2> ArgIndexToPlaceholderMap;

static ParseResult parse_multi_arg_format_string(const String &s)
{
   ParseResult result;
   
   const Character *uc = s.getConstRawData();
   const int len = s.size();
   const int end = len - 1;
   int i = 0;
   int last = 0;
   
   while (i < end) {
      if (uc[i] == Latin1Character('%')) {
         int percent = i;
         int number = get_escape(uc, &i, len);
         if (number != -1) {
            if (last != percent) {
               result.push_back(Part(s, last, percent - last)); // literal text (incl. failed placeholders)
            }
            result.push_back(Part(s, percent, i - percent, number));  // parsed placeholder
            last = i;
            continue;
         }
      }
      ++i;
   }
   
   if (last < len)
      result.push_back(Part(s, last, len - last)); // trailing literal text
   
   return result;
}

ArgIndexToPlaceholderMap make_arg_index_to_placeholder_map(const ParseResult &parts)
{
   ArgIndexToPlaceholderMap result;
   for (ParseResult::const_iterator iter = parts.begin(), end = parts.end(); iter != end; ++iter) {
      if (iter->m_number >= 0) {
         result.push_back(iter->m_number);
      }
   }
   std::sort(result.begin(), result.end());
   result.erase(std::unique(result.begin(), result.end()),
                result.end());
   return result;
}

int resolveStringRefsAndReturnTotalSize(ParseResult &parts, const ArgIndexToPlaceholderMap &argIndexToPlaceholderMap, const String *args[])
{
   int totalSize = 0;
   for (ParseResult::iterator piter = parts.begin(), end = parts.end(); piter != end; ++piter) {
      if (piter->m_number != -1) {
         const ArgIndexToPlaceholderMap::const_iterator aiter
               = std::find(argIndexToPlaceholderMap.begin(), argIndexToPlaceholderMap.end(), piter->m_number);
         if (aiter != argIndexToPlaceholderMap.end()) {
            piter->m_stringRef = StringRef(args[aiter - argIndexToPlaceholderMap.begin()]);
         }
      }
      totalSize += piter->m_stringRef.size();
   }
   return totalSize;
}

} // anonymous namespace

String String::multiArg(int numArgs, const String **args) const
{
   // Step 1-2 above
   ParseResult parts = parse_multi_arg_format_string(*this);
   
   // 3-4
   ArgIndexToPlaceholderMap argIndexToPlaceholderMap = make_arg_index_to_placeholder_map(parts);
   if (argIndexToPlaceholderMap.size() > numArgs) {// 3a
      argIndexToPlaceholderMap.resize(numArgs);
   } else if (argIndexToPlaceholderMap.size() < numArgs) {// 3b
      warning_stream("String::arg: %d argument(s) missing in %s",
                     numArgs - argIndexToPlaceholderMap.size(), toLocal8Bit().getRawData());  
   }
   // 5
   const int totalSize = resolveStringRefsAndReturnTotalSize(parts, argIndexToPlaceholderMap, args);
   
   // 6:
   String result(totalSize, pdk::Uninitialized);
   Character *out = result.getRawData();
   
   for (ParseResult::const_iterator iter = parts.begin(), end = parts.end(); iter != end; ++iter) {
      if (const int sz = iter->m_stringRef.size()) {
         memcpy(out, iter->m_stringRef.getConstRawData(), sz * sizeof(Character));
         out += sz;
      }
   }
   
   return result;
}

bool String::isSimpleText() const
{
   const char16_t *p = m_data->getData();
   const char16_t * const end = p + m_data->m_size;
   while (p < end) {
      char16_t uc = *p;
      // sort out regions of complex text formatting
      if (uc > 0x058f && (uc < 0x1100 || uc > 0xfb0f)) {
         return false;
      }
      p++;
   }
   return true;
}

bool String::isRightToLeft() const
{
   return StringRef(this).isRightToLeft();
}

String String::fromRawData(const Character *str, int size)
{
   Data *x;
   if (!str) {
      x = Data::getSharedNull();
   } else if (!size) {
      x = Data::allocate(0);
   } else {
      x = Data::fromRawData(reinterpret_cast<const char16_t *>(str), size);
      PDK_CHECK_ALLOC_PTR(x);
   }
   StringDataPtr dataPtr = { x };
   return String(dataPtr);
}

String &String::setRawData(const Character *str, int size)
{
   if (m_data->m_ref.isShared() || m_data->m_alloc) {
      *this = fromRawData(str, size);
   } else {
      if (str) {
         m_data->m_size = size;
         m_data->m_offset = reinterpret_cast<const char *>(str) - reinterpret_cast<char *>(m_data);
      } else {
         m_data->m_offset = sizeof(StringData);
         m_data->m_size = 0;
      }
   }
   return *this;
}

String StringRef::toString() const
{
   if (!m_str) {
      return String();
   }
   if (m_size && m_position == 0 && m_size == m_str->size()) {
      return *m_str;
   }
   return String(m_str->unicode() + m_position, m_size);
}

#if !defined(PDK_NO_DATASTREAM)
DataStream &operator<<(DataStream &out, const String &str)
{
   if (!str.isNull()) {
      if ((out.getByteOrder() == DataStream::ByteOrder::BigEndian) == (SysInfo::ByteOrder == SysInfo::BigEndian)) {
         out.writeBytes(reinterpret_cast<const char *>(str.unicode()), sizeof(Character) * str.length());
      } else {
         VarLengthArray<ushort> buffer(str.length());
         const ushort *data = reinterpret_cast<const ushort *>(str.getConstRawData());
         for (int i = 0; i < str.length(); i++) {
            buffer[i] = pdk::bswap(*data);
            ++data;
         }
         out.writeBytes(reinterpret_cast<const char *>(buffer.getRawData()), sizeof(ushort) * buffer.size());
      }
   } else {
      // write null marker
      out << (pdk::puint32)0xffffffff;
   }
   return out;
}

DataStream &operator>>(DataStream &in, String &str)
{
   pdk::puint32 bytes = 0;
   in >> bytes;                                  // read size of string
   if (bytes == 0xffffffff) {                    // null string
      str.clear();
   } else if (bytes > 0) {                       // not empty
      if (bytes & 0x1) {
         str.clear();
         in.setStatus(DataStream::Status::ReadCorruptData);
         return in;
      }
      const pdk::puint32 Step = 1024 * 1024;
      pdk::puint32 len = bytes / 2;
      pdk::puint32 allocated = 0;
      while (allocated < len) {
         int blockSize = std::min(Step, len - allocated);
         str.resize(allocated + blockSize);
         if (in.readRawData(reinterpret_cast<char *>(str.getRawData()) + allocated * 2,
                            blockSize * 2) != blockSize * 2) {
            str.clear();
            in.setStatus(DataStream::Status::ReadPastEnd);
            return in;
         }
         allocated += blockSize;
      }
      
      if ((in.getByteOrder() == DataStream::ByteOrder::BigEndian)
          != (SysInfo::ByteOrder == SysInfo::BigEndian)) {
         ushort *data = reinterpret_cast<ushort *>(str.getRawData());
         while (len--) {
            *data = pdk::bswap(*data);
            ++data;
         }
      }
   } else {
      str = String(Latin1String(""));
   }
   return in;
}
#endif // PDK_NO_DATASTREAM

bool operator==(const StringRef &lhs,const StringRef &rhs) noexcept
{
   return lhs.size() == rhs.size() && pdk_compare_strings(lhs, rhs, pdk::CaseSensitivity::Sensitive) == 0;
}

bool operator==(const String &lhs,const StringRef &rhs) noexcept
{
   return lhs.size() == rhs.size() && pdk_compare_strings(lhs, rhs, pdk::CaseSensitivity::Sensitive) == 0;
}

bool operator==(Latin1String lhs, const StringRef &rhs) noexcept
{
   if (lhs.size() != rhs.size()) {
      return false;
   }
   return pdk_compare_strings(rhs, lhs, pdk::CaseSensitivity::Sensitive) == 0;
}

bool operator<(const StringRef &lhs,const StringRef &rhs) noexcept
{
   return pdk_compare_strings(lhs, rhs, pdk::CaseSensitivity::Sensitive) < 0;
}

StringRef StringRef::appendTo(String *string) const
{
   if (!string) {
      return StringRef();
   }
   int pos = string->size();
   string->insert(pos, unicode(), size());
   return StringRef(string, pos, size());
}

String &String::append(const StringRef &str)
{
   if (str.getStr() == this) {
      str.appendTo(this);
   } else if (!str.isNull()) {
      int oldSize = size();
      resize(oldSize + str.size());
      memcpy(getRawData() + oldSize, str.unicode(), str.size() * sizeof(Character));
   }
   return *this;
}

StringRef StringRef::left(int n) const
{
   if (uint(n) >= uint(m_size)) {
      return *this;
   }
   return StringRef(m_str, m_position, n);
}

StringRef String::leftRef(int n) const
{
   return StringRef(this).left(n);
}

StringRef StringRef::right(int n) const
{
   if (uint(n) >= uint(m_size)) {
      return *this;
   }
   return StringRef(m_str, m_size - n + m_position, n);
}

StringRef String::rightRef(int n) const
{
   return StringRef(this).right(n);
}

StringRef StringRef::substring(int pos, int n) const
{
   using pdk::ds::internal::ContainerImplHelper;
   switch (ContainerImplHelper::mid(m_size, &pos, &n)) {
   case ContainerImplHelper::CutResult::Null:
      return StringRef();
   case ContainerImplHelper::CutResult::Empty:
      return StringRef(m_str, 0, 0);
   case ContainerImplHelper::CutResult::Full:
      return *this;
   case ContainerImplHelper::CutResult::Subset:
      return StringRef(m_str, pos + m_position, n);
   }
   PDK_UNREACHABLE();
   return StringRef();
}

StringRef String::substringRef(int pos, int n) const
{
   return StringRef(this).substring(pos, n);
}

int StringRef::indexOf(const String &str, int from, CaseSensitivity cs) const
{
   return internal::find_string(unicode(), length(), from, str.unicode(), str.length(), cs);
}

int StringRef::indexOf(Character c, int from, CaseSensitivity cs) const
{
   return find_char(unicode(), length(), c, from, cs);
}

int StringRef::indexOf(Latin1String str, int from, CaseSensitivity cs) const
{
   return find_latin1_string(unicode(), size(), str, from, cs);
}

int StringRef::indexOf(const StringRef &str, int from, CaseSensitivity cs) const
{
   return internal::find_string(unicode(), size(), from, str.unicode(), str.size(), cs);
}

int StringRef::lastIndexOf(const String &str, int from, CaseSensitivity cs) const
{
   return lastIndexOf(StringRef(&str), from, cs);
}

int StringRef::lastIndexOf(Character c, int from, CaseSensitivity cs) const
{
   return last_index_of(unicode(), size(), c, from, cs);
}

namespace {

template<typename T>
int last_index_of_impl(const StringRef &haystack, int from, const T &needle, pdk::CaseSensitivity cs)
{
   const int sl = needle.size();
   if (sl == 1) {
      return haystack.lastIndexOf(needle.at(0), from, cs);
   }
   const int l = haystack.size();
   if (from < 0) {
      from += l;
   }
   
   int delta = l - sl;
   if (from == l && sl == 0) {
      return from;
   }
   
   if (uint(from) >= uint(l) || delta < 0) {
      return -1;
   }
   
   if (from > delta) {
      from = delta;
   }
   return last_index_of_helper(haystack, from, needle, cs);
}

} // anonymous namespace

int StringRef::lastIndexOf(Latin1String str, int from, CaseSensitivity cs) const
{
   return last_index_of_impl(*this, from, str, cs);
}

int StringRef::lastIndexOf(const StringRef &str, int from, CaseSensitivity cs) const
{
   return last_index_of_impl(*this, from, str, cs);
}

int StringRef::count(const String &needle, CaseSensitivity cs) const
{
   return string_count(unicode(), size(), needle.unicode(), needle.size(), cs);
}

int StringRef::count(Character needle, CaseSensitivity cs) const
{
   return string_count(unicode(), size(), needle, cs);
}

int StringRef::count(const StringRef &needle, CaseSensitivity cs) const
{
   return string_count(unicode(), size(), needle.unicode(), needle.size(), cs);
}

bool StringRef::isRightToLeft() const
{
   const ushort *p = reinterpret_cast<const ushort*>(unicode());
   const ushort * const end = p + size();
   while (p < end) {
      uint ucs4 = *p;
      if (Character::isHighSurrogate(ucs4) && p < end - 1) {
         ushort low = p[1];
         if (Character::isLowSurrogate(low)) {
            ucs4 = Character::surrogateToUcs4(ucs4, low);
            ++p;
         }
      }
      switch (Character::getDirection(ucs4))
      {
      case Character::Direction::DirL:
         return false;
      case Character::Direction::DirR:
      case Character::Direction::DirAL:
         return true;
      default:
         break;
      }
      ++p;
   }
   return false;  
}

bool StringRef::startsWith(const String &needle, CaseSensitivity cs) const
{
   return starts_with(*this, needle, cs);
}

bool StringRef::startsWith(Latin1String needle, CaseSensitivity cs) const
{
   return starts_with(*this, needle, cs);
}

bool StringRef::startsWith(const StringRef &needle, CaseSensitivity cs) const
{
   return starts_with(*this, needle, cs);
}

bool StringRef::startsWith(Character needle, CaseSensitivity cs) const
{
   return starts_with(*this, needle, cs);
}

bool StringRef::endsWith(const String &needle, CaseSensitivity cs) const
{
   return ends_with(*this, needle, cs);
}

bool StringRef::endsWith(Character needle, CaseSensitivity cs) const
{
   return ends_with(*this, needle, cs);
}

bool StringRef::endsWith(Latin1String needle, CaseSensitivity cs) const
{
   return ends_with(*this, needle, cs);
}

bool StringRef::endsWith(const StringRef &needle, CaseSensitivity cs) const
{
   return ends_with(*this, needle, cs);
}

ByteArray StringRef::toLatin1() const
{
   return pdk_convert_to_latin1(*this);
}

ByteArray StringRef::toLocal8Bit() const
{
   return pdk_convert_to_local_8bit(*this);
}

ByteArray StringRef::toUtf8() const
{
   return pdk_convert_to_utf8(*this);
}

std::vector<char32_t> StringRef::toUcs4() const
{
   return pdk_convert_to_ucs4(*this);
}

StringRef StringRef::trimmed() const
{
   const Character *begin = cbegin();
   const Character *end = cend();
   internal::StringAlgorithms<const StringRef>::trimmedHelperPositions(begin, end);
   if (begin == cbegin() && end == cend()) {
      return *this;
   }
   int position = m_position + (begin - cbegin());
   return StringRef(m_str, position, end - begin);
}

pdk::plonglong StringRef::toLongLong(bool *ok, int base) const
{
   return String::toIntegralHelper<pdk::pint64>(getConstRawData(), size(), ok, base);
}

pdk::pulonglong StringRef::toULongLong(bool *ok, int base) const
{
   return String::toIntegralHelper<pdk::puint64>(getConstRawData(), size(), ok, base);
}

long StringRef::toLong(bool *ok, int base) const
{
   return String::toIntegralHelper<long>(getConstRawData(), size(), ok, base);
}

ulong StringRef::toULong(bool *ok, int base) const
{
   return String::toIntegralHelper<ulong>(getConstRawData(), size(), ok, base);
}

int StringRef::toInt(bool *ok, int base) const
{
   return String::toIntegralHelper<int>(getConstRawData(), size(), ok, base);
}

uint StringRef::toUInt(bool *ok, int base) const
{
   return String::toIntegralHelper<uint>(getConstRawData(), size(), ok, base);
}

short StringRef::toShort(bool *ok, int base) const
{
   return String::toIntegralHelper<short>(getConstRawData(), size(), ok, base);
}

ushort StringRef::toUShort(bool *ok, int base) const
{
   return String::toIntegralHelper<ushort>(getConstRawData(), size(), ok, base);
}

double StringRef::toDouble(bool *ok) const
{
   return LocaleData::c()->stringToDouble(*this, ok, Locale::NumberOption::RejectGroupSeparator);
}

float StringRef::toFloat(bool *ok) const
{
   return LocaleData::convertDoubleToFloat(toDouble(ok), ok);
}

String String::toHtmlEscaped() const
{
   String rich;
   const int len = length();
   rich.reserve(int(len * 1.1));
   for (int i = 0; i < len; ++i) {
      if (at(i) == Latin1Character('<')) {
         rich += Latin1String("&lt;");
      } else if (at(i) == Latin1Character('>')) {
         rich += Latin1String("&gt;");
      } else if (at(i) == Latin1Character('&')) {
         rich += Latin1String("&amp;");
      } else if (at(i) == Latin1Character('"')) {
         rich += Latin1String("&quot;");
      } else {
         rich += at(i);
      }   
   }
   rich.squeeze();
   return rich;
}

void AbstractConcatenable::appendLatin1To(const char *a, int len, Character *out) noexcept
{
   internal::utf16_from_latin1(reinterpret_cast<char16_t *>(out), a, uint(len));
}

} // lang
} // pdk
