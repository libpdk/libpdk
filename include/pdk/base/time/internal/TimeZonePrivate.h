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

#ifndef PDK_M_BASE_TIME_INTERNAL_TIMEZONE_PRIVATE_H
#define PDK_M_BASE_TIME_INTERNAL_TIMEZONE_PRIVATE_H

#include "pdk/base/time/TimeZone.h"
#include "pdk/utils/internal/LocalePrivate.h"
#include "pdk/utils/SharedData.h"

#if PDK_CONFIG(icu)
#include <unicode/ucal.h>
#endif

#ifdef PDK_OS_DARWIN
PDK_FORWARD_DECLARE_OBJC_CLASS(NSTimeZone);
#endif // PDK_OS_DARWIN

#ifdef PDK_OS_WIN
#include "pdk/global/Windows.h"
#endif // PDK_OS_WIN

#include <vector>
#include <list>

namespace pdk {
namespace time {
namespace internal {

using pdk::utils::SharedData;
using pdk::utils::Locale;
using pdk::ds::ByteArray;
using pdk::time::TimeZone;
using pdk::io::DataStream;
using pdk::ds::ByteArray;

class PDK_UNITTEST_EXPORT TimeZonePrivate : public SharedData
{
public:
   //Version of TimeZone::OffsetData struct using msecs for efficiency
   struct Data {
      String abbreviation;
      pdk::pint64 atMSecsSinceEpoch;
      int offsetFromUtc;
      int standardTimeOffset;
      int daylightTimeOffset;
   };
   typedef std::vector<Data> DataList;
   
   // Create null time zone
   TimeZonePrivate();
   TimeZonePrivate(const TimeZonePrivate &other);
   virtual ~TimeZonePrivate();
   
   virtual TimeZonePrivate *clone() const;
   
   bool operator==(const TimeZonePrivate &other) const;
   bool operator!=(const TimeZonePrivate &other) const;
   
   bool isValid() const;
   
   ByteArray id() const;
   virtual Locale::Country getCountry() const;
   virtual String getComment() const;
   
   virtual String displayName(pdk::pint64 atMSecsSinceEpoch,
                              TimeZone::NameType nameType,
                              const Locale &locale) const;
   virtual String displayName(TimeZone::TimeType timeType,
                              TimeZone::NameType nameType,
                              const Locale &locale) const;
   virtual String abbreviation(pdk::pint64 atMSecsSinceEpoch) const;
   
   virtual int offsetFromUtc(pdk::pint64 atMSecsSinceEpoch) const;
   virtual int standardTimeOffset(pdk::pint64 atMSecsSinceEpoch) const;
   virtual int daylightTimeOffset(pdk::pint64 atMSecsSinceEpoch) const;
   
   virtual bool hasDaylightTime() const;
   virtual bool isDaylightTime(pdk::pint64 atMSecsSinceEpoch) const;
   
   virtual Data data(pdk::pint64 forMSecsSinceEpoch) const;
   Data dataForLocalTime(pdk::pint64 forLocalMSecs, int hint) const;
   
   virtual bool hasTransitions() const;
   virtual Data nextTransition(pdk::pint64 afterMSecsSinceEpoch) const;
   virtual Data previousTransition(pdk::pint64 beforeMSecsSinceEpoch) const;
   DataList transitions(pdk::pint64 fromMSecsSinceEpoch, pdk::pint64 toMSecsSinceEpoch) const;
   
   virtual ByteArray getSystemTimeZoneId() const;
   
   virtual std::list<ByteArray> getAvailableTimeZoneIds() const;
   virtual std::list<ByteArray> getAvailableTimeZoneIds(Locale::Country country) const;
   virtual std::list<ByteArray> getAvailableTimeZoneIds(int utcOffset) const;
   
   virtual void serialize(DataStream &ds) const;
   
   // Static Utility Methods
   static inline pdk::pint64 getMaxMSecs() { return std::numeric_limits<pdk::pint64>::max(); }
   static inline pdk::pint64 getMinMSecs() { return std::numeric_limits<pdk::pint64>::min() + 1; }
   static inline pdk::pint64 getInvalidMSecs() { return std::numeric_limits<pdk::pint64>::min(); }
   static inline pdk::pint64 getInvalidSeconds() { return std::numeric_limits<int>::min(); }
   static Data getInvalidData();
   static TimeZone::OffsetData getInvalidOffsetData();
   static TimeZone::OffsetData toOffsetData(const Data &data);
   static bool isValidId(const ByteArray &ianaId);
   static String isoOffsetFormat(int offsetFromUtc);
   
