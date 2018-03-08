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

#ifndef PDK_KERNEL_INTERNAL_EVENT_LOOP_PRIVATE_H
#define PDK_KERNEL_INTERNAL_EVENT_LOOP_PRIVATE_H

#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/base/os/thread/Atomic.h"

namespace pdk {
namespace kernel {

class EventLoop;

namespace internal {

using pdk::os::thread::BasicAtomicInt;
using pdk::os::thread::AtomicInt;
using pdk::kernel::EventLoop;

class EventLoopPrivate : public ObjectPrivate
{
public:
   inline EventLoopPrivate()
      : m_inExec(false)
   {
      m_returnCode.store(-1);
      m_exit.store(true);
   }
   
   AtomicInt m_quitLockRef;
   BasicAtomicInt m_exit; // bool
   BasicAtomicInt m_returnCode;
   bool m_inExec;
   
   void ref()
   {
      m_quitLockRef.ref();
   }
   
   void deref()
   {
      if (!m_quitLockRef.deref() && m_inExec) {
         PDK_RETRIEVE_APP_INSTANCE()->postEvent(m_apiPtr, new Event(Event::Type::Quit));
      }
   }
   
private:
   PDK_DECLARE_PUBLIC(EventLoop);
};

} // internal
} // kernel
} // pdk

#endif // PDK_KERNEL_INTERNAL_EVENT_LOOP_PRIVATE_H
