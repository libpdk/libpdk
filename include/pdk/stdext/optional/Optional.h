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
#include "pdk/stdext/optional/BadAccess.h"
#include "pdk/stdext/utility/OptionalPointee.h"
#include <type_traits>
#include <istream>
#include <ostream>

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
   using StorageType = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
   using ThisType = OptionalBase<T>;
   
protected:
   using ValueType = T;
   using value_type = ValueType;
   using ReferenceType = T &;
   using ReferenceConstType = const T &;
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
      using RefType = typename Optional<U>::RvalReferenceType;
      if (isInitialized()) {
         if (other.isInitialized()) {
            assignValue(static_cast<RefType>(other.get()) );
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
   
   template<class Expr, class ExprPtr>
   void assignExpr(Expr&& expr, ExprPtr const* tag)
   {
      if (isInitialized()) {
         assginExprToInitialized(std::forward<Expr>(expr),tag);
      } else {
         construct(std::forward<Expr>(expr), tag);
      }
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
      construct(in_place_init, std::forward<Args>(args)...);
   }
   
   template <typename... Args>
   explicit OptionalBase(InPlaceInit, Args&&... args)
      : m_initialized(false)
   {
      construct(in_place_init, std::forward<Args>(args)...);
   }
   
   template <typename... Args>
   explicit OptionalBase(InPlaceInitIf, bool cond, Args&&... args)
      : m_initialized(false)
   {
      if (cond) {
         construct(in_place_init, std::forward<Args>(args)...);
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
      return *getPtrImpl();
   }
   
   ReferenceType getImpl()
   {
      return const_cast<ReferenceType>(const_cast<const ThisType *>(this)->getImpl());
   }
   
   PointerConstType getPtrImpl() const
   {
      return reinterpret_cast<PointerConstType>(&m_storage);
   }
   
   PointerType getPtrImpl()
   {
      return const_cast<PointerType>(const_cast<const ThisType *>(this)->getPtrImpl());
   }
private:
   
   bool m_initialized;
   StorageType m_storage;
};

#include "pdk/stdext/optional/internal/TriviallyCopyableBase.h"

template <typename U>
struct IsOptionalRelated
      : std::conditional<std::is_base_of<OptionalTag, typename std::decay<U>::type>::value
      || std::is_same<typename std::decay<U>::type, None>::value
      || std::is_same<typename std::decay<U>::type, InPlaceInit>::value
      || std::is_same<typename std::decay<U>::type, InPlaceInitIf>::value,
      std::true_type, std::false_type>::type
{};

template <typename T, typename U>
struct IsConvertibleToT
      : std::conditional<std::is_constructible<T, U&&>::value
      && !std::is_same<T, typename std::decay<U>::type>::value,
      std::true_type, std::false_type>::type
{};

template <typename T, typename U>
struct IsOptionalConstructible : std::is_constructible<T, U>
{};

template <typename T, typename U>
struct IsOptionalValInitCandidate
      : std::conditional<!IsOptionalRelated<U>::value
      && IsConvertibleToT<T, U>::value,
      std::true_type, std::false_type>::type
{};

} // internal

namespace config {

template <typename T>
struct OptionalUsesDirectStorageFor
      : std::conditional<(std::is_scalar<T>::value && !std::is_const<T>::value && !std::is_volatile<T>::value),
std::true_type, std::false_type>::type
{};

} // config

#define PDK_OPTIONAL_BASE_TYPE(T) std::conditional<config::OptionalUsesDirectStorageFor<T>::value,\
   internal::TriviallyCopyableOptionalBase<T>,\
   internal::OptionalBase<T>>::type

template <typename T>
class Optional 
      : public PDK_OPTIONAL_BASE_TYPE(T)
{
   using BaseType = typename PDK_OPTIONAL_BASE_TYPE(T);
   
   public:
   using ThisType = Optional<T>;
   using ValueType = typename BaseType::ValueType;
   using ReferenceType = typename BaseType::ReferenceType;
   using ReferenceConstType = typename BaseType::ReferenceConstType;
   using RvalReferenceType = typename BaseType::RvalReferenceType;
   using ReferenceTypeOfTemporaryWrapper = typename BaseType::ReferenceTypeOfTemporaryWrapper;
   using PointerType = typename BaseType::PointerType;
   using PointerConstType = typename BaseType::PointerConstType;
   using ArgumentType = typename BaseType::ArgumentType;
   
   using this_type = ThisType;
   using value_type = ValueType;
   using reference_type = ReferenceType;
   using reference_const_type = ReferenceConstType;
   using rval_reference_type = RvalReferenceType;
   using reference_type_of_temporary_wrapper = ReferenceTypeOfTemporaryWrapper;
   using pointer_type = PointerType;
   using pointer_const_type = PointerConstType;
   using argument_type = ArgumentType;
   
   // Creates an Optional<T> uninitialized.
   // No-throw
   Optional() noexcept
      : BaseType()
   {}
   
   // Creates an Optional<T> uninitialized.
   // No-throw
   Optional(None none) noexcept
      : BaseType(none)
   {}
   
   // Creates an Optional<T> initialized with 'value'.
   // Can throw if T::T(T const&) does
   Optional(ArgumentType value)
      : BaseType(value)
   {}
   
   // Creates an Optional<T> initialized with 'move(val)'.
   // Can throw if T::T(T &&) does
   Optional(RvalReferenceType value)
      : BaseType(std::forward<T>(value))
   {}
   
   // Creates an Optional<T> initialized with 'value' IFF cond is true, otherwise creates an uninitialized optional.
   // Can throw if T::T(T const&) does
   Optional(bool cond, ArgumentType value)
      : BaseType(cond, value)
   {}
   
   // Creates an Optional<T> initialized with 'value' IFF cond is true, otherwise creates an uninitialized optional.
   // Can throw if T::T(T &&) does
   Optional(bool cond, RvalReferenceType value)
      : BaseType(cond, std::forward<T>(value))
   {}
   
   // Creates a deep copy of another convertible Optional<U>
   // Requires a valid conversion from U to T.
   // Can throw if T::T(U const&) does
   template <typename U>
   explicit Optional(const Optional<U> &other,
                     typename std::enable_if<internal::IsOptionalConstructible<T, U const&>::value>::type * = nullptr)
      : BaseType()
   {
      if (other.isInitialized()) {
         this->construct(other.get());
      }
   }
   
   // Creates a deep move of another convertible Optional<U>
   // Requires a valid conversion from U to T.
   // Can throw if T::T(U&&) does
   template<class U>
   explicit Optional(Optional<U> &&other,
                     typename std::enable_if<internal::IsOptionalConstructible<T, U>::value>::type * = nullptr)
      : BaseType()
   {
      if (other.isInitialized()) {
         this->construct(std::move(other.get()));
      }
   }
   
   // Creates an optional<T> with an expression which can be either
   //  (a) An instance of InPlaceFactory (i.e. in_place(a,b,...,n);
   //  (b) An instance of TypedInPlaceFactory ( i.e. in_place<T>(a,b,...,n);
   //  (c) Any expression implicitly convertible to the single type
   //      of a one-argument T's constructor.
   //  (d*) Weak compilers (BCB) might also resolved Expr as optional<T> and optional<U>
   //       even though explicit overloads are present for these.
   // Depending on the above some T ctor is called.
   // Can throw if the resolved T ctor throws.
   template<class Expr>
   explicit Optional(Expr&& expr, 
                     typename std::enable_if<internal::IsOptionalValInitCandidate<T, Expr>::value>::type * = nullptr)
      : BaseType(std::forward<Expr>(expr), std::addressof(expr)) 
   {}
   
   // Creates a deep copy of another Optional<T>
   // Can throw if T::T(const T &) does
   Optional(const Optional &) = default;
   Optional(Optional &&) = default;
   
   // Copy-assigns from another convertible Optional<U> (converts && deep-copies the rhs value)
   // Requires a valid conversion from U to T.
   // Basic Guarantee: If T::T(const U & ) throws, this is left UNINITIALIZED
   template<class U>
   Optional &operator= (const Optional<U> &other)
   {
      this->assign(other);
      return *this ;
   }
   
   // Move-assigns from another convertible Optional<U> (converts && deep-moves the rhs value)
   // Requires a valid conversion from U to T.
   // Basic Guarantee: If T::T( U && ) throws, this is left UNINITIALIZED
   template <typename U>
   Optional &operator =(Optional<U> &&other)
   {
      this->assign(std::move(other));
      return *this;
   }
   
   Optional &operator= (const Optional &) = default;
   Optional &operator= (Optional &&) = default;
   
   // Assigns from a T (deep-moves/copies the other value)
   template <typename X>
   typename std::enable_if<std::is_same<T, typename std::decay<X>::type>::value, Optional&>::type
         operator =(X &&value)
   {
      this->assign(std::forward<X>(value));
      return *this;
   }
   
   // Assigns from a "none"
   // Which destroys the current value, if any, leaving this UNINITIALIZED
   // No-throw (assuming T::~T() doesn't)
   Optional &operator =(None none) noexcept
   {
      this->assign(none);
      return *this;
   }
   
   template<class Expr>
   typename std::enable_if<internal::IsOptionalValInitCandidate<T, Expr>::value, Optional&>::type 
         operator= (Expr&& expr)
   {
      this->assignExpr(std::forward<Expr>(expr), std::addressof(expr));
      return *this;
   }
   
   // Constructs in-place
   // upon exception *this is always uninitialized
   template <typename... Args>
   void emplace(Args&&... args)
   {
      this->emplaceAssign(std::forward<Args>(args)...);
   }
   
   template <typename... Args>
   explicit Optional(InPlaceInit, Args&&... args )
      : BaseType(in_place_init, std::forward<Args>(args)... )
   {}
   
   template <typename... Args>
   explicit Optional(InPlaceInitIf, bool cond, Args&&... args )
      : BaseType(in_place_init, std::forward<Args>(args)... )
   {}
   
   void swap(Optional &arg) 
         noexcept(std::is_nothrow_move_constructible<T>::value && std::is_nothrow_move_assignable<T>::value)
   {
      std::swap(*this, arg);
   }
   
   // Returns a reference to the value if this is initialized, otherwise,
   // the behaviour is UNDEFINED
   // No-throw
   ReferenceConstType get() const
   {
      PDK_ASSERT(this->isInitialized());
      return this->getImpl();
   }
   
   ReferenceType get()
   {
      PDK_ASSERT(this->isInitialized());
      return this->getImpl();
   }
   
   // Returns a copy of the value if this is initialized, 'value' otherwise
   ReferenceConstType getValueOr(ReferenceConstType value) const
   {
      return this->isInitialized() ? get() : value;
   }
   
   ReferenceType getValueOr(ReferenceType value)
   {
      return this->isInitialized() ? get() : value;
   }
   
   // Returns a pointer to the value if this is initialized, otherwise,
   // the behaviour is UNDEFINED
   // No-throw
   PointerConstType operator ->() const
   {
      PDK_ASSERT(this->isInitialized());
      return this->getPtrImpl();
   }
   
   PointerType operator ->()
   {
      PDK_ASSERT(this->isInitialized());
      return this->getPtrImpl();
   }
   
   // Returns a reference to the value if this is initialized, otherwise,
   // the behaviour is UNDEFINED
   // No-throw
   ReferenceConstType operator *() const &
   {
      return get();
   }
   
   ReferenceType operator *() &
   {
      return get();
   }
   
   ReferenceTypeOfTemporaryWrapper operator *() &&
   {
      return std::move(get());
   }
   
   ReferenceConstType value() const &
   {
      if (this->isInitialized()) {
         return get();
      }
      throw BadAccess();
   }
   
   ReferenceType value() &
   {
      if (this->isInitialized()) {
         return get();
      }
      throw BadAccess();
   }
   
   ReferenceTypeOfTemporaryWrapper value() &&
   {
      if (this->isInitialized()) {
         return std::move(get());
      }
      throw BadAccess();
   }
   
   template <typename U>
   ValueType valueOr(U &&value) const &
   {
      if (this->isInitialized()) {
         return get();
      }
      return std::forward<U>(value);
   }
   
   template <class U>
   ValueType valueOr(U &&value) &&
   {
      if (this->isInitialized()) {
         return std::move(get());
      }
      return std::forward<U>(value);
   }
   
   ValueType valueOrEval(std::function<ValueType()> f) const &
   {
      if (this->isInitialized()) {
         return get();
      } else {
         return f();
      }
   }
   
   ValueType valueOrEval(std::function<ValueType()> f) &&
   {
      if (this->isInitialized()) {
         return std::move(get());
      } else {
         return f();
      }
   }
   
   bool operator!() const noexcept
   {
      return !this->isInitialized();
   }
   
   explicit operator bool () const noexcept
   {
      return !this->operator!();
   }
};

template <class T>
inline void swap(Optional<T> &lhs, Optional<T> &rhs)
noexcept(::std::is_nothrow_move_constructible<T>::value && noexcept(std::swap(*lhs, *rhs)))
{
   if (lhs) {
      if (rhs) {
         std::swap(*lhs, *rhs);
      } else {
         rhs = std::move(*lhs);
         lhs = none;
      }
   } else {
      if (rhs) {
         lhs = std::move(*rhs);
         rhs = none;
      }
   }
}

template<class T>
class Optional<T &&>
{
   PDK_STATIC_ASSERT_X(sizeof(T) == 0, "Optional rvalue references are illegal.");
};

} // optional

// Returns Optional<T>(value)
template <typename T>
inline optional::Optional<typename std::decay<T>::type> make_optional(T &&value)
{
   return optional::Optional<typename std::decay<T>::type>(std::forward<T>(value));
}

// Returns Optional<T>(cond, value)
template <typename T>
inline optional::Optional<typename std::decay<T>::type> make_optional(bool cond, T &&value)
{
   return optional::Optional<typename std::decay<T>::type>(cond, std::forward<T>(value));
}

} // stdext
} // pdk

#include "pdk/stdext/optional/internal/ReferenceSpec.h"
#include "pdk/stdext/optional/internal/RelationOps.h"

#endif // PDK_STDEXT_OPTIONAL_OPTIONAL_H
