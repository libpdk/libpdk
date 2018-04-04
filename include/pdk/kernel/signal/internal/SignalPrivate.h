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
// Created by softboy on 2018/01/19.

/*
  Author: Frank Mori Hess <fmhess@users.sourceforge.net>
  Begin: 2007-01-23
*/
// Copyright Frank Mori Hess 2007-2008
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef PDK_KERNEL_SIGNAL_INTERNAL_SIGNAL_PRIVATE_H
#define PDK_KERNEL_SIGNAL_INTERNAL_SIGNAL_PRIVATE_H

#include "pdk/kernel/signal/internal/ResultTypeWrapper.h"
#include "pdk/kernel/signal/internal/VariadicSlotInvoker.h"
#include "pdk/kernel/signal/internal/SlotCallIterator.h"
#include "pdk/kernel/signal/internal/SlotGroup.h"
#include "pdk/kernel/signal/internal/ReplaceSlotFunction.h"
#include "pdk/kernel/signal/internal/SignalCommon.h"
#include "pdk/kernel/signal/internal/SlotCallIterator.h"
#include "pdk/kernel/signal/internal/VariadicArgType.h"
#include "pdk/kernel/signal/internal/ResultTypeWrapper.h"
#include "pdk/kernel/signal/OptionalLastValue.h"
#include "pdk/kernel/signal/Connection.h"
#include "pdk/stdext/typetraits/FunctionTraits.h"
#include <algorithm>
#include <functional>
#include <mutex>

namespace pdk {
namespace kernel {

class Object;

namespace signal {

using pdk::kernel::Object;

class Connection;

namespace internal {

using pdk::kernel::signal::ConnectPosition;

template<typename Signature> 
class VariadicExtendedSignature;

// partial template specialization
template<typename R, typename ... Args>
class VariadicExtendedSignature<R (Args...)>
{
public:
   using FunctionType = std::function<R (const Connection &, Args...)>;
   using function_type = FunctionType;
};

template <typename R>
class BoundExtendedSlotFunctionInvoker
{
public:
   using ResultType = R;
   using result_type = ResultType;
   template <typename ExtendedSlotFunction, typename ... Args>
   ResultType operator()(ExtendedSlotFunction &func, const Connection &conn,
                         Args&& ... args) const
   {
      return func(conn, std::forward<Args>(args)...);
   }
};

template <>
class BoundExtendedSlotFunctionInvoker<void>
{
public:
   using result_type = ResultTypeWrapper<void>::type;
   using ResultType = result_type;
   template <typename ExtendedSlotFunction, typename ... Args>
   ResultType operator()(ExtendedSlotFunction &func, const Connection &conn,
                         Args&& ... args) const
   {
      func(conn, std::forward<Args>(args)...);
      return ResultType();
   }
};

// wrapper around an signalN::extended_slot_function which binds the
// connection argument so it looks like a normal
// signalN::slot_function

template <typename ExtendedSlotFunction>
class BoundExtendedSlotFunction
{
public:
   using ResultType = typename ResultTypeWrapper<typename ExtendedSlotFunction::result_type>::type;
   using result_type = ResultType;
   BoundExtendedSlotFunction(const ExtendedSlotFunction &func)
      : m_func(func),
        m_connection(new Connection)
   {}
   
   void setConnection(const Connection &conn)
   {
      *m_connection = conn;
   }
   
   template <typename ... Args>
   ResultType operator()(Args && ... args)
   {
      return BoundExtendedSlotFunctionInvoker<typename ExtendedSlotFunction::result_type>()
            (m_func, *m_connection, std::forward<Args>(args)...);
   }
   
   template <typename ... Args>
   ResultType operator()(Args && ... args) const
   {
      return BoundExtendedSlotFunctionInvoker<typename ExtendedSlotFunction::result_type>()
            (m_func, *m_connection, std::forward<Args>(args)...);
   }
   
private:
   BoundExtendedSlotFunction()
   {}
   
