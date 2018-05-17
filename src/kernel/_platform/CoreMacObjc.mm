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
// Created by softboy on 2018/02/27.

#include "pdk/kernel/internal/CoreMacPrivate.h"
#include "pdk/base/lang/String.h"
#include "pdk/global/Logging.h"

#ifdef PDK_OS_OSX
#include <AppKit/NSText.h>
#include <CoreFoundation/CFString.h>
#endif

#include "pdk/base/io/Debug.h"

CFStringRef sd;
using pdk::io::Debug;
using pdk::lang::String;
using pdk::lang::Character;

namespace pdk {
namespace io {
Debug operator<<(Debug dbg, const NSObject *nsObject)
{
   return dbg << (nsObject ?
                     dbg.verbosity() > 2 ?
                        nsObject.debugDescription.UTF8String :
                        nsObject.description.UTF8String
                      : "NSObject(0x0)");
}

Debug operator<<(Debug dbg, CFStringRef stringRef)
{
   if (!stringRef) {
      return dbg << "CFStringRef(0x0)";
   }
   if (const UniChar *chars = CFStringGetCharactersPtr(stringRef)) {
      dbg << String::fromRawData(reinterpret_cast<const Character *>(chars), CFStringGetLength(stringRef));
   } else {
      dbg << String::fromCFString(stringRef);
   }
   return dbg;
}

} // io

// Prevents breaking the ODR in case we introduce support for more types
// later on, and lets the user override our default Debug operators.
#define PDK_DECLARE_WEAK_DEBUG_OPERATOR_FOR_CF_TYPE(CFType) \
   __attribute__((weak)) PDK_DECLARE_DEBUG_OPERATOR_FOR_CF_TYPE(CFType)

} // pdk

PDK_FOR_EACH_CORE_FOUNDATION_TYPE(PDK_DECLARE_WEAK_DEBUG_OPERATOR_FOR_CF_TYPE)
PDK_FOR_EACH_MUTABLE_CORE_FOUNDATION_TYPE(PDK_DECLARE_WEAK_DEBUG_OPERATOR_FOR_CF_TYPE)

using pdk::kernel::MacAutoReleasePool;
using pdk::MessageLogger;

@interface PdkMacAutoReleasePoolTracker : NSObject
{
   NSAutoreleasePool **m_pool;
}
-(id)initWithPool:(NSAutoreleasePool**)pool;
@end
@implementation PdkMacAutoReleasePoolTracker
-(id)initWithPool:(NSAutoreleasePool**)pool
{
   if (self = [super init])
      m_pool = pool;
   return self;
}
-(void)dealloc
{
   if (*m_pool) {
      // The pool is still valid, which means we're not being drained from
      // the corresponding MacAutoReleasePool (see below).
      
      // MacAutoReleasePool has only a single member, the NSAutoreleasePool*
      // so the address of that member is also the MacAutoReleasePool itself.
      MacAutoReleasePool *pool = reinterpret_cast<MacAutoReleasePool *>(m_pool);
      warning_stream() << "Premature drain of" << pool << "This can happen if you've allocated"
                       << "the pool on the heap, or as a member of a heap-allocated object. This is not a"
                       << "supported use of MacAutoReleasePool, and might result in crashes when objects"
                       << "in the pool are deallocated and then used later on under the assumption they"
                       << "will be valid until" << pool << "has been drained.";
      
      // Reset the pool so that it's not drained again later on
      *m_pool = nullptr;
   }
   
   [super dealloc];
}
@end

