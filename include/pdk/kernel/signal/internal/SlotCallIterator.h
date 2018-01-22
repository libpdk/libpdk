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
// Created by softboy on 2018/01/21.

// Boost.Signals2 library

// Copyright Douglas Gregor 2001-2004.
// Copyright Frank Mori Hess 2007-2008.
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef PDK_KERNEL_SIGNAL_INTERNAL_SLOT_CALL_ITERATOR_H
#define PDK_KERNEL_SIGNAL_INTERNAL_SLOT_CALL_ITERATOR_H

#include <optional>
#include "pdk/kernel/signal/Connection.h"
#include "pdk/kernel/signal/SignalBase.h"
#include "pdk/kernel/signal/internal/AutoBuffer.h"

namespace pdk {
namespace kernel {
namespace signal {
namespace internal {

template<typename ResultType, typename Function>
class SlotCallIteratorCache
{
public:
   SlotCallIteratorCache(const Function &func):
      m_func(func),
      m_connectedSlotCount(0),
      m_disconnectedSlotCount(0),
      m_activeSlot(nullptr)
   {}
   
   ~SlotCallIteratorCache()
   {
      if(m_activeSlot)
      {
         GarbageCollectingLock<ConnectionBodyBase> lock(*m_activeSlot);
         m_activeSlot->decSlotRefcount(lock);
      }
   }
   
   template<typename M>
   void setActiveSlot(GarbageCollectingLock<M> &lock, ConnectionBodyBase *activeSlot)
   {
      if(m_activeSlot) {
         m_activeSlot->decSlotRefcount(lock);
      }
      m_activeSlot = activeSlot;
      if(m_activeSlot) {
         m_activeSlot->incSlotRefcount(lock);
      }
   }
   
   std::optional<ResultType> m_result;
   using TrackedPtrsType = AutoBuffer<VoidSharedPtrVariant, StoreNObjects<10>>;
   TrackedPtrsType m_trackedPtrs;
   Function m_func;
   unsigned m_connectedSlotCount;
   unsigned m_disconnectedSlotCount;
   ConnectionBodyBase *m_activeSlot;
};


// Generates a slot call iterator. Essentially, this is an iterator that:
//   - skips over disconnected slots in the underlying list
//   - calls the connected slots when dereferenced
//   - caches the result of calling the slots
template<typename Function, typename Iterator, typename ConnectionBody>
class SlotCallIterator
{
public:
   using ResultType = typename Function::ResultType;
   
   using CacheType = SlotCallIteratorCache<ResultType, Function>;
   using ValueType = ResultType;
   using Reference = typename std::add_lvalue_reference<typename std::add_const<ResultType>::type>::type;
   using Difference = std::ptrdiff_t;
   using IteratorCategory = std::forward_iterator_tag;
   
   using result_type = ResultType;
   using value_type = ValueType;
   using reference = Reference;
   using difference = Difference;
   using iterator_category = IteratorCategory;
   
public:
   SlotCallIterator(Iterator begin, Iterator end, CacheType &cacheType)
      : m_iter(begin),
        m_end(end),
        m_cache(&cacheType),
        m_callableIter(end)
   {
      lockNextCallable();
   }
   
   reference operator *() const
   {
      if (!m_cache->m_result) {
         try {
            m_cache->m_result.reset();
            m_cache->m_result = m_cache->m_func(*m_iter);
         } catch(ExpiredSlot &) {
            (*m_iter)->disconnect();
            throw;
         }
      }
      return m_cache->m_result.value();
   }
   
   const SlotCallIterator &operator ++() const
   {
      ++m_iter;
      lockNextCallable();
      m_cache->m_result.reset();
      return *this;
   }
   
   bool operator ==(const SlotCallIterator &other) const
   {
      return m_iter == other.m_iter;
   }
   
   bool operator !=(const SlotCallIterator &other) const
   {
      return m_iter != other.m_iter;
   }
   
private:
   using LockType = GarbageCollectingLock<ConnectionBodyBase>;
   
   void setCallableIter(LockType &lock, Iterator newValue) const
   {
      m_callableIter = newValue;
      if(m_callableIter == m_end)
         m_cache->setActiveSlot(lock, 0);
      else
         m_cache->setActiveSlot(lock, (*m_callableIter).get());
   }
   
   void lockNextCallable() const
   {
      if(m_iter == m_callableIter) {
         return;
      }
      if(m_iter == m_end) {
         if(m_callableIter != m_end) {
            LockType lock(**m_callableIter);
            setCallableIter(lock, m_end);
            return;
         }
      }
      // we're only locking the first connection body,
      // but it doesn't matter they all use the same mutex
      LockType lock(**m_iter);
      for(;m_iter != m_end; ++m_iter) {
         m_cache->m_trackedPtrs.clear();
         (*m_iter)->nolockGrabTrackedObjects(lock, std::back_inserter(m_cache->m_trackedPtrs));
         if((*m_iter)->nolockNograbConnected()) {
            ++m_cache->m_connectedSlotCount;
         } else {
            ++m_cache->m_disconnectedSlotCount;
         }
         if((*m_iter)->nolockNograbBlocked() == false) {
            setCallableIter(lock, m_iter);
            break;
         }
      }
      if(m_iter == m_end) {
         setCallableIter(lock, m_end);
      }
   }
   
   mutable Iterator m_iter;
   Iterator m_end;
   CacheType *m_cache;
   mutable Iterator m_callableIter;
};

} // internal
} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_INTERNAL_SLOT_CALL_ITERATOR_H
