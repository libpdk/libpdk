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

#include "pdk/kernel/SocketNotifier.h"
#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"
#include "pdk/global/Logging.h"
#include "pdk/kernel/CallableInvoker.h"

namespace pdk {
namespace kernel {

namespace internal {

class SocketNotifierPrivate : public ObjectPrivate
{
   PDK_DECLARE_PUBLIC(SocketNotifier);
public:
   pdk::intptr m_sockfd;
   SocketNotifier::Type m_sntype;
   bool m_snenabled;
};

} // internal

using internal::SocketNotifierPrivate;
using pdk::os::thread::internal::ThreadData;

SocketNotifier::SocketNotifier(pdk::intptr socket, Type type, Object *parent)
   : Object(*new SocketNotifierPrivate, parent)
{
   PDK_D(SocketNotifier);
   implPtr->m_sockfd = socket;
   implPtr->m_sntype = type;
   implPtr->m_snenabled = true;
   
   if (socket < 0) {
      warning_stream("SocketNotifier: Invalid socket specified");
   } else if (!implPtr->m_threadData->m_eventDispatcher.load()) {
      warning_stream("SocketNotifier: Can only be used with threads started with Thread");
   } else {
      implPtr->m_threadData->m_eventDispatcher.load()->registerSocketNotifier(this);
   }
   
}

SocketNotifier::~SocketNotifier()
{
   setEnabled(false);
}

pdk::intptr SocketNotifier::getSocket() const
{
   PDK_D(const SocketNotifier);
   return implPtr->m_sockfd;
}

SocketNotifier::Type SocketNotifier::getType() const
{
   PDK_D(const SocketNotifier);
   return implPtr->m_sntype;
}

bool SocketNotifier::isEnabled() const
{
   PDK_D(const SocketNotifier);
   return implPtr->m_snenabled;
}

void SocketNotifier::setEnabled(bool enable)
{
   PDK_D(SocketNotifier);
   if (implPtr->m_sockfd < 0) {
      return;
   }
   if (implPtr->m_snenabled == enable) {                       // no change
      return;
   }
   implPtr->m_snenabled = enable;
   if (!implPtr->m_threadData->m_eventDispatcher.load()) {// perhaps application/thread is shutting down
      return;
   }
   if (PDK_UNLIKELY(getThread() != Thread::getCurrentThread())) {
      warning_stream("SocketNotifier: Socket notifiers cannot be enabled or disabled from another thread");
      return;
   }
   if (implPtr->m_snenabled) {
      implPtr->m_threadData->m_eventDispatcher.load()->registerSocketNotifier(this);
   } else {
      implPtr->m_threadData->m_eventDispatcher.load()->unregisterSocketNotifier(this);
   }
}

bool SocketNotifier::event(Event *event)
{
   PDK_D(SocketNotifier);
   // Emits the activated() signal when a Event::SockAct or Event::SockClose is
   // received.
   if (event->getType() == Event::Type::ThreadChange) {
      if (implPtr->m_snenabled) {
         CallableInvoker::invokeAsync(this, &SocketNotifier::setEnabled, implPtr->m_snenabled);
         setEnabled(false);
      }
   }
   Object::event(event);                        // will activate filters
   if ((event->getType() == Event::Type::SocketActive) || (event->getType() == Event::Type::SocketClose)) {
      emitActivatedSignal(implPtr->m_sockfd);
      return true;
   }
   return false;
}

} // kernel
} // pdk