namespace pdk {
namespace kernel {

using pdk::io::DebugStateSaver;

MacAutoReleasePool::MacAutoReleasePool()
   : m_pool([[NSAutoreleasePool alloc] init])
{
   [[[PdkMacAutoReleasePoolTracker alloc] initWithPool:
      reinterpret_cast<NSAutoreleasePool **>(&m_pool)] autorelease];
}

MacAutoReleasePool::~MacAutoReleasePool()
{
   if (!m_pool) {
      warning_stream() << "Prematurely drained pool" << this << "finally drained. Any objects belonging"
                       << "to this pool have already been released, and have potentially been invalid since the"
                       << "premature drain earlier on.";
      return;
   }
   
   // Save and reset pool before draining, so that the pool tracker can know
   // that it's being drained by its owning pool.
   NSAutoreleasePool *savedPool = static_cast<NSAutoreleasePool*>(m_pool);
   m_pool = nullptr;
   
   // Drain behaves the same as release, with the advantage that
   // if we're ever used in a garbage-collected environment, the
   // drain acts as a hint to the garbage collector to collect.
   [savedPool drain];
}

#ifndef PDK_NO_DEBUG_STREAM
Debug operator<<(Debug debug, const MacAutoReleasePool *pool)
{
   DebugStateSaver saver(debug);
   debug.nospace();
   debug << "MacAutoReleasePool(" << (const void *)pool << ')';
   return debug;
}
#endif // !PDK_NO_DEBUG_STREAM

} // kernel
} // pdk

#ifdef PDK_OS_MACOS

@interface PDK_ROOT_LEVEL_POOL_THESE_OBJECTS_WILL_BE_RELEASED_WHEN_QAPP_GOES_OUT_OF_SCOPE: NSObject
@end
@implementation PDK_ROOT_LEVEL_POOL_THESE_OBJECTS_WILL_BE_RELEASED_WHEN_QAPP_GOES_OUT_OF_SCOPE
@end

namespace pdk {
namespace kernel { 

const char ROOT_LEVEL_POOL_DISABLE_SWITCH[] = "QT_DISABLE_ROOT_LEVEL_AUTORELEASE_POOL";

MacRootLevelAutoReleasePool::MacRootLevelAutoReleasePool()
{
   if (pdk::env_var_isset(ROOT_LEVEL_POOL_DISABLE_SWITCH))
      return;
   
   m_pool.reset(new MacAutoReleasePool);
   
   [[[PDK_ROOT_LEVEL_POOL_THESE_OBJECTS_WILL_BE_RELEASED_WHEN_QAPP_GOES_OUT_OF_SCOPE alloc] init] autorelease];
   
   if (pdk::strcmp(pdk::get_env("OBJC_DEBUG_MISSING_POOLS"), "YES") == 0) {
      debug_stream("CoreApplication root level NSAutoreleasePool in place. Break on ~%s and use\n" \
             "'p [NSAutoreleasePool showPools]' to show leaked objects, or set %s",
             __FUNCTION__, ROOT_LEVEL_POOL_DISABLE_SWITCH);
   }
}

MacRootLevelAutoReleasePool::~MacRootLevelAutoReleasePool()
{}

void apple_check_os_version()
{
#if defined(__MAC_OS_X_VERSION_MIN_REQUIRED)
    const char *os = "macOS";
    const int version = __MAC_OS_X_VERSION_MIN_REQUIRED;
#endif
    NSOperatingSystemVersion required;
    required.majorVersion = version / 10000;
    required.minorVersion = version / 100 % 100;
    required.patchVersion = version % 100;
    const NSOperatingSystemVersion current = NSProcessInfo.processInfo.operatingSystemVersion;
    if (![NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:required]) {
        fprintf(stderr, "You can't use this version of %s with this version of %s. "
                "You have %s %ld.%ld.%ld. Qt requires %s %ld.%ld.%ld or later.\n",
                (reinterpret_cast<const NSString *>(
                    NSBundle.mainBundle.infoDictionary[@"CFBundleName"]).UTF8String),
                os,
                os, long(current.majorVersion), long(current.minorVersion), long(current.patchVersion),
                os, long(required.majorVersion), long(required.minorVersion), long(required.patchVersion));
        abort();
    }
}

} // kernel
} // pdk

#endif
