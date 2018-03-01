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

#include "pdk/kernel/internal/CfSocketNotifierPrivate.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/SocketNotifier.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/global/Logging.h"

namespace pdk {
namespace kernel {
namespace internal {

using pdk::os::thread::Thread;

void mac_socket_callback(CFSocketRef s, CFSocketCallBackType callbackType, CFDataRef,
                         const void *, void *info)
{
   
   CFSocketNotifier *cfSocketNotifier = static_cast<CFSocketNotifier *>(info);
   int nativeSocket = CFSocketGetNative(s);
   MacSocketInfo *socketInfo = cfSocketNotifier->m_macSockets.at(nativeSocket);
   Event notifierEvent(Event::Type::SocketActive);
   
   // There is a race condition that happen where we disable the notifier and
   // the kernel still has a notification to pass on. We then get this
   // notification after we've successfully disabled the CFSocket, but our Qt
   // notifier is now gone. The upshot is we have to check the notifier
   // every time.
   if (callbackType == kCFSocketReadCallBack) {
      if (socketInfo->m_readNotifier && socketInfo->m_readEnabled) {
         socketInfo->m_readEnabled = false;
         CoreApplication::sendEvent(socketInfo->m_readNotifier, &notifierEvent);
      }
   } else if (callbackType == kCFSocketWriteCallBack) {
      if (socketInfo->m_writeNotifier && socketInfo->m_writeEnabled) {
         socketInfo->m_writeEnabled = false;
         CoreApplication::sendEvent(socketInfo->m_writeNotifier, &notifierEvent);
      }
   }
   
   if (cfSocketNotifier->m_maybeCancelWaitForMoreEvents) {
      cfSocketNotifier->m_maybeCancelWaitForMoreEvents(cfSocketNotifier->m_eventDispatcher);
   }
}

/*
    Adds a loop source for the given socket to the current run loop.
*/
CFRunLoopSourceRef mac_add_socket_to_runloop(const CFSocketRef socket)
{
   CFRunLoopSourceRef loopSource = CFSocketCreateRunLoopSource(kCFAllocatorDefault, socket, 0);
   if (!loopSource) {
      return nullptr;
   }
   CFRunLoopAddSource(CFRunLoopGetMain(), loopSource, kCFRunLoopCommonModes);
   return loopSource;
}

/*
    Removes the loop source for the given socket from the current run loop.
*/
void mac_remove_socket_from_runloop(const CFSocketRef socket, CFRunLoopSourceRef runloop)
{
   PDK_ASSERT(runloop);
   CFRunLoopRemoveSource(CFRunLoopGetMain(), runloop, kCFRunLoopCommonModes);
   CFSocketDisableCallBacks(socket, kCFSocketReadCallBack);
   CFSocketDisableCallBacks(socket, kCFSocketWriteCallBack);
}

CFSocketNotifier::CFSocketNotifier()
   : m_eventDispatcher(nullptr),
     m_maybeCancelWaitForMoreEvents(nullptr),
     m_enableNotifiersObserver(nullptr)
{}

CFSocketNotifier::~CFSocketNotifier()
{}

void CFSocketNotifier::setHostEventDispatcher(AbstractEventDispatcher *hostEventDispacher)
{
   m_eventDispatcher = hostEventDispacher;
}

void CFSocketNotifier::setMaybeCancelWaitForMoreEventsCallback(MaybeCancelWaitForMoreEventsFn callBack)
{
   m_maybeCancelWaitForMoreEvents = callBack;
}

void CFSocketNotifier::registerSocketNotifier(SocketNotifier *notifier)
{
   PDK_ASSERT(notifier);
   int nativeSocket = notifier->getSocket();
   SocketNotifier::Type type = notifier->getType();
#ifndef PDK_NO_DEBUG
   if (nativeSocket < 0 || nativeSocket > FD_SETSIZE) {
      warning_stream("SocketNotifier: Internal error");
      return;
   } else if (notifier->getThread() != m_eventDispatcher->getThread()
              || m_eventDispatcher->getThread() != Thread::getCurrentThread()) {
      warning_stream("SocketNotifier: socket notifiers cannot be enabled from another thread");
      return;
   }
#endif
   
   if (type == SocketNotifier::Type::Exception) {
      warning_stream("SocketNotifier::Exception is not supported on iOS");
      return;
   }
   
   // Check if we have a CFSocket for the native socket, create one if not.
   MacSocketInfo *socketInfo = m_macSockets.at(nativeSocket);
   if (!socketInfo) {
      socketInfo = new MacSocketInfo();
      
      // Create CFSocket, specify that we want both read and write callbacks (the callbacks
      // are enabled/disabled later on).
      const int callbackTypes = kCFSocketReadCallBack | kCFSocketWriteCallBack;
      CFSocketContext context = {0, this, 0, 0, 0};
      socketInfo->m_socket = CFSocketCreateWithNative(kCFAllocatorDefault, nativeSocket, callbackTypes, mac_socket_callback, &context);
      if (CFSocketIsValid(socketInfo->m_socket) == false) {
         warning_stream("EventDispatcherMac::registerSocketNotifier: Failed to create CFSocket");
         return;
      }
      
      CFOptionFlags flags = CFSocketGetSocketFlags(socketInfo->m_socket);
      // SocketNotifier doesn't close the socket upon destruction/invalidation
      flags &= ~kCFSocketCloseOnInvalidate;
      // Expicitly disable automatic re-enable, as we do that manually on each runloop pass
      flags &= ~(kCFSocketAutomaticallyReenableWriteCallBack | kCFSocketAutomaticallyReenableReadCallBack);
      CFSocketSetSocketFlags(socketInfo->m_socket, flags);
      m_macSockets[nativeSocket] = socketInfo;
   }
   
   if (type == SocketNotifier::Type::Read) {
      PDK_ASSERT(socketInfo->m_readNotifier == nullptr);
      socketInfo->m_readNotifier = notifier;
      socketInfo->m_readEnabled = false;
   } else if (type == SocketNotifier::Type::Write) {
      PDK_ASSERT(socketInfo->m_writeNotifier == nullptr);
      socketInfo->m_writeNotifier = notifier;
      socketInfo->m_writeEnabled = false;
   }
   
   if (!m_enableNotifiersObserver) {
      // Create a run loop observer which enables the socket notifiers on each
      // pass of the run loop, before any sources are processed.
      CFRunLoopObserverContext context = {};
      context.info = this;
      m_enableNotifiersObserver = CFRunLoopObserverCreate(kCFAllocatorDefault, kCFRunLoopBeforeSources,
                                                          true, 0, enableSocketNotifiers, &context);
      PDK_ASSERT(m_enableNotifiersObserver);
      CFRunLoopAddObserver(CFRunLoopGetMain(), m_enableNotifiersObserver, kCFRunLoopCommonModes);
   }
}

void CFSocketNotifier::unregisterSocketNotifier(SocketNotifier *notifier)
{
   PDK_ASSERT(notifier);
   int nativeSocket = notifier->getSocket();
   SocketNotifier::Type type = notifier->getType();
#ifndef PDK_NO_DEBUG
   if (nativeSocket < 0 || nativeSocket > FD_SETSIZE) {
      warning_stream("SocketNotifier: Internal error");
      return;
   } else if (notifier->getThread() != m_eventDispatcher->getThread() || 
              m_eventDispatcher->getThread() != Thread::getCurrentThread()) {
      warning_stream("SocketNotifier: socket notifiers cannot be disabled from another thread");
      return;
   }
#endif
   
   if (type == SocketNotifier::Type::Exception) {
      warning_stream("SocketNotifier::Exception is not supported on iOS");
      return;
   }
   MacSocketInfo *socketInfo = m_macSockets.at(nativeSocket);
   if (!socketInfo) {
      warning_stream("EventDispatcherMac::unregisterSocketNotifier: Tried to unregister a not registered notifier");
      return;
   }
   
   // Decrement read/write counters and disable callbacks if necessary.
   if (type == SocketNotifier::Type::Read) {
      PDK_ASSERT(notifier == socketInfo->m_readNotifier);
      socketInfo->m_readNotifier = nullptr;
      socketInfo->m_readEnabled = false;
      CFSocketDisableCallBacks(socketInfo->m_socket, kCFSocketReadCallBack);
   } else if (type == SocketNotifier::Type::Write) {
      PDK_ASSERT(notifier == socketInfo->m_writeNotifier);
      socketInfo->m_writeNotifier = 0;
      socketInfo->m_writeEnabled = false;
      CFSocketDisableCallBacks(socketInfo->m_socket, kCFSocketWriteCallBack);
   }
   
   // Remove CFSocket from runloop if this was the last SocketNotifier.
   if (socketInfo->m_readNotifier == 0 && socketInfo->m_writeNotifier == 0) {
      unregisterSocketInfo(socketInfo);
      delete socketInfo;
      m_macSockets.erase(nativeSocket);
   }
}

void CFSocketNotifier::removeSocketNotifiers()
{
   // Remove CFSockets from the runloop.
   for (auto &socketInfo : std::as_const(m_macSockets)) {
      unregisterSocketInfo(socketInfo.second);
      delete socketInfo.second;
   }
   m_macSockets.clear();
   destroyRunLoopObserver();
}

void CFSocketNotifier::destroyRunLoopObserver()
{
   if (!m_enableNotifiersObserver) {
      return;
   }
   CFRunLoopObserverInvalidate(m_enableNotifiersObserver);
   CFRelease(m_enableNotifiersObserver);
   m_enableNotifiersObserver = nullptr;
}

void CFSocketNotifier::unregisterSocketInfo(MacSocketInfo *socketInfo)
{
   if (socketInfo->m_runloop) {
      if (CFSocketIsValid(socketInfo->m_socket)) {
         mac_remove_socket_from_runloop(socketInfo->m_socket, socketInfo->m_runloop);
      }
      CFRunLoopSourceInvalidate(socketInfo->m_runloop);
      CFRelease(socketInfo->m_runloop);
   }
   CFSocketInvalidate(socketInfo->m_socket);
   CFRelease(socketInfo->m_socket);
}

void CFSocketNotifier::enableSocketNotifiers(CFRunLoopObserverRef ref, CFRunLoopActivity activity, void *info)
{
   PDK_UNUSED(ref);
   PDK_UNUSED(activity);
   
   const CFSocketNotifier *that = static_cast<CFSocketNotifier *>(info);
   
   for (auto &socketInfo : that->m_macSockets) {
      if (!CFSocketIsValid(socketInfo.second->m_socket)) {
         continue;
      }
      if (!socketInfo.second->m_runloop) {
         // Add CFSocket to runloop.
         if (!(socketInfo.second->m_runloop = mac_add_socket_to_runloop(socketInfo.second->m_socket))) {
            warning_stream("EventDispatcherMac::registerSocketNotifier: Failed to add CFSocket to runloop");
            CFSocketInvalidate(socketInfo.second->m_socket);
            continue;
         }
         
         // Apple docs say: "If a callback is automatically re-enabled,
         // it is called every time the condition becomes true ... If a
         // callback is not automatically re-enabled, then it gets called
         // exactly once, and is not called again until you manually
         // re-enable that callback by calling CFSocketEnableCallBacks()".
         // So, we don't need to enable callbacks on registering.
         socketInfo.second->m_readEnabled = (socketInfo.second->m_readNotifier != nullptr);
         if (!socketInfo.second->m_readEnabled) {
            CFSocketDisableCallBacks(socketInfo.second->m_socket, kCFSocketReadCallBack);
         }
         socketInfo.second->m_writeEnabled = (socketInfo.second->m_writeNotifier != nullptr);
         if (!socketInfo.second->m_writeEnabled) {
            CFSocketDisableCallBacks(socketInfo.second->m_socket, kCFSocketWriteCallBack);
         }
         continue;
      }
      
      if (socketInfo.second->m_readNotifier && !socketInfo.second->m_readEnabled) {
         socketInfo.second->m_readEnabled = true;
         CFSocketEnableCallBacks(socketInfo.second->m_socket, kCFSocketReadCallBack);
      }
      if (socketInfo.second->m_writeNotifier && !socketInfo.second->m_writeEnabled) {
         socketInfo.second->m_writeEnabled = true;
         CFSocketEnableCallBacks(socketInfo.second->m_socket, kCFSocketWriteCallBack);
      }
   }
}

} // internal
} // kernel
} // pdk
