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
// Created by softboy on 2018/03/1.

#ifndef PDK_KERNEL_EVENT_DISPATCHER_CF_H
#define PDK_KERNEL_EVENT_DISPATCHER_CF_H

#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/internal/TimerInfoUnixPrivate.h"
#include "pdk/kernel/internal/CfSocketNotifierPrivate.h"
#include "pdk/kernel/internal/CoreMacPrivate.h"

PDK_FORWARD_DECLARE_OBJC_CLASS(PdkRunLoopModeTracker);

namespace pdk {
namespace kernel {

class EventDispatcherCoreFoundation;

template <class T = EventDispatcherCoreFoundation>
class RunLoopSource
{
public:
   using CallbackFunction = bool (T::*)();
   
   enum { kHighestPriority = 0 } RunLoopSourcePriority;
   
   RunLoopSource(T *delegate, CallbackFunction callback)
      : m_delegate(delegate), 
        m_callback(callback)
   {
      CFRunLoopSourceContext context = {};
      context.info = this;
      context.perform = RunLoopSource::process;
      
      m_source = CFRunLoopSourceCreate(kCFAllocatorDefault, kHighestPriority, &context);
      PDK_ASSERT(m_source);
   }
   
   ~RunLoopSource()
   {
      CFRunLoopSourceInvalidate(m_source);
      CFRelease(m_source);
   }
   
   void addToMode(CFStringRef mode, CFRunLoopRef runLoop = 0)
   {
      if (!runLoop)
         runLoop = CFRunLoopGetCurrent();
      
      CFRunLoopAddSource(runLoop, m_source, mode);
   }
   
   void signal() { CFRunLoopSourceSignal(m_source); }
   
private:
   static void process(void *info)
   {
      RunLoopSource *self = static_cast<RunLoopSource *>(info);
      ((self->m_delegate)->*(self->m_callback))();
   }
   
   T *m_delegate;
   CallbackFunction m_callback;
   CFRunLoopSourceRef m_source;
};


} // kernel
} // pdk

#endif // PDK_KERNEL_EVENT_DISPATCHER_CF_H
