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
   
}

int stringprivate::compare_strings(StringView lhs, Latin1String rhs, CaseSensitivity cs) noexcept
{
   
}

int stringprivate::compare_strings(Latin1String lhs, StringView rhs, CaseSensitivity cs) noexcept
{
   
}

int stringprivate::compare_strings(Latin1String lhs, Latin1String rhs, CaseSensitivity cs) noexcept
{
   
}

ByteArray stringprivate::convert_to_latin1(StringView str)
{
   
}

ByteArray stringprivate::convert_to_local_8bit(StringView str)
{
   
}

ByteArray stringprivate::convert_to_utf8(StringView str)
{
   
}

std::vector<uint> stringprivate::convert_to_ucs4(StringView str)
{
   
}

Latin1String stringprivate::trimmed(Latin1String str) noexcept
{
   
}

StringView stringprivate::trimmed(StringView str) noexcept
{
   
}

bool stringprivate::starts_with(Latin1String haystack, Latin1String needle, CaseSensitivity cs) noexcept
{}

bool stringprivate::starts_with(Latin1String haystack, StringView needle, CaseSensitivity cs) noexcept
{}

bool stringprivate::starts_with(StringView haystack, Latin1String needle, CaseSensitivity cs) noexcept
{}

bool stringprivate::starts_with(StringView haystack, StringView needle, CaseSensitivity cs) noexcept
{}

bool stringprivate::ends_with(Latin1String haystack, Latin1String needle, CaseSensitivity cs) noexcept
{}

bool stringprivate::ends_with(Latin1String haystack, StringView needle, CaseSensitivity cs) noexcept
{}

bool stringprivate::ends_with(StringView haystack, Latin1String needle, CaseSensitivity cs) noexcept
{}

bool stringprivate::ends_with(StringView haystack, StringView needle, CaseSensitivity cs) noexcept
{}

int String::toUcs4Helper(const char16_t *str, int length, char32_t *out)
{
   
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
   
}

String::String(int size, Initialization)
{}

String::String(Character c)
{}

void String::resize(int size)
{}

void String::resize(int size, Character fillChar)
{}

void String::reallocData(uint alloc, bool grow)
{}

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
{}

String &String::operator =(Character ch)
{}

String &String::insert(int i, Latin1String str)
{}

String &String::insert(int i, const Character *str, int length)
{}

String &String::insert(int i, Character c)
{}

String &String::append(const String &str)
{}

String &String::append(const Character *str, int length)
{}

String &String::append(Latin1String str)
{}

String &String::append(Character ch)
{}

String &String::remove(int pos, int length)
{}

String &String::remove(const String &str, CaseSensitivity cs)
{}

String &String::remove(Character c, CaseSensitivity cs)
{}

String &String::replace(int pos, int length, const String &after)
{}

String &String::replace(int pos, int length, const Character *after, int alength)
{}

String &String::replace(int pos, int length, Character after)
{}

String &String::replace(const String &before, const String &after, CaseSensitivity cs)
{}

void String::replaceHelper(uint *indices, int nIndices, int blength, 
                           const Character *after, int alength)
{
   
}

String &String::replace(const Character *before, int blength, 
                        const Character *after, int alength, CaseSensitivity cs)
{}

String &String::replace(Character c, const String &after, CaseSensitivity cs)
{}

String &String::replace(Character before, Character after, CaseSensitivity cs)
{}

String &String::replace(Latin1String before, Latin1String after, CaseSensitivity cs)
{}

String &String::replace(Latin1String before, const String &after, CaseSensitivity cs)
{}

String &String::replace(const String &before, Latin1String after, CaseSensitivity cs)
{}

String &String::replace(Character c, Latin1String after, CaseSensitivity cs)
{}

bool operator ==(const String &lhs, const String &rhs) noexcept
{
   if (lhs.m_data->m_size != rhs.m_data->m_size) {
      return false;
   }
   return pdk_compare_strings(lhs, rhs, pdk::CaseSensitivity::Sensitive) == 0;
}

