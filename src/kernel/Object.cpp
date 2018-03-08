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
#include "pdk/kernel/CoreEvent.h"
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

static int DIRECT_CONNECTION_ONLY = 0;

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
   return true;
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
   return getImplPtr()->m_threadData->m_thread;
}

void Object::moveToThread(Thread *targetThread)
{
   PDK_D(Object);
   if (implPtr->m_threadData->m_thread == targetThread) {
      // object is already in this thread
      return;
   }
   if (implPtr->m_parent != nullptr) {
      warning_stream("Object::moveToThread: Cannot move objects with a parent");
      return;
   }
   if (implPtr->m_isWidget) {
      warning_stream("Object::moveToThread: Widgets cannot be moved to a new thread");
      return;
   }
   
   ThreadData *currentData = ThreadData::current();
   ThreadData *targetData = targetThread ? ThreadData::get(targetThread) : nullptr;
   if (implPtr->m_threadData->m_thread == nullptr && currentData == targetData) {
      // one exception to the rule: we allow moving objects with no thread affinity to the current thread
      currentData = implPtr->m_threadData;
   } else if (implPtr->m_threadData != currentData) {
      warning_stream("Object::moveToThread: Current thread (%p) is not the object's thread (%p).\n"
                     "Cannot move to target thread (%p)\n",
                     (void *)currentData->m_thread.load(), (void *)implPtr->m_threadData->m_thread.load(), targetData ? (void *)targetData->m_thread.load() : nullptr);
      
#ifdef PDK_OS_MAC
      warning_stream("You might be loading two sets of Qt binaries into the same process. "
                     "Check that all plugins are compiled against the right Qt binaries. Export "
                     "DYLD_PRINT_LIBRARIES=1 and check that only one set of binaries are being loaded.");
#endif
      return;
   }
   // prepare to move
   implPtr->moveToThreadHelper();
   if (!targetData) {
      targetData = new ThreadData(0);
   }
   OrderedMutexLocker locker(&currentData->m_postEventList.m_mutex,
                             &targetData->m_postEventList.m_mutex);
   
   // keep currentData alive (since we've got it locked)
   currentData->ref();
   // move the object
   getImplPtr()->setThreadDataHelper(currentData, targetData);
   locker.unlock();
   // now currentData can commit suicide if it wants to
   currentData->deref();
}

using pdk::kernel::Event;
using pdk::kernel::ChildEvent;
using pdk::os::thread::internal::PostEvent;

namespace internal {

void ObjectPrivate::moveToThreadHelper()
{
   PDK_Q(Object);
   Event event(Event::Type::ThreadChange);
   CoreApplication::sendEvent(apiPtr, &event);
   ObjectList::iterator iter = m_children.begin();
   ObjectList::iterator endMark = m_children.end();
   while (iter != endMark) {
      Object *child = *iter;
      child->getImplPtr()->moveToThreadHelper();
      ++iter;
   }
}

void ObjectPrivate::setThreadDataHelper(ThreadData *currentData, ThreadData *targetData)
{
   PDK_Q(Object);
   // move posted events
   int eventsMoved = 0;
   for (size_t i = 0; i < currentData->m_postEventList.size(); ++i) {
      const PostEvent &pe = currentData->m_postEventList.at(i);
      if (!pe.m_event) {
         continue;
      }
      if (pe.m_receiver == apiPtr) {
         // move this post event to the targetList
         targetData->m_postEventList.addEvent(pe);
         const_cast<PostEvent &>(pe).m_event = 0;
         ++eventsMoved;
      }
   }
   if (eventsMoved > 0 && targetData->m_eventDispatcher.load()) {
      targetData->m_canWait = false;
      targetData->m_eventDispatcher.load()->wakeUp();
   }
   
   // the current emitting thread shouldn't restore currentSender after calling moveToThread()
   if (m_currentSender) {
      m_currentSender->m_ref = 0;
   }
   m_currentSender = 0;
   // set new thread data
   targetData->ref();
   m_threadData->deref();
   m_threadData = targetData;
   ObjectList::iterator iter = m_children.begin();
   ObjectList::iterator endMark = m_children.end();
   while (iter != endMark) {
      Object *child = *iter;
      child->getImplPtr()->setThreadDataHelper(currentData, targetData);
      ++iter;
   }
}

void ObjectPrivate::reregisterTimers(void *pointer)
{
   PDK_Q(Object);
   std::list<AbstractEventDispatcher::TimerInfo> *timerList = reinterpret_cast<std::list<AbstractEventDispatcher::TimerInfo> *>(pointer);
   AbstractEventDispatcher *eventDispatcher = m_threadData->m_eventDispatcher.load();
   auto iter = timerList->begin();
   auto endMark = timerList->end();
   while (iter != endMark) {
      const AbstractEventDispatcher::TimerInfo &ti = *iter;
      eventDispatcher->registerTimer(ti.m_timerId, ti.m_interval, ti.m_timerType, apiPtr);
      ++iter;
   }
   delete timerList;
}

ObjectPrivate::Connection::~Connection()
{
   if (m_ownArgumentTypes) {
      const int *v = m_argumentTypes.load();
      if (v != &DIRECT_CONNECTION_ONLY) {
         delete [] v;
      }
   }
   if (m_isSlotObject) {
      m_slotObj->destroyIfLastRef();
   }
}

void ObjectPrivate::deleteChildren()
{
   PDK_ASSERT_X(!m_isDeletingChildren, "ObjectPrivate::deleteChildren()", "isDeletingChildren already set, did this function recurse?");
   m_isDeletingChildren = true;
   // delete children objects
   // don't use qDeleteAll as the destructor of the child might
   // delete siblings
   ObjectList::iterator iter = m_children.begin();
   ObjectList::iterator endMark = m_children.end();
   while (iter != endMark) {
      m_currentChildBeingDeleted = *iter;
      *iter = nullptr;
      delete m_currentChildBeingDeleted;
      ++iter;
   }
   m_children.clear();
   m_currentChildBeingDeleted = 0;
   m_isDeletingChildren = false;
}

void ObjectPrivate::setParentHelper(Object *object)
{
   PDK_Q(Object);
   if (object == m_parent) {
      return;
   } 
   if (m_parent) {
      ObjectPrivate *parentD = m_parent->getImplPtr();
      if (parentD->m_isDeletingChildren && m_wasDeleted
          && parentD->m_currentChildBeingDeleted == apiPtr) {
         // don't do anything since ObjectPrivate::deleteChildren() already
         // cleared our entry in parentD->m_children.
      } else {
         ObjectList::iterator iter = std::find(parentD->m_children.begin(),
                                               parentD->m_children.end(), apiPtr);
         if (parentD->m_isDeletingChildren) {
            *iter = 0;
         } else {
            parentD->m_children.erase(iter);
            if (m_sendChildEvents && parentD->m_receiveChildEvents) {
               ChildEvent event(Event::Type::ChildRemoved, apiPtr);
               CoreApplication::sendEvent(m_parent, &event);
            }
         }
      }
   }
   m_parent = object;
   if (m_parent) {
      // object hierarchies are constrained to a single thread
      if (m_threadData != m_parent->getImplPtr()->m_threadData) {
         warning_stream("Object::setParent: Cannot set parent, new parent is in a different thread");
         m_parent = nullptr;
         return;
      }
      m_parent->getImplPtr()->m_children.push_back(apiPtr);
      if(m_sendChildEvents && m_parent->getImplPtr()->m_receiveChildEvents) {
         if (!m_isWidget) {
            ChildEvent e(Event::Type::ChildAdded, apiPtr);
            CoreApplication::sendEvent(m_parent, &e);
         }
      }
   }
}

} // internal