   static ByteArray ianaIdToWindowsId(const ByteArray &ianaId);
   static ByteArray windowsIdToDefaultIanaId(const ByteArray &windowsId);
   static ByteArray windowsIdToDefaultIanaId(const ByteArray &windowsId,
                                             Locale::Country country);
   static std::list<ByteArray> windowsIdToIanaIds(const ByteArray &windowsId);
   static std::list<ByteArray> windowsIdToIanaIds(const ByteArray &windowsId,
                                                  Locale::Country country);
   
   // returns "UTC" String and ByteArray
   PDK_REQUIRED_RESULT static inline String getUtcString()
   {
      return StringLiteral("UTC");
   }
   
   PDK_REQUIRED_RESULT static inline ByteArray getUtcByteArray()
   {
      return ByteArrayLiteral("UTC");
   }
   
protected:
   ByteArray m_id;
};

class PDK_UNITTEST_EXPORT UtcTimeZonePrivate final : public TimeZonePrivate
{
public:
   // Create default UTC time zone
   UtcTimeZonePrivate();
   // Create named time zone
   UtcTimeZonePrivate(const ByteArray &utcId);
   // Create offset from UTC
   UtcTimeZonePrivate(int offsetSeconds);
   // Create custom offset from UTC
   UtcTimeZonePrivate(const ByteArray &zoneId, int offsetSeconds, const String &name,
                      const String &abbreviation, Locale::Country country,
                      const String &comment);
   UtcTimeZonePrivate(const UtcTimeZonePrivate &other);
   virtual ~UtcTimeZonePrivate();
   
   UtcTimeZonePrivate *clone() const override;
   
   Data data(pdk::pint64 forMSecsSinceEpoch) const override;
   
   Locale::Country getCountry() const override;
   String getComment() const override;
   
   String displayName(TimeZone::TimeType timeType,
                      TimeZone::NameType nameType,
                      const Locale &locale) const override;
   String abbreviation(pdk::pint64 atMSecsSinceEpoch) const override;
   
   int standardTimeOffset(pdk::pint64 atMSecsSinceEpoch) const override;
   int daylightTimeOffset(pdk::pint64 atMSecsSinceEpoch) const override;
   
   ByteArray getSystemTimeZoneId() const override;
   
   std::list<ByteArray> getAvailableTimeZoneIds() const override;
   std::list<ByteArray> getAvailableTimeZoneIds(Locale::Country country) const override;
   std::list<ByteArray> getAvailableTimeZoneIds(int utcOffset) const override;
   
   void serialize(DataStream &ds) const override;
   
private:
   void init(const ByteArray &zoneId);
   void init(const ByteArray &zoneId, int offsetSeconds, const String &name,
             const String &abbreviation, Locale::Country country,
             const String &comment);
   
   String m_name;
   String m_abbreviation;
   String m_comment;
   Locale::Country m_country;
   int m_offsetFromUtc;
};

#if PDK_CONFIG(icu)
class PDK_UNITTEST_EXPORT IcuTimeZonePrivate final : public TimeZonePrivate
{
public:
   // Create default time zone
   IcuTimeZonePrivate();
   // Create named time zone
   IcuTimeZonePrivate(const ByteArray &ianaId);
   IcuTimeZonePrivate(const IcuTimeZonePrivate &other);
   ~IcuTimeZonePrivate();
   
   IcuTimeZonePrivate *clone() const override;
   
   String displayName(TimeZone::TimeType timeType, TimeZone::NameType nameType,
                      const Locale &locale) const override;
   String abbreviation(pdk::pint64 atMSecsSinceEpoch) const override;
   
   int offsetFromUtc(pdk::pint64 atMSecsSinceEpoch) const override;
   int standardTimeOffset(pdk::pint64 atMSecsSinceEpoch) const override;
   int daylightTimeOffset(pdk::pint64 atMSecsSinceEpoch) const override;
   
   bool hasDaylightTime() const override;
   bool isDaylightTime(pdk::pint64 atMSecsSinceEpoch) const override;
   
   Data data(pdk::pint64 forMSecsSinceEpoch) const override;
   
