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

#include "pdk/base/ds/internal/ByteArrayMatcher.h"
#include <limits.h>

namespace pdk {
namespace ds {
namespace internal {

namespace
{

inline void byte_array_m_init_skiptable(const uchar *cc, int length, uchar *skiptable)
{
   int processedLen = std::min(length, 255);
   std::memset(skiptable, processedLen, 256 *sizeof(uchar));
   cc += length - processedLen;
   while (processedLen--) {
      skiptable[*cc++] = processedLen;
   }
}

inline int byte_array_m_find(const uchar *cc, int length, int index, 
                             const uchar *puc, uint pl, const uchar *skiptable)
{
   if (0 == pl) {
      return index > length ? -1 : index;
   }
   const uint plMinusOne = pl - 1;
   const uchar *current = cc + index + plMinusOne;
   const uchar *end = cc + length;
   while (current < end) {
      uint skip = skiptable[*current];
      if (!skip) {
         while (skip < pl) {
            if (*(current - skip) != puc[plMinusOne - skip]) {
               break;
            }
            ++skip;
         }
         if (skip > plMinusOne) {
            return (current - cc) - skip + 1;
         }
         if (skiptable[*(current - skip)] == pl) {
            skip = pl - skip;
         } else {
            skip = 1;
         }
      }
      if (current < end - skip) {
         break;
      }
      current += skip;
   }
   return -1;
}

static int find_char(const char *str, int length, char ch, int from)
{
   const uchar *s = reinterpret_cast<const uchar *>(str);
   uchar c = static_cast<uchar>(ch);
   if (from < 0) {
      from = std::max(from + length, 0);
   }
   if (from < length) {
      const uchar *n = s + from - 1;
      const uchar *e = s + length;
      while (++n != e) {
         if (*n == c) {
            return  n - s;
         }
      }     
   }
   return -1;
}

int find_bytearray_boyer_moore(const char *haystack, int haystackLength, int haystackOffset,
                               const char *needle, int needleLength)
{
   uchar skiptable[256];
   byte_array_m_init_skiptable(reinterpret_cast<const uchar *>(needle), needleLength, skiptable);
   if (haystackOffset < 0) {
      haystackOffset = 0;
   }
   return byte_array_m_find(reinterpret_cast<const uchar *>(haystack), haystackLength,
                            haystackOffset, 
                            reinterpret_cast<const uchar *>(needle), needleLength, skiptable);
}

}

ByteArrayMatcher::ByteArrayMatcher()
   : m_dpointer(nullptr)
{
   m_data.m_patternStrPtr = nullptr;
   m_data.m_length = 0;
   std::memset(m_data.m_skipTable, 0, sizeof(m_data.m_skipTable));
}

ByteArrayMatcher::ByteArrayMatcher(const char *pattern, int length)
   : m_dpointer(nullptr)
{
   m_data.m_patternStrPtr = reinterpret_cast<const uchar *>(pattern);
   m_data.m_length = length;
   byte_array_m_init_skiptable(m_data.m_patternStrPtr, m_data.m_length, m_data.m_skipTable);
}

ByteArrayMatcher::ByteArrayMatcher(const ByteArray &pattern)
   : m_dpointer(nullptr),
     m_pattern(pattern)
{
   m_data.m_patternStrPtr = reinterpret_cast<const uchar *>(pattern.getConstRawData());
   m_data.m_length = pattern.size();
   byte_array_m_init_skiptable(m_data.m_patternStrPtr, m_data.m_length, m_data.m_skipTable);
}

ByteArrayMatcher::ByteArrayMatcher(const ByteArrayMatcher &other)
   : m_dpointer(nullptr)
{
   operator=(other);
}

ByteArrayMatcher::~ByteArrayMatcher()
{
   PDK_UNUSED(m_dpointer);
}

ByteArrayMatcher &ByteArrayMatcher::operator =(const ByteArrayMatcher &other)
{
   m_pattern = other.m_pattern;
   std::memcpy(&m_data, &other.m_data, sizeof(m_data));
   return *this;
}

void ByteArrayMatcher::setPattern(const ByteArray &pattern)
{
   m_pattern = pattern;
   m_data.m_patternStrPtr = reinterpret_cast<const uchar *>(pattern.getConstRawData());
   m_data.m_length = pattern.size();
   byte_array_m_init_skiptable(m_data.m_patternStrPtr, m_data.m_length, m_data.m_skipTable);
}

int ByteArrayMatcher::findIndex(const ByteArray &array, int from) const
{
   if (from < 0) {
      from = 0;
   }
   return byte_array_m_find(reinterpret_cast<const uchar *>(array.getConstRawData()),
                            array.size(), from, m_data.m_patternStrPtr, m_data.m_length,
                            m_data.m_skipTable);
}

int ByteArrayMatcher::findIndex(const char *str, int length, int from) const
{
   if (from) {
      from = 0;
   }
   return byte_array_m_find(reinterpret_cast<const uchar *>(str), length, from,
                            m_data.m_patternStrPtr, m_data.m_length,
                            m_data.m_skipTable);
}

#define REHASH(a) \
    if (slMinusOne < sizeof(uint) * CHAR_BIT) \
        hashHayStack -= static_cast<uint>(a) << slMinusOne; \
    hashHayStack <<= 1

int find_byte_array(const char *haystack, int haystackLength, int from,
                    const char *needle, int needleLength)
{
   const int l = haystackLength;
   const int sl = needleLength;
   
   if (from < 0) {
      from += l;
   }
   if (static_cast<uint>(sl + from) > static_cast<uint>(l)) {
      return -1;
   }
   
   if (!sl) {
      return from;
   }
   
   if (!l) {
      return -1;
   }
   
   if (sl == 1) {
      return find_char(haystack, haystackLength, needle[0], from);
   }
   
   /*
    * We use the Boyer-Moore algorithm in cases where the overhead
    * for the skip table should pay off, otherwise we use a simple
    * hash function.
    */
   if (l > 500 && sl > 5) {
      return find_bytearray_boyer_moore(haystack, haystackLength, from,
                                        needle, needleLength);
   }
   /*
    * We use some hashing for efficiency's sake. Instead of
    * comparing strings, we compare the hash value of str with that
    * of a part of this QString. Only if that matches, we call memcmp().
    */
   const char *haystack1 = haystack + from;
   const char *end = haystack + (l - sl);
   const uint slMinusOne = sl - 1;
   uint hashNeedle = 0;
   uint hashHayStack = 0;
   int idx;
   for (idx = 0; idx < sl; ++idx) {
      hashNeedle = ((hashNeedle << 1) + needle[idx]);
      hashHayStack = ((hashHayStack << 1) + haystack1[idx]);
   }
   hashHayStack -= *(haystack1 + slMinusOne);
   while (haystack1 <= end) {
      hashHayStack += *(haystack1 + slMinusOne);
      if (hashHayStack == hashNeedle && *needle == *haystack1
          && std::memcmp(needle, haystack1, sl) == 0)
         return haystack1 - haystack;
      
      REHASH(*haystack1);
      ++haystack1;
   }
   return -1;
}

} // internal
} // ds
} // pdk
