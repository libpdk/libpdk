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
#include "pdk/kernel/signal/Slot.h"
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
      m_garbage.pushBack(pieceOfTrash);
   }
   
private:
   PDK_DISABLE_COPY(GarbageCollectingLock);
   // garbage must be declared before lock
   // to insure it is destroyed after lock is
   // destroyed.
   AutoBuffer<std::shared_ptr<void>, StoreNObjects<10>> m_garbage;
   std::unique_lock<Mutex> m_lock;
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
      return !m_weakBlocker.expired();
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
      PDK_ASSERT(m_slotRefcount != 0);
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
      nolockGrabTrackedObjects(lock, NullOutputIterator());
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
      SlotBase::TrackedContainerType::const_iterator it;
      for(it = slot().tracked_objects().begin();
          it != slot().tracked_objects().end();
          ++it)
      {
         VoidSharedPtrVariant lockedObject
               (
                  std::visit
                  (
                     internal::LockWeakPtrVisitor(),
                     *it
                     )
                  );
         if(std::visit(internal::ExpiredWeakPtrVisitor(), *it))
         {
            nolockDisconnect(lock);
            return;
         }
         *inserter++ = lockedObject;
      }
   }
   
   // expose Lockable concept of mutex
   virtual void lock()
   {
      m_mutex->lock();
   }
   
   virtual void unlock()
   {
      m_mutex->unlock();
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
   virtual std::shared_ptr<void> releaseSlot() const
   {
      std::shared_ptr<void> releasedSlot = m_slot;
      m_slot.reset();
      return releasedSlot;
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
   Connection() {}
   Connection(const Connection &other)
      : m_weakConnectionBody(other.m_weakConnectionBody)
   {}
   
   Connection(const std::weak_ptr<internal::ConnectionBodyBase> &connectionBody):
      m_weakConnectionBody(connectionBody)
   {}
   
   // move support
   Connection(Connection &&other): m_weakConnectionBody(std::move(other.m_weakConnectionBody))
   {
      // make sure other is reset, in case it is a scoped_connection (so it
      // won't disconnect on destruction after being moved away from).
      other.m_weakConnectionBody.reset();
   }
   
   Connection & operator=(Connection && other)
   {
      if(&other == this) {
         return *this;
      }
      m_weakConnectionBody = std::move(other.m_weakConnectionBody);
      // make sure other is reset, in case it is a scoped_connection (so it
      // won't disconnect on destruction after being moved away from).
      other.m_weakConnectionBody.reset();
      return *this;
   }

   Connection & operator=(const Connection &other)
   {
      if(&other == this) {
         return *this;
      }
      m_weakConnectionBody = other.m_weakConnectionBody;
      return *this;
   }
   
   ~Connection() {}
   
   void disconnect() const
   {
      std::shared_ptr<internal::ConnectionBodyBase> connectionBody(m_weakConnectionBody.lock());
      if(connectionBody == 0) {
         return;
      }
      connectionBody->disconnect();
   }
   
   bool connected() const
   {
      std::shared_ptr<internal::ConnectionBodyBase> connectionBody(m_weakConnectionBody.lock());
      if(connectionBody == 0) {
         return false;
      }
      return connectionBody->connected();
   }
   
   bool blocked() const
   {
      std::shared_ptr<internal::ConnectionBodyBase> connectionBody(m_weakConnectionBody.lock());
      if(connectionBody == 0) {
         return true;
      }
      return connectionBody->blocked();
   }
   
   bool operator==(const Connection &other) const
   {
      std::shared_ptr<internal::ConnectionBodyBase> connectionBody(m_weakConnectionBody.lock());
      std::shared_ptr<internal::ConnectionBodyBase> otherConnectionBody(other.m_weakConnectionBody.lock());
      return connectionBody == otherConnectionBody;
   }
   
   bool operator!=(const Connection &other) const
   {
      return !(*this == other);
   }
   
   bool operator<(const Connection &other) const
   {
      std::shared_ptr<internal::ConnectionBodyBase> connectionBody(m_weakConnectionBody.lock());
      std::shared_ptr<internal::ConnectionBodyBase> otherConnectionBody(other.m_weakConnectionBody.lock());
      return connectionBody < otherConnectionBody;
   }
   
   void swap(Connection &other)
   {
      using std::swap;
      swap(m_weakConnectionBody, other.m_weakConnectionBody);
   }
   
   friend class shared_connection_block;
protected:
   std::weak_ptr<internal::ConnectionBodyBase> m_weakConnectionBody;
};

inline void swap(Connection &conn1, Connection &conn2)
{
   conn1.swap(conn2);
}

class scoped_connection: public Connection
{
public:
   scoped_connection() 
   {}
   
   scoped_connection(const Connection &other)
      : Connection(other)
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
   scoped_connection(scoped_connection && other)
      : Connection(std::move(other))
   {}
   
   scoped_connection(Connection &&other)
      : Connection(std::move(other))
   {}
   
   scoped_connection & operator=(scoped_connection && other)
   {
      if(&other == this) {
         return *this;
      }
      disconnect();
      Connection::operator=(std::move(other));
      return *this;
   }
   
   scoped_connection & operator=(Connection && other)
   {
      if(&other == this) {
         return *this;
      }
      disconnect();
      Connection::operator=(std::move(other));
      return *this;
   }
   
   Connection release()
   {
      Connection conn(m_weakConnectionBody);
      m_weakConnectionBody.reset();
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