   ExtendedSlotFunction m_func;
   std::shared_ptr<Connection> m_connection;
};

template <typename Signature,
          typename Combiner,
          typename Group,
          typename GroupCompare,
          typename SlotFunction,
          typename ExtendedSlotFunction,
          typename Mutex>
class SignalImpl;

template <typename Combiner,
          typename Group,
          typename GroupCompare,
          typename SlotFunction,
          typename ExtendedSlotFunction,
          typename Mutex,
          typename R,
          typename ... Args>
class SignalImpl <R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex>
{
public:
   using SlotFunctionType = SlotFunction;
   using SlotType = Slot<R (Args...), SlotFunctionType>;
   using ExtendedSlotFunctionType = ExtendedSlotFunction;
   using ExtendedSlotType = Slot<R (const Connection &, Args...), ExtendedSlotFunctionType>;
   using NonVoidSlotResultType = typename NonVoid<typename SlotFunctionType::result_type>::type;
   
private:
   using SlotInvoker = VariadicSlotInvoker<NonVoidSlotResultType, Args...>;
   using SlotCallIteratorCacheType = SlotCallIteratorCache<NonVoidSlotResultType, SlotInvoker>;
   using GroupKeyType = typename GroupKey<Group>::type;
   using ConnectionBodyType = std::shared_ptr<ConnectionBody<GroupKeyType, SlotType, Mutex>>;
   using ConnectionListType = GroupedList<Group, GroupCompare, ConnectionBodyType>;
   using BoundExtendedSlotFunctionType = BoundExtendedSlotFunction<ExtendedSlotFunctionType>;
   
public:
   using CombinerType = Combiner;
   using ResultType = typename ResultTypeWrapper<typename CombinerType::ResultType>::type;
   using GroupType = Group;
   using GroupCompareType = GroupCompare;
   using SlotCallIterator = typename internal::SlotCallIterator<SlotInvoker,
   typename ConnectionListType::iterator, ConnectionBody<GroupKeyType, SlotType, Mutex>>;
   
   SignalImpl(const CombinerType &combinerArg,
              const GroupCompareType &groupCompare)
      : m_sharedState(new InvocationState(ConnectionListType(groupCompare), combinerArg)),
        m_garbageCollectorIter(m_sharedState->connectionBodies().end()),
        m_mutex(new MutexType())
   {}
   
   // connect slot
   Connection connect(const SlotType &slot, 
                      ConnectPosition position = ConnectPosition::AtBack)
   {
      GarbageCollectingLock<MutexType> lock(*m_mutex);
      return nolockConnect(lock, slot, position);
   }
   
   Connection connect(const GroupType &group, const SlotType &slot, 
                      ConnectPosition position = ConnectPosition::AtBack)
   {
      GarbageCollectingLock<MutexType> lock(*m_mutex);
      return nolockConnect(lock, group, slot, position);
   }
   
   Connection connectExtended(const ExtendedSlotType &extSlot,
                              ConnectPosition position = ConnectPosition::AtBack)
   {
      GarbageCollectingLock<MutexType> lock(*m_mutex);
      BoundExtendedSlotFunctionType boundSlot(extSlot.slotFunc());
      SlotType slot = replace_slot_function<SlotType>(extSlot, boundSlot);
      Connection conn = nolockConnect(lock, slot, position);
      boundSlot.setConnection(conn);
      return conn;
   }
   
   Connection connectExtended(const GroupType &group, const ExtendedSlotType &extSlot, 
                              ConnectPosition position = ConnectPosition::AtBack)
   {
      GarbageCollectingLock<Mutex> lock(*m_mutex);
      BoundExtendedSlotFunctionType boundSlot(extSlot.slotFunc());
      SlotType slot = replace_slot_function<SlotType>(extSlot, boundSlot);
      Connection conn = nolockConnect(lock, group, slot, position);
      boundSlot.setConnection(conn);
      return conn;
   }
   
   // disconnect slot(s)
   void disconnectAllSlots()
   {
      std::shared_ptr<InvocationState> localState = getReadableState();
      typename ConnectionListType::iterator iter;
      for(iter = localState->connectionBodies().begin();
          iter != localState->connectionBodies().end(); ++iter) {
         (*iter)->disconnect();
      }
   }
   
   void disconnect(const GroupType &group)
   {
      std::shared_ptr<InvocationState> localState = getReadableState();
      GroupKeyType groupKey(SlotMetaGroup::GroupedSlots, group);
      typename ConnectionListType::iterator iter;
      typename ConnectionListType::iterator endIter =
            localState->connectionBodies().upperBound(groupKey);
      for(iter = localState->connectionBodies().lowerBound(groupKey);
          iter != endIter; ++iter)
      {
         (*iter)->disconnect();
      }
   }
   
