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
// Created by softboy on 2018/01/31.

#ifndef PDK_M_BASE_LANG_STRING_MATCHER_H
#define PDK_M_BASE_LANG_STRING_MATCHER_H

#include "pdk/base/lang/String.h"

namespace pdk {
namespace lang {

namespace internal {
class StringMatcherPrivate;
} // internal

using internal::StringMatcherPrivate;
class PDK_CORE_EXPORT StringMatcher
{
public:
   StringMatcher();
   explicit StringMatcher(const String &pattern,
                          pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   StringMatcher(const Character *uc, int len,
                 pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   StringMatcher(const StringMatcher &other);
   ~StringMatcher();
   
   StringMatcher &operator=(const StringMatcher &other);
   
   void setPattern(const String &pattern);
   void setCaseSensitivity(pdk::CaseSensitivity cs);
   
   int indexIn(const String &str, int from = 0) const;
   int indexIn(const Character *str, int length, int from = 0) const;
   String getPattern() const;
   inline pdk::CaseSensitivity getCaseSensitivity() const
   {
      return m_cs;
   }
   
private:
   StringMatcherPrivate *m_implPtr;
   String m_pattern;
   pdk::CaseSensitivity m_cs;
   struct Data {
      uchar m_skiptable[256];
      const Character *m_uc;
      int m_len;
   };
   union {
      uint m_data[256];
      Data m_p;
   };
};


} // lang
} // pdk

#endif // PDK_M_BASE_LANG_STRING_LITERAL_H
