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
// Created by softboy on 2018/01/20.

//  deconstruct.hpp
//
// A factory function for creating a shared_ptr which creates
// an object and its owning shared_ptr with one allocation, similar
// to make_shared<T>().  It also supports postconstructors
// and predestructors through unqualified calls of adl_postconstruct() and
// adl_predestruct, relying on argument-dependent
// lookup to find the appropriate postconstructor or predestructor.
// Passing arguments to postconstructors is also supported.
//
//  based on make_shared.hpp and make_shared_access patch from Michael Marcin
//
//  Copyright (c) 2007, 2008 Peter Dimov
//  Copyright (c) 2008 Michael Marcin
//  Copyright (c) 2009 Frank Mori Hess
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  See http://www.boost.org
//  for more information

#ifndef PDK_KERNEL_SIGNAL_DECONSTRUCT_H
#define PDK_KERNEL_SIGNAL_DECONSTRUCT_H

#include <cstddef>
#include <new>
#include <memory>

namespace pdk {
namespace kernel {
namespace signal {

class DeconstructAccess;

namespace internal {

inline void adl_predestruct(...)
{}

} // internal

template<typename T>
class PostConstructorInvoker
{
public:
   operator const std::shared_ptr<T> &() const
   {
      return postConstruct();
   }
   
   const std::shared_ptr<T>& postConstruct() const
   {
      if(!m_postConstructed)
      {
         adl_postconstruct(m_sp, const_cast<typename std::remove_const<T>::type *>(m_sp.get()));
         m_postConstructed = true;
      }
      return m_sp;
   }
   
   template<typename... Args>
   const shared_ptr<T>& postConstruct(Args && ... args) const
   {
      if(!m_postConstructed)
      {
         adl_postconstruct(m_sp, const_cast<typename std::remove_const<T>::type *>(m_sp.get()),
                           std::forward<Args>(args)...);
         m_postConstructed = true;
      }
      return m_sp;
   }
   
private:
   friend class DeconstructAccess;
   PostConstructorInvoker(const std::shared_ptr<T> &sp)
      : m_sp(sp), 
        m_postConstructed(false)
   {}
   std::shared_ptr<T> m_sp;
   mutable bool m_postConstructed;
};

namespace internal {

template<typename T> 
class DeconstructDeleter
{
private:
   using StorageType = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
   bool m_initialized;
   StorageType m_storage;
private:
   void destroy()
   {
      if(m_initialized)
      {
         T* p = reinterpret_cast<T *>(&m_storage);
         using pdk::kernel::signal::internal::adl_predestruct;
         adl_predestruct(const_cast<typename std::remove_const<T>::type *>(p));
         p->~T();
         m_initialized = false;
      }
   }
   
public:
   
   DeconstructDeleter()
      : m_initialized(false)
   {}
   
   // this copy constructor is an optimization: we don't need to copy the storage_ member,
   // and shouldn't be copying anyways after initialized_ becomes true
   DeconstructDeleter(const DeconstructDeleter &)
      : m_initialized(false)
   {}
   
   ~DeconstructDeleter()
   {
      destroy();
   }
   
   void operator()( T * )
   {
      destroy();
   }
   
   void *getAddress()
   {
      return &m_storage;
   }
   
   void setInitialized()
   {
      m_initialized = true;
   }
};

} // internal

class DeconstructAccess
{
public:
   template<typename T>
   static PostConstructorInvoker<T> deconstruct()
   {
      std::shared_ptr<T> pt(static_cast<T *>(nullptr), internal::DeconstructDeleter<T>());
      internal::DeconstructDeleter<T> *pd = std::get_deleter<internal::DeconstructDeleter<T>>(pt);
      void * pv = pd->getAddress();
      new( pv ) T();
      pd->setInitialized();
      std::shared_ptr<T> retval(pt, static_cast< T* >(pv));
      boost::detail::sp_enable_shared_from_this(&retval, retval.get(), retval.get());
      return retval;
   }
   
   // Variadic templates, rvalue reference
   
   template<typename T, typename ... Args>
   static PostConstructorInvoker<T> deconstruct(Args && ... args)
   {
      std::shared_ptr<T> pt(static_cast<T*>(nullptr), detail::deconstruct_deleter<T>());
      detail::deconstruct_deleter<T> *pd = std::get_deleter<detail::deconstruct_deleter<T>>(pt);
      void *pv = pd->address();
      new(pv) T(std::forward<Args>(args)...);
      pd->set_initialized();
      std::shared_ptr<T> retval( pt, static_cast<T *>(pv));
      boost::detail::sp_enable_shared_from_this(&retval, retval.get(), retval.get());
      return retval;
   }
};

// Zero-argument versions
//
// Used even when variadic templates are available because of the new T() vs new T issue

template<typename T >
PostConstructorInvoker<T> deconstruct()
{
   return DeconstructAccess::deconstruct<T>();
}

// Variadic templates, rvalue reference
template<typename T, typename... Args > 
PostConstructorInvoker<T> deconstruct(Args && ... args)
{
   return DeconstructAccess::deconstruct<T>(std::forward<Args>(args)...);
}

} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_DECONSTRUCT_H
