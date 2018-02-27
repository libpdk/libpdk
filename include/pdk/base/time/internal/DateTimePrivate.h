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
// Created by softboy on 2018/02/05.

#ifndef PDK_M_BASE_TIME_INTERNAL_DATETIME_PRIVATE_H
#define PDK_M_BASE_TIME_INTERNAL_DATETIME_PRIVATE_H

#include "pdk/global/PlatformDefs.h"
#include "pdk/global/Global.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/base/time/Time.h"
#include "pdk/base/time/Date.h"
#include "pdk/base/time/DateTime.h"

#if PDK_CONFIG(timezone)
#include "pdk/base/time/TimeZone.h"
#endif

namespace pdk {
namespace time {
namespace internal {

using pdk::os::thread::AtomicInt;

class DateTimePrivate
{
public:
   // forward the declarations from DateTime (this makes them public)
   using DateTimeShortData = DateTime::ShortData;
   using DateTimeData = DateTime::Data;
   
   // Never change or delete this enum, it is required for backwards compatible
   // serialization of DateTime before 5.2, so is essentially public API
   enum class Spec : int
   {
      LocalUnknown = -1,
      LocalStandard = 0,
      LocalDST = 1,
      UTC = 2,
      OffsetFromUTC = 3,
      TimeZone = 4
   };
   
   // Daylight Time Status
   enum class DaylightStatus : int
   {
      UnknownDaylightTime = -1,
      StandardTime = 0,
      DaylightTime = 1
   };
   
   // Status of date/time
   enum class StatusFlag : uint
   {
      ShortData           = 0x01,
      
      ValidDate           = 0x02,
      ValidTime           = 0x04,
      ValidDateTime       = 0x08,
      
      TimeSpecMask        = 0x30,
      
      SetToStandardTime   = 0x40,
      SetToDaylightTime   = 0x80
   };
   PDK_DECLARE_FLAGS(StatusFlags, StatusFlag);
   
   enum : uint
   {
      TimeSpecShift = 4,
      ValidityMask        = uint(StatusFlag::ValidDate) | uint(StatusFlag::ValidTime) | uint(StatusFlag::ValidDateTime),
      DaylightMask        = uint(StatusFlag::SetToStandardTime) | uint(StatusFlag::SetToDaylightTime)
   };
   
   DateTimePrivate() : m_msecs(0),
      m_status(StatusFlag(pdk::as_integer<pdk::TimeSpec>(pdk::TimeSpec::LocalTime) << TimeSpecShift)),
      m_offsetFromUtc(0),
      m_ref(0)
   {
   }
   
   static DateTime::Data create(const Date &toDate, const Time &toTime, pdk::TimeSpec toSpec,
                                int offsetSeconds);
   
#if PDK_CONFIG(timezone)
   static DateTime::Data create(const Date &toDate, const Time &toTime, const TimeZone & timeZone);
#endif // timezone
   
   pdk::pint64 m_msecs;
   StatusFlags m_status;
   int m_offsetFromUtc;
   mutable AtomicInt m_ref;
#if PDK_CONFIG(timezone)
   TimeZone m_timeZone;
#endif // timezone
   
#if PDK_CONFIG(timezone)
   static pdk::pint64 zoneMSecsToEpochMSecs(pdk::pint64 msecs, const TimeZone &zone,
                                            DaylightStatus hint = DaylightStatus::UnknownDaylightTime,
                                            Date *localDate = 0, Time *localTime = 0);
   
   // Inlined for its one caller in DateTime.cpp
   inline void setUtcOffsetByTZ(pdk::pint64 atMSecsSinceEpoch);
#endif // timezone
};


} // internal
} // time
} // pdk

#endif // PDK_M_BASE_TIME_INTERNAL_DATETIME_PRIVATE_H
