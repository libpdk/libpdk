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
// Created by softboy on 2018/03/19.

#ifndef PDK_UNITTEST_OS_THREAD_EVENTLOOP_H
#define PDK_UNITTEST_OS_THREAD_EVENTLOOP_H

#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/EventLoop.h"
#include "pdk/kernel/Object.h"
#include "pdk/kernel/Pointer.h"
#include "pdk/kernel/CallableInvoker.h"
#include "pdk/base/os/thread/Thread.h"

namespace pdkunittest {

using pdk::kernel::Object;
using pdk::kernel::Pointer;
using pdk::kernel::EventLoop;
using pdk::kernel::TimerEvent;
using pdk::kernel::CoreApplication;
using pdk::os::thread::Thread;
using pdk::kernel::CallableInvoker;

class PDK_UNITTEST_EXPORT TestEventLoop : public Object
{
public:
   inline TestEventLoop(Object *aParent = nullptr)
      : Object(aParent),
        m_inLoop(false),
        m_timeout(false),
        m_timerId(-1),
        m_loop(nullptr) {}
   
   inline void enterLoopMSecs(int ms);
   
   inline void enterLoop(int secs)
   {
      enterLoopMSecs(secs * 1000);
   }
   
   inline void changeInterval(int secs)
   {
      killTimer(timerId);
      m_timerId = startTimer(secs * 1000);
   }
   
   inline bool getTimeout() const
   {
      return m_timeout;
   }
   
   inline static TestEventLoop &instance()
   {
      static Pointer<TestEventLoop> testLoop;
      if (testLoop.isNull()) {
         testLoop = new TestEventLoop(CoreApplication::getInstance());
      }
      return *static_cast<TestEventLoop *>(testLoop);
   }
   
   inline void exitLoop();
   
protected:
   inline void timerEvent(TimerEvent *e) override;
   
private:
   bool m_inLoop;
   bool m_timeout;
   int m_timerId;
   
   EventLoop *m_loop;
};

inline void TestEventLoop::enterLoopMSecs(int ms)
{
   PDK_ASSERT(!m_loop);
   EventLoop l;
   m_inLoop = true;
   m_timeout = false;
   m_timerId = startTimer(ms);
   m_loop = &l;
   l.exec();
   m_loop = nullptr;
}

inline void TestEventLoop::exitLoop()
{
   if (getThread() != Thread::getThread())
   {
      CallableInvoker::invokeAsync([&](){
         this->exitLoop();
      }, this);
      return;
   }
   if (m_timerId != -1) {
      killTimer(timerId);
   }
   timerId = -1;
   if (m_loop) {
      m_loop->exit();
   }
   m_inLoop = false;
}

inline void TestEventLoop::timerEvent(TimerEvent *event)
{
   if (event->getTimerId() != m_timerId) {
      return;
   }
   m_timeout = true;
   exitLoop();
}

} // pdkunittest

#endif // PDK_UNITTEST_OS_THREAD_EVENTLOOP_H