   void disconnect(const Connection &connection)
   {
      connection.disconnect();
   }
   
   // emit signal
   ResultType operator ()(Args&& ...args)
   {
      std::shared_ptr<InvocationState> localState;
      {
         GarbageCollectingLock<MutexType> lock(*m_mutex);
         // only clean up if it is safe to do so
         if(m_sharedState.use_count() == 1) {
            nolockCleanupConnections(lock, false, 1);
         }
         // Make a local copy of m_sharedState while holding mutex, so we are
         //  thread safe against the combiner or connection list getting modified
         // during invocation. 
         localState = m_sharedState;
      }
      SlotInvoker invoker = SlotInvoker(std::forward<Args>(args)...);
      SlotCallIteratorCacheType cache(invoker);
      InvocationJanitor janitor(cache, *this, &localState->connectionBodies());
      return CombinerInvoker<typename CombinerType::ResultType>()
            (
               localState->getCombiner(),
               SlotCallIterator(localState->connectionBodies().begin(), localState->connectionBodies().end(), cache),
               SlotCallIterator(localState->connectionBodies().end(), localState->connectionBodies().end(), cache)
               );
   }
   
   ResultType operator ()(Args&& ...args) const
   {
      std::shared_ptr<InvocationState> localState;
      {
         GarbageCollectingLock<MutexType> lock(*m_mutex);
         // only clean up if it is safe to do so
         if(m_sharedState.use_count() == 1) {
            nolockCleanupConnections(lock, false, 1);
         }
         // Make a local copy of m_sharedState while holding mutex, so we are
         // thread safe against the combiner or connection list getting modified
         // during invocation.
         localState = m_sharedState;
      }
      SlotInvoker invoker = SlotInvoker(std::forward<Args>(args)...);
      SlotCallIteratorCacheType cache(invoker);
      InvocationJanitor janitor(cache, *this, &localState->connectionBodies());
      return internal::CombinerInvoker<typename CombinerType::ResultType>()(
               localState->getCombiner(),
               SlotCallIterator(localState->connectionBodies().begin(), localState->connectionBodies().end(), cache),
               SlotCallIterator(localState->connectionBodies().end(), localState->connectionBodies().end(), cache)
               );
   }
   
   std::size_t getNumSlots() const
   {
      std::shared_ptr<InvocationState> localState = getReadableState();
      typename ConnectionListType::iterator iter;
      std::size_t count = 0;
      for(iter = localState->connectionBodies().begin();
          iter != localState->connectionBodies().end(); ++iter) {
         if((*iter)->connected()) {
            ++count;
         }
      }
      return count;
   }
   
   bool empty() const
   {
      std::shared_ptr<InvocationState> localState = getReadableState();
      typename ConnectionListType::iterator iter;
      for(iter = localState->connectionBodies().begin();
          iter != localState->connectionBodies().end(); ++iter)
      {
         if((*iter)->connected()) {
            return false;
         }
      }
      return true;
   }
   
   CombinerType getCombiner() const
   {
      std::unique_lock<MutexType> lock(*m_mutex);
      return m_sharedState->getCombiner();
   }
   
   void setCombiner(const CombinerType &combinerArg)
   {
      std::unique_lock<MutexType> lock(*m_mutex);
      if(m_sharedState.unique()) {
         m_sharedState->getCombiner() = combinerArg;
      } else {
         m_sharedState.reset(new InvocationState(*m_sharedState, combinerArg));
      }
   }
   
private:
   typedef Mutex MutexType;
   // a struct used to optimize (minimize) the number of shared_ptrs that need to be created
   // inside operator()
   class InvocationState
   {
   public:
      InvocationState(const ConnectionListType &connections, const CombinerType &combiner)
         : m_connectionBodies(new ConnectionListType(connections)),
           m_combiner(new CombinerType(combiner))
      {}
      
      InvocationState(const InvocationState &other, const ConnectionListType &connections):
         m_connectionBodies(new ConnectionListType(connections)),
         m_combiner(other.m_combiner)
      {}
      
      InvocationState(const InvocationState &other, const CombinerType &combiner):
         m_connectionBodies(other.m_connectionBodies),
         m_combiner(new CombinerType(combiner))
      {}
      
      ConnectionListType &connectionBodies()
      {
         return *m_connectionBodies;
      }
      
      const ConnectionListType & connectionBodies() const
      {
         return *m_connectionBodies;
      }
      
