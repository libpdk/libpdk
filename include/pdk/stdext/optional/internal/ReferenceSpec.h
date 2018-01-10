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

#ifndef PDK_STDEXT_OPTIONAL_INTERNAL_REFERENCE_SPEC_H
#define PDK_STDEXT_OPTIONAL_INTERNAL_REFERENCE_SPEC_H

#include "pdk/stdext/optional/Optional.h"

namespace pdk {
namespace stdext {
namespace optional {

namespace internal {

template <typename Form>
void PreventBindingRvalue()
{
   PDK_STATIC_ASSERT_X(std::is_lvalue_reference<Form>::value, 
                       "binding rvalue references to optional lvalue references is disallowed");
}

template <typename T>
typename std::remove_reference<T>::type &forward_reference(T &&value)
{
   PDK_STATIC_ASSERT_X(std::is_lvalue_reference<T>::value, 
                       "binding rvalue references to optional lvalue references is disallowed");
   return std::forward<T>(value);
}

template <typename T>
struct IsOptional
{
   static const bool value = false;
};

template <typename T>
struct IsOptional<Optional<T>>
{
   static const bool value = true; 
};

template <typename T>
struct IsNoOptional
{
   static const bool value = !IsOptional<typename std::decay<T>::type>::value; 
};

template <typename T, typename U>
struct IsSameDecayed
{
   static const bool value = std::is_same<T, typename std::remove_reference<U>::type>::value
   || std::is_same<T, const typename std::remove_reference<U>::type>::value;
};

template <typename T, typename U>
struct NoUnboxingCond
{
   static const bool value = IsNoOptional<U>::value && !IsSameDecayed<T, U>::value;
};

} // internal

template <typename T>
class Optional<T &> : public internal::OptionalTag
{
public:
   using ValueType = T &;
   using ReferenceType = T &;
   using ReferenceConstType = T &;
   using RvalReferenceType = T &;
   using Pointer = T *;
   using PointerConstType = T *;
   
public:
   Optional() noexcept
      : m_ptr(nullptr)
   {}
   
   Optional(None) noexcept
      : m_ptr(nullptr)
   {}
   
   template <typename U>
   explicit Optional(const Optional<U &> &other) noexcept
      : m_ptr(other.getPtr())
   {}
   
   Optional(const Optional &other) noexcept
      : m_ptr(other.getPtr())
   {}
   
   Optional(T &&) noexcept
   {
      internal::PreventBindingRvalue<T&&>();
   }
   
   template <class R>
   Optional(R &&other, typename std::enable_if<internal::NoUnboxingCond<T, R>::value>::type * = nullptr) noexcept
      : m_ptr(std::addressof(other))
   {
      internal::PreventBindingRvalue<R>();
   }
   
   template <class R>
   Optional(bool cond, R &&other, typename std::enable_if<internal::IsNoOptional<R>::value>::type * = nullptr) noexcept
      : m_ptr(cond ? std::addressof(other) : nullptr)
   {
      internal::PreventBindingRvalue<R>();
   }
   
   Optional &operator =(const Optional &other) noexcept
   {
      m_ptr = other.getPtr();
      return *this;
   }
   
   template <typename U>
   Optional &operator =(const Optional<U &> &other) noexcept
   {
      m_ptr = other.getPtr();
      return *this;
   }
   
   Optional &operator=(None) noexcept
   {
      m_ptr = nullptr;
      return *this;
   }
   
   template <typename R>
   typename std::enable_if<internal::IsNoOptional<R>::value, Optional<T &> &>::type
   operator= (R&& value) noexcept
   {
      internal::PreventBindingRvalue<R>();
      m_ptr = std::addressof(value);
      return *this;
   }
   
   template <class R>
   void emplace(R &&value, typename std::enable_if<internal::IsNoOptional<R>::value>::type * = nullptr) noexcept
   {
      internal::PreventBindingRvalue<R>();
      m_ptr = std::addressof(value);
   }
   
   template <class R>
   T &getValueOr(R &&value, typename std::enable_if<internal::IsNoOptional<R>::value>::type * = nullptr) const noexcept
   {
      internal::PreventBindingRvalue<R>();
      return m_ptr ? *m_ptr : value;
   }
   
   template <class R>
   T &valueOr(R &&value, typename std::enable_if<internal::IsNoOptional<R>::value>::type * = nullptr) const noexcept
   {
      internal::PreventBindingRvalue<R>();
      return m_ptr ? *m_ptr : value;
   }
   
   template <class F>
   T &valueOrEval(F f) const
   {
      return m_ptr ? *m_ptr : internal::forward_reference(f());
   }
   
   void swap(Optional &other) noexcept
   {
      std::swap(m_ptr, other.m_ptr);
   }
   
   T &get() const
   {
      PDK_ASSERT(m_ptr);
      return *m_ptr;
   }
   
   T *getPtr() const noexcept
   {
      return m_ptr;
   }
   
   T *operator ->() const
   {
      PDK_ASSERT(m_ptr);
      return m_ptr;
   }
   
   T &operator *() const
   {
      PDK_ASSERT(m_ptr);
      return *m_ptr;
   }
   
   T &value() const
   {
      return m_ptr ? *m_ptr : (throw BadAccess(), m_ptr);
   }
   
   bool operator!() const noexcept
   {
      return m_ptr == nullptr;
   }
   
   operator bool () const noexcept
   {
      return !this->operator!();
   }
   
   bool isInitialized() const noexcept
   {
      return m_ptr != nullptr;
   }
   
private:
   T *m_ptr;
};

template <class T> 
void swap(Optional<T &> &lhs, Optional<T &> &rhs) noexcept
{
   lhs.swap(rhs);
}

} // optional
} // stdext
} // pdk

#endif // PDK_STDEXT_OPTIONAL_INTERNAL_REFERENCE_SPEC_H
