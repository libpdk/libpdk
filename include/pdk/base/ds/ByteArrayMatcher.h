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
// Created by softboy on 2018/02/02.

#ifndef PDK_M_BASE_DS_BYTE_ARRAY_ITERATOR_H
#define PDK_M_BASE_DS_BYTE_ARRAY_ITERATOR_H

#include "pdk/base/ds/ByteArray.h"

namespace pdk {
namespace ds {

// forward declare class with namespace
namespace internal {
class ByteArrayMatcherPrivate;
}

using internal::ByteArrayMatcherPrivate;
class PDK_CORE_EXPORT ByteArrayMatcher
{
public:
   ByteArrayMatcher();
   explicit ByteArrayMatcher(const ByteArray &pattern);
   explicit ByteArrayMatcher(const char *pattern, int length);
   ByteArrayMatcher(const ByteArrayMatcher &other);
   ~ByteArrayMatcher();
   
   ByteArrayMatcher &operator=(const ByteArrayMatcher &other);
   
   void setPattern(const ByteArray &pattern);
   
   int indexIn(const ByteArray &ba, int from = 0) const;
   int indexIn(const char *str, int len, int from = 0) const;
   inline ByteArray getPattern() const
   {
      if (m_pattern.isNull()) {
         return ByteArray(reinterpret_cast<const char*>(m_data.m_ptr), m_data.m_len);
      }
      return m_pattern;
   }
   
private:
   ByteArrayMatcherPrivate *m_impl;
   ByteArray m_pattern;
   struct Data {
      uchar m_skiptable[256];
      const uchar *m_ptr;
      int m_len;
   };
   union {
      uint m_dummy[256];
      Data m_data;
   };
};

class StaticByteArrayMatcherBase
{
   PDK_DECL_ALIGN(16)
   struct Skiptable {
      uchar m_data[256];
   } m_skiptable;
protected:
   explicit PDK_DECL_RELAXED_CONSTEXPR StaticByteArrayMatcherBase(const char *pattern, uint n) noexcept
      : m_skiptable(generate(pattern, n)) {}
   // compiler-generated copy/more ctors/assignment operators are ok!
   // compiler-generated dtor is ok!
   
   PDK_CORE_EXPORT int indexOfIn(const char *needle, uint nlen, const char *haystack, int hlen, int from) const noexcept;
   
private:
   static PDK_DECL_RELAXED_CONSTEXPR Skiptable generate(const char *pattern, uint n) noexcept
   {
      const auto uchar_max = (std::numeric_limits<uchar>::max)();
      uchar max = n > uchar_max ? uchar_max : n;
      Skiptable table = {
         // this verbose initialization code aims to avoid some opaque error messages
         // even on powerful compilers such as GCC 5.3. Even though for GCC a loop
         // format can be found that v5.3 groks, it's probably better to go with this
         // for the time being:
         {
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
            max, max, max, max, max, max, max, max,   max, max, max, max, max, max, max, max,
         }
      };
      pattern += n - max;
      while (max--)
         table.m_data[uchar(*pattern++)] = max;
      return table;
   }
};

PDK_WARNING_PUSH
PDK_WARNING_DISABLE_MSVC(4351) // MSVC 2013: "new behavior: elements of array ... will be default initialized"
// remove once we drop MSVC 2013 support
template <uint N>
class StaticByteArrayMatcher : StaticByteArrayMatcherBase
{
                                   char m_pattern[N];
PDK_STATIC_ASSERT_X(N > 2, "StaticByteArrayMatcher makes no sense for finding a single-char pattern");
public:
explicit PDK_DECL_RELAXED_CONSTEXPR StaticByteArrayMatcher(const char (&patternToMatch)[N]) noexcept
   : StaticByteArrayMatcherBase(patternToMatch, N - 1), m_pattern()
{
   for (uint i = 0; i < N; ++i) {
      m_pattern[i] = patternToMatch[i];
   }
}

int indexIn(const ByteArray &haystack, int from = 0) const noexcept
{
   return this->indexOfIn(m_pattern, N - 1, haystack.getRawData(), haystack.size(), from);
}

int indexIn(const char *haystack, int hlen, int from = 0) const noexcept
{
   return this->indexOfIn(m_pattern, N - 1, haystack, hlen, from);
}

ByteArray getPattern() const
{
   return ByteArray(m_pattern, int(N - 1));
}
};

PDK_WARNING_POP

template <uint N>
PDK_DECL_RELAXED_CONSTEXPR StaticByteArrayMatcher<N> pdk_make_static_byte_array_matcher(const char (&pattern)[N]) noexcept
{
   return StaticByteArrayMatcher<N>(pattern);
}

} // ds
} // pdk

#endif // PDK_M_BASE_DS_BYTE_ARRAY_ITERATOR_H
