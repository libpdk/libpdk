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

#define PDK_SIGNAL_SIGNAL_TEMPLATE_INSTANTIATION \
   R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex

namespace pdk {
namespace kernel {
namespace signal {
namespace internal {

template <typename R>
class BoundExtendedSlotFunctionInvoker
{
   using result_type = R;
   template <typename ExtendedSlotFunction, typename ... Args>
   result_type operator()(ExtendedSlotFunction &func, const Connection &conn,
                          Args&& ... args) const
   {
      return func(conn, std::forward<Args>(args)...);
   }
};

template <>
class BoundExtendedSlotFunctionInvoker<void>
{
public:
   typedef ResultTypeWrapper<void>::type result_type;
   
   template <typename ExtendedSlotFunction, typename ... Args>
   result_type operator()(ExtendedSlotFunction &func, const Connection &conn,
                          Args&& ... args) const
   {
      func(conn, std::forward<Args>(args)...);
      return result_type();
   }
};

// wrapper around an signalN::extended_slot_function which binds the
// connection argument so it looks like a normal
// signalN::slot_function

template <typename ExtendedSlotFunction>
class BoundExtendedSlotFunction
{
public:
   using result_type = typename ResultTypeWrapper<typename ExtendedSlotFunction::result_type>::type;
   
   BoundExtendedSlotFunction(const ExtendedSlotFunction &func)
      : m_func(func),
        m_connection(new Connection)
   {}
   
   void setConnection(const Connection &conn)
   {
      *m_connection = conn;
   }
   
   template <typename ... Args>
   result_type operator()(Args && ... args)
   {
      return bound_extended_slot_function_invoker<typename ExtendedSlotFunction::result_type>()
            (m_func, *m_connection, std::forward<Args>(args)...);
   }
   
   template <typename ... Args>
   result_type operator()(Args && ... args) const
   {
      return bound_extended_slot_function_invoker<typename ExtendedSlotFunction::result_type>()
            (m_func, *m_connection, std::forward<Args>(args)...);
   }
   
   template <typename T>
   bool operator ==(const T &other) const
   {
      return m_func = other;
   }
   
private:
   BoundExtendedSlotFunction()
   {}
   
   ExtendedSlotFunction m_func;
   std::shared_ptr<connection> m_connection;
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
   using slot_function_type = SlotFunction;
   using slot_type = Slot<R (Args...), slot_function_type>;
   using extended_slot_function_type = ExtendedSlotFunction;
   using extended_slot_type = Slot<R (const Connection &, Args...), extended_slot_function_type>;
   using nonvoid_slot_result_type = typename Nonvoid<typename slot_function_type::result_type>::type;
   
   private:
   using slot_invoker = VariadicSlotInvoker<nonvoid_slot_result_type, Args...>;
   using slot_call_iterator_cache_type = SlotCallIteratorCache<nonvoid_slot_result_type, slot_invoker>;
   using group_key_type = typename GroupKey<Group>::type;
   using connection_body_type = std::shared_ptr<connection_body<group_key_type, slot_type, Mutex>>;
   using connection_list_type = GroupedList<Group, GroupCompare, connection_body_type>;
   using bound_extended_slot_function_type = BoundExtendedSlotFunction<extended_slot_function_type>;
   
   public:
   using combiner_type = Combiner;
   using result_type = typename ResultTypeWrapper<typename combiner_type::result_type>::type;
   using group_type = Group;
   using group_compare_type = GroupCompare;
   using slot_call_iterator = typename internal::slot_call_iterator_t<slot_invoker,
   typename connection_list_type::iterator, ConnectionBody<group_key_type, slot_type, Mutex>>;
   
   SignalImpl(const combiner_type &combinerArg,
              const group_compare_type &groupCompare)
      : m_sharedState(new InvocationState(connection_list_type(groupCompare), combinerArg)),
        m_garbageCollectorIt(m_sharedState->connectionBodies().end()),
        m_mutex(new MutexType())
   {}
   
