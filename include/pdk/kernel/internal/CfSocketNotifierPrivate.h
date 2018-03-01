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
// Created by softboy on 2018/03/01.

#ifndef PDK_KERNEL_INTERNAL_CF_SOCKET_NOTIFIER_PRIVATE_H
#define PDK_KERNEL_INTERNAL_CF_SOCKET_NOTIFIER_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/HashFuncs.h"
#include <CoreFoundation/CoreFoundation.h>
#include <map>

namespace pdk {
namespace kernel {
namespace internal {

struct MacSocketInfo {
   MacSocketInfo()
      : m_socket(nullptr),
        m_runloop(nullptr),
        m_readNotifier(nullptr), 
        m_writeNotifier(nullptr),
        m_readEnabled(false),
        m_writeEnabled(false)
   {}
   
   CFSocketRef m_socket;
   CFRunLoopSourceRef m_runloop;
   Object *m_readNotifier;
   Object *m_writeNotifier;
   bool m_readEnabled;
   bool m_writeEnabled;
};
using MacSocketHash = std::map<int, MacSocketInfo *>;
using MaybeCancelWaitForMoreEventsFn = void (*)(AbstractEventDispatcher *hostEventDispacher);

// The CoreFoundationSocketNotifier class implements socket notifiers support using
// CFSocket for event dispatchers running on top of the Core Foundation run loop system.
// (currently Mac and iOS)
//
// The principal functions are registerSocketNotifier() and unregisterSocketNotifier().
//
// setHostEventDispatcher() should be called at startup.
// removeSocketNotifiers() should be called at shutdown.
//
class PDK_CORE_EXPORT CFSocketNotifier
{
public:
   CFSocketNotifier();
   ~CFSocketNotifier();
   void setHostEventDispatcher(AbstractEventDispatcher *hostEventDispacher);
   void setMaybeCancelWaitForMoreEventsCallback(MaybeCancelWaitForMoreEventsFn callBack);
   void registerSocketNotifier(SocketNotifier *notifier);
   void unregisterSocketNotifier(SocketNotifier *notifier);
   void removeSocketNotifiers();
   
private:
   void destroyRunLoopObserver();
   
   static void unregisterSocketInfo(MacSocketInfo *socketInfo);
   static void enableSocketNotifiers(CFRunLoopObserverRef ref, CFRunLoopActivity activity, void *info);
   
   MacSocketHash m_macSockets;
   AbstractEventDispatcher *m_eventDispatcher;
   MaybeCancelWaitForMoreEventsFn m_maybeCancelWaitForMoreEvents;
   CFRunLoopObserverRef m_enableNotifiersObserver;
   
   friend void mac_socket_callback(CFSocketRef, CFSocketCallBackType, CFDataRef, const void *, void *);
};

} // internal
} // kernel
} // pdk

#endif // PDK_KERNEL_INTERNAL_CF_SOCKET_NOTIFIER_PRIVATE_H