bool String::operator ==(Latin1String other) const noexcept
{}

bool operator<(const String &lhs, const String &rhs) noexcept
{
}

bool String::operator <(Latin1String other) const noexcept
{}

bool String::operator >(Latin1String other) const noexcept
{}

int String::indexOf(const String &needle, int from, CaseSensitivity cs) const
{}

int String::indexOf(Latin1String needle, int from, CaseSensitivity cs) const
{}

int String::indexOf(Character needle, int from, CaseSensitivity cs) const
{}

int String::indexOf(const StringRef &needle, int from, CaseSensitivity cs) const
{}

int String::lastIndexOf(const String &needle, int from, CaseSensitivity cs) const
{}

int String::lastIndexOf(Latin1String needle, int from, CaseSensitivity cs) const
{}

int String::lastIndexOf(Character needle, int from, CaseSensitivity cs) const
{}

int String::lastIndexOf(const StringRef &needle, int from, CaseSensitivity cs) const
{}

int String::count(const String &needle, CaseSensitivity cs) const
{}

int String::count(Character needle, CaseSensitivity cs) const
{}

int String::count(const StringRef &needle, CaseSensitivity cs) const
{}

String String::section(const String &separator, int start, int end, SectionFlags flags) const
{}

String String::left(int n) const
{
   
}

String String::right(int n) const
{}

String String::substring(int pos, int n) const
{
   
}

#if PDK_STRINGVIEW_LEVEL < 2

bool String::startsWith(const String &needle, CaseSensitivity cs) const
{}

#endif

bool String::startsWith(Latin1String needle, CaseSensitivity cs) const
{}

bool String::startsWith(Character needle, CaseSensitivity cs) const
{}

#if PDK_STRINGVIEW_LEVEL < 2

bool String::startsWith(const StringRef &needle, CaseSensitivity cs) const
{}

#endif

#if PDK_STRINGVIEW_LEVEL < 2

bool String::endsWith(const String &needle, CaseSensitivity cs) const
{}

bool String::endsWith(const StringRef &needle, CaseSensitivity cs) const
{}

#endif

bool String::endsWith(Latin1String needle, CaseSensitivity cs) const
{}

bool String::endsWith(Character needle, CaseSensitivity cs) const
{}

ByteArray String::toLatin1Helper(const String &str)
{}

ByteArray String::toLatin1Helper(const Character *str, int size)
{}

ByteArray String::toLatin1HelperInplace(String &str)
{}

ByteArray String::toLocal8BitHelper(const Character *str, int size)
{}

ByteArray String::toUtf8Helper(const String &str)
{}

std::vector<char32_t> String::toUcs4() const
{}

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
{}

String String::fromLocal8BitHelper(const char *str, int size)
{}

String String::fromUtf8Helper(const char *str, int size)
{}

String String::fromUtf16(const char16_t *str, int size)
{}

String String::fromUcs4(const char32_t *str, int size)
{}

String &String::setUnicode(const Character *unicode, int size)
{}

String String::simplifiedHelper(const String &str)
{}

String String::simplifiedHelper(String &str)
{}

String String::trimmedHelper(const String &str)
{}

String String::trimmedHelper(String &str)
{}

void String::truncate(int pos)
{}

void String::chop(int n)
{}

String &String::fill(Character c, int size)
{}

int String::compare(const String &str, CaseSensitivity cs) const noexcept
{}

int String::compareHelper(const Character *lhs, int lhsLength, 
                          const Character *rhs, int rhsLength, CaseSensitivity cs) noexcept
{
   
}

int String::compare(Latin1String str, CaseSensitivity cs) const noexcept
{}

int String::compareHelper(const Character *lhs, int lhsLength, 
                          const char *rhs, int rhsLength, CaseSensitivity cs) noexcept
{
   
}

int String::compareHelper(const Character *lhs, int lhsLength, Latin1String rhs, CaseSensitivity cs) noexcept
{
   
}

