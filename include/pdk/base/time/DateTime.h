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

#ifndef PDK_M_BASE_TIME_DATETIME_H
#define PDK_M_BASE_TIME_DATETIME_H

#include "pdk/base/lang/String.h"

namespace pdk {
namespace time {

namespace internal {
class DateTimePrivate;
} // internal

class TimeZone;
class Time;
class Date;
using pdk::lang::String;
using internal::DateTimePrivate;
class PDK_CORE_EXPORT DateTime
{
   struct ShortData
   {
#if PDK_BYTE_ORDER == PDK_LITTLE_ENDIAN
      pdk::uintptr m_status : 8;
#endif
      // note: this is only 24 bits on 32-bit systems...
      pdk::intptr m_msecs : sizeof(void *) * 8 - 8;
      
#if PDK_BYTE_ORDER == PDK_BIG_ENDIAN
      pdk::intptr m_status : 8;
#endif
   };
   
   union Data {
      enum {
         // To be of any use, we need at least 60 years around 1970, which
         // is 1,893,456,000,000 ms. That requires 41 bits to store, plus
         // the sign bit. With the status byte, the minimum size is 50 bits.
         CanBeSmall = sizeof(ShortData) * 8 > 50
      };
      
      Data();
      Data(pdk::TimeSpec);
      Data(const Data &other);
      Data(Data &&other);
      Data &operator=(const Data &other);
      ~Data();
      
      bool isShort() const;
      void detach();
      
      const DateTimePrivate *operator->() const;
      DateTimePrivate *operator->();
      
      DateTimePrivate *d;
      ShortData m_data;
   };
public:
   DateTime() noexcept(Data::CanBeSmall);
   explicit DateTime(const Date &);
   DateTime(const Date &, const Time &, pdk::TimeSpec spec = pdk::TimeSpec::LocalTime,
            int offsetSeconds = 0);
#ifdef PDK_CONFIG_TIMEZONE
   DateTime(const Date &date, const Time &time, const TimeZone &timeZone);
#endif
   DateTime(const DateTime &other) noexcept;
   DateTime(DateTime &&other) noexcept;
   ~DateTime();
   
   DateTime &operator=(DateTime &&other) noexcept
   {
      swap(other);
      return *this;
   }
   
   DateTime &operator=(const DateTime &other) noexcept;
   void swap(DateTime &other) noexcept
   {
      std::swap(m_implPtr.d, other.m_implPtr.d);
   }
   
   bool isNull() const;
   bool isValid() const;
   
   Date getDate() const;
   Time getTime() const;
   pdk::TimeSpec getTimeSpec() const;
   int getOffsetFromUtc() const;
#ifdef PDK_CONFIG_TIMEZONE
   TimeZone getTimeZone() const;
#endif
   String timeZoneAbbreviation() const;
   bool isDaylightTime() const;
   
   pdk::pint64 toMSecsSinceEpoch() const;
   pdk::pint64 toSecsSinceEpoch() const;
   
   void setDate(const Date &date);
   void setTime(const Time &time);
   void setTimeSpec(pdk::TimeSpec spec);
   void setOffsetFromUtc(int offsetSeconds);
#ifdef PDK_CONFIG_TIMEZONE
   TimeZone setTimeZone(const QTimeZone &toZone);
#endif
   void setMSecsSinceEpoch(pdk::pint64 msecs);
   void setSecsSinceEpoch(pdk::pint64 secs);
   
   String toString(pdk::DateFormat f = pdk::DateFormat::TextDate) const;
   String toString(const String &format) const;
   
   DateTime addDays(pdk::pint64 days) const PDK_REQUIRED_RESULT;
   DateTime addMonths(int months) const PDK_REQUIRED_RESULT;
   DateTime addYears(int years) const PDK_REQUIRED_RESULT;
   DateTime addSecs(pdk::pint64 secs) const PDK_REQUIRED_RESULT;
   DateTime addMSecs(pdk::pint64 msecs) const PDK_REQUIRED_RESULT;
   
   DateTime toTimeSpec(pdk::TimeSpec spec) const;
   inline DateTime toLocalTime() const 
   {
      return toTimeSpec(pdk::TimeSpec::LocalTime);
   }
   
   inline DateTime toUTC() const
   {
      return toTimeSpec(pdk::TimeSpec::UTC);
   }
   DateTime toOffsetFromUtc(int offsetSeconds) const;
#ifdef PDK_CONFIG_TIMEZONE
   DateTime toTimeZone(const TimeZone &toZone) const;
#endif
   
   pdk::pint64 daysTo(const DateTime &) const;
   pdk::pint64 secsTo(const DateTime &) const;
   pdk::pint64 msecsTo(const DateTime &) const;
   
   bool operator==(const DateTime &other) const;
   inline bool operator!=(const DateTime &other) const
   {
      return !(*this == other);
   }
   bool operator<(const DateTime &other) const;
   inline bool operator<=(const DateTime &other) const
   { 
      return !(other < *this);
   }
   
   inline bool operator>(const DateTime &other) const
   {
      return other < *this;
   }
   
   inline bool operator>=(const DateTime &other) const
   {
      return !(*this < other);
   }
   
   static DateTime getCurrentDateTime();
   static DateTime getCurrentDateTimeUtc();
   static DateTime fromString(const String &s, pdk::DateFormat f = pdk::DateFormat::TextDate);
   static DateTime fromString(const String &s, const String &format);
   
   static DateTime fromMSecsSinceEpoch(pdk::pint64 msecs, pdk::TimeSpec spec = pdk::TimeSpec::LocalTime, int offsetFromUtc = 0);
   static DateTime fromSecsSinceEpoch(pdk::pint64 secs, pdk::TimeSpec spe = pdk::TimeSpec::LocalTime, int offsetFromUtc = 0);
   
#ifdef PDK_CONFIG_TIMEZONE
   static DateTime fromMSecsSinceEpoch(pdk::pint64 msecs, const TimeZone &timeZone);
   static DateTime fromSecsSinceEpoch(pdk::pint64 secs, const TimeZone &timeZone);
#endif
   
   static pdk::pint64 getCurrentMSecsSinceEpoch() noexcept;
   static pdk::pint64 getCurrentSecsSinceEpoch() noexcept;
private:
   friend class DateTimePrivate;
   Data m_implPtr;
};

PDK_CORE_EXPORT uint hash(const DateTime &key, uint seed = 0);

} // time
} // pdk

PDK_DECLARE_SHARED(pdk::time::DateTime)

#endif // PDK_M_BASE_TIME_DATETIME_H
