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

#include "pdk/stdext/utility/DisableIf.h"
#include "pdk/global/Global.h"
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
   
   template <typename ValueType>
   Any(ValueType &&value,
       typename DisableIf<std::is_same<Any&, ValueType>::value>::type * = nullptr, // disable if value has type `Any &`
       typename DisableIf<std::is_const<ValueType>::value>::type * = nullptr) // disable if value has type `const ValueType &&`
      : m_content(new Holder<typename std::decay<ValueType>::type>(static_cast<ValueType &&>(value)))
   {}
   
   ~Any() noexcept
   {
      delete m_content;
   }
   
public:
   Any &swap(Any &other) noexcept
   {
      std::swap(m_content, other.m_content);
      return *this;
   }
   
   template <typename ValueType>
   Any &operator =(const ValueType &other)
   {
      Any(other).swap(*this);
      return *this;
   }
   
   Any &operator =(Any other)
   {
      Any(other).swap(*this);
      return *this;
   }
   
   bool empty() const noexcept
   {
      return !m_content;
   }
   
   void clear() noexcept
   {}
   
   const std::type_info &getType() const noexcept
   {
      return m_content ? m_content->getType() : typeid(void);
   }
   
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
         : m_held(static_cast<ValueType &&>(value))
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

class PDK_CORE_EXPORT BadAnyCast : public std::bad_cast
{
public:
   virtual const char *what() const noexcept
   {
      return "pdk::stdext::BadAnyCast: "
             "failed conversion using pdk::stdext::BadAnyCast";
   }
};

template <typename ValueType>
ValueType *any_cast(Any *operand) noexcept
{
   return operand && operand->getType() == typeid(ValueType) ?
            std::addressof(
               static_cast<Any::Holder<typename std::remove_cv<ValueType>::type> *>(operand->m_content)->m_held
               ) : nullptr;
}

template <typename ValueType>
inline const ValueType *any_cast(const Any* operand) noexcept
{
   return any_cast<ValueType>(const_cast<Any *>(operand));
}

template <typename ValueType>
ValueType any_cast(Any &operand)
{
   using NonRef = typename std::remove_reference<ValueType>::type;
   
   NonRef *result = any_cast<NonRef>(std::addressof(operand));
   if (!result) {
      throw BadAnyCast();
   }
   // Attempt to avoid construction of a temporary object in cases when 
   // `ValueType` is not a reference. Example:
   // `static_cast<std::string>(*result);` 
   // which is equal to `std::string(*result);`
   using RefType = typename std::conditional<
   std::is_reference<ValueType>,
   ValueType,
   typename std::add_lvalue_reference<ValueType>::type
   >::type;
#ifdef PDK_CC_MSVC
#   pragma warning(push)
#   pragma warning(disable: 4172) // "returning address of local variable or temporary" but *result is not local!
#endif
   return static_cast<RefType>(*result);
#ifdef PDK_CC_MSVC
#   pragma warning(pop)
#endif
}

template <typename ValueType>
inline ValueType any_cast(const Any &operand)
{
   using NonRef = typename std::remove_reference<ValueType>::type;
   return any_cast<const NonRef &>(const_cast<Any &>(operand));
}

template <typename ValueType>
inline ValueType any_cast(Any &&operand)
{
   PDK_STATIC_ASSERT_X(std::is_rvalue_reference<ValueType &&>::value/*true if ValueType is rvalue or just a value*/
                       || std::is_const<typename std::remove_reference<ValueType>::type>::value,
                       "pdk::stdext::any_cast shall not be used for getting nonconst references to temporary objects");
   return any_cast<ValueType>(operand);
}

// Note: The "unsafe" versions of any_cast are not part of the
// public interface and may be removed at any time. They are
// required where we know what type is stored in the Any and can't
// use typeid() comparison, e.g., when our types may travel across
// different shared libraries.
template<typename ValueType>
inline ValueType * unsafe_any_cast(Any *operand) noexcept
{
   return std::addressof(
            static_cast<Any::Holder<ValueType> *>(operand->m_content)->m_held
            );
}

template<typename ValueType>
inline const ValueType * unsafe_any_cast(const Any *operand) noexcept
{
   return unsafe_any_cast<ValueType>(const_cast<Any *>(operand));
}

} // stdext
} // pdk

#endif // PDK_STDEXT_ANY_H
