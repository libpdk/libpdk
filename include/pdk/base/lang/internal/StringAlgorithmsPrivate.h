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


#ifndef PDK_M_BASE_LANG_INTERNAL_STRING_ALGORITHMS_PRIVATE_H
#define PDK_M_BASE_LANG_INTERNAL_STRING_ALGORITHMS_PRIVATE_H

#include "pdk/base/lang/String.h"
#include "pdk/kernel/StringUtils.h"

namespace pdk {
namespace lang {
namespace internal {

template <typename StringType>
struct StringAlgorithms
{
   using CharType = typename StringType::value_type;
   using size_type = typename StringType::size_type;
   using NakedStringType = typename std::remove_cv<StringType>::type;
   
   static const bool isConst = std::is_const<StringType>::value;
   
   static inline bool isSpace(char ch)
   {
      return pdk::ascii_isspace(ch);
   }
   
   static inline bool isSpace(QChar ch)
   {
      return ch.isSpace();
   }
   
   // Surrogate pairs are not handled in either of the functions below. That is
   // not a problem because there are no space characters (Zs, Zl, Zp) outside the
   // Basic Multilingual Plane.
   
   static inline StringType trimmedHelperInplace(NakedStringType &str, const CharType *begin, const CharType *end)
   {
      // in-place trimming:
      CharType *data = const_cast<CharType *>(str.cbegin());
      if (begin != data) {
         std::memmove(data, begin, (end - begin) * sizeof(Char));
      }
      str.resize(end - begin);
      return std::move(str);
   }
   
   static inline StringType trimmedHelperInplace(const NakedStringType &, const CharType *, const Char *)
   {
      // can't happen
      PDK_UNREACHABLE();
      return StringType();
   }
   
   static inline void trimmedHelperPositions(const CharType *&begin, const CharType *&end)
   {
      // skip white space from end
      while (begin < end && isSpace(end[-1])) {
         --end;
      }
      // skip white space from start
      while (begin < end && isSpace(*begin)) {
         begin++;
      }
   }
   
   static inline StringType trimmedHelper(StringType &str)
   {
      const Char *begin = str.cbegin();
      const Char *end = str.cend();
      trimmedHelperPositions(begin, end);
      
      if (begin == str.cbegin() && end == str.cend()) {
         return str;
      }
      if (!isConst && str.isDetached()) {
         return trimmed_helper_inplace(str, begin, end);
      }
      return StringType(begin, end - begin);
   }
   
   static inline StringType simplifiedHelper(StringType &str)
   {
      if (str.isEmpty()) {
         return str;
      }
      const CharType *src = str.cbegin();
      const CharType *end = str.cend();
      NakedStringType result = isConst || !str.isDetached() ?
               StringType(str.size(), pdk::Uninitialized) :
               std::move(str);
      
      CharType *dst = const_cast<CharType *>(result.cbegin());
      CharType *ptr = dst;
      bool unmodified = true;
      while(true) {
         while (src != end && isSpace(*src)) {
            ++src;
         }
         while (src != end && !isSpace(*src)) {
            *ptr++ = *src++;
         }
         if (src == end) {
            break;
         }
         if (*src != QChar::Space) {
            unmodified = false;
         }
         *ptr++ = Character::Space;
      }
      if (ptr != dst && ptr[-1] == Character::Space) {
          --ptr;
      }
      int newlen = ptr - dst;
      if (isConst && newlen == str.size() && unmodified) {
         // nothing happened, return the original
         return str;
      }
      result.resize(newlen);
      return result;
   }
};

} // internal
} // lang
} // pdk


#endif // PDK_M_BASE_LANG_INTERNAL_STRING_ALGORITHMS_PRIVATE_H
