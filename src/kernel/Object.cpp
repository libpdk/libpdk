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
// Created by softboy on 2018/01/28.

#include "pdk/kernel/Object.h"
#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/AbstractEventDispatcherPrivate.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/kernel/internal/ObjectDefsPrivate.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"
#include "pdk/base/os/thread/Semaphore.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/base/os/thread/internal/OrderedMutexLockerPrivate.h"
#include "pdk/global/internal/HooksPrivate.h"
#include "pdk/global/Logging.h"
#include "pdk/stdext/utility/Algorithms.h"
#include "pdk/utils/SharedPointer.h"

#include <utility>
#include <memory>
#include <set>
#include <new>
#include <ctype.h>
#include <limits.h>

namespace pdk {
namespace kernel {

ObjectData::~ObjectData()
{}

namespace internal {

ObjectPrivate::ObjectPrivate(int version)
   : m_currentChildBeingDeleted(0),
     m_threadData(0)
{
   PDK_UNUSED(version);
   m_apiPtr = nullptr;
   m_parent = nullptr;                                 // no parent yet. It is set by setParent()
   m_isWidget = false;                           // assume not a widget object
   m_wasDeleted = false;                         // double-delete catcher
   m_isDeletingChildren = false;                 // set by deleteChildren()
   m_sendChildEvents = true;                     // if we should send ChildAdded and ChildRemoved events to parent
   m_receiveChildEvents = true;
   m_postedEvents = 0;
   m_extraData = 0;
   m_isWindow = false;
   m_deleteLaterCalled = false;
}

ObjectPrivate::~ObjectPrivate()
{
   if (m_extraData && !m_extraData->m_runningTimers.empty()) {
      if (PDK_LIKELY(m_threadData->m_thread == Thread::getCurrentThread())) {
         // unregister pending timers
         if (m_threadData->m_eventDispatcher.load()) {
            m_threadData->m_eventDispatcher.load()->unregisterTimers(m_apiPtr);
         }
         // release the timer ids back to the pool
         for (size_t i = 0; i < m_extraData->m_runningTimers.size(); ++i) {
            AbstractEventDispatcherPrivate::releaseTimerId(m_extraData->m_runningTimers.at(i));
         }
      } else {
         warning_stream("Object::~Object: Timers cannot be stopped from another thread");
      }
   }
   if (m_postedEvents) {
      CoreApplication::removePostedEvents(m_apiPtr, Event::Type::None);
   }
   m_threadData->deref();
   
#ifndef PDK_NO_USERDATA
   if (m_extraData)
      pdk::stdext::delete_all(m_extraData->m_userData);
#endif
   delete m_extraData;
}

void delete_in_event_handler(Object *object)
{
   delete object;
}

class ObjectConnectionListVector : public std::vector<ObjectPrivate::ConnectionList>
{
public:
   bool m_orphaned; //the Object owner of this vector has been destroyed while the vector was inUse
   bool m_dirty; //some Connection have been disconnected (their receiver is 0) but not removed from the list yet
   int m_inUse; //number of functions that are currently accessing this object or its connections
   ObjectPrivate::ConnectionList m_allsignals;
   
   ObjectConnectionListVector()
      : std::vector<ObjectPrivate::ConnectionList>(),
        m_orphaned(false),
        m_dirty(false),
        m_inUse(0)
   {}
   
