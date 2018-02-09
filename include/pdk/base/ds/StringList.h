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
#include <list>

namespace pdk {
namespace ds {

using pdk::lang::String;

class StringList : public std::list<String>
{
public:
   inline StringList() noexcept
   {}
   
   inline explicit StringList(const String &string)
   {
      push_back(string);
   }
   
   inline StringList(const std::list<String> &lits) : std::list<String>(lits)
   {}
   
   inline StringList(std::list<String> &&list) noexcept 
      : std::list<String>(std::move(list))
   {}
   
   inline StringList(std::initializer_list<String> args) 
      : std::list<String>(args)
   {}
   
   StringList &operator=(const std::list<String> &other)
   { 
      std::list<String>::operator=(other);
      return *this;
   }
   
   StringList &operator=(std::list<String> &&other) noexcept
   {
      std::list<String>::operator=(std::move(other));
      return *this;
   }
   
   inline const_reference at(size_type idx) const noexcept
   {
      PDK_ASSERT_X((idx >= 0 && idx < size()), "StringList::at", "index out of range");
      auto iter = cbegin();
      std::advance(iter, idx);
      return *iter;
   }
   
   inline bool contains(const_reference value) const
   {
      return std::find(begin(), end(), value) != end();
   }
   
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
   
   inline StringList &operator +=(const std::list<String> &other)
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
   
   inline StringList &operator<<(const std::list<String> &list)
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

} // ds
} // pdk

#endif // PDK_M_BASE_DS_STRING_LIST_H
