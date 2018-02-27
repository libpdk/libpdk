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

#ifndef PDK_M_BASE_TIME_DATE_H
#define PDK_M_BASE_TIME_DATE_H

#include "pdk/base/lang/String.h"
#include <limits>

namespace pdk {

#if !defined(PDK_NO_DEBUG_STREAM) && !defined(PDK_NO_DATESTRING)
namespace io {
class Debug;
}
#endif

#ifndef PDK_NO_DATASTREAM
namespace io {
class DataStream;
}
#endif

namespace time {

using pdk::lang::String;
using pdk::lang::StringView;

// forward declare class 
class TimeZone;
class DateTime;
namespace internal {
class DateTimePrivate;
} // internal
#if !defined(PDK_NO_DEBUG_STREAM) && !defined(PDK_NO_DATESTRING)
using pdk::io::Debug;
#endif

#ifndef PDK_NO_DATASTREAM
using pdk::io::DataStream;
#endif

using internal::DateTimePrivate;

class PDK_CORE_EXPORT Date
{
public:
   enum class MonthNameType
   {
      DateFormat = 0,
      StandaloneFormat
   };
private:
   explicit PDK_DECL_CONSTEXPR Date(pdk::pint64 julianDay) 
      : m_jd(julianDay)
   {}
public:
   constexpr Date()
      : m_jd(nullJd())
   {}
   
   Date(int year, int month, int day);
   
   constexpr bool isNull() const
   {
      return !isValid();
   }
   
   constexpr bool isValid() const
   {
      return m_jd >= minJd() && m_jd <= maxJd();
   }
   
   int getYear() const;
   int getMonth() const;
   int getDay() const;
   int getDayOfWeek() const;
   int getDayOfYear() const;
   int getDaysInMonth() const;
   int getDaysInYear() const;
   int getWeekNumber(int *yearNum = nullptr) const;
   
#ifndef PDK_NO_DATESTRING
   String toString(pdk::DateFormat f = pdk::DateFormat::TextDate) const;
#if PDK_STRINGVIEW_LEVEL < 2
   String toString(const String &format) const;
#endif
   String toString(StringView format) const;
#endif
   
   static String getShortMonthName(int month, MonthNameType type = MonthNameType::DateFormat);
   static String getShortDayName(int weekday, MonthNameType type = MonthNameType::DateFormat);
   static String getLongMonthName(int month, MonthNameType type = MonthNameType::DateFormat);
   static String getLongDayName(int weekday, MonthNameType type = MonthNameType::DateFormat);
   
   bool setDate(int year, int month, int day);
   void getDate(int *year, int *month, int *day) const;
   Date addDays(pdk::pint64 days) const PDK_REQUIRED_RESULT;
   Date addMonths(int months) const PDK_REQUIRED_RESULT;
   Date addYears(int years) const PDK_REQUIRED_RESULT;
   pdk::pint64 daysTo(const Date &) const;
   
   constexpr bool operator==(const Date &other) const
   {
      return m_jd == other.m_jd;
   }
   
   constexpr bool operator!=(const Date &other) const
   {
      return m_jd != other.m_jd;
   }
   
   constexpr bool operator< (const Date &other) const
   {
      return m_jd <  other.m_jd;
   }
   
   constexpr bool operator<=(const Date &other) const
   {
      return m_jd <= other.m_jd;
   }
   
   constexpr bool operator> (const Date &other) const
   {
      return m_jd >  other.m_jd;
   }
   
   constexpr bool operator>=(const Date &other) const
   {
      return m_jd >= other.m_jd;
   }
   
   static Date getCurrentDate();
#ifndef PDK_NO_DATESTRING
   static Date fromString(const String &s, pdk::DateFormat f = pdk::DateFormat::TextDate);
   static Date fromString(const String &s, const String &format);
#endif
   static bool isValid(int year, int month, int day);
   static bool isLeapYear(int year);
   
   static constexpr inline Date fromJulianDay(pdk::pint64 jd)
   {
      return jd >= minJd() && jd <= maxJd() ? Date(jd) : Date();
   }
   
   constexpr inline pdk::pint64 toJulianDay() const
   {
      return m_jd;
   }
   
private:
   // using extra parentheses around min to avoid expanding it if it is a macro
   static constexpr inline pdk::pint64 nullJd()
   {
      return (std::numeric_limits<pdk::pint64>::min)();
   }
   
   static constexpr inline pdk::pint64 minJd()
   {
      return PDK_INT64_C(-784350574879);
   }
   
   static constexpr inline pdk::pint64 maxJd()
   {
      return PDK_INT64_C(784354017364);
   }
private:
   friend class DateTime;
   friend class DateTimePrivate;
#ifndef PDK_NO_DATASTREAM
    friend PDK_CORE_EXPORT DataStream &operator<<(DataStream &, const Date &);
    friend PDK_CORE_EXPORT DataStream &operator>>(DataStream &, Date &);
#endif
   pdk::pint64 m_jd;
};

#ifndef PDK_NO_DATASTREAM
PDK_CORE_EXPORT DataStream &operator<<(DataStream &, const Date &);
#endif

#if !defined(PDK_NO_DEBUG_STREAM) && !defined(PDK_NO_DATESTRING)
PDK_CORE_EXPORT Debug operator<<(Debug, const Date &);
#endif

PDK_CORE_EXPORT uint pdk_hash(const Date &key, uint seed = 0) noexcept;

} // time
} // pdk

PDK_DECLARE_TYPEINFO(pdk::time::Date, PDK_MOVABLE_TYPE);

#endif // PDK_M_BASE_TIME_DATE_H