   // connect slot
   Connection connect(const slot_type &slot, connect_position position = at_back)
   {
      GarbageCollectingLock<MutexType> lock(m_mutex);
      return nolockConnect(lock, slot, position);
   }
   
   Connection connect(const group_type &group,
                      const slot_type &slot, connect_position position = at_back)
   {
      GarbageCollectingLock<MutexType> lock(m_mutex);
      return nolockConnect(lock, group, slot, position);
   }
   
   connection connectExtended(const extended_slot_type &ext_slot, connect_position position = at_back)
   {
      garbage_collecting_lock<mutex_type> lock(*_mutex);
      bound_extended_slot_function_type bound_slot(ext_slot.slot_function());
      slot_type slot = replace_slot_function<slot_type>(ext_slot, bound_slot);
      connection conn = nolock_connect(lock, slot, position);
      bound_slot.set_connection(conn);
      return conn;
   }
   
   connection connect_extended(const group_type &group,
                               const extended_slot_type &ext_slot, connect_position position = at_back)
   {
      garbage_collecting_lock<Mutex> lock(*_mutex);
      bound_extended_slot_function_type bound_slot(ext_slot.slot_function());
      slot_type slot = replace_slot_function<slot_type>(ext_slot, bound_slot);
      connection conn = nolock_connect(lock, group, slot, position);
      bound_slot.set_connection(conn);
      return conn;
   }
   
   // disconnect slot(s)
   void disconnect_all_slots()
   {
      shared_ptr<invocation_state> local_state =
            get_readable_state();
      typename connection_list_type::iterator it;
      for(it = local_state->connection_bodies().begin();
          it != local_state->connection_bodies().end(); ++it)
      {
         (*it)->disconnect();
      }
   }
   
   void disconnect(const group_type &group)
   {
      shared_ptr<invocation_state> local_state =
            get_readable_state();
      group_key_type group_key(grouped_slots, group);
      typename connection_list_type::iterator it;
      typename connection_list_type::iterator end_it =
            local_state->connection_bodies().upper_bound(group_key);
      for(it = local_state->connection_bodies().lower_bound(group_key);
          it != end_it; ++it)
      {
         (*it)->disconnect();
      }
   }
   
   template <typename T>
   void disconnect(const T &slot)
   {
      typedef mpl::bool_<(is_convertible<T, group_type>::value)> is_group;
      do_disconnect(slot, is_group());
   }
   
   // emit signal
   result_type operator ()(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(BOOST_SIGNALS2_NUM_ARGS))
   {
      shared_ptr<invocation_state> local_state;
      typename connection_list_type::iterator it;
      {
         garbage_collecting_lock<mutex_type> list_lock(*_mutex);
         // only clean up if it is safe to do so
         if(_shared_state.unique())
            nolock_cleanup_connections(list_lock, false, 1);
         /* Make a local copy of _shared_state while holding mutex, so we are
               thread safe against the combiner or connection list getting modified
               during invocation. */
         local_state = _shared_state;
      }
      slot_invoker invoker = slot_invoker(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(BOOST_SIGNALS2_NUM_ARGS));
      slot_call_iterator_cache_type cache(invoker);
      invocation_janitor janitor(cache, *this, &local_state->connection_bodies());
      return detail::combiner_invoker<typename combiner_type::result_type>()
            (
               local_state->combiner(),
               slot_call_iterator(local_state->connection_bodies().begin(), local_state->connection_bodies().end(), cache),
               slot_call_iterator(local_state->connection_bodies().end(), local_state->connection_bodies().end(), cache)
               );
   }
   
