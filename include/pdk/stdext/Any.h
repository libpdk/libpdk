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
// Created by softboy on 2017/01/09.

// Copyright (C) 2014, Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/optional for documentation.
//
// You are welcome to contact the author at:
//  akrzemi1@gmail.com
//

#ifndef PDK_STDEXT_ANY_H
#define PDK_STDEXT_ANY_H

#include <algorithm>
#include <typeinfo>

namespace pdk {
namespace stdext {

class Any
{
public:
   Any() noexcept
      : m_content(nullptr)
   {}
   
   template <typename ValueType>
   Any(const ValueType &value)
      : m_content(new Holder<typename std::remove_cv<typename std::decay<const ValueType>::type>::type>(value))
   {}
   
   Any(const Any &other)
      : m_content(other.m_content ? other.m_content->clone() : nullptr)
   {}
   
   Any(Any &&other) noexcept
      : m_content(other.m_content)
   {
      other.m_content = nullptr;
   }
   
//   template <typename ValueType>
//   Any(ValueType &&value,
//       typename )
   
//   {}
private:
   class PlaceHolder
   {
   public:
      virtual ~PlaceHolder()
      {}
      virtual const std::type_info &getType() const noexcept = 0;
      virtual PlaceHolder *clone() const = 0;
   };
   
   template <typename ValueType>
   class Holder : public PlaceHolder
   {
   public:
      Holder(const ValueType &value)
         : m_held(value)
      {}
      
      Holder(ValueType &&value)
         : m_held(std::move(value))
      {}
      
      virtual const std::type_info &getType() const noexcept
      {
         return typeid(ValueType);
      }
      
      virtual PlaceHolder *clone() const
      {
         return new Holder(m_held);
      }
   public:
      ValueType m_held;
      
   private:
      Holder &operator =(const Holder &);
   };
   
private: // representation
   
   template<typename ValueType>
   friend ValueType *any_cast(any *) noexcept;
   template<typename ValueType>
   friend ValueType *unsafe_any_cast(any *) noexcept;
private:
   PlaceHolder *m_content;
};

inline void swap(Any &lhs, Any &rhs) noexcept
{
   lhs.swap(rhs);
}

} // stdext
} // pdk

#endif // PDK_STDEXT_ANY_H
