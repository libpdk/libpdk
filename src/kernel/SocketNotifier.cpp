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
}

pdk::intptr SocketNotifier::getSocket() const
{
}

SocketNotifier::Type SocketNotifier::getType() const
{
}

bool SocketNotifier::isEnabled() const
{
}

void SocketNotifier::setEnabled(bool enable)
{
   
}

bool SocketNotifier::event(Event *e)
{
}

} // kernel
} // pdk