      CombinerType &getCombiner()
      {
         return *m_combiner;
      }
      
      const CombinerType &getCombiner() const
      {
         return *m_combiner;
      }
      
private:
      InvocationState(const InvocationState &);
      std::shared_ptr<ConnectionListType> m_connectionBodies;
      std::shared_ptr<CombinerType> m_combiner;
   };
   
   // Destructor of invocation_janitor does some cleanup when a signal invocation completes.
   // Code can't be put directly in signal's operator() due to complications from void return types.
   class InvocationJanitor
   {
   public:
      using SignalType = SignalImpl;
      InvocationJanitor (
               const SlotCallIteratorCacheType &cache,
               const SignalType &sig,
               const ConnectionListType *connectionBodies)
         : m_cache(cache), 
           m_sig(sig), 
           m_connectionBodies(connectionBodies)
      {}
      
      ~InvocationJanitor()
      {
         // force a full cleanup of disconnected slots if there are too many
         if(m_cache.m_disconnectedSlotCount > m_cache.m_connectedSlotCount)
         {
            m_sig.forceCleanupConnections(m_connectionBodies);
         }
      }
      
   private:
      PDK_DISABLE_COPY(InvocationJanitor);
      const SlotCallIteratorCacheType &m_cache;
      const SignalType &m_sig;
      const ConnectionListType *m_connectionBodies;
   };
   
   // clean up disconnected connections
   void nolockCleanupConnectionsFrom(GarbageCollectingLock<MutexType> &lock,
                                     bool grabTracked,
                                     const typename ConnectionListType::iterator &begin, unsigned count = 0) const
   {
      PDK_ASSERT(m_sharedState.use_count() == 1);
      typename ConnectionListType::iterator iter;
      unsigned i;
      for(iter = begin, i = 0;
          iter != m_sharedState->connectionBodies().end() && (count == 0 || i < count);
          ++i)
      {
         bool connected;
         if(grabTracked) {
            (*iter)->disconnectExpiredSlot(lock);
         }
         connected = (*iter)->nolockNograbConnected();
         if(connected == false) {
            iter = m_sharedState->connectionBodies().erase((*iter)->getGroupKey(), iter);
         }else {
            ++iter;
         }
      }
      m_garbageCollectorIter = iter;
   }
   
   // clean up a few connections in constant time
   void nolockCleanupConnections(GarbageCollectingLock<MutexType> &lock,
                                 bool grabTracked, unsigned count) const
   {
      PDK_ASSERT(m_sharedState.use_count() == 1);
      typename ConnectionListType::iterator begin;
      if(m_garbageCollectorIter == m_sharedState->connectionBodies().end()) {
         begin = m_sharedState->connectionBodies().begin();
      }else {
         begin = m_garbageCollectorIter;
      }
      nolockCleanupConnectionsFrom(lock, grabTracked, begin, count);
   }
   
   // Make a new copy of the slot list if it is currently being read somewhere else
   void nolockForceUniqueConnectionList(GarbageCollectingLock<MutexType> &lock)
   {
      if(m_sharedState.use_count() > 1) {
         m_sharedState.reset(new InvocationState(*m_sharedState, m_sharedState->connectionBodies()));
         nolockCleanupConnectionsFrom(lock, true, m_sharedState->connectionBodies().begin());
      } else {
         // We need to try and check more than just 1 connection here to avoid corner
         // cases where certain repeated connect/disconnect patterns cause the slot
         // list to grow without limit.
         nolockCleanupConnections(lock, true, 2);
      }
   }
   
   // force a full cleanup of the connection list
   void forceCleanupConnections(const ConnectionListType *connectionBodies) const
   {
      GarbageCollectingLock<MutexType> lock(*m_mutex);
      // if the connection list passed in as a parameter is no longer in use,
      // we don't need to do any cleanup.
      if(&m_sharedState->connectionBodies() != connectionBodies) {
         return;
      }
      if(m_sharedState.use_count() > 1) {
         m_sharedState.reset(new InvocationState(*m_sharedState, m_sharedState->connectionBodies()));
      }
      nolockCleanupConnectionsFrom(lock, false, m_sharedState->connectionBodies().begin());
   }
   
   std::shared_ptr<InvocationState> getReadableState() const
   {
      std::unique_lock<MutexType> lock(*m_mutex);
      return m_sharedState;
   }
   
