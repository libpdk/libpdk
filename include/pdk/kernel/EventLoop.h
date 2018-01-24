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

#ifndef PDK_KERNEL_EVENT_LOOP_H
#define PDK_KERNEL_EVENT_LOOP_H

#include "pdk/kernel/Object.h"

namespace pdk {
namespace kernel {

// forward declared class with namespace
namespace internal {
class EventLoopPrivate;
}

using internal::EventLoopPrivate;

class PDK_CORE_EXPORT EventLoop : public Object
{
public:
   enum ProcessEventsFlag
   {
      AllEvents = 0x00,
      ExcludeUserInputEvents = 0x01,
      ExcludeSocketNotifiers = 0x02,
      WaitForMoreEvents = 0x04,
      X11ExcludeTimers = 0x08,
      EventLoopExec = 0x20,
      DialogExec = 0x40
   };
   
   PDK_DECLARE_FLAGS(ProcessEventsFlags, ProcessEventsFlag);
public:
   explicit EventLoop(Object *parent = nullptr);
   ~EventLoop();
   bool processEvents(ProcessEventsFlags flags = AllEvents);
   void processEvents(ProcessEventsFlags flags, int maximumTime);
   int exec(ProcessEventsFlags flags = AllEvents);
   void exit(int returnCode = 0);
   bool isRunning() const;
   void wakeUp();
   bool event(Event *event) override;
   void quit();
private:
   PDK_DECLARE_PRIVATE(EventLoop);
};

PDK_DECLARE_OPERATORS_FOR_FLAGS(EventLoop::ProcessEventsFlags)

// forward declared class with namespace
namespace internal{
class EventLoopLockerPrivate;
}
using internal::EventLoopLockerPrivate;

class PDK_CORE_EXPORT EventLoopLocker
{
public:
   EventLoopLocker();
   explicit EventLoopLocker(EventLoop *loop);
   explicit EventLoopLocker(Thread *thread);
   ~EventLoopLocker();
   
private:
   PDK_DISABLE_COPY(EventLoopLocker);
   EventLoopLockerPrivate *m_implPtr;
};

} // kernel
} // pdk

#endif // PDK_KERNEL_EVENT_LOOP_H
