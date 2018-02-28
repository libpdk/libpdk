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
// Created by softboy on 2018/02/01.

#include "pdk/base/lang/StringMatcher.h"
#include "pdk/base/lang/internal/StringHelper.h"
namespace pdk {
namespace lang {

namespace {

void bm_init_skiptable(const char16_t *uc, int len, uchar *skiptable, pdk::CaseSensitivity cs)
{
   int l = std::min(len, 255);
   std::memset(skiptable, l, 256*sizeof(uchar));
   uc += len - l;
   if (cs == pdk::CaseSensitivity::Sensitive) {
      while (l--) {
         skiptable[*uc & 0xff] = l;
         uc++;
      }
   } else {
      const char16_t *start = uc;
      while (l--) {
         skiptable[internal::fold_case(uc, start) & 0xff] = l;
         uc++;
      }
   }
}

inline int bm_find(const char16_t *uc, uint l, int index, const char16_t *puc, uint pl,
                   const uchar *skiptable, pdk::CaseSensitivity cs)
{
   if (pl == 0)
      return index > (int)l ? -1 : index;
   const uint pl_minus_one = pl - 1;
   
   const char16_t *current = uc + index + pl_minus_one;
   const char16_t *end = uc + l;
   if (cs == pdk::CaseSensitivity::Sensitive) {
      while (current < end) {
         uint skip = skiptable[*current & 0xff];
         if (!skip) {
            // possible match
            while (skip < pl) {
               if (*(current - skip) != puc[pl_minus_one-skip])
                  break;
               skip++;
            }
            if (skip > pl_minus_one) // we have a match
               return (current - uc) - pl_minus_one;
            
            // in case we don't have a match we are a bit inefficient as we only skip by one
            // when we have the non matching char in the string.
            if (skiptable[*(current - skip) & 0xff] == pl)
               skip = pl - skip;
            else
               skip = 1;
         }
         if (current > end - skip)
            break;
         current += skip;
      }
   } else {
      while (current < end) {
         uint skip = skiptable[internal::fold_case(current, uc) & 0xff];
         if (!skip) {
            // possible match
            while (skip < pl) {
               if (internal::fold_case(current - skip, uc) != internal::fold_case(puc + pl_minus_one - skip, puc))
                  break;
               skip++;
            }
            if (skip > pl_minus_one) // we have a match
               return (current - uc) - pl_minus_one;
            // in case we don't have a match we are a bit inefficient as we only skip by one
            // when we have the non matching char in the string.
            if (skiptable[internal::fold_case(current - skip, uc) & 0xff] == pl)
               skip = pl - skip;
            else
               skip = 1;
         }
         if (current > end - skip)
            break;
         current += skip;
      }
   }
   return -1; // not found
}

} // anonymous namespace

StringMatcher::StringMatcher()
   : m_implPtr(nullptr),
     m_cs(pdk::CaseSensitivity::Sensitive)
{
   std::memset(m_data, 0, sizeof(m_data));
}

StringMatcher::StringMatcher(const String &pattern, pdk::CaseSensitivity cs)
   : m_implPtr(nullptr),
     m_pattern(pattern),
     m_cs(cs)
{
   m_p.m_uc = pattern.unicode();
   m_p.m_len = pattern.size();
   bm_init_skiptable(reinterpret_cast<const char16_t *>(m_p.m_uc), m_p.m_len, m_p.m_skiptable, cs);
}

StringMatcher::StringMatcher(const Character *uc, int len, pdk::CaseSensitivity cs)
   : m_implPtr(nullptr),
     m_cs(cs)
{
   m_p.m_uc = uc;
   m_p.m_len = len;
   bm_init_skiptable(reinterpret_cast<const char16_t *>(m_p.m_uc), len, m_p.m_skiptable, cs);
}

StringMatcher::StringMatcher(const StringMatcher &other)
   : m_implPtr(nullptr)
{
   operator=(other);
}

StringMatcher::~StringMatcher()
{
   PDK_UNUSED(m_implPtr);
}

StringMatcher &StringMatcher::operator=(const StringMatcher &other)
{
   if (this != &other) {
      m_pattern = other.m_pattern;
      m_cs = other.m_cs;
      std::memcpy(m_data, other.m_data, sizeof(m_data));
   }
   return *this;
}

void StringMatcher::setPattern(const String &pattern)
{
   m_pattern = pattern;
   m_p.m_uc = pattern.unicode();
   m_p.m_len = pattern.size();
   bm_init_skiptable(reinterpret_cast<const char16_t *>(pattern.unicode()), pattern.size(), m_p.m_skiptable, m_cs);
}

String StringMatcher::getPattern() const
{
   if (!m_pattern.isEmpty()) {
      return m_pattern;
   }  
   return String(m_p.m_uc, m_p.m_len);
}

void StringMatcher::setCaseSensitivity(pdk::CaseSensitivity cs)
{
   if (cs == m_cs) {
      return;
   }
   bm_init_skiptable(reinterpret_cast<const char16_t *>(m_p.m_uc), m_p.m_len, m_p.m_skiptable, cs);
   m_cs = cs;
}

int StringMatcher::indexIn(const String &str, int from) const
{
   if (from < 0) {
      from = 0;
   }
   return bm_find(reinterpret_cast<const char16_t *>(str.unicode()), str.size(), from,
                  reinterpret_cast<const char16_t *>(m_p.m_uc), m_p.m_len,
                  m_p.m_skiptable, m_cs);
}

int StringMatcher::indexIn(const Character *str, int length, int from) const
{
   if (from < 0) {
      from = 0;
   }
   return bm_find(reinterpret_cast<const char16_t *>(str), length, from,
                  reinterpret_cast<const char16_t *>(m_p.m_uc), m_p.m_len,
                  m_p.m_skiptable, m_cs);
}

namespace internal {

int find_string_boyer_moore(
      const Character *haystack, int haystackLen, int haystackOffset,
      const Character *needle, int needleLen, pdk::CaseSensitivity cs)
{
   uchar skiptable[256];
   bm_init_skiptable(reinterpret_cast<const char16_t *>(needle), needleLen, skiptable, cs);
   if (haystackOffset < 0)
      haystackOffset = 0;
   return bm_find(reinterpret_cast<const char16_t *>(haystack), haystackLen, haystackOffset,
                  reinterpret_cast<const char16_t *>(needle), needleLen, skiptable, cs);
}
} // internal

} // lang
} // pdk