   ConnectionBodyType createNewConnection(GarbageCollectingLock<MutexType> &lock,
                                          const SlotType &slot)
   {
      nolockForceUniqueConnectionList(lock);
      return ConnectionBodyType(new ConnectionBody<GroupKeyType, SlotType, Mutex>(slot, m_mutex));
   }
   
   // connect slot
   Connection nolockConnect(GarbageCollectingLock<MutexType> &lock,
                            const SlotType &slot, ConnectPosition position)
   {
      ConnectionBodyType newConnectionBody = createNewConnection(lock, slot);
      GroupKeyType groupKey;
      if(position == ConnectPosition::AtBack) {
         groupKey.first = SlotMetaGroup::BackUngroupedSlots;
         m_sharedState->connectionBodies().pushBack(groupKey, newConnectionBody);
      } else {
         groupKey.first = SlotMetaGroup::FrontUngroupedSlots;
         m_sharedState->connectionBodies().pushFront(groupKey, newConnectionBody);
      }
      newConnectionBody->setGroupKey(groupKey);
      return Connection(newConnectionBody);
   }
   
   Connection nolockConnect(GarbageCollectingLock<MutexType> &lock,
                            const GroupType &group, const SlotType &slot, ConnectPosition position)
   {
      ConnectionBodyType newConnectionBody = createNewConnection(lock, slot);
      // update map to first connection body in group if needed
      GroupKeyType groupKey(SlotMetaGroup::GroupedSlots, group);
      newConnectionBody->setGroupKey(groupKey);
      if(position == ConnectPosition::AtBack) {
         m_sharedState->connectionBodies().pushBack(groupKey, newConnectionBody);
      } else {
         // at_front
         m_sharedState->connectionBodies().pushFront(groupKey, newConnectionBody);
      }
      return Connection(newConnectionBody);
   }
   