int Object::startTimer(int interval, pdk::TimerType timerType)
{
   PDK_D(Object);
   if (PDK_UNLIKELY(interval < 0)) {
      warning_stream("Object::startTimer: Timers cannot have negative intervals");
      return 0;
   }
   if (PDK_UNLIKELY(!implPtr->m_threadData->m_eventDispatcher.load())) {
      warning_stream("Object::startTimer: Timers can only be used with threads started with QThread");
      return 0;
   }
   if (PDK_UNLIKELY(getThread() != Thread::getCurrentThread())) {
      warning_stream("Object::startTimer: Timers cannot be started from another thread");
      return 0;
   }
   int timerId = implPtr->m_threadData->m_eventDispatcher.load()->registerTimer(interval, timerType, this);
   if (!implPtr->m_extraData) {
      implPtr->m_extraData = new ObjectPrivate::ExtraData;
   }
   implPtr->m_extraData->m_runningTimers.push_back(timerId);
   return timerId;
}

void Object::killTimer(int id)
{
   PDK_D(Object);
   if (PDK_UNLIKELY(getThread() != Thread::getCurrentThread())) {
      warning_stream("Object::killTimer: Timers cannot be stopped from another thread");
      return;
   }
   if (id) {
      int at = -1;
      if (implPtr->m_extraData) {
         std::vector<int> &timers = implPtr->m_extraData->m_runningTimers;
         auto iter = std::find(timers.begin(), timers.end(), id);
         if (iter != timers.end()) {
            at = std::distance(timers.begin(), iter);
         }
      }
      if (at == -1) {
         // timer isn't owned by this object
         warning_stream("Object::killTimer(): Error: timer id %d is not valid for object %p (%s, %s), timer has not been killed",
                        id,
                        (void *)this,
                        typeid(this).name(),
                        pdk_printable(getObjectName()));
         return;
      }
      if (implPtr->m_threadData->m_eventDispatcher.load()) {
         implPtr->m_threadData->m_eventDispatcher.load()->unregisterTimer(id);
      }
      std::vector<int> &timers = implPtr->m_extraData->m_runningTimers;
      auto iter = timers.begin();
      std::advance(iter, at);
      timers.erase(iter);
      AbstractEventDispatcherPrivate::releaseTimerId(id);
   }
}

void Object::setParent(Object *parent)
{
   PDK_D(Object);
   PDK_ASSERT(!implPtr->m_isWidget);
   implPtr->setParentHelper(parent);
}

void Object::installEventFilter(Object *obj)
{
   PDK_D(Object);
   if (!obj) {
      return;
   }
   if (implPtr->m_threadData != obj->getImplPtr()->m_threadData) {
      warning_stream("Object::installEventFilter(): Cannot filter events for objects in a different thread.");
      return;
   }
   if (!implPtr->m_extraData) {
      implPtr->m_extraData = new ObjectPrivate::ExtraData;
   }
   // clean up unused items in the list
   implPtr->m_extraData->m_eventFilters.remove((Object*)0);
   implPtr->m_extraData->m_eventFilters.remove(obj);
   implPtr->m_extraData->m_eventFilters.push_front(obj);
}


void Object::removeEventFilter(Object *obj)
{
   PDK_D(Object);
   if (implPtr->m_extraData) {
      std::list<Pointer<Object>> &filters = implPtr->m_extraData->m_eventFilters;
      auto iter = filters.begin();
      auto endMark = filters.end();
      while (iter != endMark) {
         if (*iter == obj) {
            *iter = nullptr;
         }
         ++iter;
      }
   }
}

void Object::deleteLater()
{
   CoreApplication::postEvent(this, new DeferredDeleteEvent());
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

