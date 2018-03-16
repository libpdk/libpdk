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

#ifndef PDK_KERNEL_CORE_EVENT_H
#define PDK_KERNEL_CORE_EVENT_H

#include "pdk/global/Global.h"

namespace pdk {

// forward declare class with namespace
namespace os {
namespace thread {
namespace internal {
class ThreadData;
} // internal
} // thread
} // os

namespace kernel {

// forward declare class
class Object;

// forward declare class with namespace
namespace internal {
class EventPrivate;
class CoreApplicationPrivate;
} // internal

class CoreApplication;
using pdk::os::thread::internal::ThreadData;

class PDK_CORE_EXPORT Event
{
public:
   enum class Type : int {
      None,
      Timer,
      Quit,
      DeferredDelete,
      ParentChange,
      ParentAboutToChange,
      ThreadChange,
      MetaCall,
      SocketActive,
      SocketClose,
      LanguageChange,
      ChildAdded,                        // new child widget
      ChildPolished,                     // polished child widget
      ChildRemoved,                      // deleted child widget
      UpdateRequest,                     // widget should be repainted
      UpdateLater,                       // request update() later
      User = 1000, // first user event id
      MaxUser = 65535
   };
   
   explicit Event(Type type);
   Event(const Event &other);
   virtual ~Event();
   Event &operator=(const Event &other);
   
   inline Type getType() const
   {
      return static_cast<Type>(m_type);
   }
   
   inline bool getSpontaneous() const
   {
      return m_spont;
   }
   
   inline void setAccepted(bool accepted)
   {
      m_accept = accepted;
   }
   
   inline bool isAccepted() const
   {
      return m_accept;
   }
   
   inline void accept()
   {
      m_accept = true;
   }
   
   inline void ignore()
   {
      m_accept = false;
   }
   
   static int registerEventType(int hint = -1) noexcept;
   
protected:
   internal::EventPrivate *m_implPtr;
   int m_type;
   
private:
   ushort m_posted : 1;
   ushort m_spont : 1;
   ushort m_accept : 1;
   ushort m_reserved : 13;
   
   friend class CoreApplication;
   friend class internal::CoreApplicationPrivate;
   friend class ThreadData;
   friend class Application;
   // needs this:
   PDK_ALWAYS_INLINE void setSpontaneous()
   {
      m_spont = true;
   }
};

class PDK_CORE_EXPORT TimerEvent : public Event
{
public:
   explicit TimerEvent(int timerId);
   ~TimerEvent();
   
   int getTimerId() const
   {
      return m_id;
   }
protected:
   int m_id;
};

class PDK_CORE_EXPORT ChildEvent : public Event
{
public:
   ChildEvent(Type type, Object *child);
   ~ChildEvent();
   
   Object *getChild() const
   {
      return m_child;
   }
   
   bool added() const
   {
      return getType() == Type::ChildAdded;
   }
   
   bool polished() const
   {
      return getType() == Type::ChildPolished;
   }
   
   bool removed() const
   {
      return getType() == Type::ChildRemoved;
   }
protected:
   Object *m_child;
};


class PDK_CORE_EXPORT DeferredDeleteEvent : public Event
{
public:
   explicit DeferredDeleteEvent();
   
   ~DeferredDeleteEvent();
   
   int getLoopLevel() const
   {
      return m_level;
   }
private:
   int m_level;
   friend class CoreApplication;
};

} // kernel
} // pdk

#endif // PDK_KERNEL_CORE_EVENT_H
