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

#include "pdk/base/ds/ByteArrayMatcher.h"
#include <limits.h>

namespace pdk {
namespace ds {

namespace {

inline void bm_init_skiptable(const uchar *cc, int len, uchar *skiptable)
{
   int l = std::min(len, 255);
   std::memset(skiptable, l, 256*sizeof(uchar));
   cc += len - l;
   while (l--) {
      skiptable[*cc++] = l;
   }
}

inline int bm_find(const uchar *cc, int l, int index, const uchar *puc, uint pl,
                   const uchar *skiptable)
{
   if (pl == 0) {
      return index > l ? -1 : index;
   }
   const uint pl_minus_one = pl - 1;
   
   const uchar *current = cc + index + pl_minus_one;
   const uchar *end = cc + l;
   while (current < end) {
      uint skip = skiptable[*current];
      if (!skip) {
         // possible match
         while (skip < pl) {
            if (*(current - skip) != puc[pl_minus_one - skip]) {
               break;
            }
            skip++;
         }
         if (skip > pl_minus_one) // we have a match
            return (current - cc) - skip + 1;
         
         // in case we don't have a match we are a bit inefficient as we only skip by one
         // when we have the non matching char in the string.
         if (skiptable[*(current - skip)] == pl) {
            skip = pl - skip;
         }else {
            skip = 1;
         }
      }
      if (current > end - skip) {
         break;
      }
      current += skip;
   }
   return -1; // not found
}

} // anonymous namespace

ByteArrayMatcher::ByteArrayMatcher()
   : m_impl(nullptr)
{
   m_data.m_ptr = 0;
   m_data.m_len = 0;
   std::memset(m_data.m_skiptable, 0, sizeof(m_data.m_skiptable));
}

ByteArrayMatcher::ByteArrayMatcher(const char *pattern, int length)
   : m_impl(nullptr)
{
   m_data.m_ptr = reinterpret_cast<const uchar *>(pattern);
   m_data.m_len = length;
   bm_init_skiptable(m_data.m_ptr, m_data.m_len, m_data.m_skiptable);
}

ByteArrayMatcher::ByteArrayMatcher(const ByteArray &pattern)
   : m_impl(nullptr),
     m_pattern(pattern)
{
   m_data.m_ptr = reinterpret_cast<const uchar *>(pattern.getConstRawData());
   m_data.m_len = pattern.size();
   bm_init_skiptable(m_data.m_ptr, m_data.m_len, m_data.m_skiptable);
}

ByteArrayMatcher::ByteArrayMatcher(const ByteArrayMatcher &other)
   : m_impl(nullptr)
{
   operator=(other);
}

ByteArrayMatcher::~ByteArrayMatcher()
{
   PDK_UNUSED(m_impl);
}

ByteArrayMatcher &ByteArrayMatcher::operator=(const ByteArrayMatcher &other)
{
   m_pattern = other.m_pattern;
   memcpy(&m_data, &other.m_data, sizeof(m_data));
   return *this;
}

void ByteArrayMatcher::setPattern(const ByteArray &pattern)
{
   m_pattern = pattern;
   m_data.m_ptr = reinterpret_cast<const uchar *>(pattern.getConstRawData());
   m_data.m_len = pattern.size();
   bm_init_skiptable(m_data.m_ptr, m_data.m_len, m_data.m_skiptable);
}

int ByteArrayMatcher::indexIn(const ByteArray &ba, int from) const
{
   if (from < 0) {
      from = 0;
   }
   return bm_find(reinterpret_cast<const uchar *>(ba.getConstRawData()), ba.size(), from,
                  m_data.m_ptr, m_data.m_len, m_data.m_skiptable);
}

int ByteArrayMatcher::indexIn(const char *str, int len, int from) const
{
   if (from < 0) {
      from = 0;
   }
   return bm_find(reinterpret_cast<const uchar *>(str), len, from,
                  m_data.m_ptr, m_data.m_len, m_data.m_skiptable);
}

namespace {
int pdk_find_char(const char *str, int len, char ch, int from)
{
   const uchar *s = (const uchar *)str;
   uchar c = (uchar)ch;
   if (from < 0) {
      from = std::max(from + len, 0);
   }
   if (from < len) {
      const uchar *n = s + from - 1;
      const uchar *e = s + len;
      while (++n != e) {
         if (*n == c) {
            return  n - s;
         } 
      }
   }
   return -1;
}

int pdk_find_byte_array_boyer_moore(
      const char *haystack, int haystackLen, int haystackOffset,
      const char *needle, int needleLen)
{
   uchar skiptable[256];
   bm_init_skiptable((const uchar *)needle, needleLen, skiptable);
   if (haystackOffset < 0) {
      haystackOffset = 0;
   }
   return bm_find((const uchar *)haystack, haystackLen, haystackOffset,
                  (const uchar *)needle, needleLen, skiptable);
}

} // anonymous namespace

#define REHASH(a) \
   if (sl_minus_1 < sizeof(uint) * CHAR_BIT) \
   hashHaystack -= uint(a) << sl_minus_1; \
   hashHaystack <<= 1

int pdk_find_byte_array(
      const char *haystack0, int haystackLen, int from,
      const char *needle, int needleLen)
{
   const int l = haystackLen;
   const int sl = needleLen;
   if (from < 0)
      from += l;
   if (uint(sl + from) > (uint)l)
      return -1;
   if (!sl)
      return from;
   if (!l)
      return -1;
   
   if (sl == 1)
      return pdk_find_char(haystack0, haystackLen, needle[0], from);
   
   /*
      We use the Boyer-Moore algorithm in cases where the overhead
      for the skip table should pay off, otherwise we use a simple
      hash function.
    */
   if (l > 500 && sl > 5)
      return pdk_find_byte_array_boyer_moore(haystack0, haystackLen, from,
                                             needle, needleLen);
   
   /*
      We use some hashing for efficiency's sake. Instead of
      comparing strings, we compare the hash value of str with that
      of a part of this QString. Only if that matches, we call memcmp().
    */
   const char *haystack = haystack0 + from;
   const char *end = haystack0 + (l - sl);
   const uint sl_minus_1 = sl - 1;
   uint hashNeedle = 0, hashHaystack = 0;
   int idx;
   for (idx = 0; idx < sl; ++idx) {
      hashNeedle = ((hashNeedle<<1) + needle[idx]);
      hashHaystack = ((hashHaystack<<1) + haystack[idx]);
   }
   hashHaystack -= *(haystack + sl_minus_1);
   
   while (haystack <= end) {
      hashHaystack += *(haystack + sl_minus_1);
      if (hashHaystack == hashNeedle && *needle == *haystack
          && memcmp(needle, haystack, sl) == 0) {
         return haystack - haystack0;
      }
      REHASH(*haystack);
      ++haystack;
   }
   return -1;
}

int StaticByteArrayMatcherBase::indexOfIn(const char *needle, uint nlen, const char *haystack, int hlen, int from) const noexcept
{
   if (from < 0) {
      from = 0;
   }
   return bm_find(reinterpret_cast<const uchar *>(haystack), hlen, from,
                  reinterpret_cast<const uchar *>(needle),   nlen, m_skiptable.m_data);
}


} // ds
} // pdk
