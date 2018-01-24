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
// Created by softboy on 2018/01/03.

#ifndef PDK_KERNEL_OBJECT_H
#define PDK_KERNEL_OBJECT_H

#include "pdk/global/Global.h"
#include "pdk/utils/ScopedPointer.h"
#include <list>

namespace pdk {

// forward declare with namespace
namespace os {
namespace thread {
class Thread;
} // thread
} // os

namespace kernel {

using ObjectList = std::list<Object *>;
using Thread = ::pdk::os::thread::Thread;
class Object;
class Event;
class TimerEvent;
class ChildEvent;

namespace internal {
class ObjectPrivate;
}

using internal::ObjectPrivate;

class PDK_CORE_EXPORT ObjectData
{
public:
   virtual ~ObjectData();
   Object *m_apiPtr;
   Object *m_parent;
   ObjectList m_children;
   uint wasDeleted : 1;
   uint isDeletingChildren : 1;
   uint sendChildEvents : 1;
   uint receiveChildEvents : 1;
   uint unused : 28;
   int postedEvents;
};

class Object
{
public:
   explicit Object(Object *parent = nullptr);
   virtual ~Object();
   
   virtual bool event(Event *event);
   virtual bool eventFilter(Object *watched, Event *event);
   
   Thread *thread() const;
   void moveToThread(Thread *thread);
   
   int startTimer(int interval/*, Qt::TimerType timerType = Qt::CoarseTimer*/);
   void killTimer(int id);
   inline Object *parent() const
   { 
      return m_implPtr->m_parent;
   }
   void deleteLater();
protected:
   virtual void timerEvent(TimerEvent *event);
   virtual void childEvent(ChildEvent *event);
   virtual void customEvent(Event *event);
   
protected:
   Object(ObjectPrivate &dd, Object *parent = nullptr);
   
   friend class Application;
   friend class ApplicationPrivate;
   friend class CoreApplication;
   friend class CoreApplicationPrivate;
   friend class ThreadData;
   
protected:
   pdk::utils::ScopedPointer<ObjectData> m_implPtr;
private:
   PDK_DECLARE_PRIVATE(Object);
   PDK_DISABLE_COPY(Object);
};

} // kernel
} // pdk

#endif // PDK_KERNEL_OBJECT_H
