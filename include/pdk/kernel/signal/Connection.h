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

/*
  boost::signals2::connection provides a handle to a signal/slot connection.
  
  Author: Frank Mori Hess <fmhess@users.sourceforge.net>
  Begin: 2007-01-23
*/
// Copyright Frank Mori Hess 2007-2008.
// Distributed under the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/signals2 for library home page.

#ifndef PDK_KERNEL_SIGNAL_CONNECTION_H
#define PDK_KERNEL_SIGNAL_CONNECTION_H

#include "pdk/global/Global.h"
#include "pdk/kernel/signal/internal/AutoBuffer.h"
#include "pdk/kernel/signal/internal/NullOutputIterator.h"
#include <mutex>

namespace pdk {
namespace kernel {
namespace signal {

inline void null_deleter(const void *)
{}

namespace internal {

// This lock maintains a list of std::shared_ptr<void>
// which will be destroyed only after the lock
// has released its mutex.  Used to garbage
// collect disconnected slots
template <typename Mutex>
class GarbageCollectingLock
{
public:
   GarbageCollectingLock(Mutex &mutex)
      : m_lock(mutex)
   {}
   
   void addTrash(const std::shared_ptr<void> &pieceOfTrash)
   {
      garbage.pushBack(pieceOfTrash);
   }
   
private:
   PDK_DISABLE_COPY(GarbageCollectingLock);
   // garbage must be declared before lock
   // to insure it is destroyed after lock is
   // destroyed.
   AutoBuffer<shared_ptr<void>, store_n_objects<10>> m_garbage;
   std::unique<Mutex> lock;
};

class ConnectionBodyBase
{
public:
   ConnectionBodyBase()
      : m_connected(true),
        m_slotRefcount(1)
   {}
   
   virtual ~ConnectionBodyBase()
   {}
   
   void disconnect()
   {
      GarbageCollectingLock<ConnectionBodyBase> local_lock(*this);
      nolockDisconnect(local_lock);
   }
   
   template<typename Mutex>
   void nolockDisconnect(GarbageCollectingLock<Mutex> &lockArg) const
   {
      if(m_connected)
      {
         m_connected = false;
         decSlotRefcount(lockArg);
      }
   }
   
   virtual bool connected() const = 0;
   
   std::shared_ptr<void> getBlocker()
   {
      std::unique_lock<ConnectionBodyBase> localLock(*this);
      std::shared_ptr<void> blocker = m_weakBlocker.lock();
      if(blocker == std::shared_ptr<void>())
      {
         blocker.reset(this, &null_deleter);
         m_weakBlocker = blocker;
      }
      return blocker;
   }
   
   bool blocked() const
   {
      return !_weak_blocker.expired();
   }
   
   bool nolockNograbBlocked() const
   {
      return nolockNograbConnected() == false || blocked();
   }
   
   bool nolockNograbConnected() const
   {
      return m_connected;
   }
   
   // expose part of Lockable concept of mutex
   virtual void lock() = 0;
   virtual void unlock() = 0;
   
   // Slot refcount should be incremented while
   // a signal invocation is using the slot, in order
   // to prevent slot from being destroyed mid-invocation.
   // garbage_collecting_lock parameter enforces 
   // the existance of a lock before this
   // method is called
   template<typename Mutex>
   void incSlotRefcount(const GarbageCollectingLock<Mutex> &)
   {
      PDK_ASSERT(m_slotRefcount != 0);
      ++m_slotRefcount;
   }
   
   // if slot refcount decrements to zero due to this call, 
   // it puts a
   // shared_ptr to the slot in the garbage collecting lock,
   // which will destroy the slot only after it unlocks.
   template<typename Mutex>
   void decSlotRefcount(GarbageCollectingLock<Mutex> &lockArg) const
   {
      BOOST_ASSERT(m_slotRefcount != 0);
      if(--m_slotRefcount == 0)
      {
         lockArg.addTrash(releaseSlot());
      }
   }
   
protected:
   virtual std::shared_ptr<void> releaseSlot() const = 0;
   std::weak_ptr<void> m_weakBlocker;
   
private:
   mutable bool m_connected;
   mutable unsigned m_slotRefcount;
};

template<typename GroupKey, typename SlotType, typename Mutex>
class ConnectionBody : public ConnectionBodyBase
{
public:
   using MutexType = Mutex;
   ConnectionBody(const SlotType &slotIn, const std::shared_ptr<MutexType> &signalMutex)
      : m_slot(new SlotType(slotIn)), 
        m_mutex(signalMutex)
   {}
   
   virtual ~ConnectionBody()
   {}
   
   virtual bool connected() const
   {
      GarbageCollectingLock<MutexType> lock(*m_mutex);
      nolockGrabTrackedObjects(lock, NullOutputIterator);
      return nolockNograbConnected();
   }
   
   const GroupKey& groupKey() const
   {
      return m_groupKey;
   }
   
   void setGroupKey(const GroupKey &key)
   {
      m_groupKey = key;
   }
   
   template<typename M>
   void disconnectExpiredSlot(GarbageCollectingLock<M> &lock)
   {
      if(!m_slot) return;
      bool expired = slot().expired();
      if(expired == true)
      {
         nolockDisconnect(lock);
      }
   }
   
