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
// Created by softboy on 2018/02/26.

#include "pdk/base/lang/String.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/time/TimeZone.h"
#include "pdk/base/ds/ByteArray.h"

#if PDK_CONFIG(timezone) && !defined(PDK_NO_SYSTEMLOCALE)
#include "pdk/base/time/TimeZone.h"
#include "pdk/base/time/internal/TimeZonePrivate.h"
#include "pdk/kernel/internal/CoreMacPrivate.h"
#endif

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>

#if defined(PDK_PLATFORM_UIKIT)
#import <CoreGraphics/CoreGraphics.h>
#endif

namespace pdk {

namespace ds {
ByteArray ByteArray::fromCFData(CFDataRef data)
{
   if (!data) {
      return ByteArray();
   }
   return ByteArray(reinterpret_cast<const char *>(CFDataGetBytePtr(data)), CFDataGetLength(data));
}

ByteArray ByteArray::fromRawCFData(CFDataRef data)
{
   if (!data) {
      return ByteArray();
   }
   return ByteArray::fromRawData(reinterpret_cast<const char *>(CFDataGetBytePtr(data)), CFDataGetLength(data));
}

CFDataRef ByteArray::toCFData() const
{
   return CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(getRawData()), length());
}

CFDataRef ByteArray::toRawCFData() const
{
   return CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(getRawData()),
                                      length(), kCFAllocatorNull);
}

ByteArray ByteArray::fromNSData(const NSData *data)
{
   if (!data) {
      return ByteArray();
   }
   return ByteArray(reinterpret_cast<const char *>([data bytes]), [data length]);
}

ByteArray ByteArray::fromRawNSData(const NSData *data)
{
   if (!data) {
      return ByteArray();
   }
   return ByteArray::fromRawData(reinterpret_cast<const char *>([data bytes]), [data length]);
}

NSData *ByteArray::toNSData() const
{
   return [NSData dataWithBytes:getConstRawData() length:size()];
}

NSData *ByteArray::toRawNSData() const
{
   // const_cast is fine here because NSData is immutable thus will never modify bytes we're giving it
   return [NSData dataWithBytesNoCopy:const_cast<char *>(getConstRawData()) length:size() freeWhenDone:NO];
}

} // ds

namespace lang {

String String::fromCFString(CFStringRef string)
{
   if (!string) {
      return String();
   }
   CFIndex length = CFStringGetLength(string);
   // Fast path: CFStringGetCharactersPtr does not copy but may
   // return null for any and no reason.
   const UniChar *chars = CFStringGetCharactersPtr(string);
   if (chars) {
      return String(reinterpret_cast<const Character *>(chars), length);
   }
   String ret(length, pdk::Uninitialized);
   CFStringGetCharacters(string, CFRangeMake(0, length), reinterpret_cast<UniChar *>(ret.getRawData()));
   return ret;
}

CFStringRef String::toCFString() const
{
   return CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar *>(unicode()), length());
}

String String::fromNSString(const NSString *string)
{
   if (!string) {
      return String();
   }
   String String;
   String.resize([string length]);
   [string getCharacters: reinterpret_cast<unichar*>(String.getRawData()) range: NSMakeRange(0, [string length])];
   return String;
}

NSString *String::toNSString() const
{
   return [NSString stringWithCharacters: reinterpret_cast<const UniChar*>(unicode()) length: length()];
}
} // lang

namespace time {

DateTime DateTime::fromCFDate(CFDateRef date)
{
   if (!date) {
      return DateTime();
   }      
   CFAbsoluteTime sSinceEpoch = kCFAbsoluteTimeIntervalSince1970 + CFDateGetAbsoluteTime(date);
   return DateTime::fromMSecsSinceEpoch(std::llround(sSinceEpoch * 1000));
}

CFDateRef DateTime::toCFDate() const
{
   return CFDateCreate(kCFAllocatorDefault, (static_cast<CFAbsoluteTime>(toMSecsSinceEpoch())
                                             / 1000) - kCFAbsoluteTimeIntervalSince1970);
}

DateTime DateTime::fromNSDate(const NSDate *date)
{
   if (!date) {
      return DateTime();
   }
   return DateTime::fromMSecsSinceEpoch(std::llround([date timeIntervalSince1970] * 1000));
}

NSDate *DateTime::toNSDate() const
{
   return [NSDate
         dateWithTimeIntervalSince1970:static_cast<NSTimeInterval>(toMSecsSinceEpoch()) / 1000];
}

#if PDK_CONFIG(timezone) && !defined(PDK_NO_SYSTEMLOCALE)

TimeZone TimeZone::fromCFTimeZone(CFTimeZoneRef timeZone)
{
   if (!timeZone) {
      return TimeZone();
   }
   return TimeZone(String::fromCFString(CFTimeZoneGetName(timeZone)).toLatin1());
}

using internal::MacTimeZonePrivate;

CFTimeZoneRef TimeZone::toCFTimeZone() const
{
#ifndef PDK_NO_DYNAMIC_CAST
   PDK_ASSERT(dynamic_cast<const MacTimeZonePrivate *>(m_implPtr.data()));
#endif
   const MacTimeZonePrivate *p = static_cast<const MacTimeZonePrivate *>(m_implPtr.data());
   return reinterpret_cast<CFTimeZoneRef>([p->getNsTimeZone() copy]);
}

TimeZone TimeZone::fromNSTimeZone(const NSTimeZone *timeZone)
{
   if (!timeZone) {
      return TimeZone();
   }
   return TimeZone(String::fromNSString(timeZone.name).toLatin1());
}

NSTimeZone *TimeZone::toNSTimeZone() const
{
   return [static_cast<NSTimeZone *>(toCFTimeZone()) autorelease];
}
#endif

} // time

} // pdk