   bool hasTransitions() const override;
   Data nextTransition(pdk::pint64 afterMSecsSinceEpoch) const override;
   Data previousTransition(pdk::pint64 beforeMSecsSinceEpoch) const override;
   
   ByteArray getSystemTimeZoneId() const override;
   
   std::list<ByteArray> getAvailableTimeZoneIds() const override;
   std::list<ByteArray> getAvailableTimeZoneIds(Locale::Country country) const override;
   std::list<ByteArray> getAvailableTimeZoneIds(int offsetFromUtc) const override;
   
private:
   void init(const ByteArray &ianaId);
   
   UCalendar *m_ucal;
};
#endif

#if defined(PDK_OS_UNIX) && !defined(PDK_OS_DARWIN)
struct TzTransitionTime
{
   pdk::pint64 m_atMSecsSinceEpoch;
   pdk::puint8 m_ruleIndex;
};

struct TzTransitionRule
{
   int m_stdOffset;
   int m_dstOffset;
   pdk::puint8 m_abbreviationIndex;
};

constexpr inline bool operator==(const TzTransitionRule &lhs, const TzTransitionRule &rhs) noexcept
{
   return lhs.m_stdOffset == rhs.m_stdOffset && 
         lhs.m_dstOffset == rhs.m_dstOffset && 
         lhs.m_abbreviationIndex == rhs.m_abbreviationIndex;
}
constexpr inline bool operator!=(const TzTransitionRule &lhs, const TzTransitionRule &rhs) noexcept
{
   return !operator==(lhs, rhs);
}

class PDK_UNITTEST_EXPORT TzTimeZonePrivate final : public TimeZonePrivate
{
   TzTimeZonePrivate(const TzTimeZonePrivate &) = default;
public:
   // Create default time zone
   TzTimeZonePrivate();
   // Create named time zone
   TzTimeZonePrivate(const ByteArray &ianaId);
   ~TzTimeZonePrivate();
   
   TzTimeZonePrivate *clone() const override;
   
   Locale::Country getCountry() const override;
   String getComment() const override;
   
   String displayName(pdk::pint64 atMSecsSinceEpoch,
                      TimeZone::NameType nameType,
                      const Locale &locale) const override;
   String displayName(TimeZone::TimeType timeType,
                      TimeZone::NameType nameType,
                      const Locale &locale) const override;
   String abbreviation(pdk::pint64 atMSecsSinceEpoch) const override;
   
   int offsetFromUtc(pdk::pint64 atMSecsSinceEpoch) const override;
   int standardTimeOffset(pdk::pint64 atMSecsSinceEpoch) const override;
   int daylightTimeOffset(pdk::pint64 atMSecsSinceEpoch) const override;
   
   bool hasDaylightTime() const override;
   bool isDaylightTime(pdk::pint64 atMSecsSinceEpoch) const override;
   
   Data data(pdk::pint64 forMSecsSinceEpoch) const override;
   
   bool hasTransitions() const override;
   Data nextTransition(pdk::pint64 afterMSecsSinceEpoch) const override;
   Data previousTransition(pdk::pint64 beforeMSecsSinceEpoch) const override;
   
   ByteArray getSystemTimeZoneId() const override;
   
   std::list<ByteArray> getAvailableTimeZoneIds() const override;
   std::list<ByteArray> getAvailableTimeZoneIds(Locale::Country country) const override;
   
private:
   void init(const ByteArray &ianaId);
   
