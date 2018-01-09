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

#ifndef PDK_STDEXT_OPTIONAL_OPTIONAL_H
#define PDK_STDEXT_OPTIONAL_OPTIONAL_H

#include "pdk/stdext/None.h"
#include "pdk/global/Global.h"
#include "pdk/stdext/optional/OptionalFwd.h"
#include <type_traits>

namespace pdk {
namespace stdext {
namespace optional {

using pdk::stdext::None;

struct InPlaceInit
{
   struct InitTag {};
   explicit InPlaceInit(InitTag) {}
};

const InPlaceInit in_place_init((InPlaceInit::InitTag()));

struct InPlaceInitIf
{
   struct InitTag{};
   explicit InPlaceInitIf(InitTag) {}
};

const InPlaceInitIf in_place_init_if((InPlaceInitIf::InitTag()));

namespace internal {

struct OptionalTag {};

template <typename T>
class OptionalBase : public OptionalTag
{
private:
   using StorageType = std::aligned_storage<sizeof(T), alignof(T)>;
   using ThisType = OptionalBase<T>;
   
protected:
   using ValueType = T;
   using value_type = ValueType;
   using ReferenceType = T &;
   using ReferenceConstType = T const &;
   using RvalReferenceType = T &&;
   using ReferenceTypeOfTemporaryWrapper = T &&;
   using PointerType = T *;
   using PointerConstType = const T *;
   using ArgumentType = const T &;
   
   using reference_type = ReferenceType;
   using reference_const_type = ReferenceConstType;
   using rval_reference_type = RvalReferenceType;
   using reference_type_of_temporary_wrapper = ReferenceTypeOfTemporaryWrapper;
   using pointer_type = PointerType;
   using pointer_const_type = PointerConstType;
   using argument_type = ArgumentType;
   
   OptionalBase()
      : m_initialized(false)
   {}
   
   // Creates an Optional<T> uninitialized.
   // No-throw
   OptionalBase(None)
      : m_initialized(false)
   {}
   
   // Creates an Optional<T> initialized with 'val'.
   // Can throw if T::T(T const&) does
   OptionalBase(ArgumentType value)
      : m_initialized(false)
   {
      construct(value);
   }
   
   // move-construct an Optional<T> initialized from an rvalue-ref to 'val'.
   // Can throw if T::T(T&&) does
   OptionalBase(RvalReferenceType value)
      : m_initialized(false)
   {
      construct(std::move(value));
   }
   
   // Creates an Optional<T> initialized with 'value' IFF cond is true, 
   // otherwise creates an uninitialzed Optional<T>.
   // Can throw if T::T(T const&) does
   OptionalBase(bool cond, ArgumentType value)
      : m_initialized(false)
   {
      if (cond) {
         construct(value);
      }
   }
   
   // Creates an Optional<T> initialized with 'std::move(value)' IFF cond is true, 
   // otherwise creates an uninitialzed optional<T>.
   // Can throw if T::T(T &&) does
   OptionalBase(bool cond, RvalReferenceType value)
      : m_initialized(false)
   {
      if (cond) {
         construct(std::move(value));
      }
   }
   
   // Creates a deep copy of another Optional<T>
   // Can throw if T::T(T const&) does
   OptionalBase(OptionalBase const &other)
      : m_initialized(false)
   {
      if (other.isInitialized()) {
         construct(other.getImpl());
      }
   }
   
   // Creates a deep move of another Optional<T>
   // Can throw if T::T(T&&) does
   OptionalBase(OptionalBase &&other)
   noexcept(std::is_nothrow_move_constructible<T>::value)
      : m_initialized(false)
   {
      if (other.isInitialized()) {
         construct(std::move(other.getImpl()));
      }
   }
   
   template<class Expr, class PtrExpr>
   explicit OptionalBase(Expr&& expr, PtrExpr const* tag)
      : m_initialized(false)
   {
      construct(std::forward<Expr>(expr), tag);
   }
   
   OptionalBase &operator= (const OptionalBase &other)
   {
      if (this != &other) {
         this->assign(other);
      }
      return *this;
   }
   
   OptionalBase &operator= (OptionalBase &&other)
   noexcept(std::is_nothrow_move_constructible<T>::value && std::is_nothrow_move_assignable<T>::value)
   {
      PDK_ASSERT(this != &other);
      this->assign(static_cast<OptionalBase &&>(other));
      return *this;
   }
   
   inline ~OptionalBase()
   {
      destroy();
   }
   
   void assign(const OptionalBase &other)
   {
      if (isInitialized()) {
         if (other.isInitialized()) {
            assignValue(other.getImpl());
         } else {
            destroy();
         }
      } else {
         if (other.isInitialized()) {
            construct(other.getImpl());
         }
      }
   }
   
   void assign(OptionalBase &&other)
   {
      if (isInitialized()) {
         if (other.isInitialized()) {
            assignValue(std::move(other.getImpl()));
         } else {
            destroy();
         }
      } else {
         if (other.isInitialized()) {
            construct(std::move(other.getImpl()));
         }
      }
   }
   
