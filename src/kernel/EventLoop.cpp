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

#include "pdk/kernel/internal/EventLoopPrivate.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/kernel/EventLoop.h"

namespace pdk {
namespace kernel {

EventLoop::EventLoop(Object *parent)
   : Object(*new EventLoopPrivate, parent)
{
}

EventLoop::~EventLoop()
{}

bool EventLoop::processEvents(ProcessEventsFlags flags)
{
}

int EventLoop::exec(ProcessEventsFlags flags)
{
}

void EventLoop::processEvents(ProcessEventsFlags flags, int maxTime)
{
}

void EventLoop::exit(int returnCode)
{
}

bool EventLoop::isRunning() const
{
}

void EventLoop::wakeUp()
{
}

bool EventLoop::event(Event *event)
{
}

void EventLoop::quit()
{
}

namespace internal {

class EventLoopLockerPrivate
{
public:
   explicit EventLoopLockerPrivate(EventLoopPrivate *loop)
      : m_loop(loop), 
        m_type(EventLoop)
   {
      
   }
   
   explicit EventLoopLockerPrivate(ThreadPrivate *thread)
      : m_thread(thread),
        m_type(Thread)
   {
   }
   
   explicit EventLoopLockerPrivate(CoreApplicationPrivate *app)
      : m_app(app),
        m_type(Application)
   {
   }
   
   ~EventLoopLockerPrivate()
   {
      
   }
   
private:
   union {
      EventLoopPrivate *m_loop;
      ThreadPrivate *m_thread;
      CoreApplicationPrivate *m_app;
   };
   enum Type {
      EventLoop,
      Thread,
      Application
   };
   const Type m_type;
};

} // internal

EventLoopLocker::EventLoopLocker()
  : m_implPtr(new EventLoopLockerPrivate(static_cast<CoreApplicationPrivate *>(ObjectPrivate::get(CoreApplication::instance()))))
{

}

EventLoopLocker::EventLoopLocker(EventLoop *loop)
  : m_implPtr(new EventLoopLockerPrivate(static_cast<EventLoopPrivate *>(ObjectPrivate::get(loop))))
{

}

EventLoopLocker::EventLoopLocker(Thread *thread)
  : m_implPtr(new EventLoopLockerPrivate(static_cast<ThreadPrivate *>(ObjectPrivate::get(thread))))
{

}

EventLoopLocker::~EventLoopLocker()
{
}

} // kernel
} // pdk