#if !defined(CSTR_LESS_THAN)
#define CSTR_LESS_THAN    1
#define CSTR_EQUAL        2
#define CSTR_GREATER_THAN 3
#endif

int String::localeAwareCompare(const String &str) const
{}

int String::localeAwareCompareHelper(const Character *lhs, int lhsLength, const Character *rhs, int rhsLength)
{}

const char16_t *String::utf16() const
{}

String String::leftJustified(int width, Character fill, bool truncate) const
{}

String String::rightJustified(int width, Character fill, bool truncate) const
{}

String String::toLowerHelper(const String &str)
{}

String String::toLowerHelper(String &str)
{
   
}

String String::toCaseFoldedHelper(const String &str)
{}

String String::toCaseFoldedHelper(String &str)
{}

String String::toUpperHelper(const String &str)
{}

String String::toUpperHelper(String &str)
{}

String &String::sprintf(const char *format, ...)
{
   
}

String String::asprintf(const char *format, ...)
{
   
}

String &String::vsprintf(const char *format, va_list ap)
{}

String String::vasprintf(const char *format, va_list ap)
{}

pdk::pint64 String::toLongLong(bool *ok, int base) const
{}

pdk::plonglong String::toIntegralHelper(const Character *data, int len, bool *ok, int base)
{}

pdk::puint64 String::toULongLong(bool *ok, int base) const
{}

pdk::pulonglong String::toIntegralHelper(const Character *data, uint len, bool *ok, int base)
{}

long String::toLong(bool *ok, int base) const
{}

ulong String::toULong(bool *ok, int base) const
{}

int String::toInt(bool *ok, int base) const
{}

uint String::toUInt(bool *ok, int base) const
{
   
}

short String::toShort(bool *ok, int base) const
{}

ushort String::toUShort(bool *ok, int base) const
{}

double String::toDouble(bool *ok) const
{}

float String::toFloat(bool *ok) const
{}

String &String::setNum(pdk::plonglong, int base)
{}

String &String::setNum(pdk::pulonglong, int base)
{}

String &String::setNum(double, char f, int prec)
{}

String String::number(long, int base)
{}

String String::number(ulong, int base)
{}

String String::number(int, int base)
{}

String String::number(uint, int base)
{}

String String::number(pdk::plonglong, int base)
{
   
}

String String::number(pdk::pulonglong, int base)
{
   
}

String String::number(double, char f, int prec)
{}

StringList String::split(const String &separator, SplitBehavior behavior, 
                         CaseSensitivity cs) const
{
   
}

std::vector<StringRef> String::splitRef(const String &separator, SplitBehavior behavior,
                                        CaseSensitivity cs) const
{
   
}

StringList String::split(Character separator, SplitBehavior behavior, CaseSensitivity cs) const
{}

std::vector<StringRef> String::splitRef(Character separator, SplitBehavior behavior, CaseSensitivity cs) const
{
   
}

std::vector<StringRef> StringRef::split(const String &separator, String::SplitBehavior behavior, 
                                        CaseSensitivity cs) const
{
   
}

std::vector<StringRef> StringRef::split(Character separator, String::SplitBehavior behavior, 
                                        CaseSensitivity cs) const
{
   
}

String String::repeated(int times) const
{}

String String::normalized(NormalizationForm mode, Character::UnicodeVersion version) const
{}

#if PDK_STRINGVIEW_LEVEL < 2
String String::arg(const String &a, int fieldWidth, Character fillChar) const
{}
#endif

String String::arg(StringView a, int fieldWidth, Character fillChar) const
{}

String String::arg(Latin1String a, int fieldWidth, Character fillChar) const
{}

String String::arg(pdk::plonglong a, int fieldwidth, int base, Character fillChar) const
{}

String String::arg(pdk::pulonglong a, int fieldwidth, int base, Character fillChar) const
{}

String String::arg(Character a, int fieldWidth, Character fillChar) const
{}

String String::arg(char a, int fieldWidth, Character fillChar) const
{
   
}