   template<typename M, typename OutputIterator>
   void nolockGrabTrackedObjects(GarbageCollectingLock<M> &lock,
                                 OutputIterator inserter) const
   {
      if(!m_slot) return;
      slot_base::tracked_container_type::const_iterator it;
      for(it = slot().tracked_objects().begin();
          it != slot().tracked_objects().end();
          ++it)
      {
         void_shared_ptr_variant locked_object
               (
                  apply_visitor
                  (
                     detail::lock_weak_ptr_visitor(),
                     *it
                     )
                  );
         if(apply_visitor(detail::expired_weak_ptr_visitor(), *it))
         {
            nolock_disconnect(lock_arg);
            return;
         }
         *inserter++ = locked_object;
      }
   }
   
   // expose Lockable concept of mutex
   virtual void lock()
   {
      _mutex->lock();
   }
   
   virtual void unlock()
   {
      _mutex->unlock();
   }
   
   SlotType &slot()
   {
      return *m_slot;
   }
   
   const SlotType &slot() const
   {
      return *m_slot;
   }
   
protected:
   virtual shared_ptr<void> releaseSlot() const
   {
      shared_ptr<void> released_slot = m_slot;
      m_slot.reset();
      return released_slot;
   }
private:
   mutable std::shared_ptr<SlotType> m_slot;
   const std::shared_ptr<MutexType> m_mutex;
   GroupKey m_groupKey;
};

} // internal

class SharedConnectionBlock;

class Connection
{
public:
   friend class shared_connection_block;
   
   Connection() {}
   Connection(const Connection &other): _weak_connection_body(other._weak_connection_body)
   {}
   Connection(const boost::weak_ptr<detail::connection_body_base> &connectionBody):
      _weak_connection_body(connectionBody)
   {}
   
   // move support
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
   Connection(Connection && other): _weak_connection_body(std::move(other._weak_connection_body))
   {
      // make sure other is reset, in case it is a scoped_connection (so it
      // won't disconnect on destruction after being moved away from).
      other._weak_connection_body.reset();
   }
   Connection & operator=(connection && other)
   {
      if(&other == this) return *this;
      _weak_connection_body = std::move(other._weak_connection_body);
      // make sure other is reset, in case it is a scoped_connection (so it
      // won't disconnect on destruction after being moved away from).
      other._weak_connection_body.reset();
      return *this;
   }
#endif // !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
   Connection & operator=(const connection & other)
   {
      if(&other == this) return *this;
      _weak_connection_body = other._weak_connection_body;
      return *this;
   }
   
   ~Connection() {}
   void disconnect() const
   {
      boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
      if(connectionBody == 0) return;
      connectionBody->disconnect();
   }
   bool connected() const
   {
      boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
      if(connectionBody == 0) return false;
      return connectionBody->connected();
   }
   bool blocked() const
   {
      boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
      if(connectionBody == 0) return true;
      return connectionBody->blocked();
   }
   bool operator==(const connection& other) const
   {
      boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
      boost::shared_ptr<detail::connection_body_base> otherConnectionBody(other._weak_connection_body.lock());
      return connectionBody == otherConnectionBody;
   }
   bool operator!=(const connection& other) const
   {
      return !(*this == other);
   }
   bool operator<(const connection& other) const
   {
      boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
      boost::shared_ptr<detail::connection_body_base> otherConnectionBody(other._weak_connection_body.lock());
      return connectionBody < otherConnectionBody;
   }
   void swap(connection &other)
   {
      using std::swap;
      swap(_weak_connection_body, other._weak_connection_body);
   }
protected:
   
   boost::weak_ptr<detail::connection_body_base> _weak_connection_body;
};

inline void swap(connection &conn1, connection &conn2)
{
   conn1.swap(conn2);
}

class scoped_connection: public Connection
{
public:
   scoped_connection() 
   {}
   
   scoped_connection(const connection &other)
      : connection(other)
   {}
   
   ~scoped_connection()
   {
      disconnect();
   }
   
   scoped_connection& operator=(const Connection &rhs)
   {
      disconnect();
      Connection::operator=(rhs);
      return *this;
   }
   
   // move support
   scoped_connection(scoped_connection && other): Connection(std::move(other))
   {
   }
   
   scoped_connection(connection && other): Connection(std::move(other))
   {
   }
   
   scoped_connection & operator=(scoped_connection && other)
   {
      if(&other == this) return *this;
      disconnect();
      connection::operator=(std::move(other));
      return *this;
   }
   scoped_connection & operator=(connection && other)
   {
      if(&other == this) return *this;
      disconnect();
      connection::operator=(std::move(other));
      return *this;
   }
   
   Connection release()
   {
      connection conn(_weak_connection_body);
      _weak_connection_body.reset();
      return conn;
   }
private:
   scoped_connection(const scoped_connection &other);
   scoped_connection& operator=(const scoped_connection &rhs);
};

// Sun 5.9 compiler doesn't find the swap for base connection class when
// arguments are scoped_connection, so we provide this explicitly.
inline void swap(scoped_connection &conn1, scoped_connection &conn2)
{
   conn1.swap(conn2);
}

} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_CONNECTION_H