   result_type operator ()(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(BOOST_SIGNALS2_NUM_ARGS)) const
   {
      shared_ptr<invocation_state> local_state;
      typename connection_list_type::iterator it;
      {
         garbage_collecting_lock<mutex_type> list_lock(*_mutex);
         // only clean up if it is safe to do so
         if(_shared_state.unique())
            nolock_cleanup_connections(list_lock, false, 1);
         /* Make a local copy of _shared_state while holding mutex, so we are
               thread safe against the combiner or connection list getting modified
               during invocation. */
         local_state = _shared_state;
      }
      slot_invoker invoker = slot_invoker(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(BOOST_SIGNALS2_NUM_ARGS));
      slot_call_iterator_cache_type cache(invoker);
      invocation_janitor janitor(cache, *this, &local_state->connection_bodies());
      return detail::combiner_invoker<typename combiner_type::result_type>()
            (
               local_state->combiner(),
               slot_call_iterator(local_state->connection_bodies().begin(), local_state->connection_bodies().end(), cache),
               slot_call_iterator(local_state->connection_bodies().end(), local_state->connection_bodies().end(), cache)
               );
   }
   
   std::size_t num_slots() const
   {
      shared_ptr<invocation_state> local_state =
            get_readable_state();
      typename connection_list_type::iterator it;
      std::size_t count = 0;
      for(it = local_state->connection_bodies().begin();
          it != local_state->connection_bodies().end(); ++it)
      {
         if((*it)->connected()) ++count;
      }
      return count;
   }
   
   bool empty() const
   {
      shared_ptr<invocation_state> local_state =
            get_readable_state();
      typename connection_list_type::iterator it;
      for(it = local_state->connection_bodies().begin();
          it != local_state->connection_bodies().end(); ++it)
      {
         if((*it)->connected()) return false;
      }
      return true;
   }
   
   combiner_type combiner() const
   {
      unique_lock<mutex_type> lock(*_mutex);
      return _shared_state->combiner();
   }
   
   void set_combiner(const combiner_type &combiner_arg)
   {
      unique_lock<mutex_type> lock(*_mutex);
      if(_shared_state.unique())
         _shared_state->combiner() = combiner_arg;
      else
         _shared_state.reset(new invocation_state(*_shared_state, combiner_arg));
   }
   private:
   typedef Mutex mutex_type;
   // a struct used to optimize (minimize) the number of shared_ptrs that need to be created
   // inside operator()
   class invocation_state
   {
   public:
      invocation_state(const connection_list_type &connections_in,
                       const combiner_type &combiner_in): _connection_bodies(new connection_list_type(connections_in)),
         _combiner(new combiner_type(combiner_in))
      {}
      invocation_state(const invocation_state &other, const connection_list_type &connections_in):
         _connection_bodies(new connection_list_type(connections_in)),
         _combiner(other._combiner)
      {}
      invocation_state(const invocation_state &other, const combiner_type &combiner_in):
         _connection_bodies(other._connection_bodies),
         _combiner(new combiner_type(combiner_in))
      {}
      connection_list_type & connection_bodies() { return *_connection_bodies; }
      const connection_list_type & connection_bodies() const { return *_connection_bodies; }
      combiner_type & combiner() { return *_combiner; }
      const combiner_type & combiner() const { return *_combiner; }
   private:
      invocation_state(const invocation_state &);
      
      shared_ptr<connection_list_type> _connection_bodies;
      shared_ptr<combiner_type> _combiner;
   };
   
   // Destructor of invocation_janitor does some cleanup when a signal invocation completes.
   // Code can't be put directly in signal's operator() due to complications from void return types.
   class invocation_janitor: noncopyable
   {
   public:
      typedef BOOST_SIGNALS2_SIGNAL_IMPL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS) signal_type;
      invocation_janitor
            (
               const slot_call_iterator_cache_type &cache,
               const signal_type &sig,
               const connection_list_type *connection_bodies
               ):_cache(cache), _sig(sig), _connection_bodies(connection_bodies)
      {}
      ~invocation_janitor()
      {
         // force a full cleanup of disconnected slots if there are too many
         if(_cache.disconnected_slot_count > _cache.connected_slot_count)
         {
            _sig.force_cleanup_connections(_connection_bodies);
         }
      }
   private:
      const slot_call_iterator_cache_type &_cache;
      const signal_type &_sig;
      const connection_list_type *_connection_bodies;
   };
   