String String::arg(double a, int fieldWidth, char fmt, int prec, Character fillChar) const
{}

String String::multiArg(int numArgs, const String **args) const
{}

bool String::isSimpleText() const
{}

bool String::isRightToLeft() const
{}

String String::fromRawData(const Character *str, int size)
{}

String &String::setRawData(const Character *unicode, int size)
{}

String StringRef::toString() const
{}

bool operator==(const StringRef &lhs,const StringRef &rhs) noexcept
{
   
}

bool operator==(const String &lhs,const StringRef &rhs) noexcept
{
}

bool operator==(Latin1String lhs, const StringRef &rhs) noexcept
{
}

bool operator<(const StringRef &lhs,const StringRef &rhs) noexcept
{
}

StringRef StringRef::appendTo(String *string) const
{
}

String &String::append(const StringRef &str)
{}

StringRef StringRef::left(int n) const
{}

StringRef String::leftRef(int n) const
{}

StringRef StringRef::right(int n) const
{}

StringRef String::rightRef(int n) const
{}

StringRef StringRef::substring(int pos, int n) const
{}

StringRef String::substringRef(int pos, int n) const
{}

int StringRef::indexOf(const String &str, int from, CaseSensitivity cs) const
{}

int StringRef::indexOf(Character c, int from, CaseSensitivity cs) const
{
   
}

int StringRef::indexOf(Latin1String str, int from, CaseSensitivity cs) const
{}

int StringRef::indexOf(const StringRef &str, int from, CaseSensitivity cs) const
{
   
}

int StringRef::lastIndexOf(const String &str, int from, CaseSensitivity cs) const
{}

int StringRef::lastIndexOf(Character c, int from, CaseSensitivity cs) const
{}

int StringRef::lastIndexOf(Latin1String str, int from, CaseSensitivity cs) const
{}

int StringRef::lastIndexOf(const StringRef &str, int from, CaseSensitivity cs) const
{}

int StringRef::count(const String &needle, CaseSensitivity cs) const
{}

int StringRef::count(Character needle, CaseSensitivity cs) const
{}

int StringRef::count(const StringRef &needle, CaseSensitivity cs) const
{}

bool StringRef::isRightToLeft() const
{
   
}

bool StringRef::startsWith(const String &needle, CaseSensitivity cs) const
{}

bool StringRef::startsWith(Latin1String needle, CaseSensitivity cs) const
{}

bool StringRef::startsWith(const StringRef &needle, CaseSensitivity cs) const
{}

bool StringRef::startsWith(Character needle, CaseSensitivity cs) const
{}

bool StringRef::endsWith(const String &needle, CaseSensitivity cs) const
{
   
}

bool StringRef::endsWith(Character needle, CaseSensitivity cs) const
{}

bool StringRef::endsWith(Latin1String needle, CaseSensitivity cs) const
{}

bool StringRef::endsWith(const StringRef &needle, CaseSensitivity cs) const
{}

ByteArray StringRef::toLatin1() const
{
   
}

ByteArray StringRef::toLocal8Bit() const
{}

ByteArray StringRef::toUtf8() const
{}

std::vector<char32_t> StringRef::toUcs4() const
{}

StringRef StringRef::trimmed() const
{}

pdk::plonglong StringRef::toLongLong(bool *ok, int base) const
{}

pdk::pulonglong StringRef::toULongLong(bool *ok, int base) const
{}

long StringRef::toLong(bool *ok, int base) const
{}

ulong StringRef::toULong(bool *ok, int base) const
{}

int StringRef::toInt(bool *ok, int base) const
{
   
}

uint StringRef::toUInt(bool *ok, int base) const
{
   
}

short StringRef::toShort(bool *ok, int base) const
{
   
}

ushort StringRef::toUShort(bool *ok, int base) const
{
   
}

double StringRef::toDouble(bool *ok) const
{
   
}

float StringRef::toFloat(bool *ok) const
{
}

String String::toHtmlEscaped() const
{
   
}

} // lang
} // pdk