   Data dataForTzTransition(TzTransitionTime tran) const;
   std::vector<TzTransitionTime> m_tranTimes;
   std::vector<TzTransitionRule> m_tranRules;
   std::list<ByteArray> m_abbreviations;
#if PDK_CONFIG(icu)
   mutable pdk::utils::SharedDataPointer<TimeZonePrivate> m_icu;
#endif
   ByteArray m_posixRule;
};

#endif // PDK_OS_UNIX

#ifdef PDK_OS_MAC
class PDK_UNITTEST_EXPORT MacTimeZonePrivate final : public TimeZonePrivate
{
public:
   // Create default time zone
   MacTimeZonePrivate();
   // Create named time zone
   MacTimeZonePrivate(const ByteArray &ianaId);
   MacTimeZonePrivate(const MacTimeZonePrivate &other);
   ~MacTimeZonePrivate();
   MacTimeZonePrivate *clone() const override;
   String getComment() const override;
   String displayName(TimeZone::TimeType timeType, TimeZone::NameType nameType,
                      const Locale &locale) const override;
   String abbreviation(pdk::pint64 atMSecsSinceEpoch) const override;
   int offsetFromUtc(pdk::pint64 atMSecsSinceEpoch) const override;
   int standardTimeOffset(pdk::pint64 atMSecsSinceEpoch) const override;
   int daylightTimeOffset(pdk::pint64 atMSecsSinceEpoch) const override;
   bool hasDaylightTime() const override;
   bool isDaylightTime(pdk::pint64 atMSecsSinceEpoch) const override;
   Data data(pdk::pint64 forMSecsSinceEpoch) const override;
   bool hasTransitions() const override;
   Data nextTransition(pdk::pint64 afterMSecsSinceEpoch) const override;
   Data previousTransition(pdk::pint64 beforeMSecsSinceEpoch) const override;
   ByteArray getSystemTimeZoneId() const override;
   std::list<ByteArray> getAvailableTimeZoneIds() const override;
   NSTimeZone *getNsTimeZone() const;
private:
   void init(const ByteArray &zoneId);
   NSTimeZone *m_nstz;
};
#endif // PDK_OS_MAC

#ifdef PDK_OS_WIN
class PDK_UNITTEST_EXPORT WinTimeZonePrivate final : public TimeZonePrivate
{
public:
   struct WinTransitionRule {
      int m_startYear;
      int m_standardTimeBias;
      int m_daylightTimeBias;
      SYSTEMTIME m_standardTimeRule;
      SYSTEMTIME m_daylightTimeRule;
   };
   
   // Create default time zone
   WinTimeZonePrivate();
   // Create named time zone
   WinTimeZonePrivate(const ByteArray &ianaId);
   WinTimeZonePrivate(const WinTimeZonePrivate &other);
   ~WinTimeZonePrivate();
   WinTimeZonePrivate *clone() const override;
   String getComment() const override;
   String displayName(TimeZone::TimeType timeType, TimeZone::NameType nameType,
                      const Locale &locale) const override;
   String abbreviation(pdk::pint64 atMSecsSinceEpoch) const override;
   int offsetFromUtc(pdk::pint64 atMSecsSinceEpoch) const override;
   int standardTimeOffset(pdk::pint64 atMSecsSinceEpoch) const override;
   int daylightTimeOffset(pdk::pint64 atMSecsSinceEpoch) const override;
   bool hasDaylightTime() const override;
   bool isDaylightTime(pdk::pint64 atMSecsSinceEpoch) const override;
   Data data(pdk::pint64 forMSecsSinceEpoch) const override;
   bool hasTransitions() const override;
   Data nextTransition(pdk::pint64 afterMSecsSinceEpoch) const override;
   Data previousTransition(pdk::pint64 beforeMSecsSinceEpoch) const override;
   ByteArray getSystemTimeZoneId() const override;
   std::list<ByteArray> getAvailableTimeZoneIds() const override;
   
private:
   void init(const ByteArray &ianaId);
   WinTransitionRule ruleForYear(int year) const;
   TimeZonePrivate::Data ruleToData(const WinTransitionRule &rule, pdk::pint64 atMSecsSinceEpoch,
                                    TimeZone::TimeType type) const;
   
   ByteArray m_windowsId;
   String m_displayName;
   String m_standardName;
   String m_daylightName;
   std::list<WinTransitionRule> m_tranRules;
};
#endif // PDK_OS_WIN

} // internal
} // time
} // pdk

namespace pdk {
namespace utils {

using pdk::time::internal::TimeZonePrivate;
template<>
TimeZonePrivate *SharedDataPointer<TimeZonePrivate>::clone();

} // utils
} // pdk

PDK_DECLARE_TYPEINFO(pdk::time::internal::TimeZonePrivate::Data, PDK_MOVABLE_TYPE);
#if defined(PDK_OS_UNIX) && !defined(PDK_OS_DARWIN)
PDK_DECLARE_TYPEINFO(pdk::time::internal::TzTransitionRule, PDK_MOVABLE_TYPE);
PDK_DECLARE_TYPEINFO(pdk::time::internal::TzTransitionTime, PDK_MOVABLE_TYPE);
#endif

#endif // PDK_M_BASE_TIME_INTERNAL_TIMEZONE_PRIVATE_H