   // _shared_state is mutable so we can do force_cleanup_connections during a const invocation
   mutable std::shared_ptr<InvocationState> m_sharedState;
   mutable typename ConnectionListType::iterator m_garbageCollectorIter;
   // connection list mutex must never be locked when attempting a blocking lock on a slot,
   // or you could deadlock.
   const std::shared_ptr<MutexType> m_mutex;
};

template <typename Signature,
          typename Combiner,
          typename Group,
          typename GroupCompare,
          typename SlotFunction,
          typename ExtendedSlotFunction,
          typename Mutex>
class WeakSignal;


} // internal

template <typename Signature,
          typename Combiner = OptionalLastValue<typename pdk::stdext::FunctionTraits<Signature>::ResultType>,
          typename Group = int,
          typename GroupCompare = std::less<Group>,
          typename SlotFunction = std::function<Signature>,
          typename ExtendedSlotFunction = typename internal::VariadicExtendedSignature<Signature>::FunctionType,
          typename Mutex = std::mutex>
class Signal;

template <typename Combiner,
          typename Group,
          typename GroupCompare,
          typename SlotFunction,
          typename ExtendedSlotFunction,
          typename Mutex,
          typename R,
          typename ... Args>
class Signal <R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex> 
   : public SignalBase, public internal::StdFuncBase<Args...>
{
using ImplClass = internal::SignalImpl<R (Args...), Combiner, Group, GroupCompare, SlotFunction, 
                                       ExtendedSlotFunction, Mutex>;
public:
   using WeakSignalType = internal::WeakSignal<R (Args...), Combiner, Group, GroupCompare, SlotFunction, 
                                               ExtendedSlotFunction, Mutex>;
   using SlotFunctionType = SlotFunction;
   using SlotType = typename ImplClass::SlotType;
   using ExtendedSlotFunctionType = typename ImplClass::ExtendedSlotFunctionType;
   using ExtendedSlotType = typename ImplClass::ExtendedSlotType;
   using SlotResultType = typename SlotFunctionType::result_type;
   using CombinerType = Combiner;
   using ResultType = typename ImplClass::ResultType;
   using GroupType = Group;
   using GroupCompareType = GroupCompare;
   using SlotCallIterator = typename ImplClass::SlotCallIterator;
   using SignatureType = R (Args...);
   
   friend class internal::WeakSignal<R (Args...), Combiner, Group, GroupCompare, SlotFunction, 
                                     ExtendedSlotFunction, Mutex>;

   template<unsigned N> class Arg
   {
   public:
      typedef typename internal::VariadicArgType<N, Args...>::type type;
   };

   static constexpr int arity = sizeof...(Args);
   
   Signal(const CombinerType &combiner = CombinerType(),
          const GroupCompareType &groupCompare = GroupCompareType())
      : m_pimpl(new ImplClass(combiner, groupCompare))
   {}

   virtual ~Signal()
   {}
   
   Signal(Signal &&other)
   {
      using std::swap;
      swap(m_pimpl, other.m_pimpl);
   }
   
   Signal &operator=(Signal &&other)
   {
      if(this == &other) {
         return *this;
      }
      m_pimpl.reset();
      using std::swap;
      swap(m_pimpl, other.m_pimpl);
      return *this;
   }
   
   Connection connect(const SlotType &slot, ConnectPosition position = ConnectPosition::AtBack)
   {
      return (*m_pimpl).connect(slot, position);
   }
   
   Connection connect(const GroupType &group, const SlotType &slot,
                      ConnectPosition position = ConnectPosition::AtBack)
   {
      return (*m_pimpl).connect(group, slot, position);
   }
   
   Connection connectExtended(const ExtendedSlotType &slot,
                              ConnectPosition position = ConnectPosition::AtBack)
   {
      return (*m_pimpl).connectExtended(slot, position);
   }
   
   Connection connectExtended(const GroupType &group, const ExtendedSlotType &slot,
                              ConnectPosition position = ConnectPosition::AtBack)
   {
      return (*m_pimpl).connectExtended(group, slot, position);
   }
   
   void disconnectAllSlots()
   {
      (*m_pimpl).disconnectAllSlots();
   }
   
   void disconnect(const GroupType &group)
   {
      (*m_pimpl).disconnect(group);
   }
   
   void disconnect(const Connection &connection)
   {
      (*m_pimpl).disconnect(connection);
   }
   
   ResultType operator ()(Args&&... args)
   {
      return (*m_pimpl)(std::forward<Args>(args)...);
   }
   
   ResultType operator ()(Args&&... args) const
   {
      return (*m_pimpl)(std::forward<Args>(args)...);
   }
   
   std::size_t getNumSlots() const
   {
      return (*m_pimpl).getNumSlots();
   }
   
   bool empty() const
   {
      return (*m_pimpl).empty();
   }
   
   CombinerType getCombiner() const
   {
      return (*m_pimpl).getCombiner();
   }
   
   void setCombiner(const CombinerType &combinerArg)
   {
      return (*m_pimpl).setCombiner(combinerArg);
   }
   
   void swap(Signal &other)
   {
      using std::swap;
      swap(m_pimpl, other.m_pimpl);
   }

protected:
   virtual std::shared_ptr<void> getLockPimpl() const
   {
      return m_pimpl;
   }

private:
   std::shared_ptr<ImplClass> m_pimpl;
};

namespace internal {

// wrapper class for storing other signals as slots with automatic lifetime tracking
template <typename Signature,
          typename Combiner,
          typename Group,
          typename GroupCompare,
          typename SlotFunction,
          typename ExtendedSlotFunction,
          typename Mutex>
class WeakSignal;

template <typename Combiner,
          typename Group,
          typename GroupCompare,
          typename SlotFunction,
          typename ExtendedSlotFunction,
          typename Mutex,
          typename R,
          typename ... Args>
class WeakSignal <R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex>
{
public:
   using ResultType = typename Signal<R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex>::ResultType;
   
   WeakSignal(const Signal<R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex> &signal)
      : m_weakPimpl(signal.m_pimpl)
   {}
   
   ResultType operator ()(Args&& ...args)
   {
      std::shared_ptr<internal::SignalImpl <R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex>>
      shared_pimpl(m_weakPimpl);
      return (*shared_pimpl)(std::forward<Args>(args)...);
   }
   
   ResultType operator ()(Args&& ...args) const
   {
      std::shared_ptr<internal::SignalImpl <R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex>>
      shared_pimpl(m_weakPimpl);
      return (*shared_pimpl)(std::forward<Args>(args)...);
   }
   
private:
   std::weak_ptr<internal::SignalImpl<R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex>> m_weakPimpl;
};

template<int arity, typename Signature>
class ExtendedSignature: public VariadicExtendedSignature<Signature>
{};

} // internal

} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_INTERNAL_SIGNAL_PRIVATE_H