   ObjectPrivate::ConnectionList &operator[](int at)
   {
      if (at < 0) {
         return m_allsignals;
      }
      return std::vector<ObjectPrivate::ConnectionList>::operator[](at);
   }
};

} // internal

namespace {

bool check_parent_thread(Object *parent,
                         ThreadData *parentThreadData,
                         ThreadData *currentThreadData)
{
   if (parent && parentThreadData != currentThreadData) {
      Thread *parentThread = parentThreadData->m_thread;
      Thread *currentThread = currentThreadData->m_thread;
      warning_stream("Object: Cannot create children for a parent that is in a different thread.\n"
                     "(Parent is %s(%p), parent's thread is %s(%p), current thread is %s(%p)",
                     typeid(parent).name(),
                     (void *)parent,
                     parentThread ? typeid(parentThread).name() : "Thread",
                     (void *)parentThread,
                     currentThread ? typeid(currentThread).name() : "Thread",
                     (void *)currentThread);
      return false;
   }
   return true;
}

} // anonymous namespace

static std::mutex sg_ObjectMutexPool[131];

namespace {

inline std::mutex &signal_slot_lock(const Object *object)
{
   return *static_cast<std::mutex *>(&sg_ObjectMutexPool[
                                     uint(pdk::uintptr(object)) % sizeof(sg_ObjectMutexPool) / sizeof(std::mutex)]);
}

} // anonymous namespace

using pdk::os::thread::internal::OrderedMutexLocker;

Object::Object(Object *parent)
   : m_implPtr(new ObjectPrivate)
{
   PDK_D(Object);
   implPtr->m_apiPtr = this;
   implPtr->m_threadData = (parent && !parent->getThread()) ? parent->getImplPtr()->m_threadData : ThreadData::current();
   implPtr->m_threadData->ref();
   if (parent) {
      try {
         if (!check_parent_thread(parent, parent ? parent->getImplPtr()->m_threadData : 0, implPtr->m_threadData)){
            parent = nullptr;
         }
         setParent(parent);
      } catch(...) {
         implPtr->m_threadData->deref();
         throw;
      }
   }
   if (PDK_UNLIKELY(pdk::sg_pdkHookData[pdk::hooks::AddObject])) {
      reinterpret_cast<pdk::hooks::AddObjectCallback>(pdk::sg_pdkHookData[pdk::hooks::AddObject])(this);
   }
}

Object::Object(ObjectPrivate &dd, Object *parent)
   : m_implPtr(&dd)
{
   PDK_D(Object);
   implPtr->m_apiPtr = this;
   implPtr->m_threadData = (parent && !parent->getThread()) ? parent->getImplPtr()->m_threadData : ThreadData::current();
   implPtr->m_threadData->ref();
   if (parent) {
      try {
         if (!check_parent_thread(parent, parent ? parent->getImplPtr()->m_threadData : 0, implPtr->m_threadData)) {
            parent = nullptr;
         }
         if (implPtr->m_isWidget) {
            if (parent) {
               implPtr->m_parent = parent;
               implPtr->m_parent->getImplPtr()->m_children.push_back(this);
            }
            // no events sent here, this is done at the end of the Widget constructor
         } else {
            setParent(parent);
         }
      } catch(...) {
         implPtr->m_threadData->deref();
         throw;
      }
   }
   if (PDK_UNLIKELY(pdk::sg_pdkHookData[hooks::AddObject])) {
      reinterpret_cast<pdk::hooks::AddObjectCallback>(pdk::sg_pdkHookData[pdk::hooks::AddObject])(this);
   }
}

Object::~Object()
{
   PDK_D(Object);
   implPtr->m_wasDeleted = true;
   implPtr->m_blockSig = 0; // unblock signals so we always emit destroyed()
   ExternalRefCountData *sharedRefcount = implPtr->m_sharedRefcount.load();
   if (sharedRefcount) {
      if (sharedRefcount->m_strongRef.load() > 0) {
         warning_stream("Object: shared Object was deleted directly. The program is malformed and may crash.");
         // but continue deleting, it's too late to stop anyway
      }
      // indicate to all WeakPointers that this Object has now been deleted
      sharedRefcount->m_strongRef.store(0);
      if (!sharedRefcount->m_weakRef.deref()) {
         delete sharedRefcount;
      }
   }
   // @TODO check signal
   //   if (!implPtr->m_isWidget && implPtr->isSignalConnected(0)) {
   //      emit destroyed(this);
   //   }   
   // set ref to zero to indicate that this object has been deleted
   if (implPtr->m_currentSender != nullptr) {
      implPtr->m_currentSender->m_ref = 0;
   }
   implPtr->m_currentSender = nullptr;
   if (implPtr->m_connectionLists || implPtr->m_senders) {
      std::mutex &signalSlotMutex = signal_slot_lock(this);
      std::unique_lock<std::mutex> locker(signalSlotMutex);
      // disconnect all receivers
      if (implPtr->m_connectionLists) {
         ++implPtr->m_connectionLists->m_inUse;
         int connectionListsCount = implPtr->m_connectionLists->size();
         for (int signal = -1; signal < connectionListsCount; ++signal) {
            ObjectPrivate::ConnectionList &connectionList =
                  (*implPtr->m_connectionLists)[signal];
            
            while (ObjectPrivate::Connection *c = connectionList.m_first) {
               if (!c->m_receiver) {
                  connectionList.m_first = c->m_nextConnectionList;
                  c->deref();
                  continue;
               }
               
               std::mutex &m = signal_slot_lock(c->m_receiver);
               bool needToUnlock = OrderedMutexLocker::relock(&signalSlotMutex, &m);
               if (c->m_receiver) {
                  *c->m_prev = c->m_next;
                  if (c->m_next) {
                     c->m_next->m_prev = c->m_prev;
                  }
               }
               c->m_receiver = nullptr;
               if (needToUnlock) {
                  m.unlock();
               }
               connectionList.m_first = c->m_nextConnectionList;
               // The destroy operation must happen outside the lock
               if (c->m_isSlotObject) {
                  c->m_isSlotObject = false;
                  locker.unlock();
                  c->m_slotObj->destroyIfLastRef();
                  locker.lock();
               }
               c->deref();
            }
         }
         
         if (!--implPtr->m_connectionLists->m_inUse) {
            delete implPtr->m_connectionLists;
         } else {
            implPtr->m_connectionLists->m_orphaned = true;
         }
         implPtr->m_connectionLists = nullptr;
      }
      
      // Disconnect all senders:
      // This loop basically just does
      // for (node = implPtr->m_senders; node; node = node->m_next) { ... }
      //
      // We need to temporarily unlock the receiver mutex to destroy the functors or to lock the
      // sender's mutex. And when the mutex is released, node->next might be destroyed by another
      // thread. That's why we set node->prev to &node, that way, if node is destroyed, node will
      // be updated.
      ObjectPrivate::Connection *node = implPtr->m_senders;
      while (node) {
         Object *sender = node->m_sender;
         // Send disconnectNotify before removing the connection from sender's connection list.
         // This ensures any eventual destructor of sender will block on getting receiver's lock
         // and not finish until we release it.
         //sender->disconnectNotify(QMetaObjectPrivate::signal(sender->metaObject(), node->signal_index));
         std::mutex &m = signal_slot_lock(sender);
         node->m_prev = &node;
         bool needToUnlock = OrderedMutexLocker::relock(&signalSlotMutex, &m);
         //the node has maybe been removed while the mutex was unlocked in relock?
         if (!node || node->m_sender != sender) {
            // We hold the wrong mutex
            PDK_ASSERT(needToUnlock);
            m.unlock();
            continue;
         }
         node->m_receiver = nullptr;
         internal::ObjectConnectionListVector *senderLists = sender->getImplPtr()->m_connectionLists;
         if (senderLists) {
            senderLists->m_dirty = true;
         }
         internal::SlotObjectBase *slotObj = nullptr;
         if (node->m_isSlotObject) {
            slotObj = node->m_slotObj;
            node->m_isSlotObject = false;
         }
         
         node = node->m_next;
         if (needToUnlock) {
            m.unlock();
         }
         if (slotObj) {
            if (node) {
               node->m_prev = &node;
            }
            locker.unlock();
            slotObj->destroyIfLastRef();
            locker.lock();
         }
      }
   }
   
   if (!implPtr->m_children.empty()) {
      implPtr->deleteChildren();
   }
   if (PDK_UNLIKELY(sg_pdkHookData[hooks::RemoveObject])) {
      reinterpret_cast<hooks::RemoveObjectCallback>(sg_pdkHookData[hooks::RemoveObject])(this);
   }
   if (implPtr->m_parent) {      // remove it from parent object
      implPtr->setParentHelper(nullptr);
   }
}

String Object::getObjectName() const
{
   PDK_D(const Object);
   return implPtr->m_extraData ? implPtr->m_extraData->m_objectName : String();
}

void Object::setObjectName(const String &name)
{
   PDK_D(Object);
   if (!implPtr->m_extraData) {
      implPtr->m_extraData = new internal::ObjectPrivate::ExtraData;
   }
   if (implPtr->m_extraData->m_objectName != name) {
      implPtr->m_extraData->m_objectName = name;
      // emit objectNameChanged
   }
}

bool Object::event(Event *e)
{
}

void Object::timerEvent(TimerEvent *)
{
}

void Object::childEvent(ChildEvent * /* event */)
{
}

void Object::customEvent(Event * /* event */)
{
}

bool Object::eventFilter(Object * /* watched */, Event * /* event */)
{
   return false;
}

Thread *Object::getThread() const
{
}

void Object::moveToThread(Thread *targetThread)
{
}

int Object::startTimer(int interval, pdk::TimerType timerType)
{
}

void Object::killTimer(int id)
{
   
}

void Object::setParent(Object *parent)
{
}

void Object::installEventFilter(Object *obj)
{
}


void Object::removeEventFilter(Object *obj)
{
}

void Object::deleteLater()
{
}

#ifndef PDK_NO_USERDATA
uint Object::registerUserData()
{
   static int user_data_registration = 0;
   return user_data_registration++;
}

ObjectUserData::~ObjectUserData()
{}

void Object::setUserData(uint id, ObjectUserData *data)
{
   PDK_D(Object);
   if (!implPtr->m_extraData) {
      implPtr->m_extraData = new ObjectPrivate::ExtraData;
   }
   if (implPtr->m_extraData->m_userData.size() <= id) {
      implPtr->m_extraData->m_userData.resize((int) id + 1);
   }
   implPtr->m_extraData->m_userData[id] = data;
}

ObjectUserData *Object::getUserData(uint id) const
{
   PDK_D(const Object);
   if (!implPtr->m_extraData) {
      return nullptr;
   }
   if (id < implPtr->m_extraData->m_userData.size()) {
      return implPtr->m_extraData->m_userData.at(id);
   }
   return 0;
}

#endif // PDK_NO_USERDATA

} // kernel
} // pdk

