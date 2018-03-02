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

#include "pdk/kernel/EventDispatcherCf.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"
#include "pdk/base/lang/String.h"
#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/kernel/internal/CoreUnixPrivate.h"
#include "pdk/global/Logging.h"
#include <limits>

#ifdef PDK_OS_OSX
#  include <AppKit/NSApplication.h>
#else
#  include <UIKit/UIApplication.h>
#endif

@interface PdkRunLoopModeTracker : NSObject
{
   std::stack<CFStringRef> m_runLoopModes;
}
@end

PDK_NAMESPACE_ALIAS_OBJC_CLASS(PdkRunLoopModeTracker)

using pdk::lang::String;
using pdk::MessageLogger;

@implementation PdkRunLoopModeTracker

- (id) init
{
   if (self = [super init]) {
      m_runLoopModes.push(kCFRunLoopDefaultMode);
      [[NSNotificationCenter defaultCenter]
            addObserver:self
                        selector:@selector(receivedNotification:)
       name:nil
#ifdef PDK_OS_OSX
       object:[NSApplication sharedApplication]];
#else
       // Use performSelector so this can work in an App Extension
       object:[[UIApplication class] performSelector:@selector(sharedApplication)]];
#endif
   }
   return self;
}

- (void) dealloc
{
   [[NSNotificationCenter defaultCenter] removeObserver:self];
   [super dealloc];
}

namespace {

CFStringRef runLoopMode(NSDictionary *dictionary)
{
   for (NSString *key in dictionary) {
      if (CFStringHasSuffix((CFStringRef)key, CFSTR("RunLoopMode"))) {
         return (CFStringRef)[dictionary objectForKey: key];
      } 
   }
   return nil;
}

} // anonymous namespace

- (void) receivedNotification:(NSNotification *) notification
{
   if (CFStringHasSuffix((CFStringRef)notification.name, CFSTR("RunLoopModePushNotification"))) {
      if (CFStringRef mode = runLoopMode(notification.userInfo)) {
         m_runLoopModes.push(mode);
      } else {
         warning_stream("Encountered run loop push notification without run loop mode!");
      }
   } else if (CFStringHasSuffix((CFStringRef)notification.name, CFSTR("RunLoopModePopNotification"))) {
      CFStringRef mode = runLoopMode(notification.userInfo);
      if (CFStringCompare(mode, [self currentMode], 0) == kCFCompareEqualTo) {
         m_runLoopModes.pop();
      } else {
         warning_stream("Tried to pop run loop mode '%s' that was never pushed!", pdk_printable(String::fromCFString(mode)));
      }
      PDK_ASSERT(m_runLoopModes.size() >= 1);
   }
}

- (CFStringRef) currentMode
{
    return m_runLoopModes.top();
}
@end