   template <typename U>
   void assign(const Optional<U> &other)
   {
      if (isInitialized()) {
         if (other.isInitialized()) {
            assignValue(static_cast<ValueType>(other.get()));
         } else {
            destroy();
         }
      } else {
         if (other.isInitialized()) {
            construct(static_cast<ValueType>(other.get()));
         }
      }
   }
   
   // move-assigns from another _convertible_ Optional<U> (deep-moves from the other value)
   template <typename U>
   void assign(Optional<U> &&other)
   {
      using RefType = Optional<U>::RvalReferenceType;
      if (isInitialized()) {
         if (other.isInitialized()) {
            assignValue( static_cast<RefType>(other.get()) );
         } else {
            destroy();
         }
      } else {
         if (other.isInitialized()) {
            construct(static_cast<RefType>(other.get()));
         }
      }
   }
   
   void assign(ArgumentType value)
   {
      if (isInitialized()) {
         assignValue(value);
      } else {
         construct(value);
      }
   }
   
   void assign(RvalReferenceType value)
   {
      if (isInitialized()) {
         assignValue(std::move(value));
      } else {
         construct(std::move(value));
      }
   }
   
   // Assigns from "none", destroying the current value, if any, leaving this UNINITIALIZED
   // No-throw (assuming T::~T() doesn't)
   void assign(None) noexcept
   {
      destroy();
   }
   
public:
   // **DEPPRECATED** Destroys the current value, if any, leaving this UNINITIALIZED
   // No-throw (assuming T::~T() doesn't)
   
   
   PointerConstType getPtr() const
   {
      return m_initialized ? getPtrImpl() : nullptr;
   }
   
   PointerType getPtr()
   {
      return m_initialized ? getPtrImpl() : nullptr;
   }
   
   bool isInitialized() const
   {
      return m_initialized;
   }
   
protected:
   void construct(ArgumentType value)
   {
      ::new (&m_storage) ValueType(value);
      m_initialized = true;
   }
   
   void construct(RvalReferenceType value)
   {
      ::new (&m_storage) ValueType(std::move(value));
      m_initialized = true;
   }
   
   // Constructs in-place
   // upon exception *this is always uninitialized
   template <typename... Args>
   void construct(InPlaceInit, Args&&... args)
   {
      ::new (&m_storage) ValueType(std::forward<Args>(args)...);
      m_initialized = true;
   }
   
   template <typename... Args>
   void emplaceAssign(Args&&... args)
   {
      destroy();
      construct(InPlaceInit, std::forward<Args>(args)...);
   }
   
   template <typename... Args>
   explicit OptionalBase(InPlaceInit, Args&&... args)
      : m_initialized(false)
   {
      construct(InPlaceInit, std::forward<Args>(args)...);
   }
   
   template <typename... Args>
   explicit OptionalBase(InPlaceInitIf, bool cond, Args&&... args)
      : m_initialized(false)
   {
      if (cond) {
         construct(InPlaceInit, std::forward<Args>(args)...);
      }
   }
   
   // Constructs using any expression implicitly convertible to the single argument
   // of a one-argument T constructor.
   // Converting constructions of Optional<T> from Optional<U> uses this function with
   // 'Expr' being of type 'U' and relying on a converting constructor of T from U.
   template <class Expr>
   void construct(Expr &&expr, void const *)
   {
      new (&m_storage) ValueType(std::forward<Expr>(expr));
      m_initialized = true;
   }
   
   // Assigns using a form any expression implicitly convertible to the single argument
   // of a T's assignment operator.
   // Converting assignments of Optional<T> from Optional<U> uses this function with
   // 'Expr' being of type 'U' and relying on a converting assignment of T from U.
   template <typename Expr>
   void assginExprToInitialized(Expr &&expr, void const *)
   {
      assignValue(std::forward<Expr>(expr));
   }
   
   void assignValue(ArgumentType value)
   {
      getImpl() = value;
   }
   
   void assignValue(RvalReferenceType value)
   {
      getImpl() = static_cast<RvalReferenceType>(value);
   }
   
   void destroy()
   {
      if (m_initialized) {
         reinterpret_cast<T *>(getPtrImpl())->~T();
         m_initialized = false;
      }
   }
   
   ReferenceConstType getImpl() const
   {
      return m_storage;
   }
   
   ReferenceType getImpl()
   {
      return m_storage;
   }
   
   PointerConstType getPtrImpl() const
   {
      return &m_storage;
   }
   
   PointerType getPtrImpl()
   {
      return &m_storage;
   }
private:
   
   bool m_initialized;
   StorageType m_storage;
};

} // internal

#include "pdk/stdext/optional/internal/TriviallyCopyableBase.h"

namespace internal {



} // internal

} // optional
} // stdext
} // pdk

#endif // PDK_STDEXT_OPTIONAL_OPTIONAL_H
