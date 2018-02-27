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
// Created by softboy on 2018/01/30.

#ifndef PDK_KERNEL_INTERNAL_CORE_MAC_PRIVATE_H
#define PDK_KERNEL_INTERNAL_CORE_MAC_PRIVATE_H

#ifndef __IMAGECAPTURE__
#  define __IMAGECAPTURE__
#endif

#include <CoreFoundation/CoreFoundation.h>

#include "pdk/global/Global.h"
#include "pdk/base/lang/String.h"

#ifdef __OBJC__
#include <Foundation/Foundation.h>
#endif

#include "pdk/base/lang/String.h"
#include "pdk/utils/ScopedPointer.h"

#define PDK_NAMESPACE_ALIAS_OBJC_CLASS(__KLASS__)

namespace pdk {

// forward declare class with namespace
namespace io {
class Debug;
} // io

namespace kernel {

using pdk::lang::String;
using pdk::io::Debug;

template <typename T, typename U, U (*RetainFunction)(U), void (*ReleaseFunction)(U)>
class AppleRefCounted
{
public:
   AppleRefCounted(const T &value = T()) 
      : m_value(value)
   {}
   
   AppleRefCounted(AppleRefCounted &&other) 
      : m_value(other.m_value)
   {
      other.m_value = T();
   }
   
   AppleRefCounted(const AppleRefCounted &other)
      : m_value(other.m_value)
   {
      if (m_value) {
         RetainFunction(m_value);
      }
   }
   
   ~AppleRefCounted()
   {
      if (m_value) {
         ReleaseFunction(m_value);
      }
   }
   
   operator T()
   {
      return m_value;
   }
   
   void swap(AppleRefCounted &other) noexcept(noexcept(std::swap(m_value, other.m_value)))
   {
      std::swap(m_value, other.m_value);
   }
   
   AppleRefCounted &operator=(const AppleRefCounted &other)
   {
      AppleRefCounted copy(other);
      swap(copy);
      return *this;
   }
   
   AppleRefCounted &operator=(AppleRefCounted &&other)
   {
      AppleRefCounted moved(std::move(other));
      swap(moved);
      return *this;
   }
   
   T *operator&() {
      return &m_value;
   }
   
protected:
   T m_value;
};

#ifdef PDK_OS_MACOS
class MacRootLevelAutoReleasePool
{
public:
   MacRootLevelAutoReleasePool();
   ~MacRootLevelAutoReleasePool();
private:
   pdk::utils::ScopedPointer<MacAutoReleasePool> m_pool;
};
#endif

/*
    Helper class that automates refernce counting for CFtypes.
    After constructing the CFType object, it can be copied like a
    value-based type.
    
    Note that you must own the object you are wrapping.
    This is typically the case if you get the object from a Core
    Foundation function with the word "Create" or "Copy" in it. If
    you got the object from a "Get" function, either retain it or use
    constructFromGet(). One exception to this rule is the
    HIThemeGet*Shape functions, which in reality are "Copy" functions.
*/
template <typename T>
class CFType : public AppleRefCounted<T, CFTypeRef, CFRetain, CFRelease>
{
public:
   using AppleRefCounted<T, CFTypeRef, CFRetain, CFRelease>::AppleRefCounted;
   template <typename X> X as() const { return reinterpret_cast<X>(this->m_value); }
   static CFType constructFromGet(const T &t)
   {
      if (t) {
         CFRetain(t);
      }
      return CFType<T>(t);
   }
};

class PDK_CORE_EXPORT CFString : public CFType<CFStringRef>
{
public:
   inline CFString(const String &str)
      : CFType<CFStringRef>(0),
        m_string(str)
   {}
   
   inline CFString(const CFStringRef cfstr = 0)
      : CFType<CFStringRef>(cfstr)
   {}
   
   inline CFString(const CFType<CFStringRef> &other) 
      : CFType<CFStringRef>(other)
   {}
   
   operator String() const;
   operator CFStringRef() const;
   
private:
   String m_string;
};

#ifndef PDK_NO_DEBUG_STREAM
Debug operator<<(Debug debug, const MacAutoReleasePool *pool);
#endif

} // kernel
} // pdk

#endif // PDK_KERNEL_INTERNAL_CORE_MAC_PRIVATE_H
