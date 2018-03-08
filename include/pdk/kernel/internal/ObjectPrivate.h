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
// Created by softboy on 2018/01/23.

#ifndef PDK_KERNEL_INTERNAL_OBJECT_PRIVATE_H
#define PDK_KERNEL_INTERNAL_OBJECT_PRIVATE_H

#include "pdk/kernel/Object.h"
#include "pdk/kernel/Pointer.h"
#include "pdk/kernel/signal/Signal.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/os/thread/Atomic.h"
#include <vector>
#include <list>
#include <any>
#include <string>

namespace pdk {

// forward declare with namespace
namespace os {
namespace thread {
namespace internal {
class ThreadData;
} // internal
} // thread
} // os

namespace utils {
namespace internal {
namespace sharedptr {
struct ExternalRefCountData;
} // sharedptr
} // internal
} // utils

namespace kernel {

using pdk::utils::internal::sharedptr::ExternalRefCountData;

namespace internal {

using pdk::kernel::signal::Signal;
using pdk::os::thread::internal::ThreadData;
using pdk::ds::ByteArray;
using pdk::os::thread::AtomicPointer;
using pdk::os::thread::AtomicInt;
using pdk::kernel::Object;
// forward declare
class SlotObjectBase;
class ObjectConnectionListVector;

PDK_CORE_EXPORT void delete_in_event_handler(Object *);

class PDK_CORE_EXPORT ObjectPrivate : public ObjectData
{
   PDK_DECLARE_PUBLIC(Object);
   
public:
   ObjectPrivate(int version = PDK_VERSION);
   virtual ~ObjectPrivate();
   
public:
   struct ExtraData
   {
      ExtraData() {}
#ifndef PDK_NO_USERDATA
      std::vector<ObjectUserData *> m_userData;
#endif
      std::list<ByteArray> m_propertyNames;
      std::vector<std::any> m_propertyValues;
      std::vector<int> m_runningTimers;
      std::list<Pointer<Object>> m_eventFilters;
      String m_objectName;
   };
   using StaticMetaCallFunc = void (*)(Object *, int, void **);
   struct Connection
   {
      Object *m_sender;
      Object *m_receiver;
      union {
         StaticMetaCallFunc m_callFunc;
         SlotObjectBase *m_slotObj;
      };
      // The next pointer for the singly-linked ConnectionList
      Connection *m_nextConnectionList;
      //senders linked list
      Connection *m_next;
      Connection **m_prev;
      AtomicPointer<const int> m_argumentTypes;
      AtomicInt m_ref;
      ushort m_connectionType : 3; // 0 == auto, 1 == direct, 2 == queued, 4 == blocking
      ushort m_isSlotObject : 1;
      ushort m_ownArgumentTypes : 1;
      Connection()
         : m_nextConnectionList(0),
           m_ref(2),
           m_ownArgumentTypes(true)
      {
         //ref_ is 2 for the use in the internal lists
      }
      ~Connection();
      void ref()
      {
         m_ref.ref();
      }
      
      void deref()
      {
         if (!m_ref.deref()) {
            PDK_ASSERT(!m_receiver);
            delete this;
         }
      }
   };
   
   // ConnectionList is a singly-linked list
   struct ConnectionList
   {
      ConnectionList() 
         : m_first(nullptr),
           m_last(nullptr)
      {}
      
      Connection *m_first;
      Connection *m_last;
   };
   
   struct Sender
   {
      Object *m_sender;
      int m_signal;
      int m_ref;
   };
   
   static ObjectPrivate *get(Object *object)
   {
      return object->getImplPtr();
   }
   
   static const ObjectPrivate *get(const Object *object)
   {
      return object->getImplPtr();
   }
   
   void setParentHelper(Object *);
   void deleteChildren();
   void moveToThreadHelper();
   void setThreadDataHelper(ThreadData *currentData, ThreadData *targetData);
   void reregisterTimers(void *pointer);
   
   bool isSender(const Object *receiver, const char *signal) const;
   ObjectList receiverList(const char *signal) const;
   ObjectList senderList() const;
   
   void addConnection(int signal, Connection *connection);
   void cleanConnectionLists();
   
   static inline Sender *setCurrentSender(Object *receiver, Sender *sender);
   static inline void resetCurrentSender(Object *receiver, Sender *currentSender, Sender *previousSender);
public:
   union {
      Object *m_currentChildBeingDeleted;
   };
   
   ObjectConnectionListVector *m_connectionLists;
   Connection *m_senders;     // linked list of connections connected to this object
   Sender *m_currentSender;
   ExtraData *m_extraData;
   ThreadData *m_threadData;
   // these objects are all used to indicate that a Object was deleted
   // plus Pointer, which keeps a separate list
   AtomicPointer<ExternalRefCountData> m_sharedRefcount;
};

} // internal
} // kernel
} // pdk

#endif // PDK_KERNEL_INTERNAL_OBJECT_PRIVATE_H