   // clean up disconnected connections
   void nolock_cleanup_connections_from(garbage_collecting_lock<mutex_type> &lock,
                                        bool grab_tracked,
                                        const typename connection_list_type::iterator &begin, unsigned count = 0) const
   {
      BOOST_ASSERT(_shared_state.unique());
      typename connection_list_type::iterator it;
      unsigned i;
      for(it = begin, i = 0;
          it != _shared_state->connection_bodies().end() && (count == 0 || i < count);
          ++i)
      {
         bool connected;
         if(grab_tracked)
            (*it)->disconnect_expired_slot(lock);
         connected = (*it)->nolock_nograb_connected();
         if(connected == false)
         {
            it = _shared_state->connection_bodies().erase((*it)->group_key(), it);
         }else
         {
            ++it;
         }
      }
      _garbage_collector_it = it;
   }
   
   // clean up a few connections in constant time
   void nolock_cleanup_connections(garbage_collecting_lock<mutex_type> &lock,
                                   bool grab_tracked, unsigned count) const
   {
      BOOST_ASSERT(_shared_state.unique());
      typename connection_list_type::iterator begin;
      if(_garbage_collector_it == _shared_state->connection_bodies().end())
      {
         begin = _shared_state->connection_bodies().begin();
      }else
      {
         begin = _garbage_collector_it;
      }
      nolock_cleanup_connections_from(lock, grab_tracked, begin, count);
   }
   
   /* Make a new copy of the slot list if it is currently being read somewhere else
           */
   void nolock_force_unique_connection_list(garbage_collecting_lock<mutex_type> &lock)
   {
      if(_shared_state.unique() == false)
      {
         _shared_state.reset(new invocation_state(*_shared_state, _shared_state->connection_bodies()));
         nolock_cleanup_connections_from(lock, true, _shared_state->connection_bodies().begin());
      }else
      {
         /* We need to try and check more than just 1 connection here to avoid corner
               cases where certain repeated connect/disconnect patterns cause the slot
               list to grow without limit. */
         nolock_cleanup_connections(lock, true, 2);
      }
   }
   
   // force a full cleanup of the connection list
   void force_cleanup_connections(const connection_list_type *connection_bodies) const
   {
      garbage_collecting_lock<mutex_type> list_lock(*_mutex);
      // if the connection list passed in as a parameter is no longer in use,
      // we don't need to do any cleanup.
      if(&_shared_state->connection_bodies() != connection_bodies)
      {
         return;
      }
      if(_shared_state.unique() == false)
      {
         _shared_state.reset(new invocation_state(*_shared_state, _shared_state->connection_bodies()));
      }
      nolock_cleanup_connections_from(list_lock, false, _shared_state->connection_bodies().begin());
   }
   
   shared_ptr<invocation_state> get_readable_state() const
   {
      unique_lock<mutex_type> list_lock(*_mutex);
      return _shared_state;
   }
   
   connection_body_type create_new_connection(garbage_collecting_lock<mutex_type> &lock,
                                              const slot_type &slot)
   {
      nolock_force_unique_connection_list(lock);
      return connection_body_type(new connection_body<group_key_type, slot_type, Mutex>(slot, _mutex));
   }
   
   void do_disconnect(const group_type &group, mpl::bool_<true> /* is_group */)
   {
      disconnect(group);
   }
   
