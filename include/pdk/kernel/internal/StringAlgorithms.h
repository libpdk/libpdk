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

#ifndef PDK_KERNEL_INTERNAL_STRING_ALGORITHMS_H
#define PDK_KERNEL_INTERNAL_STRING_ALGORITHMS_H

#include "pdk/global/Global.h"
#include "pdk/kernel/StringUtils.h" // for pdk::ascii_isspace
#include "pdk/base/lang/Character.h"
#include "pdk/global/EnumDefs.h"

namespace pdk {
namespace internal {

using pdk::lang::Character;

template <typename StringType>
struct StringAlgorithms
{
   using CharType = typename StringType::value_type;
   using SizeType = typename StringType::size_type;
   using NakedStringType = typename std::remove_cv<StringType>::type;
   
   static const bool isConst = std::is_const<StringType>::value;
   
   static inline bool isSpace(char c)
   {
      return pdk::ascii_isspace(c);
   }
   
   static inline bool isSpace(Character c)
   {
      return c.isSpace();
   }
   
   static inline StringType trimmedInplace(NakedStringType &str, const CharType *begin, 
                                           const CharType *end)
   {
      CharType *data = const_cast<CharType *>(str.cbegin());
      if (begin != data) {
         std::memmove(data, begin, (end - begin) * sizeof(CharType));
      }
      str.resize(end - begin);
      return std::move(str);
   }
   
   static inline StringType trimmedInplace(const NakedStringType &, const CharType *, const CharType *)
   {
      // can't happen, just lie to compiler
      PDK_UNREACHABLE();
      return StringType();
   }
   
   static inline void trimmedPositions(const CharType *&begin, const CharType *&end)
   {
      while (begin < end && isSpace(*begin)) {
         ++begin;
      }
      if (begin < end) {
         while (begin < end && isSpace(end[-1])) {
            --end;
         }
      }
   }
   
   static inline StringType trimmed(StringType &str)
   {
      const CharType *begin = str.cbegin();
      const CharType *end = str.cend();
      trimmedPositions(begin, end);
      if (begin == str.cbegin() && end == str.cend()) {
         return str;
      }
      if (!isConst && str.isDetached()) {
         return trimmedInplace(str, begin, end);
      }
      return StringType(begin, end - begin);
   }
   
   static inline StringType simplified(StringType &str)
   {
      if (str.isEmpty()) {
         return str;
      }
      const CharType *src = str.cbegin();
      const CharType *end = str.cend();
      NakedStringType result = isConst || !str.isDetached() 
            ? StringType(str.size(), pdk::Initialization::Uninitialized)
            : std::move(str);
      CharType *dest = const_cast<CharType *>(result.cbegin());
      CharType *ptr = dest;
      bool unmodified = true;
      while (true) {
         while (src != end && isSpace(*src)) {
            ++src;
         }
         while (src != end && !isSpace(*src)) {
            *ptr++ = *src++;
         }
         if (src == end) {
            break;
         }
         if (*src != Character::Space) {
            unmodified = false;
         }
         *ptr++ = Character::Space;
      }
      if (ptr != dest && ptr[-1] == Character::Space) {
         --ptr;
      }
      int newLength = ptr - dest;
      if (isConst && newLength == str.size() && unmodified) {
         return str;
      }
      result.resize(newLength);
      return result;
   }
};

} // internal 
} // pdk

#endif // PDK_KERNEL_INTERNAL_STRING_ALGORITHMS_H
