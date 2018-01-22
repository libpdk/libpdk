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
//  typedef boost::iterator_facade<slot_call_iterator_t<Function, Iterator, ConnectionBody>,
//    typename Function::result_type,
//    boost::single_pass_traversal_tag,
//    typename boost::add_reference<typename boost::add_const<typename Function::result_type>::type>::type >
//  inherited;

//  typedef typename Function::result_type result_type;

//  typedef slot_call_iterator_cache<result_type, Function> cache_type;

//  friend class boost::iterator_core_access;

//public:
//  slot_call_iterator_t(Iterator iter_in, Iterator end_in,
//    cache_type &c):
//    iter(iter_in), end(end_in),
//    cache(&c), callable_iter(end_in)
//  {
//    lock_next_callable();
//  }

//  typename inherited::reference
//  dereference() const
//  {
//    if (!cache->result) {
//      BOOST_TRY
//      {
//        cache->result.reset(cache->f(*iter));
//      }
//      BOOST_CATCH(expired_slot &)
//      {
//        (*iter)->disconnect();
//        BOOST_RETHROW
//      }
//      BOOST_CATCH_END
//    }
//    return cache->result.get();
//  }

//  void increment()
//  {
//    ++iter;
//    lock_next_callable();
//    cache->result.reset();
//  }

//  bool equal(const slot_call_iterator_t& other) const
//  {
//    return iter == other.iter;
//  }

//private:
//  typedef garbage_collecting_lock<connection_body_base> lock_type;

//  void set_callable_iter(lock_type &lock, Iterator newValue) const
//  {
//    callable_iter = newValue;
//    if(callable_iter == end)
//      cache->set_active_slot(lock, 0);
//    else
//      cache->set_active_slot(lock, (*callable_iter).get());
//  }

//  void lock_next_callable() const
//  {
//    if(iter == callable_iter)
//    {
//      return;
//    }
//    if(iter == end)
//    {
//      if(callable_iter != end)
//      {
//        lock_type lock(**callable_iter);
//        set_callable_iter(lock, end);
//        return;
//      }
//    }
//    // we're only locking the first connection body,
//    // but it doesn't matter they all use the same mutex
//    lock_type lock(**iter);
//    for(;iter != end; ++iter)
//    {
//      cache->tracked_ptrs.clear();
//      (*iter)->nolock_grab_tracked_objects(lock, std::back_inserter(cache->tracked_ptrs));
//      if((*iter)->nolock_nograb_connected())
//      {
//        ++cache->connected_slot_count;
//      }else
//      {
//        ++cache->disconnected_slot_count;
//      }
//      if((*iter)->nolock_nograb_blocked() == false)
//      {
//        set_callable_iter(lock, iter);
//        break;
//      }
//    }
//    if(iter == end)
//    {
//      set_callable_iter(lock, end);
//    }
//  }

//  mutable Iterator iter;
//  Iterator end;
//  cache_type *cache;
//  mutable Iterator callable_iter;
};

} // internal
} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_INTERNAL_SLOT_CALL_ITERATOR_H