   template<typename T>
   void do_disconnect(const T &slot, mpl::bool_<false> /* is_group */)
   {
      shared_ptr<invocation_state> local_state =
            get_readable_state();
      typename connection_list_type::iterator it;
      for(it = local_state->connection_bodies().begin();
          it != local_state->connection_bodies().end(); ++it)
      {
         garbage_collecting_lock<connection_body_base> lock(**it);
         if((*it)->nolock_nograb_connected() == false) continue;
         if((*it)->slot().slot_function() == slot)
         {
            (*it)->nolock_disconnect(lock);
         }else
         {
            // check for wrapped extended slot
            bound_extended_slot_function_type *fp;
            fp = (*it)->slot().slot_function().template target<bound_extended_slot_function_type>();
            if(fp && *fp == slot)
            {
               (*it)->nolock_disconnect(lock);
            }
         }
      }
   }
   
   
   // connect slot
   connection nolock_connect(garbage_collecting_lock<mutex_type> &lock,
                             const slot_type &slot, connect_position position)
   {
      connection_body_type newConnectionBody =
            create_new_connection(lock, slot);
      group_key_type group_key;
      if(position == at_back)
      {
         group_key.first = back_ungrouped_slots;
         _shared_state->connection_bodies().push_back(group_key, newConnectionBody);
      }else
      {
         group_key.first = front_ungrouped_slots;
         _shared_state->connection_bodies().push_front(group_key, newConnectionBody);
      }
      newConnectionBody->set_group_key(group_key);
      return connection(newConnectionBody);
   }
   connection nolock_connect(garbage_collecting_lock<mutex_type> &lock,
                             const group_type &group,
                             const slot_type &slot, connect_position position)
   {
      connection_body_type newConnectionBody =
            create_new_connection(lock, slot);
      // update map to first connection body in group if needed
      group_key_type group_key(grouped_slots, group);
      newConnectionBody->set_group_key(group_key);
      if(position == at_back)
      {
         _shared_state->connection_bodies().push_back(group_key, newConnectionBody);
      }else  // at_front
      {
         _shared_state->connection_bodies().push_front(group_key, newConnectionBody);
      }
      return connection(newConnectionBody);
   }
   
