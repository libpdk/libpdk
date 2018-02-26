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

#ifndef PDK_M_BASE_TIME_TIMEZONE_H
#define PDK_M_BASE_TIME_TIMEZONE_H

#include "pdk/utils/Locale.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/utils/SharedData.h"

#include <vector>

PDK_REQUIRE_CONFIG(TIMEZONE);

#if defined(PDK_OS_DARWIN) && !defined(PDK_NO_SYSTEMLOCALE)
PDK_FORWARD_DECLARE_CF_TYPE(CFTimeZone);
PDK_FORWARD_DECLARE_OBJC_CLASS(NSTimeZone);
#endif

namespace pdk {

// forward declare class with namespace
namespace ds {
class ByteArray;
} // ds

namespace time {

// forward declare class with namespace
namespace internal {
class TimeZonePrivate;
class DateTimePrivate;
} // internal

using pdk::utils::Locale;
using internal::TimeZonePrivate;
using internal::DateTimePrivate;
using pdk::lang::String;
using pdk::ds::ByteArray;

class PDK_CORE_EXPORT TimeZone
{
public:
   // Sane UTC offsets range from -14 to +14 hours:
   enum {
      // No known zone > 12 hrs West of Greenwich (Baker Island, USA)
      MinUtcOffsetSecs = -14 * 3600,
      // No known zone > 14 hrs East of Greenwich (Kiritimati, Christmas Island, Kiribati)
      MaxUtcOffsetSecs = +14 * 3600
   };
   
   enum class TimeType : uint
   {
      StandardTime = 0,
      DaylightTime = 1,
      GenericTime = 2
   };
   
   enum class NameType : uint
   {
      DefaultName = 0,
      LongName = 1,
      ShortName = 2,
      OffsetName = 3
   };
   
   struct OffsetData
   {
      String m_abbreviation;
      DateTime m_atUtc;
      int m_offsetFromUtc;
      int m_standardTimeOffset;
      int m_daylightTimeOffset;
   };
   using OffsetDataList = std::vector<OffsetData>;
   
   TimeZone() noexcept;
   explicit TimeZone(const ByteArray &ianaId);
   explicit TimeZone(int offsetSeconds);
   // implicit
   TimeZone(const ByteArray &zoneId, int offsetSeconds, const String &name,
            const String &abbreviation, Locale::Country country = Locale::Country::AnyCountry,
            const String &comment = String());
   TimeZone(const TimeZone &other);
   ~TimeZone();
   
   TimeZone &operator=(const TimeZone &other);
   TimeZone &operator=(TimeZone &&other) noexcept
   {
      swap(other);
      return *this;
   }
   
   void swap(TimeZone &other) noexcept
   {
      m_imptr.swap(other.m_imptr);
   }
   
   bool operator==(const TimeZone &other) const;
   bool operator!=(const TimeZone &other) const;
   
   bool isValid() const;
   
   ByteArray getId() const;
   Locale::Country getCountry() const;
   String getComment() const;
   
   String displayName(const DateTime &atDateTime,
                      TimeZone::NameType nameType = NameType::DefaultName,
                      const Locale &locale = Locale()) const;
   String displayName(TimeZone::TimeType timeType,
                      TimeZone::NameType nameType = NameType::DefaultName,
                      const Locale &locale = Locale()) const;
   String abbreviation(const DateTime &atDateTime) const;
   
   int offsetFromUtc(const DateTime &atDateTime) const;
   int standardTimeOffset(const DateTime &atDateTime) const;
   int daylightTimeOffset(const DateTime &atDateTime) const;
   
   bool hasDaylightTime() const;
   bool isDaylightTime(const DateTime &atDateTime) const;
   
   OffsetData offsetData(const DateTime &forDateTime) const;
   
   bool hasTransitions() const;
   OffsetData nextTransition(const DateTime &afterDateTime) const;
   OffsetData previousTransition(const DateTime &beforeDateTime) const;
   OffsetDataList transitions(const DateTime &fromDateTime, const DateTime &toDateTime) const;
   
   static ByteArray getSystemTimeZoneId();
   static TimeZone getSystemTimeZone();
   static TimeZone getUtc();
   
   static bool isTimeZoneIdAvailable(const ByteArray &ianaId);
   
   static std::list<ByteArray> getAvailableTimeZoneIds();
   static std::list<ByteArray> getAvailableTimeZoneIds(Locale::Country country);
   static std::list<ByteArray> getAvailableTimeZoneIds(int offsetSeconds);
   
   static ByteArray ianaIdToWindowsId(const ByteArray &ianaId);
   static ByteArray windowsIdToDefaultIanaId(const ByteArray &windowsId);
   static ByteArray windowsIdToDefaultIanaId(const ByteArray &windowsId,
                                             Locale::Country country);
   static std::list<ByteArray> windowsIdToIanaIds(const ByteArray &windowsId);
   static std::list<ByteArray> windowsIdToIanaIds(const ByteArray &windowsId,
                                                  Locale::Country country);
   
#if (defined(PDK_OS_DARWIN)) && !defined(PDK_NO_SYSTEMLOCALE)
   static TimeZone fromCFTimeZone(CFTimeZoneRef timeZone);
   CFTimeZoneRef toCFTimeZone() const PDK_DECL_CF_RETURNS_RETAINED;
   static TimeZone fromNSTimeZone(const NSTimeZone *timeZone);
   NSTimeZone *toNSTimeZone() const PDK_DECL_NS_RETURNS_AUTORELEASED;
#endif
   
   
private:
   TimeZone(TimeZonePrivate &dd);
   friend class TimeZonePrivate;
   friend class DateTime;
   friend class DateTimePrivate;
   pdk::utils::SharedDataPointer<TimeZonePrivate> m_imptr;
};

#ifndef PDK_NO_DATASTREAM
PDK_CORE_EXPORT DataStream &operator<<(DataStream &ds, const TimeZone &tz);
PDK_CORE_EXPORT DataStream &operator>>(DataStream &ds, TimeZone &tz);
#endif

#ifndef PDK_NO_DEBUG_STREAM
PDK_CORE_EXPORT Debug operator<<(Debug dbg, const TimeZone &tz);
#endif

} // time
} // pdk

PDK_DECLARE_SHARED(pdk::time::TimeZone)

#endif // PDK_M_BASE_TIME_TIMEZONE_H
