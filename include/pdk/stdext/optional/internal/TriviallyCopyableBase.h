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

template <typename T>
class TriviallyCopyableOptionalBase : public OptionalTag
{
private:
   using ThisType = TriviallyCopyableOptionalBase<T>;
   
protected:
   using ValueType = T;
   using ReferenceType = T &;
   using ReferenceConstType = const T &;
   using RvalReferenceType = T &&;
   using ReferenceTypeOfTemporaryWrapper = T &&;
   using PointerType = T *;
   using PointerConstType = const T *;
   using ArgumentType = const T &;
   using value_type = ValueType;
   using reference_type = ReferenceType;
   using reference_const_type = ReferenceConstType;
   using rval_reference_type = RvalReferenceType;
   using reference_type_of_temporary_wrapper = ReferenceTypeOfTemporaryWrapper;
   using pointer_type = PointerType;
   using pointer_const_type = PointerConstType;
   using argument_type = ArgumentType;
   
protected:
   TriviallyCopyableOptionalBase()
      : m_initialized(false)
   {}
   
   TriviallyCopyableOptionalBase(None)
      : m_initialized(false)
   {}
   
   TriviallyCopyableOptionalBase(ArgumentType value)
      : m_initialized(true),
        m_storage(value)
   {}
   
   TriviallyCopyableOptionalBase(bool cond, ArgumentType value)
      : m_initialized(cond),
        m_storage(value)
   {}
   
   template <typename Expr, typename PtrExpr>
   explicit TriviallyCopyableOptionalBase(Expr &&expr, const PtrExpr *tag)
      : m_initialized(false)
   {
      construct(std::forward<Expr>(expr), tag);
   }
   
   // Assigns from another Optional<T> (deep-copies the other value)
   void assign(const TriviallyCopyableOptionalBase &other)
   {
      this->operator= (other);
   }
   
   template <typename U>
   void assign(Optional<U> &other)
   {
      if (other.isInitialized()) {
         m_storage = static_cast<ValueType>(other.get());
      }
      m_initialized = other.isInitialized();
   }
   
   void assign(ArgumentType value)
   {
      construct(value);
   }
   
   void assign(None)
   {
      destroy();
   }
   
public:
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
      m_storage = value;
      m_initialized = true;
   }
   
   template <typename... Args>
   void construct(InPlaceInit, Args&&... args)
   {
      m_storage = ValueType(std::forward<Args>(args)...);
      m_initialized = true;
   }
   
   template <typename... Args>
   void emplaceAssign(Args&&... args)
   {
      construct(in_place_init, std::forward<Args>(args)...);
   }
   
   template <typename... Args>
   explicit TriviallyCopyableOptionalBase(InPlaceInit, Args&&... args)
      : m_initialized(false)
   {
      construct(in_place_init, std::forward<Args>(args)...);
   }
   
   template <typename... Args>
   explicit TriviallyCopyableOptionalBase(InPlaceInitIf, bool cond, Args&&... args)
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
   template <typename Expr>
   void construct(Expr&& expr, void const*)
   {
      m_storage = ValueType(std::forward<Expr>(expr));
      m_initialized = true;
   }
   
   // Assigns using a form any expression implicitly convertible to the single argument
   // of a T's assignment operator.
   // Converting assignments of Optional<T> from Optional<U> uses this function with
   // 'Expr' being of type 'U' and relying on a converting assignment of T from U.
   template <typename Expr>
   void assignExprToInitialized(Expr&& expr, void const*)
   {
      assignValue(std::forward<Expr>(expr));
   }
   
   void assignValue(ArgumentType value)
   {
      m_storage = value;
   }
   
   void assignValue(RvalReferenceType value)
   {
      m_storage = static_cast<RvalReferenceType>(value);
   }
   
   void destroy()
   {
      m_initialized = false;
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
      return std::addressof(m_storage);
   }
   
   PointerType getPtrImpl()
   {
      return std::addressof(m_storage);
   }
   
private:
   bool m_initialized;
   T m_storage;
};