   // _shared_state is mutable so we can do force_cleanup_connections during a const invocation
   mutable shared_ptr<invocation_state> m_shared_state;
   mutable typename connection_list_type::iterator m_garbage_collector_it;
   // connection list mutex must never be locked when attempting a blocking lock on a slot,
   // or you could deadlock.
   const boost::shared_ptr<mutex_type> m_mutex;
};

template<typename Signature,
         typename Combiner,
         typename Group,
         typename GroupCompare,
         typename SlotFunction,
         typename ExtendedSlotFunction,
         typename Mutex>
class WeakSignal;

} // internal

template <typename Signature,
          typename Combiner = optional_last_value<typename boost::function_traits<Signature>::result_type>,
          typename Group = int,
          typename GroupCompare = std::less<Group>,
          typename SlotFunction = boost::function<Signature>,
          typename ExtendedSlotFunction = typename internal::variadic_extended_signature<Signature>::function_type,
          typename Mutex = signals2::mutex>
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
   : public SignalBase, public internal::std_functional_base<Args...>
{
   using impl_class = internal::SignalImpl<R (Args...), Combiner, Group, GroupCompare, SlotFunction, 
   ExtendedSlotFunction, Mutex>;
public:
   using weak_signal_type = internal::WeakSignal<R (Args...), Combiner, Group, GroupCompare, SlotFunction, 
   ExtendedSlotFunction, Mutex>;
   friend class internal::WeakSignal<R (Args...), Combiner, Group, GroupCompare, SlotFunction, 
   ExtendedSlotFunction, Mutex>;
   using slot_function_type = SlotFunction;
   using slot_type = typename impl_class::slot_type;
   using extended_slot_function_type = typename impl_class::extended_slot_function_type;
   using extended_slot_type = typename impl_class::extended_slot_type;
   using slot_result_type = typename slot_function_type::result_type;
   using combiner_type = Combiner;
   using result_type = typename impl_class::result_type;
   using group_type = Group;
   using group_compare_type = GroupCompare;
   using slot_call_iterator = typename impl_class::slot_call_iterator;
   using signature_type = typename mpl::identity<R (Args...)>::type;
   
   template<unsigned n> class arg
   {
    public:
      typedef typename detail::variadic_arg_type<n, Args...>::type type;
   };
   static const int arity = sizeof...(Args);
   
   Signal(const combiner_type &combiner_arg = combiner_type(),
          const group_compare_type &group_compare = group_compare_type())
      : m_impl(new impl_class(combiner_arg, group_compare))
   {
      
   }
   
   virtual ~Signal()
   {
   }
   
   Signal(Signal &&other)
   {
      using std::swap;
      swap(m_pimpl, other.m_pimpl);
   }
   
   Signal &operator=(Signal &&other)
   {
      if(this == &rhs)
      {
         return *this;
      }
      m_pimpl.reset();
      using std::swap;
      swap(m_pimpl, rhs.m_pimpl);
      return *this;
   }
   
   Connection connect(const slot_type &slot, connect_position position = at_back)
   {
      return (*m_pimpl).connect(slot, position);
   }
   
   connection connect(const group_type &group,
                      const slot_type &slot, connect_position position = at_back)
   {
      return (*m_pimpl).connect(group, slot, position);
   }
   
   connection connect_extended(const extended_slot_type &slot, connect_position position = at_back)
   {
      return (*m_pimpl).connect_extended(slot, position);
   }
   
   connection connect_extended(const group_type &group,
                               const extended_slot_type &slot, connect_position position = at_back)
   {
      return (*m_pimpl).connect_extended(group, slot, position);
   }
   
   void disconnect_all_slots()
   {
      (*m_pimpl).disconnect_all_slots();
   }
   
   void disconnect(const group_type &group)
   {
      (*m_pimpl).disconnect(group);
   }
   
   template <typename T>
   void disconnect(const T &slot)
   {
      (*m_pimpl).disconnect(slot);
   }
         
   result_type operator ()(Args ... args)
   {
      return (*m_pimpl)(args...);
   }
   
   result_type operator ()(Args ... args) const
   {
      return (*m_pimpl)(args...);
   }
   
   std::size_t num_slots() const
   {
      return (*m_pimpl).num_slots();
   }
   
   bool empty() const
   {
      return (*m_pimpl).empty();
   }
   
   combiner_type combiner() const
   {
      return (*m_pimpl).combiner();
   }
   
   void set_combiner(const combiner_type &combiner_arg)
   {
      return (*_pimpl).set_combiner(combiner_arg);
   }
   
   void swap(Signal &other)
   {
      using std::swap;
      swap(m_pimpl, other.m_pimpl);
   }
   
protected:
   virtual std::shared_ptr<void> lock_pimpl() const
   {
      return m_pimpl;
   }
   
private:
   std::shared_ptr<impl_class> m_pimpl;
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
   using result_type = typename Signal<R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex>::result_type;
   WeakSignal(const Signal<R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex> &signal)
      : m_weakPimpl(signal.m_pImpl)
   {}
   
   result_type operator ()(Args ... args)
   {
      std::shared_ptr<internal::SignalImpl <R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex>>
      shared_pimpl(_weak_pimpl.lock());
      return (*shared_pimpl)(args...);
   }
   
   result_type operator ()(Args ... args) const
   {
      std::shared_ptr<internal::SignalImpl <R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex>>
      shared_pimpl(_weak_pimpl.lock());
      return (*shared_pimpl)(args...);
   }
   
private:
   std::weak_ptr<internal::SignalImpl<R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex>> m_weak_pimpl;
};

template<int arity, typename Signature>
class extended_signature: public variadic_extended_signature<Signature>
{};
 
} // internal

} // signal
} // kernel
} // pdk
