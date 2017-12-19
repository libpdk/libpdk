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

#ifndef PDK_M_BASE_DS_INTERNAL_BYTEARRAY_MATCHER_H
#define PDK_M_BASE_DS_INTERNAL_BYTEARRAY_MATCHER_H

#include "pdk/global/Global.h"
#include "pdk/base/ds/ByteArray.h"

namespace pdk {
namespace ds {
namespace internal {

using pdk::ds::ByteArray;

class ByteArrayMatcherPrivate;

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
   
   int findIndex(const ByteArray &array, int from = 0) const;
   int findIndex(const char *str, int length, int from = 0) const;
   
   inline ByteArray getPattern() const
   {
      if (m_pattern.isNull()) {
         return ByteArray(reinterpret_cast<const char*>(m_data.m_patternStrPtr), m_data.m_length);
      }
      return m_pattern;
   }
   
private:
   ByteArrayMatcherPrivate *m_dpointer;
   ByteArray m_pattern;
   struct Data
   {
      uchar m_skipTable[256];
      const uchar *m_patternStrPtr;
      int m_length;
   };
   
   union
   {
      uint m_dummy[256];
      Data m_data;
   };
};


} // internal
} // ds
} // pdk

#endif // PDK_M_BASE_DS_INTERNAL_BYTEARRAY_MATCHER_H
