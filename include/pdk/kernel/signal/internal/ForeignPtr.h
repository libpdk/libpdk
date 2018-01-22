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
// Created by softboy on 2018/01/22.

//  helper code for dealing with tracking non-boost shared_ptr/weak_ptr

// Copyright Frank Mori Hess 2009.
// Distributed under the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/signals2 for library home page.

#ifndef PDK_KERNEL_SIGNAL_INTERNAL_FOREIGN_PTR_H
#define PDK_KERNEL_SIGNAL_INTERNAL_FOREIGN_PTR_H

#include <algorithm>
#include <memory>

namespace pdk {
namespace kernel {
namespace signal {

template <typename WeakPtr>
struct WeakPtrTraits
{};

template <typename T>
struct WeakPtrTraits<std::weak_ptr<T>>
{
   using SharedType = std::shared_ptr<T>;
};

template <typename SharedPtr>
struct SharedPtrTraits
{};

template <typename T>
struct SharedPtrTraits<std::shared_ptr<T>>
{
   using WeakType = std::weak_ptr<T>;
};

namespace internal {

struct ForeignSharedPtrImplBase
{
   virtual ~ForeignSharedPtrImplBase()
   {}
   
   virtual ForeignSharedPtrImplBase *clone() const = 0;
};

template <typename FSP>
class ForeignSharedPtrImpl : public ForeignSharedPtrImplBase
{
public:
   ForeignSharedPtrImpl(const FSP &ptr)
      : m_ptr(ptr)
   {}
   
   virtual ForeignSharedPtrImpl * clone() const
   {
      return new ForeignSharedPtrImpl(*this);
   }
   
private:
   FSP m_ptr;
};

class ForeignVoidSharedPtr
{
public:
   ForeignVoidSharedPtr()
      : m_ptr(nullptr)
   {}
   
   ForeignVoidSharedPtr(const ForeignVoidSharedPtr &other)
      : m_ptr(other.m_ptr->clone())
   {}
   
   template<typename FSP>
   explicit ForeignVoidSharedPtr(const FSP &fsp):
      m_ptr(new ForeignSharedPtrImpl<FSP>(fsp))
   {}
   
   ~ForeignVoidSharedPtr()
   {
      delete m_ptr;
   }
   
   ForeignVoidSharedPtr &operator=(const ForeignVoidSharedPtr &other)
   {
      if(&other == this) {
         return *this;
      }
      ForeignVoidSharedPtr(other).swap(*this);
      return *this;
   }
   
   void swap(ForeignVoidSharedPtr &other)
   {
      std::swap(m_ptr, other.m_ptr);
   }
   
private:
   ForeignSharedPtrImplBase * m_ptr;
};

struct ForeignWeakPtrImplBase
{
   virtual ~ForeignWeakPtrImplBase() {}
   virtual ForeignVoidSharedPtr lock() const = 0;
   virtual bool expired() const = 0;
   virtual ForeignWeakPtrImplBase * clone() const = 0;
};

template<typename FWP>
class ForeignWeakPtrImpl: public ForeignWeakPtrImplBase
{
public:
   ForeignWeakPtrImpl(const FWP &p)
      : m_ptr(p)
   {}
   
   virtual ForeignVoidSharedPtr lock() const
   {
      return ForeignVoidSharedPtr(m_ptr.lock());
   }
   
   virtual bool expired() const
   {
      return m_ptr.expired();
   }
   
   virtual ForeignWeakPtrImpl *clone() const
   {
      return new ForeignWeakPtrImpl(*this);
   }
private:
   FWP m_ptr;
};

class ForeignVoidWeakPtr
{
public:
   ForeignVoidWeakPtr()
   {}
   
   ForeignVoidWeakPtr(const ForeignVoidWeakPtr &other):
      m_ptr(other.m_ptr->clone())
   {}
   template<typename FWP>
   explicit ForeignVoidWeakPtr(const FWP &fwp):
      m_ptr(new ForeignWeakPtrImpl<FWP>(fwp))
   {}
   
   ForeignVoidWeakPtr & operator=(const ForeignVoidWeakPtr &other)
   {
      if(&other == this) {
         return *this;
      }
      ForeignVoidWeakPtr(other).swap(*this);
      return *this;
   }
   
   void swap(ForeignVoidWeakPtr &other)
   {
      std::swap(m_ptr, other.m_ptr);
   }
   
   ForeignVoidSharedPtr lock() const
   {
      return m_ptr->lock();
   }
   
   bool expired() const
   {
      return m_ptr->expired();
   }
private:
   std::unique_ptr<ForeignWeakPtrImplBase> m_ptr;
};

} // internal
} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_INTERNAL_FOREIGN_PTR_H
