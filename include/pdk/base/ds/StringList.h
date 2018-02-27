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

#ifndef PDK_M_BASE_DS_STRING_LIST_H
#define PDK_M_BASE_DS_STRING_LIST_H

#include "pdk/base/lang/String.h"
#include "pdk/base/lang/StringMatcher.h"
#include <vector>

namespace pdk {
namespace ds {

using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::Character;

class StringList : public std::vector<String>
{
public:
   inline StringList() noexcept
   {}
   
   inline explicit StringList(const String &string)
   {
      push_back(string);
   }
   
   inline StringList(const std::vector<String> &lits) : std::vector<String>(lits)
   {}
   
   inline StringList(std::vector<String> &&list) noexcept 
      : std::vector<String>(std::move(list))
   {}
   
   inline StringList(std::initializer_list<String> args) 
      : std::vector<String>(args)
   {}
   
   StringList &operator=(const std::vector<String> &other)
   { 
      std::vector<String>::operator=(other);
      return *this;
   }
   
   StringList &operator=(std::vector<String> &&other) noexcept
   {
      std::vector<String>::operator=(std::move(other));
      return *this;
   }
   
   inline const_reference at(size_type idx) const noexcept
   {
      PDK_ASSERT_X((idx >= 0 && idx < size()), "StringList::at", "index out of range");
      auto iter = cbegin();
      std::advance(iter, idx);
      return *iter;
   }
   
   String takeFirst()
   {
      String t = std::move(front());
      erase(begin());
      return t;
   }
   
   String takeLast()
   {
      String t = std::move(back());
      pop_back();
      return t;
   }
   
   void swap(int i, int j);
   inline bool contains(const_reference value, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   inline bool contains(Latin1String value, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   
   inline String join(const String &sep) const;
   inline String join(Latin1String sep) const;
   inline String join(Character sep) const;
   inline int removeDuplicates();
   StringList &operator +=(const StringList &other)
   {
      StringList::const_iterator iter = other.cbegin();
      StringList::const_iterator cend = other.cend();
      while (iter != cend) {
         push_back(*iter);
         ++iter;
      }
      return *this;
   }
   
   inline StringList &operator +=(const std::vector<String> &other)
   {
      const_iterator iter = other.cbegin();
      const_iterator cend = other.cend();
      while (iter != cend) {
         push_back(*iter);
         ++iter;
      }
      return *this;
   }
   
   inline StringList &operator +=(const String &other)
   {
      push_back(other);
      return *this;
   }
   
   inline StringList &operator<<(const String &str)
   {
      push_back(str); 
      return *this; 
   }
   
   inline StringList &operator<<(const StringList &list)
   { 
      *this += list; 
      return *this;
   }
   
   inline StringList &operator<<(const std::vector<String> &list)
   { 
      *this += list;
      return *this;
   }
   
   inline reference operator [](int idx) noexcept
   {
      PDK_ASSERT_X((idx >= 0 && static_cast<size_type>(idx) < size()), "StringList::operator[]", "index out of range");
      iterator iter = begin();
      std::advance(iter, idx);
      return *iter;
   }
   
   inline const_reference operator [](int idx) const noexcept
   {
      return at(idx);
   }
};

namespace internal {

String PDK_CORE_EXPORT stringlist_join(const StringList *that, const Character *sep, int seplen);
PDK_CORE_EXPORT String stringlist_join(const StringList &list, Latin1String sep);
bool PDK_CORE_EXPORT stringlist_contains(const StringList *that, const String &str, pdk::CaseSensitivity cs);
bool PDK_CORE_EXPORT stringlist_contains(const StringList *that, Latin1String str, pdk::CaseSensitivity cs);
int PDK_CORE_EXPORT stringlist_remove_duplicates(StringList *that);

} // internal

inline int StringList::removeDuplicates()
{
   return internal::stringlist_remove_duplicates(this);
}

inline String StringList::join(const String &sep) const
{
   return internal::stringlist_join(this, sep.getConstRawData(), sep.length());
}

inline String StringList::join(Latin1String sep) const
{
   return internal::stringlist_join(*this, sep);
}

inline String StringList::join(Character sep) const
{
   return internal::stringlist_join(this, &sep, 1);
}

inline bool StringList::contains(const_reference value, pdk::CaseSensitivity cs) const
{
   return internal::stringlist_contains(this, value, cs);
}
inline bool StringList::contains(Latin1String value, pdk::CaseSensitivity cs) const
{
   return internal::stringlist_contains(this, value, cs);
}

inline void StringList::swap(int i, int j)
{
   PDK_ASSERT_X(i >= 0 && static_cast<size_t>(i) < size() && j >= 0 && static_cast<size_t>(j) < size(),
                "StringList::swap", "index out of range");
   std::swap((*this)[i], (*this)[j]);
}

} // ds
} // pdk

#endif // PDK_M_BASE_DS_STRING_LIST_H
