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

#include "pdk/global/PlatformDefs.h"
#include "pdk/base/time/internal/DateTimePrivate.h"

#if PDK_CONFIG(datetimeparser)
#include "pdk/base/time/internal/DateTimeParserPrivate.h"
#endif

#include "pdk/utils/Locale.h"
#include "pdk/base/time/Date.h"
#include "pdk/base/time/Time.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/io/DataStream.h"
#include "pdk/kernel/HashFuncs.h"
#if PDK_CONFIG(timezone)
#include "pdk/base/time/internal/TimeZonePrivate.h"
#endif
#if !defined(PDK_NO_DEBUG_STREAM) && !defined(PDK_NO_DATESTRING)
#include "pdk/base/io/Debug.h"
#endif

#ifndef PDK_OS_WIN
#include <locale.h>
#endif

#include <cmath>
#include <time.h>
#ifdef PDK_OS_WIN
#  include "pdk/global/Windows.h"
#endif

#if defined(PDK_OS_MAC)
#include "pdk/kernel/internal/CoreMacPrivate.h"
#endif

namespace pdk {
namespace time {

using pdk::lang::String;
using pdk::lang::StringRef;
using pdk::lang::Latin1Character;
using pdk::lang::Latin1String;
using pdk::lang::Character;
using pdk::lang::StringView;
using pdk::utils::Locale;
using pdk::io::DataStream;

#if !defined(PDK_NO_DEBUG_STREAM) && !defined(PDK_NO_DATESTRING)
using pdk::io::Debug;
using pdk::io::DebugStateSaver;
#endif

constexpr const static int SECS_PER_DAY = 86400;
constexpr const static int MSECS_PER_DAY = 86400000;
constexpr const static int SECS_PER_HOUR = 3600;
constexpr const static int MSECS_PER_HOUR = 3600000;
constexpr const static int SECS_PER_MIN = 60;
constexpr const static int MSECS_PER_MIN = 60000;
constexpr const static int TIME_T_MAX = 2145916799;  // int maximum 2037-12-31T23:59:59 UTC
constexpr const static int JULIAN_DAY_FOR_EPOCH = 2440588; // result of julian_day_from_date(1970, 1, 1)

namespace {

inline Date fixed_date(int y, int m, int d)
{
   Date result(y, m, 1);
   result.setDate(y, m, std::min(d, result.getDaysInMonth()));
   return result;
}

/*
  Division, rounding down (rather than towards zero).
  
  From C++11 onwards, integer division is defined to round towards zero, so we
  can rely on that when implementing this.  This is only used with denominator b
  > 0, so we only have to treat negative numerator, a, specially.
 */
inline pdk::pint64 floordiv(pdk::pint64 a, int b)
{
   return (a - (a < 0 ? b - 1 : 0)) / b;
}

inline int floordiv(int a, int b)
{
   return (a - (a < 0 ? b - 1 : 0)) / b;
}

static inline pdk::pint64 julian_day_from_date(int year, int month, int day)
{
   // Adjust for no year 0
   if (year < 0) {
      ++year;
   }
   // Math from The Calendar FAQ at http://www.tondering.dk/claus/cal/julperiod.php
   // This formula is correct for all julian days, when using mathematical integer
   // division (round to negative infinity), not c++11 integer division (round to zero)
   int    a = floordiv(14 - month, 12);
   pdk::pint64 y = (pdk::pint64)year + 4800 - a;
   int    m = month + 12 * a - 3;
   return day + floordiv(153 * m + 2, 5) + 365 * y + floordiv(y, 4) - floordiv(y, 100) + floordiv(y, 400) - 32045;
}

struct ParsedDate
{
   int m_year;
   int m_month;
   int m_day;
};

// prevent this function from being inlined into all 10 users
PDK_NEVER_INLINE
static ParsedDate get_date_from_julian_day(pdk::pint64 julianDay)
{
   /*
 * Math from The Calendar FAQ at http://www.tondering.dk/claus/cal/julperiod.php
 * This formula is correct for all julian days, when using mathematical integer
 * division (round to negative infinity), not c++11 integer division (round to zero)
 */
   pdk::pint64 a = julianDay + 32044;
   pdk::pint64 b = floordiv(4 * a + 3, 146097);
   int    c = a - floordiv(146097 * b, 4);
   int    d = floordiv(4 * c + 3, 1461);
   int    e = c - floordiv(1461 * d, 4);
   int    m = floordiv(5 * e + 2, 153);
   int    day = e - floordiv(153 * m + 2, 5) + 1;
   int    month = m + 3 - 12 * floordiv(m, 10);
   int    year = 100 * b + d - 4800 + floordiv(m, 10);
   // Adjust for no year 0
   if (year <= 0) {
      --year ;
   }
   return { year, month, day };
}

static const char sg_monthDays[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

#ifndef PDK_NO_TEXTDATE
static const char sg_shortMonthNames[][4] = {
   "Jan", "Feb", "Mar", "Apr", "May", "Jun",
   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

int month_number_from_short_name(StringRef shortName)
{
   for (unsigned int i = 0; i < sizeof(sg_shortMonthNames) / sizeof(sg_shortMonthNames[0]); ++i) {
      if (shortName == Latin1String(sg_shortMonthNames[i], 3)) {
         return i + 1;
      }
   }
   return -1;
}

int month_number_from_short_name(const String &shortName)
{
   return month_number_from_short_name(StringRef(&shortName));
}

int from_short_month_name(const StringRef &monthName)
{
   // Assume that English monthnames are the default
   int month = month_number_from_short_name(monthName);
   if (month != -1) {
      return month;
   }
   // If English names can't be found, search the localized ones
   for (int i = 1; i <= 12; ++i) {
      if (monthName == Locale::system().monthName(i, Locale::FormatType::ShortFormat)) {
         return i;
      }
   }
   return -1;
}
#endif // PDK_NO_TEXTDATE

#ifndef PDK_NO_DATESTRING
struct ParsedRfcDateTime
{
   Date m_date;
   Time m_time;
   int m_utcOffset;
};

ParsedRfcDateTime rfc_date_impl(const String &s)
{
   ParsedRfcDateTime result;
   
   //   // Matches "Wdy, dd Mon yyyy HH:mm:ss ±hhmm" (Wdy, being optional)
   //   QRegExp rex(StringLiteral("^(?:[A-Z][a-z]+,)?[ \\t]*(\\d{1,2})[ \\t]+([A-Z][a-z]+)[ \\t]+(\\d\\d\\d\\d)(?:[ \\t]+(\\d\\d):(\\d\\d)(?::(\\d\\d))?)?[ \\t]*(?:([+-])(\\d\\d)(\\d\\d))?"));
   //   if (s.indexOf(rex) == 0) {
   //      const StringList cap = rex.capturedTexts();
   //      result.date = Date(cap[3].toInt(), PDK_monthNumberFromShortName(cap[2]), cap[1].toInt());
   //      if (!cap[4].isEmpty())
   //         result.time = Time(cap[4].toInt(), cap[5].toInt(), cap[6].toInt());
   //      const bool positiveOffset = (cap[7] == Latin1String("+"));
   //      const int hourOffset = cap[8].toInt();
   //      const int minOffset = cap[9].toInt();
   //      result.utcOffset = ((hourOffset * 60 + minOffset) * (positiveOffset ? 60 : -60));
   //   } else {
   //      // Matches "Wdy Mon dd HH:mm:ss yyyy"
   //      QRegExp rex(StringLiteral("^[A-Z][a-z]+[ \\t]+([A-Z][a-z]+)[ \\t]+(\\d\\d)(?:[ \\t]+(\\d\\d):(\\d\\d):(\\d\\d))?[ \\t]+(\\d\\d\\d\\d)[ \\t]*(?:([+-])(\\d\\d)(\\d\\d))?"));
   //      if (s.indexOf(rex) == 0) {
   //         const StringList cap = rex.capturedTexts();
   //         result.date = Date(cap[6].toInt(), PDK_monthNumberFromShortName(cap[1]), cap[2].toInt());
   //         if (!cap[3].isEmpty())
   //            result.time = Time(cap[3].toInt(), cap[4].toInt(), cap[5].toInt());
   //         const bool positiveOffset = (cap[7] == Latin1String("+"));
   //         const int hourOffset = cap[8].toInt();
   //         const int minOffset = cap[9].toInt();
   //         result.utcOffset = ((hourOffset * 60 + minOffset) * (positiveOffset ? 60 : -60));
   //      }
   //   }
   return result;
}
#endif // PDK_NO_DATESTRING

// Return offset in [+-]HH:mm format
String to_offset_string(pdk::DateFormat format, int offset)
{
   return String::asprintf("%c%02d%s%02d",
                           offset >= 0 ? '+' : '-',
                           std::abs(offset) / SECS_PER_HOUR,
                           // Qt::ISODate puts : between the hours and minutes, but Qt:TextDate does not:
                           format == pdk::DateFormat::TextDate ? "" : ":",
                           (std::abs(offset) / 60) % 60);
}

// Parse offset in [+-]HH[[:]mm] format
int from_offset_string(const StringRef &offsetString, bool *valid) noexcept
{
   *valid = false;
   const int size = offsetString.size();
   if (size < 2 || size > 6) {
      return 0;
   }
   // sign will be +1 for a positive and -1 for a negative offset
   int sign;
   // First char must be + or -
   const Character signChar = offsetString.at(0);
   if (signChar == Latin1Character('+')) {
      sign = 1;
   } else if (signChar == Latin1Character('-')) {
      sign = -1;
   } else {
      return 0;
   }
   // Split the hour and minute parts
   const StringRef time = offsetString.substring(1);
   int hhLen = time.indexOf(Latin1Character(':'));
   int mmIndex;
   if (hhLen == -1) {
      mmIndex = hhLen = 2; // [+-]HHmm or [+-]HH format
   } else {
      mmIndex = hhLen + 1;
   }
   const StringRef hhRef = time.left(hhLen);
   bool ok = false;
   const int hour = hhRef.toInt(&ok);
   if (!ok) {
      return 0;
   }
   const StringRef mmRef = time.substring(mmIndex);
   const int minute = mmRef.isEmpty() ? 0 : mmRef.toInt(&ok);
   if (!ok || minute < 0 || minute > 59) {
      return 0;
   }
   *valid = true;
   return sign * ((hour * 60) + minute) * 60;
}

} // anonymous namespace

/*****************************************************************************
  Date member functions
 *****************************************************************************/
Date::Date(int y, int m, int d)
{
   setDate(y, m, d);
}


int Date::getYear() const
{
   if (isNull()) {
      return 0;
   }
   return get_date_from_julian_day(m_jd).m_year;
}

int Date::getMonth() const
{
   if (isNull()) {
      return 0;
   }
   return get_date_from_julian_day(m_jd).m_month;
}

int Date::getDay() const
{
   if (isNull()) {
      return 0;
   }
   return get_date_from_julian_day(m_jd).m_day;
}

int Date::getDayOfWeek() const
{
   if (isNull()) {
      return 0;
   }
   if (m_jd >= 0) {
      return (m_jd % 7) + 1;
   } else {
      return ((m_jd + 1) % 7) + 7;
   }
}

int Date::getDayOfYear() const
{
   if (isNull()) {
      return 0;
   }
   return m_jd - julian_day_from_date(getYear(), 1, 1) + 1;
}

int Date::getDaysInMonth() const
{
   if (isNull()) {
      return 0;
   }
   const ParsedDate pd = get_date_from_julian_day(m_jd);
   if (pd.m_month == 2 && isLeapYear(pd.m_year)) {
      return 29;
   } else {
      return sg_monthDays[pd.m_month];
   }   
}

int Date::getDaysInYear() const
{
   if (isNull()) {
      return 0;
   }
   return isLeapYear(get_date_from_julian_day(m_jd).m_year) ? 366 : 365;
}

int Date::getWeekNumber(int *yearNumber) const
{
   if (!isValid())
      return 0;
   
   int year = Date::getYear();
   int yday = getDayOfYear();
   int wday = getDayOfWeek();
   
   int week = (yday - wday + 10) / 7;
   
   if (week == 0) {
      // last week of previous year
      --year;
      week = (yday + 365 + (Date::isLeapYear(year) ? 1 : 0) - wday + 10) / 7;
      PDK_ASSERT(week == 52 || week == 53);
   } else if (week == 53) {
      // maybe first week of next year
      int w = (yday - 365 - (Date::isLeapYear(year) ? 1 : 0) - wday + 10) / 7;
      if (w > 0) {
         ++year;
         week = w;
      }
      PDK_ASSERT(week == 53 || week == 1);
   }
   if (yearNumber != 0) {
      *yearNumber = year;
   }
   return week;
}

#ifndef PDK_NO_DATESTRING

namespace {

#ifndef PDK_NO_TEXTDATE
String to_string_text_date(Date date)
{
   const ParsedDate pd = get_date_from_julian_day(date.toJulianDay());
   static const Latin1Character sp(' ');
   return Locale::system().dayName(date.getDayOfWeek(), Locale::FormatType::ShortFormat) + sp
         + Locale::system().monthName(pd.m_month, Locale::FormatType::ShortFormat) + sp
         + String::number(pd.m_day) + sp
         + String::number(pd.m_year);
}
#endif // PDK_NO_TEXTDATE

String to_string_iso_date(pdk::pint64 jd)
{
   const ParsedDate pd = get_date_from_julian_day(jd);
   if (pd.m_year >= 0 && pd.m_year <= 9999) {
      return String::asprintf("%04d-%02d-%02d", pd.m_year, pd.m_month, pd.m_day);
   } else {
      return String();
   }
}

} // anonymous namespace

String Date::toString(pdk::DateFormat format) const
{
   if (!isValid()) {
      return String();
   }
   switch (format) {
   case pdk::DateFormat::SystemLocaleShortDate:
      return Locale::system().toString(*this, Locale::FormatType::ShortFormat);
   case pdk::DateFormat::SystemLocaleLongDate:
      return Locale::system().toString(*this, Locale::FormatType::   LongFormat);
   case pdk::DateFormat::DefaultLocaleShortDate:
      return Locale().toString(*this, Locale::FormatType::ShortFormat);
   case pdk::DateFormat::DefaultLocaleLongDate:
      return Locale().toString(*this, Locale::FormatType::LongFormat);
   case pdk::DateFormat::RFC2822Date:
      return Locale::c().toString(*this, StringViewLiteral("dd MMM yyyy"));
#ifndef PDK_NO_TEXTDATE
   case pdk::DateFormat::TextDate:
      return to_string_text_date(*this);
#endif
   case pdk::DateFormat::ISODate:
   case pdk::DateFormat::ISODateWithMs:
      return to_string_iso_date(m_jd);
   }
}

String Date::toString(StringView format) const
{
   return Locale::system().toString(*this, format); // Locale::c() ### Qt6
}

#if PDK_STRINGVIEW_LEVEL < 2
String Date::toString(const String &format) const
{
   return toString(pdk::lang::to_string_view_ignoring_null(format));
}
#endif

#endif //PDK_NO_DATESTRING

bool Date::setDate(int year, int month, int day)
{
   if (isValid(year, month, day)) {
      m_jd = julian_day_from_date(year, month, day);
   } else {
      m_jd = nullJd();
   }
   return isValid();
}

void Date::getDate(int *year, int *month, int *day) const
{
   ParsedDate pd = { 0, 0, 0 };
   if (isValid()) {
      pd = get_date_from_julian_day(m_jd);
   }
   if (year) {
      *year = pd.m_year;
   }
   if (month) {
      *month = pd.m_month;
   }      
   if (day) {
      *day = pd.m_day;
   }
}

Date Date::addDays(pdk::pint64 ndays) const
{
   if (isNull()) {
      return Date();
   }
   // Due to limits on minJd() and maxJd() we know that any overflow
   // will be invalid and caught by fromJulianDay().
   return fromJulianDay(m_jd + ndays);
}

Date Date::addMonths(int nmonths) const
{
   if (!isValid()) {
      return Date();
   }
   if (!nmonths) {
      return *this;
   }
   int oldY, y, m, d;
   {
      const ParsedDate pd = get_date_from_julian_day(m_jd);
      y = pd.m_year;
      m = pd.m_month;
      d = pd.m_day;
   }
   oldY = y;
   bool increasing = nmonths > 0;
   while (nmonths != 0) {
      if (nmonths < 0 && nmonths + 12 <= 0) {
         y--;
         nmonths+=12;
      } else if (nmonths < 0) {
         m+= nmonths;
         nmonths = 0;
         if (m <= 0) {
            --y;
            m += 12;
         }
      } else if (nmonths - 12 >= 0) {
         y++;
         nmonths -= 12;
      } else if (m == 12) {
         y++;
         m = 0;
      } else {
         m += nmonths;
         nmonths = 0;
         if (m > 12) {
            ++y;
            m -= 12;
         }
      }
   }
   
   // was there a sign change?
   if ((oldY > 0 && y <= 0) ||
       (oldY < 0 && y >= 0)) {
      // yes, adjust the date by +1 or -1 years
      y += increasing ? +1 : -1;
   }
   
   return fixed_date(y, m, d);
}

Date Date::addYears(int nyears) const
{
   if (!isValid())
      return Date();
   
   ParsedDate pd = get_date_from_julian_day(m_jd);
   
   int oldy = pd.m_year;
   pd.m_year += nyears;
   
   // was there a sign change?
   if ((oldy > 0 && pd.m_year <= 0) ||
       (oldy < 0 && pd.m_year >= 0)) {
      // yes, adjust the date by +1 or -1 years
      pd.m_year += nyears > 0 ? +1 : -1;
   }
   return fixed_date(pd.m_year, pd.m_month, pd.m_day);
}

pdk::pint64 Date::daysTo(const Date &date) const
{
   if (isNull() || date.isNull()) {
      return 0;
   }
   // Due to limits on minJd() and maxJd() we know this will never overflow
   return date.m_jd - m_jd;
}

#ifndef PDK_NO_DATESTRING

Date Date::fromString(const String& string, pdk::DateFormat format)
{
   if (string.isEmpty()) {
      return Date();
   }
   switch (format) {
   case pdk::DateFormat::SystemLocaleShortDate:
      return Locale::system().toDate(string, Locale::FormatType::ShortFormat);
   case pdk::DateFormat::SystemLocaleLongDate:
      return Locale::system().toDate(string, Locale::FormatType::LongFormat);
   case pdk::DateFormat::DefaultLocaleShortDate:
      return Locale().toDate(string, Locale::FormatType::ShortFormat);
   case pdk::DateFormat::DefaultLocaleLongDate:
      return Locale().toDate(string, Locale::FormatType::LongFormat);
   case pdk::DateFormat::RFC2822Date:
      return rfc_date_impl(string).m_date;
   default:
#ifndef PDK_NO_TEXTDATE
   case pdk::DateFormat::TextDate: {
      std::vector<StringRef> parts = string.splitRef(Latin1Character(' '), String::SplitBehavior::SkipEmptyParts);
      if (parts.size() != 4) {
         return Date();
      }
      StringRef monthName = parts.at(1);
      const int month = from_short_month_name(monthName);
      if (month == -1) {
         // Month name matches neither English nor other localised name.
         return Date();
      }
      bool ok = false;
      int year = parts.at(3).toInt(&ok);
      if (!ok) {
         return Date();
      }
      return Date(year, month, parts.at(2).toInt());
   }
#endif // PDK_NO_TEXTDATE
   case pdk::DateFormat::ISODate: {
      // Semi-strict parsing, must be long enough and have non-numeric separators
      if (string.size() < 10 || string.at(4).isDigit() || string.at(7).isDigit()
          || (string.size() > 10 && string.at(10).isDigit())) {
         return Date();
      }
      const int year = string.substringRef(0, 4).toInt();
      if (year <= 0 || year > 9999) {
         return Date();
      }
      return Date(year, string.substringRef(5, 2).toInt(), string.substringRef(8, 2).toInt());
   }
   }
   return Date();
}

Date Date::fromString(const String &string, const String &format)
{
   Date date;
#if PDK_CONFIG(datetimeparser)
   //   DateTimeParser dt(QVariant::Date, DateTimeParser::FromString);
   //   // dt.setDefaultLocale(Locale::c()); ### Qt 6
   //   if (dt.parseFormat(format))
   //      dt.fromString(string, &date, 0);
#else
   PDK_UNUSED(string);
   PDK_UNUSED(format);
#endif
   return date;
}
#endif // PDK_NO_DATESTRING

bool Date::isValid(int year, int month, int day)
{
   // there is no year 0 in the Gregorian calendar
   if (year == 0) {
      return false;
   }
   return (day > 0 && month > 0 && month <= 12) &&
         (day <= sg_monthDays[month] || (day == 29 && month == 2 && isLeapYear(year)));
}

bool Date::isLeapYear(int y)
{
   // No year 0 in Gregorian calendar, so -1, -5, -9 etc are leap years
   if ( y < 1) {
      ++y;
   }
   return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

/*****************************************************************************
  Time member functions
 *****************************************************************************/
Time::Time(int h, int m, int s, int ms)
{
   setHMS(h, m, s, ms);
}

bool Time::isValid() const
{
   return m_mds > NULL_TIME && m_mds < MSECS_PER_DAY;
}

int Time::getHour() const
{
   if (!isValid()) {
      return -1;
   }
   return ds() / MSECS_PER_HOUR;
}

int Time::getMinute() const
{
   if (!isValid()) {
      return -1;
   }
   return (ds() % MSECS_PER_HOUR) / MSECS_PER_MIN;
}

int Time::getSecond() const
{
   if (!isValid()) {
      return -1;
   }
   return (ds() / 1000)%SECS_PER_MIN;
}

int Time::getMsec() const
{
   if (!isValid()) {
      return -1;
   }
   return ds() % 1000;
}

#ifndef PDK_NO_DATESTRING
String Time::toString(pdk::DateFormat format) const
{
   if (!isValid()) {
      return String();
   }
   switch (format) {
   case pdk::DateFormat::SystemLocaleShortDate:
      return Locale::system().toString(*this, Locale::FormatType::ShortFormat);
   case pdk::DateFormat::SystemLocaleLongDate:
      return Locale::system().toString(*this, Locale::FormatType::LongFormat);
   case pdk::DateFormat::DefaultLocaleShortDate:
      return Locale().toString(*this, Locale::FormatType::ShortFormat);
   case pdk::DateFormat::DefaultLocaleLongDate:
      return Locale().toString(*this, Locale::FormatType::LongFormat);
   case pdk::DateFormat::ISODateWithMs:
      return String::asprintf("%02d:%02d:%02d.%03d", getHour(), getMinute(), getSecond(), getMsec());
   case pdk::DateFormat::RFC2822Date:
   case pdk::DateFormat::ISODate:
   case pdk::DateFormat::TextDate:
      return String::asprintf("%02d:%02d:%02d", getHour(), getMinute(), getSecond());
   }
}

String Time::toString(StringView format) const
{
   return Locale::system().toString(*this, format); // Locale::c() ### Qt6
}

#if PDK_STRINGVIEW_VERSION < 2
String Time::toString(const String &format) const
{
   return toString(pdk::lang::to_string_view_ignoring_null(format));
}
#endif

#endif //PDK_NO_DATESTRING

bool Time::setHMS(int h, int m, int s, int ms)
{
   if (!isValid(h,m,s,ms)) {
      m_mds = NULL_TIME;                // make this invalid
      return false;
   }
   m_mds = (h*SECS_PER_HOUR + m*SECS_PER_MIN + s)*1000 + ms;
   return true;
}

Time Time::addSecs(int s) const
{
   s %= SECS_PER_DAY;
   return addMSecs(s * 1000);
}


int Time::secsTo(const Time &t) const
{
   if (!isValid() || !t.isValid())
      return 0;
   
   // Truncate milliseconds as we do not want to consider them.
   int ourSeconds = ds() / 1000;
   int theirSeconds = t.ds() / 1000;
   return theirSeconds - ourSeconds;
}

Time Time::addMSecs(int ms) const
{
   Time t;
   if (isValid()) {
      if (ms < 0) {
         // %,/ not well-defined for -ve, so always work with +ve.
         int negdays = (MSECS_PER_DAY - ms) / MSECS_PER_DAY;
         t.m_mds = (ds() + ms + negdays * MSECS_PER_DAY) % MSECS_PER_DAY;
      } else {
         t.m_mds = (ds() + ms) % MSECS_PER_DAY;
      }
   }
   return t;
}

int Time::msecsTo(const Time &t) const
{
   if (!isValid() || !t.isValid())
      return 0;
   return t.ds() - ds();
}

#ifndef PDK_NO_DATESTRING

static Time fromIsoTimeString(const StringRef &string, pdk::DateFormat format, bool *isMidnight24)
{
   if (isMidnight24)
      *isMidnight24 = false;
   
   const int size = string.size();
   if (size < 5)
      return Time();
   
   bool ok = false;
   int hour = string.substring(0, 2).toInt(&ok);
   if (!ok) {
      return Time();
   }
   const int minute = string.substring(3, 2).toInt(&ok);
   if (!ok) {
      return Time();
   }
   int second = 0;
   int msec = 0;
   
   if (size == 5) {
      // HH:mm format
      second = 0;
      msec = 0;
   } else if (string.at(5) == Latin1Character(',') || string.at(5) == Latin1Character('.')) {
      if (format == pdk::DateFormat::TextDate) {
         return Time();
      }
      
      // ISODate HH:mm.ssssss format
      // We only want 5 digits worth of fraction of minute. This follows the existing
      // behavior that determines how milliseconds are read; 4 millisecond digits are
      // read and then rounded to 3. If we read at most 5 digits for fraction of minute,
      // the maximum amount of millisecond digits it will expand to once converted to
      // seconds is 4. E.g. 12:34,99999 will expand to 12:34:59.9994. The milliseconds
      // will then be rounded up AND clamped to 999.
      
      const StringRef minuteFractionStr = string.substring(6, 5);
      const long minuteFractionInt = minuteFractionStr.toLong(&ok);
      if (!ok) {
         return Time();
      }
      const float minuteFraction = double(minuteFractionInt) / (std::pow(double(10), minuteFractionStr.count()));
      
      const float secondWithMs = minuteFraction * 60;
      const float secondNoMs = std::floor(secondWithMs);
      const float secondFraction = secondWithMs - secondNoMs;
      second = secondNoMs;
      msec = std::min(std::lround(secondFraction * 1000.0), (long)999);
   } else {
      // HH:mm:ss or HH:mm:ss.zzz
      second = string.substring(6, 2).toInt(&ok);
      if (!ok) {
         return Time();
      }
      if (size > 8 && (string.at(8) == Latin1Character(',') || string.at(8) == Latin1Character('.'))) {
         const StringRef msecStr(string.substring(9, 4));
         int msecInt = msecStr.isEmpty() ? 0 : msecStr.toInt(&ok);
         if (!ok) {
            return Time();
         } 
         const double secondFraction(msecInt / (std::pow(double(10), msecStr.count())));
         msec = std::min(std::lround(secondFraction * 1000.0), (long)999);
      }
   }
   
   const bool isISODate = format == pdk::DateFormat::ISODate || format == pdk::DateFormat::ISODateWithMs;
   if (isISODate && hour == 24 && minute == 0 && second == 0 && msec == 0) {
      if (isMidnight24) {
         *isMidnight24 = true;
      }
      hour = 0;
   }
   return Time(hour, minute, second, msec);
}

Time Time::fromString(const String& string, pdk::DateFormat format)
{
   if (string.isEmpty()) {
      return Time();
   }
   switch (format) {
   case pdk::DateFormat::SystemLocaleShortDate:
      return Locale::system().toTime(string, Locale::FormatType::ShortFormat);
   case pdk::DateFormat::SystemLocaleLongDate:
      return Locale::system().toTime(string, Locale::FormatType::LongFormat);
   case pdk::DateFormat::DefaultLocaleShortDate:
      return Locale().toTime(string, Locale::FormatType::ShortFormat);
   case pdk::DateFormat::DefaultLocaleLongDate:
      return Locale().toTime(string, Locale::FormatType::LongFormat);
   case pdk::DateFormat::RFC2822Date:
      return rfc_date_impl(string).m_time;
   case pdk::DateFormat::ISODate:
   case pdk::DateFormat::ISODateWithMs:
   case pdk::DateFormat::TextDate:
      return fromIsoTimeString(StringRef(&string), format, 0);
   }
}

Time Time::fromString(const String &string, const String &format)
{
   Time time;
#if PDK_CONFIG(datetimeparser)
   //   DateTimeParser dt(QVariant::Time, DateTimeParser::FromString);
   //   // dt.setDefaultLocale(Locale::c()); ### Qt 6
   //   if (dt.parseFormat(format))
   //      dt.fromString(string, 0, &time);
#else
   PDK_UNUSED(string);
   PDK_UNUSED(format);
#endif
   return time;
}

#endif // PDK_NO_DATESTRING


bool Time::isValid(int h, int m, int s, int ms)
{
   return (uint)h < 24 && (uint)m < 60 && (uint)s < 60 && (uint)ms < 1000;
}

void Time::start()
{
   *this = getCurrentTime();
}

int Time::restart()
{
   Time t = getCurrentTime();
   int n = msecsTo(t);
   if (n < 0) {                             // passed midnight
      n += 86400*1000;
   }
   *this = t;
   return n;
}

int Time::elapsed() const
{
   int n = msecsTo(getCurrentTime());
   if (n < 0) {                            // passed midnight
      n += 86400 * 1000;
   }
   return n;
}

/*****************************************************************************
  DateTime static helper functions
 *****************************************************************************/
// get the types from DateTime (through DateTimePrivate)
using ShortData = DateTimePrivate::DateTimeShortData;
using DateTimeData = DateTimePrivate::DateTimeData;

namespace {

void pdk_tzset()
{
#if defined(PDK_OS_WIN)
   _tzset();
#else
   tzset();
#endif // PDK_OS_WIN
}

// Returns the platform variant of timezone, i.e. the standard time offset
// The timezone external variable is documented as always holding the
// Standard Time offset as seconds west of Greenwich, i.e. UTC+01:00 is -3600
// Note this may not be historicaly accurate.
// Relies on tzset, mktime, or localtime having been called to populate timezone
int pdk_timezone()
{
#if defined(_MSC_VER)
   long offset;
   _get_timezone(&offset);
   return offset;
#elif defined(PDK_OS_BSD4) && !defined(PDK_OS_DARWIN)
   time_t clock = time(NULL);
   struct tm t;
   localtime_r(&clock, &t);
   // PDKBUG-36080 Workaround for systems without the POSIX timezone
   // variable. This solution is not very efficient but fixing it is up to
   // the libc implementations.
   //
   // tm_gmtoff has some important differences compared to the timezone
   // variable:
   // - It returns the number of seconds east of UTC, and we want the
   //   number of seconds west of UTC.
   // - It also takes DST into account, so we need to adjust it to always
   //   get the Standard Time offset.
   return -t.tm_gmtoff + (t.tm_isdst ? (long)SECS_PER_HOUR : 0L);
#elif defined(PDK_OS_INTEGRITY)
   return 0;
#else
   return timezone;
#endif // PDK_OS_WIN
}

// Returns the tzname, assume tzset has been called already
String pdk_tzname(DateTimePrivate::DaylightStatus daylightStatus)
{
   int isDst = (daylightStatus == DateTimePrivate::DaylightStatus::DaylightTime) ? 1 : 0;
#if defined(_MSC_VER) && _MSC_VER >= 1400
   size_t s = 0;
   char name[512];
   if (_get_tzname(&s, name, 512, isDst))
      return String();
   return String::fromLocal8Bit(name);
#else
   return String::fromLocal8Bit(tzname[isDst]);
#endif // PDK_OS_WIN
}

#if PDK_CONFIG(datetimeparser) && PDK_CONFIG(timezone)
/*
  \internal
  Implemented here to share pdk_tzname()
*/
int DateTimeParser::startsWithLocalTimeZone(const StringRef name)
{
   DateTimePrivate::DaylightStatus zones[2] = {
      DateTimePrivate::StandardTime,
      DateTimePrivate::DaylightTime
   };
   for (const auto z : zones) {
      String zone(pdk_tzname(z));
      if (name.startsWith(zone)) {
         return zone.size();
      }
   }
   return 0;
}
#endif // datetimeparser && timezone

// Calls the platform variant of mktime for the given date, time and daylightStatus,
// and updates the date, time, daylightStatus and abbreviation with the returned values
// If the date falls outside the 1970 to 2037 range supported by mktime / time_t
// then null date/time will be returned, you should adjust the date first if
// you need a guaranteed result.
pdk::pint64 pdk_mktime(Date *date, Time *time, DateTimePrivate::DaylightStatus *daylightStatus,
                       String *abbreviation, bool *ok = 0)
{
   const pdk::pint64 msec = time->getMsec();
   int yy, mm, dd;
   date->getDate(&yy, &mm, &dd);
   // All other platforms provide standard C library time functions
   tm local;
   memset(&local, 0, sizeof(local)); // tm_[wy]day plus any non-standard fields
   local.tm_sec = time->getSecond();
   local.tm_min = time->getMinute();
   local.tm_hour = time->getHour();
   local.tm_mday = dd;
   local.tm_mon = mm - 1;
   local.tm_year = yy - 1900;
   if (daylightStatus) {
      local.tm_isdst = int(*daylightStatus);
   } else {
      local.tm_isdst = -1;
   }
#if defined(PDK_OS_WIN)
   int hh = local.tm_hour;
#endif // PDK_OS_WIN
   time_t secsSinceEpoch = mktime(&local);
   if (secsSinceEpoch != time_t(-1)) {
      *date = Date(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
      *time = Time(local.tm_hour, local.tm_min, local.tm_sec, msec);
#if defined(PDK_OS_WIN)
      // Windows mktime for the missing hour subtracts 1 hour from the time
      // instead of adding 1 hour.  If time differs and is standard time then
      // this has happened, so add 2 hours to the time and 1 hour to the msecs
      if (local.tm_isdst == 0 && local.tm_hour != hh) {
         if (time->getHour() >= 22) {
            *date = date->addDays(1);
         }
         *time = time->addSecs(2 * SECS_PER_HOUR);
         secsSinceEpoch += SECS_PER_HOUR;
         local.tm_isdst = 1;
      }
#endif // PDK_OS_WIN
      if (local.tm_isdst >= 1) {
         if (daylightStatus) {
            *daylightStatus = DateTimePrivate::DaylightStatus::DaylightTime;
         }            
         if (abbreviation) {
            *abbreviation = pdk_tzname(DateTimePrivate::DaylightStatus::DaylightTime);
         } 
      } else if (local.tm_isdst == 0) {
         if (daylightStatus) {
            *daylightStatus = DateTimePrivate::DaylightStatus::StandardTime;
         }
         if (abbreviation) {
            *abbreviation = pdk_tzname(DateTimePrivate::DaylightStatus::StandardTime);
         }
      } else {
         if (daylightStatus) {
            *daylightStatus = DateTimePrivate::DaylightStatus::UnknownDaylightTime;
         }
         if (abbreviation) {
            *abbreviation = pdk_tzname(DateTimePrivate::DaylightStatus::StandardTime);
         }
      }
      if (ok) {
         *ok = true;
      }
   } else {
      *date = Date();
      *time = Time();
      if (daylightStatus) {
         *daylightStatus = DateTimePrivate::DaylightStatus::UnknownDaylightTime;
      }
      if (abbreviation) {
         *abbreviation = String();
      }
      if (ok) {
         *ok = false;
      }
   }
   return ((pdk::pint64)secsSinceEpoch * 1000) + msec;
}

// Calls the platform variant of localtime for the given msecs, and updates
// the date, time, and DST status with the returned values.
bool pdk_localtime(pdk::pint64 msecsSinceEpoch, Date *localDate, Time *localTime,
                   DateTimePrivate::DaylightStatus *daylightStatus)
{
   const time_t secsSinceEpoch = msecsSinceEpoch / 1000;
   const int msec = msecsSinceEpoch % 1000;
   
   tm local;
   bool valid = false;
   
   // localtime() is required to work as if tzset() was called before it.
   // localtime_r() does not have this requirement, so make an explicit call.
   // The explicit call should also request the timezone info be re-parsed.
   pdk_tzset();
#if !defined(pdk_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS)
   // Use the reentrant version of localtime() where available
   // as is thread-safe and doesn't use a shared static data area
   tm *res = 0;
   res = localtime_r(&secsSinceEpoch, &local);
   if (res)
      valid = true;
#elif defined(_MSC_VER) && _MSC_VER >= 1400
   if (!_localtime64_s(&local, &secsSinceEpoch))
      valid = true;
#else
   // Returns shared static data which may be overwritten at any time
   // So copy the result asap
   tm *res = 0;
   res = localtime(&secsSinceEpoch);
   if (res) {
      local = *res;
      valid = true;
   }
#endif
   if (valid) {
      *localDate = Date(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
      *localTime = Time(local.tm_hour, local.tm_min, local.tm_sec, msec);
      if (daylightStatus) {
         if (local.tm_isdst > 0) {
            *daylightStatus = DateTimePrivate::DaylightStatus::DaylightTime;
         } else if (local.tm_isdst < 0) {
            *daylightStatus = DateTimePrivate::DaylightStatus::UnknownDaylightTime;
         } else {
            *daylightStatus = DateTimePrivate::DaylightStatus::StandardTime;
         }
      }
      return true;
   } else {
      *localDate = Date();
      *localTime = Time();
      if (daylightStatus) {
         *daylightStatus = DateTimePrivate::DaylightStatus::UnknownDaylightTime;
      }
      return false;
   }
}

// Converts an msecs value into a date and time
void msecs_to_time(pdk::pint64 msecs, Date *date, Time *time)
{
   pdk::pint64 jd = JULIAN_DAY_FOR_EPOCH;
   pdk::pint64 ds = 0;
   
   if (std::abs(msecs) >= MSECS_PER_DAY) {
      jd += (msecs / MSECS_PER_DAY);
      msecs %= MSECS_PER_DAY;
   }
   
   if (msecs < 0) {
      ds = MSECS_PER_DAY - msecs - 1;
      jd -= ds / MSECS_PER_DAY;
      ds = ds % MSECS_PER_DAY;
      ds = MSECS_PER_DAY - ds - 1;
   } else {
      ds = msecs;
   }
   if (date) {
      *date = Date::fromJulianDay(jd);
   }
   if (time) {
      *time = Time::fromMSecsSinceStartOfDay(ds);
   }
}

// Converts a date/time value into msecs
pdk::pint64 time_to_msecs(const Date &date, const Time &time)
{
   return ((date.toJulianDay() - JULIAN_DAY_FOR_EPOCH) * MSECS_PER_DAY)
         + time.msecsSinceStartOfDay();
}

// Convert an MSecs Since Epoch into Local Time
bool epoch_msecs_to_localtime(pdk::pint64 msecs, Date *localDate, Time *localTime,
                              DateTimePrivate::DaylightStatus *daylightStatus = 0)
{
   if (msecs < 0) {
      // Docs state any LocalTime before 1970-01-01 will *not* have any Daylight Time applied
      // Instead just use the standard offset from UTC to convert to UTC time
      pdk_tzset();
      msecs_to_time(msecs - pdk_timezone() * 1000, localDate, localTime);
      if (daylightStatus) {
         *daylightStatus = DateTimePrivate::DaylightStatus::StandardTime;
      }
      return true;
   } else if (msecs > (pdk::pint64(TIME_T_MAX) * 1000)) {
      // Docs state any LocalTime after 2037-12-31 *will* have any DST applied
      // but this may fall outside the supported time_t range, so need to fake it.
      // Use existing method to fake the conversion, but this is deeply flawed as it may
      // apply the conversion from the wrong day number, e.g. if rule is last Sunday of month
      // TODO Use TimeZone when available to apply the future rule correctly
      Date utcDate;
      Time utcTime;
      msecs_to_time(msecs, &utcDate, &utcTime);
      int year, month, day;
      utcDate.getDate(&year, &month, &day);
      // 2037 is not a leap year, so make sure date isn't Feb 29
      if (month == 2 && day == 29) {
         --day;
      }
      Date fakeDate(2037, month, day);
      pdk::pint64 fakeMsecs = DateTime(fakeDate, utcTime, pdk::TimeSpec::UTC).toMSecsSinceEpoch();
      bool res = pdk_localtime(fakeMsecs, localDate, localTime, daylightStatus);
      *localDate = localDate->addDays(fakeDate.daysTo(utcDate));
      return res;
   } else {
      // Falls inside time_t suported range so can use localtime
      return pdk_localtime(msecs, localDate, localTime, daylightStatus);
   }
}

// Convert a LocalTime expressed in local msecs encoding and the corresponding
// DST status into a UTC epoch msecs. Optionally populate the returned
// values from mktime for the adjusted local date and time.
pdk::pint64 localMSecsToEpochMSecs(pdk::pint64 localMsecs,
                                   DateTimePrivate::DaylightStatus *daylightStatus,
                                   Date *localDate = 0, Time *localTime = 0,
                                   String *abbreviation = 0)
{
   Date dt;
   Time tm;
   msecs_to_time(localMsecs, &dt, &tm);
   const pdk::pint64 msecsMax = pdk::pint64(TIME_T_MAX) * 1000;
   if (localMsecs <= pdk::pint64(MSECS_PER_DAY)) {
      // Docs state any LocalTime before 1970-01-01 will *not* have any DST applied
      // First, if localMsecs is within +/- 1 day of minimum time_t try mktime in case it does
      // fall after minimum and needs proper DST conversion
      if (localMsecs >= -pdk::pint64(MSECS_PER_DAY)) {
         bool valid;
         pdk::pint64 utcMsecs = pdk_mktime(&dt, &tm, daylightStatus, abbreviation, &valid);
         if (valid && utcMsecs >= 0) {
            // mktime worked and falls in valid range, so use it
            if (localDate) {
               *localDate = dt;
            }
            if (localTime) {
               *localTime = tm;
            }
            return utcMsecs;
         }
      } else {
         // If we don't call mktime then need to call tzset to get offset
         pdk_tzset();
      }
      // Time is clearly before 1970-01-01 so just use standard offset to convert
      pdk::pint64 utcMsecs = localMsecs + pdk_timezone() * 1000;
      if (localDate || localTime) {
         msecs_to_time(localMsecs, localDate, localTime);
      }
      if (daylightStatus) {
         *daylightStatus = DateTimePrivate::DaylightStatus::StandardTime;
      }
      if (abbreviation) {
         *abbreviation = pdk_tzname(DateTimePrivate::DaylightStatus::StandardTime);
      }
      return utcMsecs;
      
   } else if (localMsecs >= msecsMax - MSECS_PER_DAY) {
      
      // Docs state any LocalTime after 2037-12-31 *will* have any DST applied
      // but this may fall outside the supported time_t range, so need to fake it.
      
      // First, if localMsecs is within +/- 1 day of maximum time_t try mktime in case it does
      // fall before maximum and can use proper DST conversion
      if (localMsecs <= msecsMax + MSECS_PER_DAY) {
         bool valid;
         pdk::pint64 utcMsecs = pdk_mktime(&dt, &tm, daylightStatus, abbreviation, &valid);
         if (valid && utcMsecs <= msecsMax) {
            // mktime worked and falls in valid range, so use it
            if (localDate) {
               *localDate = dt;
            }
            if (localTime) {
               *localTime = tm;
            }
            return utcMsecs;
         }
      }
      // Use existing method to fake the conversion, but this is deeply flawed as it may
      // apply the conversion from the wrong day number, e.g. if rule is last Sunday of month
      // TODO Use TimeZone when available to apply the future rule correctly
      int year, month, day;
      dt.getDate(&year, &month, &day);
      // 2037 is not a leap year, so make sure date isn't Feb 29
      if (month == 2 && day == 29) {
         --day;
      }         
      Date fakeDate(2037, month, day);
      pdk::pint64 fakeDiff = fakeDate.daysTo(dt);
      pdk::pint64 utcMsecs = pdk_mktime(&fakeDate, &tm, daylightStatus, abbreviation);
      if (localDate) {
         *localDate = fakeDate.addDays(fakeDiff);
      }
      if (localTime) {
         *localTime = tm;
      }
      Date utcDate;
      Time utcTime;
      msecs_to_time(utcMsecs, &utcDate, &utcTime);
      utcDate = utcDate.addDays(fakeDiff);
      utcMsecs = time_to_msecs(utcDate, utcTime);
      return utcMsecs;
   } else {
      // Clearly falls inside 1970-2037 suported range so can use mktime
      pdk::pint64 utcMsecs = pdk_mktime(&dt, &tm, daylightStatus, abbreviation);
      if (localDate) {
         *localDate = dt;
      }
      if (localTime) {
         *localTime = tm;
      }
      return utcMsecs;
   }
}

inline bool spec_can_be_small(pdk::TimeSpec spec)
{
   return spec == pdk::TimeSpec::LocalTime || spec == pdk::TimeSpec::UTC;
}

inline bool msecs_can_be_small(pdk::pint64 msecs)
{
   if (!DateTimeData::CanBeSmall) {
      return false;
   }
   ShortData sd;
   sd.m_msecs = pdk::intptr(msecs);
   return sd.m_msecs == msecs;
}

PDK_DECL_CONSTEXPR inline
DateTimePrivate::StatusFlags merge_spec(DateTimePrivate::StatusFlags status, pdk::TimeSpec spec)
{
   return DateTimePrivate::StatusFlags((status & ~pdk::as_integer<DateTimePrivate::StatusFlag>(DateTimePrivate::StatusFlag::TimeSpecMask)) |
                                       (int(spec) << DateTimePrivate::TimeSpecShift));
}

PDK_DECL_CONSTEXPR inline pdk::TimeSpec extract_spec(DateTimePrivate::StatusFlags status)
{
   return pdk::TimeSpec((status & pdk::as_integer<DateTimePrivate::StatusFlag>(DateTimePrivate::StatusFlag::TimeSpecMask)) >> DateTimePrivate::TimeSpecShift);
}

// Set the Daylight Status if LocalTime set via msecs
PDK_DECL_RELAXED_CONSTEXPR inline DateTimePrivate::StatusFlags
merge_daylight_status(DateTimePrivate::StatusFlags sf, DateTimePrivate::DaylightStatus status)
{
   sf &= ~DateTimePrivate::DaylightMask;
   if (status == DateTimePrivate::DaylightStatus::DaylightTime) {
      sf |= DateTimePrivate::StatusFlag::SetToDaylightTime;
   } else if (status == DateTimePrivate::DaylightStatus::StandardTime) {
      sf |= DateTimePrivate::StatusFlag::SetToStandardTime;
   }
   return sf;
}

// Get the DST Status if LocalTime set via msecs
PDK_DECL_RELAXED_CONSTEXPR inline
DateTimePrivate::DaylightStatus extract_daylight_status(DateTimePrivate::StatusFlags status)
{
   if (status & DateTimePrivate::StatusFlag::SetToDaylightTime) {
      return DateTimePrivate::DaylightStatus::DaylightTime;
   }
   if (status & DateTimePrivate::StatusFlag::SetToStandardTime) {
      return DateTimePrivate::DaylightStatus::StandardTime;
   }
   return DateTimePrivate::DaylightStatus::UnknownDaylightTime;
}

inline pdk::pint64 get_msecs(const DateTimeData &data)
{
   if (data.isShort()) {
      // same as, but producing better code
      //return data.m_data.msecs;
      return pdk::intptr(data.m_implPtr) >> 8;
   }
   return data->m_msecs;
}

inline DateTimePrivate::StatusFlags get_status(const DateTimeData &data)
{
   if (data.isShort()) {
      // same as, but producing better code
      //return StatusFlag(d.data.status);
      return DateTimePrivate::StatusFlag(pdk::intptr(data.m_implPtr) & 0xFF);
   }
   return data->m_status;
}

inline pdk::TimeSpec get_spec(const DateTimeData &data)
{
   return extract_spec(get_status(data));
}

// Refresh the LocalTime validity and offset
void refresh_datetime(DateTimeData &data)
{
   auto status = get_status(data);
   const auto spec = extract_spec(status);
   const pdk::pint64 msecs = get_msecs(data);
   pdk::pint64 epochMSecs = 0;
   int offsetFromUtc = 0;
   Date testDate;
   Time testTime;
   PDK_ASSERT(spec == pdk::TimeSpec::TimeZone || spec == pdk::TimeSpec::LocalTime);
   
#if PDK_CONFIG(timezone)
   // If not valid time zone then is invalid
   if (spec == pdk::TimeSpec::TimeZone) {
      if (!data->m_timeZone.isValid()) {
         status &= ~pdk::as_integer<DateTimePrivate::StatusFlag>(DateTimePrivate::StatusFlag::ValidDateTime);
      } else {
         epochMSecs = DateTimePrivate::zoneMSecsToEpochMSecs(msecs, data->m_timeZone, 
                                                             extract_daylight_status(status), 
                                                             &testDate, &testTime);
         data->setUtcOffsetByTZ(epochMSecs);
      }
   }
#endif // timezone
   
   // If not valid date and time then is invalid
   if (!(status & DateTimePrivate::StatusFlag::ValidDate) || !(status & DateTimePrivate::StatusFlag::ValidTime)) {
      status &= ~pdk::as_integer<DateTimePrivate::StatusFlag>(DateTimePrivate::StatusFlag::ValidDateTime);
      if (status & DateTimePrivate::StatusFlag::ShortData) {
         data.m_data.m_status = status;
      } else {
         data->m_status = status;
         data->m_offsetFromUtc = 0;
      }
      return;
   }
   
   // We have a valid date and time and a pdk::LocalTime or pdk::TimeSpec::TimeZone that needs calculating
   // LocalTime and TimeZone might fall into a "missing" DST transition hour
   // Calling toEpochMSecs will adjust the returned date/time if it does
   if (spec == pdk::TimeSpec::LocalTime) {
      auto dstStatus = extract_daylight_status(status);
      epochMSecs = localMSecsToEpochMSecs(msecs, &dstStatus, &testDate, &testTime);
   }
   if (time_to_msecs(testDate, testTime) == msecs) {
      status |= DateTimePrivate::StatusFlag::ValidDateTime;
      // Cache the offset to use in offsetFromUtc()
      offsetFromUtc = (msecs - epochMSecs) / 1000;
   } else {
      status &= ~pdk::as_integer<DateTimePrivate::StatusFlag>(DateTimePrivate::StatusFlag::ValidDateTime);
   }
   
   if (status & DateTimePrivate::StatusFlag::ShortData) {
      data.m_data.m_status = status;
   } else {
      data->m_status = status;
      data->m_offsetFromUtc = offsetFromUtc;
   }
}

// Check the UTC / offsetFromUTC validity
void check_valid_datetime(DateTimeData &data)
{
   auto status = get_status(data);
   auto spec = extract_spec(status);
   switch (spec) {
   case pdk::TimeSpec::OffsetFromUTC:
   case pdk::TimeSpec::UTC:
      // for these, a valid date and a valid time imply a valid DateTime
      if ((status & DateTimePrivate::StatusFlag::ValidDate) && 
          (status & DateTimePrivate::StatusFlag::ValidTime)) {
         status |= DateTimePrivate::StatusFlag::ValidDateTime;
      } else {
         status &= ~pdk::as_integer<DateTimePrivate::StatusFlag>(DateTimePrivate::StatusFlag::ValidDateTime);
      }
      if (status & DateTimePrivate::StatusFlag::ShortData) {
         data.m_data.m_status = status;
      } else {
         data->m_status = status;
      }
      break;
   case pdk::TimeSpec::TimeZone:
   case pdk::TimeSpec::LocalTime:
      // for these, we need to check whether the timezone is valid and whether
      // the time is valid in that timezone. Expensive, but no other option.
      refresh_datetime(data);
      break;
   }
}

void set_time_spec(DateTimeData &data, pdk::TimeSpec spec, int offsetSeconds)
{
   auto status = get_status(data);
   status &= ~(pdk::as_integer<DateTimePrivate::StatusFlag>(DateTimePrivate::StatusFlag::ValidDateTime) | 
               DateTimePrivate::DaylightMask |
               pdk::as_integer<DateTimePrivate::StatusFlag>(DateTimePrivate::StatusFlag::TimeSpecMask));
   
   switch (spec) {
   case pdk::TimeSpec::OffsetFromUTC:
      if (offsetSeconds == 0) {
         spec = pdk::TimeSpec::UTC;
      }
      break;
   case pdk::TimeSpec::TimeZone:
      // Use system time zone instead
      spec = pdk::TimeSpec::LocalTime;
      PDK_FALLTHROUGH();
   case pdk::TimeSpec::UTC:
   case pdk::TimeSpec::LocalTime:
      offsetSeconds = 0;
      break;
   }
   
   status = merge_spec(status, spec);
   if (data.isShort() && offsetSeconds == 0) {
      data.m_data.m_status = status;
   } else {
      data.detach();
      data->m_status = status & ~pdk::as_integer<DateTimePrivate::StatusFlag>(DateTimePrivate::StatusFlag::ShortData);
      data->m_offsetFromUtc = offsetSeconds;
#if PDK_CONFIG(timezone)
      data->m_timeZone = TimeZone();
#endif // timezone
   }
}

void set_datetime(DateTimeData &data, const Date &date, const Time &time)
{
   // If the date is valid and the time is not we set time to 00:00:00
   Time useTime = time;
   if (!useTime.isValid() && date.isValid()) {
      useTime = Time::fromMSecsSinceStartOfDay(0);
   }
   DateTimePrivate::StatusFlags newStatus = 0;
   // Set date value and status
   pdk::pint64 days = 0;
   if (date.isValid()) {
      days = date.toJulianDay() - JULIAN_DAY_FOR_EPOCH;
      newStatus = DateTimePrivate::StatusFlag::ValidDate;
   }
   
   // Set time value and status
   int ds = 0;
   if (useTime.isValid()) {
      ds = useTime.msecsSinceStartOfDay();
      newStatus |= DateTimePrivate::StatusFlag::ValidTime;
   }
   
   // Set msecs serial value
   pdk::pint64 msecs = (days * MSECS_PER_DAY) + ds;
   if (data.isShort()) {
      // let's see if we can keep this short
      if (msecs_can_be_small(msecs)) {
         // yes, we can
         data.m_data.m_msecs = pdk::intptr(msecs);
         data.m_data.m_status &= ~(DateTimePrivate::ValidityMask | DateTimePrivate::DaylightMask);
         data.m_data.m_status |= newStatus;
      } else {
         // nope...
         data.detach();
      }
   }
   if (!data.isShort()) {
      data.detach();
      data->m_msecs = msecs;
      data->m_status &= ~(DateTimePrivate::ValidityMask | DateTimePrivate::DaylightMask);
      data->m_status |= newStatus;
   }
   // Set if date and time are valid
   check_valid_datetime(data);
}

std::pair<Date, Time> get_datetime(const DateTimeData &data)
{
   std::pair<Date, Time> result;
   pdk::pint64 msecs = get_msecs(data);
   auto status = get_status(data);
   msecs_to_time(msecs, &result.first, &result.second);
   
   if (!status.testFlag(DateTimePrivate::StatusFlag::ValidDate)) {
      result.first = Date();
   }
   if (!status.testFlag(DateTimePrivate::StatusFlag::ValidTime)) {
      result.second = Time();
   }
   return result;
}

} // anonymous namespace

#if PDK_CONFIG(timezone)
namespace internal {
void DateTimePrivate::setUtcOffsetByTZ(pdk::pint64 atMSecsSinceEpoch)
{
   m_offsetFromUtc = m_timeZone.m_implPtr->offsetFromUtc(atMSecsSinceEpoch);
}
} // internal

#endif

/*****************************************************************************
  DateTime::Data member functions
 *****************************************************************************/
inline DateTime::Data::Data()
{
   // default-constructed data has a special exception:
   // it can be small even if CanBeSmall == false
   // (optimization so we don't allocate memory in the default constructor)
   pdk::uintptr value = pdk::uintptr(merge_spec(DateTimePrivate::StatusFlag::ShortData, pdk::TimeSpec::LocalTime));
   m_implPtr = reinterpret_cast<DateTimePrivate *>(value);
}

inline DateTime::Data::Data(pdk::TimeSpec spec)
{
   if (CanBeSmall && PDK_LIKELY(spec_can_be_small(spec))) {
      m_implPtr = reinterpret_cast<DateTimePrivate *>(pdk::uintptr(merge_spec(DateTimePrivate::StatusFlag::ShortData, spec)));
   } else {
      // the structure is too small, we need to detach
      m_implPtr = new DateTimePrivate;
      m_implPtr->m_ref.ref();
      m_implPtr->m_status = merge_spec(0, spec);
   }
}

inline DateTime::Data::Data(const Data &other)
   : m_implPtr(other.m_implPtr)
{
   if (!isShort()) {
      // check if we could shrink
      if (spec_can_be_small(extract_spec(m_implPtr->m_status)) && msecs_can_be_small(m_implPtr->m_msecs)) {
         ShortData sd;
         sd.m_msecs = pdk::intptr(m_implPtr->m_msecs);
         sd.m_status = m_implPtr->m_status | DateTimePrivate::StatusFlag::ShortData;
         m_data = sd;
      } else {
         // no, have to keep it big
         m_implPtr->m_ref.ref();
      }
   }
}

inline DateTime::Data::Data(Data &&other)
   : m_implPtr(other.m_implPtr)
{
   // reset the other to a short state
   Data dummy;
   PDK_ASSERT(dummy.isShort());
   other.m_implPtr = dummy.m_implPtr;
}

inline DateTime::Data &DateTime::Data::operator=(const Data &other)
{
   if (m_implPtr == other.m_implPtr) {
      return *this;
   }
   auto x = m_implPtr;
   m_implPtr = other.m_implPtr;
   if (!other.isShort()) {
      // check if we could shrink
      if (spec_can_be_small(extract_spec(other.m_implPtr->m_status)) && msecs_can_be_small(other.m_implPtr->m_msecs)) {
         ShortData sd;
         sd.m_msecs = pdk::intptr(other.m_implPtr->m_msecs);
         sd.m_status = other.m_implPtr->m_status | DateTimePrivate::StatusFlag::ShortData;
         m_data = sd;
      } else {
         // no, have to keep it big
         other.m_implPtr->m_ref.ref();
      }
   }
   if (!(pdk::uintptr(x) & pdk::as_integer<DateTimePrivate::StatusFlag>(DateTimePrivate::StatusFlag::ShortData)) && 
       !x->m_ref.deref()) {
      delete x;
   }
   return *this;
}

inline DateTime::Data::~Data()
{
   if (!isShort() && !m_implPtr->m_ref.deref()) {
      delete m_implPtr;
   }
}

inline bool DateTime::Data::isShort() const
{
   bool b = pdk::uintptr(m_implPtr) & pdk::as_integer<DateTimePrivate::StatusFlag>(DateTimePrivate::StatusFlag::ShortData);
   // sanity check:
   PDK_ASSERT(b || (m_implPtr->m_status & 
                    pdk::as_integer<DateTimePrivate::StatusFlag>(DateTimePrivate::StatusFlag::ShortData)) == 0);
   // even if CanBeSmall = false, we have short data for a default-constructed
   // DateTime object. But it's unlikely.
   if (CanBeSmall) {
      return PDK_LIKELY(b);
   }
   return PDK_UNLIKELY(b);
}

inline void DateTime::Data::detach()
{
   DateTimePrivate *x;
   bool wasShort = isShort();
   if (wasShort) {
      // force enlarging
      x = new DateTimePrivate;
      x->m_status = DateTimePrivate::StatusFlag(m_data.m_status & 
                                                ~pdk::as_integer<DateTimePrivate::StatusFlag>(DateTimePrivate::StatusFlag::ShortData));
      x->m_msecs = m_data.m_msecs;
   } else {
      if (m_implPtr->m_ref.load() == 1) {
         return;
      }
      x = new DateTimePrivate(*m_implPtr);
   }
   x->m_ref.store(1);
   if (!wasShort && !m_implPtr->m_ref.deref()) {
      delete m_implPtr;
   }
   m_implPtr = x;
}

inline const DateTimePrivate *DateTime::Data::operator->() const
{
   PDK_ASSERT(!isShort());
   return m_implPtr;
}

inline DateTimePrivate *DateTime::Data::operator->()
{
   // should we attempt to detach here?
   PDK_ASSERT(!isShort());
   PDK_ASSERT(m_implPtr->m_ref.load() == 1);
   return m_implPtr;
}

/*****************************************************************************
  DateTimePrivate member functions
 *****************************************************************************/

namespace internal {
PDK_NEVER_INLINE
DateTime::Data DateTimePrivate::create(const Date &toDate, const Time &toTime, pdk::TimeSpec toSpec,
                                       int offsetSeconds)
{
   DateTime::Data result(toSpec);
   set_time_spec(result, toSpec, offsetSeconds);
   set_datetime(result, toDate, toTime);
   return result;
}

#if PDK_CONFIG(timezone)
inline DateTime::Data DateTimePrivate::create(const Date &toDate, const Time &toTime,
                                              const TimeZone &toTimeZone)
{
   DateTime::Data result(pdk::TimeSpec::TimeZone);
   PDK_ASSERT(!result.isShort());
   result.m_implPtr->m_status = merge_spec(result.m_implPtr->m_status, pdk::TimeSpec::TimeZone);
   result.m_implPtr->m_timeZone = toTimeZone;
   set_datetime(result, toDate, toTime);
   return result;
}

// Convert a TimeZone time expressed in zone msecs encoding into a UTC epoch msecs
// DST transitions are disambiguated by hint.
inline pdk::pint64 DateTimePrivate::zoneMSecsToEpochMSecs(pdk::pint64 zoneMSecs, const TimeZone &zone,
                                                          DaylightStatus hint,
                                                          Date *localDate, Time *localTime)
{
   // Get the effective data from TimeZone
   TimeZonePrivate::Data data = zone.m_implPtr->dataForLocalTime(zoneMSecs, int(hint));
   // Docs state any LocalTime before 1970-01-01 will *not* have any DST applied
   // but all affected times afterwards will have DST applied.
   if (data.m_atMSecsSinceEpoch >= 0) {
      msecs_to_time(data.m_atMSecsSinceEpoch + (data.m_offsetFromUtc * 1000), localDate, localTime);
      return data.m_atMSecsSinceEpoch;
   } else {
      msecs_to_time(zoneMSecs, localDate, localTime);
      return zoneMSecs - (data.m_standardTimeOffset * 1000);
   }
}
#endif // timezone
} // internal


/*****************************************************************************
  DateTime member functions
 *****************************************************************************/
DateTime::DateTime() noexcept(Data::CanBeSmall)
{}

DateTime::DateTime(const Date &date)
   : m_data(DateTimePrivate::create(date, Time(0, 0, 0), pdk::TimeSpec::LocalTime, 0))
{}

DateTime::DateTime(const Date &date, const Time &time, pdk::TimeSpec spec, int offsetSeconds)
   : m_data(DateTimePrivate::create(date, time, spec, offsetSeconds))
{}

#if PDK_CONFIG(timezone)
DateTime::DateTime(const Date &date, const Time &time, const TimeZone &timeZone)
   : m_data(DateTimePrivate::create(date, time, timeZone))
{}
#endif // timezone

DateTime::DateTime(const DateTime &other) noexcept
   : m_data(other.m_data)
{}

DateTime::DateTime(DateTime &&other) noexcept
   : m_data(std::move(other.m_data))
{}

DateTime::~DateTime()
{
}

DateTime &DateTime::operator=(const DateTime &other) noexcept
{
   m_data = other.m_data;
   return *this;
}

bool DateTime::isNull() const
{
   auto status = get_status(m_data);
   return !status.testFlag(DateTimePrivate::StatusFlag::ValidDate) &&
         !status.testFlag(DateTimePrivate::StatusFlag::ValidTime);
}

bool DateTime::isValid() const
{
   auto status = get_status(m_data);
   return status & DateTimePrivate::StatusFlag::ValidDateTime;
}

Date DateTime::getDate() const
{
   auto status = get_status(m_data);
   if (!status.testFlag(DateTimePrivate::StatusFlag::ValidDate)) {
      return Date();
   }
   Date dt;
   msecs_to_time(get_msecs(m_data), &dt, 0);
   return dt;
}


Time DateTime::getTime() const
{
   auto status = get_status(m_data);
   if (!status.testFlag(DateTimePrivate::StatusFlag::ValidTime)) {
      return Time();
   }
   Time tm;
   msecs_to_time(get_msecs(m_data), 0, &tm);
   return tm;
}

pdk::TimeSpec DateTime::getTimeSpec() const
{
   return get_spec(m_data);
}

#if PDK_CONFIG(timezone)
TimeZone DateTime::getTimeZone() const
{
   switch (get_spec(m_data)) {
   case pdk::TimeSpec::UTC:
      return TimeZone::getUtc();
   case pdk::TimeSpec::OffsetFromUTC:
      return TimeZone(m_data->m_offsetFromUtc);
   case pdk::TimeSpec::TimeZone:
      PDK_ASSERT(m_data->m_timeZone.isValid());
      return m_data->m_timeZone;
   case pdk::TimeSpec::LocalTime:
      return TimeZone::getSystemTimeZone();
   }
   return TimeZone();
}
#endif // timezone

int DateTime::getOffsetFromUtc() const
{
   if (!m_data.isShort()) {
      return m_data->m_offsetFromUtc;
   }
   if (!isValid()) {
      return 0;
   }
   auto spec = get_spec(m_data);
   if (spec == pdk::TimeSpec::LocalTime) {
      // we didn't cache the value, so we need to calculate it now...
      pdk::pint64 msecs = get_msecs(m_data);
      return (msecs - toMSecsSinceEpoch()) / 1000;
   }
   PDK_ASSERT(spec == pdk::TimeSpec::UTC);
   return 0;
}

String DateTime::timeZoneAbbreviation() const
{
   switch (get_spec(m_data)) {
   case pdk::TimeSpec::UTC:
      return Latin1String("UTC");
   case pdk::TimeSpec::OffsetFromUTC:
      return Latin1String("UTC") + to_offset_string(pdk::DateFormat::ISODate, m_data->m_offsetFromUtc);
   case pdk::TimeSpec::TimeZone:
#if !PDK_CONFIG(timezone)
      break;
#else
      return m_data->m_timeZone.m_implPtr->abbreviation(toMSecsSinceEpoch());
#endif // timezone
   case pdk::TimeSpec::LocalTime:  {
      String abbrev;
      auto status = extract_daylight_status(get_status(m_data));
      localMSecsToEpochMSecs(get_msecs(m_data), &status, 0, 0, &abbrev);
      return abbrev;
   }
   }
   return String();
}

bool DateTime::isDaylightTime() const
{
   switch (get_spec(m_data)) {
   case pdk::TimeSpec::UTC:
   case pdk::TimeSpec::OffsetFromUTC:
      return false;
   case pdk::TimeSpec::TimeZone:
#if !PDK_CONFIG(timezone)
      break;
#else
      return m_data->m_timeZone.m_implPtr->isDaylightTime(toMSecsSinceEpoch());
#endif // timezone
   case pdk::TimeSpec::LocalTime: {
      auto status = extract_daylight_status(get_status(m_data));
      if (status == DateTimePrivate::DaylightStatus::UnknownDaylightTime) {
         localMSecsToEpochMSecs(get_msecs(m_data), &status);
      }
      return (status == DateTimePrivate::DaylightStatus::DaylightTime);
   }
   }
   return false;
}

void DateTime::setDate(const Date &date)
{
   set_datetime(m_data, date, getTime());
}

void DateTime::setTime(const Time &time)
{
   set_datetime(m_data, getDate(), time);
}

void DateTime::setTimeSpec(pdk::TimeSpec spec)
{
   set_time_spec(m_data, spec, 0);
   check_valid_datetime(m_data);
}

void DateTime::setOffsetFromUtc(int offsetSeconds)
{
   set_time_spec(m_data, pdk::TimeSpec::OffsetFromUTC, offsetSeconds);
   check_valid_datetime(m_data);
}

#if PDK_CONFIG(timezone)
void DateTime::setTimeZone(const TimeZone &toZone)
{
   m_data.detach();         // always detach
   m_data->m_status = merge_spec(m_data->m_status, pdk::TimeSpec::TimeZone);
   m_data->m_offsetFromUtc = 0;
   m_data->m_timeZone = toZone;
   refresh_datetime(m_data);
}
#endif // timezone

pdk::pint64 DateTime::toMSecsSinceEpoch() const
{
   switch (get_spec(m_data)) {
   case pdk::TimeSpec::UTC:
      return get_msecs(m_data);
      
   case pdk::TimeSpec::OffsetFromUTC:
      return m_data->m_msecs - (m_data->m_offsetFromUtc * 1000);
      
   case pdk::TimeSpec::LocalTime: {
      // recalculate the local timezone
      auto status = extract_daylight_status(get_status(m_data));
      return localMSecsToEpochMSecs(get_msecs(m_data), &status);
   }
      
   case pdk::TimeSpec::TimeZone:
#if !PDK_CONFIG(timezone)
      return 0;
#else
      return DateTimePrivate::zoneMSecsToEpochMSecs(m_data->m_msecs, m_data->m_timeZone,
                                                    extract_daylight_status(get_status(m_data)));
#endif
   }
   PDK_UNREACHABLE();
   return 0;
}

pdk::pint64 DateTime::toSecsSinceEpoch() const
{
   return toMSecsSinceEpoch() / 1000;
}

void DateTime::setMSecsSinceEpoch(pdk::pint64 msecs)
{
   const auto spec = get_spec(m_data);
   auto status = get_status(m_data);
   status &= ~DateTimePrivate::ValidityMask;
   switch (spec) {
   case pdk::TimeSpec::UTC:
      status = status
            | DateTimePrivate::StatusFlag::ValidDate
            | DateTimePrivate::StatusFlag::ValidTime
            | DateTimePrivate::StatusFlag::ValidDateTime;
      break;
   case pdk::TimeSpec::OffsetFromUTC:
      msecs = msecs + (m_data->m_offsetFromUtc * 1000);
      status = status
            | DateTimePrivate::StatusFlag::ValidDate
            | DateTimePrivate::StatusFlag::ValidTime
            | DateTimePrivate::StatusFlag::ValidDateTime;
      break;
   case pdk::TimeSpec::TimeZone:
      PDK_ASSERT(!m_data.isShort());
#if PDK_CONFIG(timezone)
      // Docs state any LocalTime before 1970-01-01 will *not* have any DST applied
      // but all affected times afterwards will have DST applied.
      m_data.detach();
      if (msecs >= 0) {
         status = merge_daylight_status(status,
                                        m_data->m_timeZone.m_implPtr->isDaylightTime(msecs)
                                        ? DateTimePrivate::DaylightStatus::DaylightTime
                                        : DateTimePrivate::DaylightStatus::StandardTime);
         m_data->m_offsetFromUtc = m_data->m_timeZone.m_implPtr->offsetFromUtc(msecs);
      } else {
         status = merge_daylight_status(status, DateTimePrivate::DaylightStatus::StandardTime);
         m_data->m_offsetFromUtc = m_data->m_timeZone.m_implPtr->standardTimeOffset(msecs);
      }
      msecs = msecs + (m_data->m_offsetFromUtc * 1000);
      status = status
            | DateTimePrivate::StatusFlag::ValidDate
            | DateTimePrivate::StatusFlag::ValidTime
            | DateTimePrivate::StatusFlag::ValidDateTime;
#endif // timezone
      break;
   case pdk::TimeSpec::LocalTime: {
      Date dt;
      Time tm;
      DateTimePrivate::DaylightStatus dstStatus;
      epoch_msecs_to_localtime(msecs, &dt, &tm, &dstStatus);
      set_datetime(m_data, dt, tm);
      msecs = get_msecs(m_data);
      status = merge_daylight_status(get_status(m_data), dstStatus);
      break;
   }
   }
   if (msecs_can_be_small(msecs) && m_data.isShort()) {
      // we can keep short
      m_data.m_data.m_msecs = pdk::intptr(msecs);
      m_data.m_data.m_status = status;
   } else {
      m_data.detach();
      m_data->m_status = status & ~pdk::as_integer<DateTimePrivate::StatusFlag>(DateTimePrivate::StatusFlag::ShortData);
      m_data->m_msecs = msecs;
   }
   if (spec == pdk::TimeSpec::LocalTime || spec == pdk::TimeSpec::TimeZone) {
      refresh_datetime(m_data);
   }
}

void DateTime::setSecsSinceEpoch(pdk::pint64 secs)
{
   setMSecsSinceEpoch(secs * 1000);
}

#ifndef PDK_NO_DATESTRING
String DateTime::toString(pdk::DateFormat format) const
{
   String buf;
   if (!isValid()) {
      return buf;
   }
   switch (format) {
   case pdk::DateFormat::SystemLocaleShortDate:
      return Locale::system().toString(*this, Locale::FormatType::ShortFormat);
   case pdk::DateFormat::SystemLocaleLongDate:
      return Locale::system().toString(*this, Locale::FormatType::LongFormat);
   case pdk::DateFormat::DefaultLocaleShortDate:
      return Locale().toString(*this, Locale::FormatType::ShortFormat);
   case pdk::DateFormat::DefaultLocaleLongDate:
      return Locale().toString(*this, Locale::FormatType::LongFormat);
   case pdk::DateFormat::RFC2822Date: {
      buf = Locale::c().toString(*this, StringViewLiteral("dd MMM yyyy hh:mm:ss "));
      buf += to_offset_string(pdk::DateFormat::TextDate, getOffsetFromUtc());
      return buf;
   }
#ifndef PDK_NO_TEXTDATE
   case pdk::DateFormat::TextDate: {
      const std::pair<Date, Time> p = get_datetime(m_data);
      buf = p.first.toString(pdk::DateFormat::TextDate);
      // Insert time between date's day and year:
      buf.insert(buf.lastIndexOf(Latin1Character(' ')),
                 Latin1Character(' ') + p.second.toString(pdk::DateFormat::TextDate));
      // Append zone/offset indicator, as appropriate:
      switch (getTimeSpec()) {
      case pdk::TimeSpec::LocalTime:
         break;
# if PDK_CONFIG(timezone)
      case pdk::TimeSpec::TimeZone:
         buf += Latin1Character(' ') + m_data->m_timeZone.abbreviation(*this);
         break;
# endif
      default:
         buf += Latin1String(" GMT");
         if (get_spec(m_data) == pdk::TimeSpec::OffsetFromUTC) {
            buf += to_offset_string(pdk::DateFormat::TextDate, getOffsetFromUtc());
         }
      }
      return buf;
   }
#endif
   case pdk::DateFormat::ISODate:
   case pdk::DateFormat::ISODateWithMs: {
      const std::pair<Date, Time> p = get_datetime(m_data);
      const Date &dt = p.first;
      const Time &tm = p.second;
      buf = dt.toString(pdk::DateFormat::ISODate);
      if (buf.isEmpty()) {
         return String();   // failed to convert
      }
      buf += Latin1Character('T');
      buf += tm.toString(format);
      switch (get_spec(m_data)) {
      case pdk::TimeSpec::UTC:
         buf += Latin1Character('Z');
         break;
      case pdk::TimeSpec::OffsetFromUTC:
#if PDK_CONFIG(timezone)
      case pdk::TimeSpec::TimeZone:
#endif
         buf += to_offset_string(pdk::DateFormat::ISODate, getOffsetFromUtc());
         break;
      default:
         break;
      }
      return buf;
   }
   }
}

String DateTime::toString(StringView format) const
{
   return Locale::system().toString(*this, format); // Locale::c() ### Qt6
}

#if PDK_STRINGVIEW_LEVEL < 2
String DateTime::toString(const String &format) const
{
   return toString(pdk::lang::to_string_view_ignoring_null(format));
}
#endif

#endif //PDK_NO_DATESTRING

namespace {

inline void massage_adjusted_datetime(const DateTimeData &data, Date *date, Time *time)
{
   /*
      If we have just adjusted to a day with a DST transition, our given time
      may lie in the transition hour (either missing or duplicated).  For any
      other time, telling mktime (deep in the bowels of localMSecsToEpochMSecs)
      we don't know its DST-ness will produce no adjustment (just a decision as
      to its DST-ness); but for a time in spring's missing hour it'll adjust the
      time while picking a DST-ness.  (Handling of autumn is trickier, as either
      DST-ness is valid, without adjusting the time.  We might want to propagate
      the daylight status in that case, but it's hard to do so without breaking
      (far more common) other cases; and it makes little difference, as the two
      answers do then differ only in DST-ness.)
    */
   auto spec = get_spec(data);
   if (spec == pdk::TimeSpec::LocalTime) {
      DateTimePrivate::DaylightStatus status = DateTimePrivate::DaylightStatus::UnknownDaylightTime;
      localMSecsToEpochMSecs(time_to_msecs(*date, *time), &status, date, time);
#if PDK_CONFIG(timezone)
   } else if (spec == pdk::TimeSpec::TimeZone) {
      DateTimePrivate::zoneMSecsToEpochMSecs(time_to_msecs(*date, *time),
                                             data->m_timeZone,
                                             DateTimePrivate::DaylightStatus::UnknownDaylightTime,
                                             date, time);
#endif // timezone
   }
}


} // anonymous namespace

DateTime DateTime::addDays(pdk::pint64 ndays) const
{
   DateTime dt(*this);
   std::pair<Date, Time> p = get_datetime(m_data);
   Date &date = p.first;
   Time &time = p.second;
   date = date.addDays(ndays);
   massage_adjusted_datetime(dt.m_data, &date, &time);
   set_datetime(dt.m_data, date, time);
   return dt;
}

DateTime DateTime::addMonths(int nmonths) const
{
   DateTime dt(*this);
   std::pair<Date, Time> p = get_datetime(m_data);
   Date &date = p.first;
   Time &time = p.second;
   date = date.addMonths(nmonths);
   massage_adjusted_datetime(dt.m_data, &date, &time);
   set_datetime(dt.m_data, date, time);
   return dt;
}

DateTime DateTime::addYears(int nyears) const
{
   DateTime dt(*this);
   std::pair<Date, Time> p = get_datetime(m_data);
   Date &date = p.first;
   Time &time = p.second;
   date = date.addYears(nyears);
   massage_adjusted_datetime(dt.m_data, &date, &time);
   set_datetime(dt.m_data, date, time);
   return dt;
}

DateTime DateTime::addSecs(pdk::pint64 s) const
{
   return addMSecs(s * 1000);
}

DateTime DateTime::addMSecs(pdk::pint64 msecs) const
{
   if (!isValid()) {
      return DateTime();
   }
   DateTime dt(*this);
   auto spec = get_spec(m_data);
   if (spec == pdk::TimeSpec::LocalTime || spec == pdk::TimeSpec::TimeZone) {
      // Convert to real UTC first in case crosses DST transition
      dt.setMSecsSinceEpoch(toMSecsSinceEpoch() + msecs);
   } else {
      // No need to convert, just add on
      if (m_data.isShort()) {
         // need to check if we need to enlarge first
         msecs += dt.m_data.m_implPtr->m_msecs;
         if (msecs_can_be_small(msecs)) {
            dt.m_data.m_implPtr->m_msecs = pdk::intptr(msecs);
         } else {
            dt.m_data.detach();
            dt.m_data->m_msecs = msecs;
         }
      } else {
         dt.m_data.detach();
         dt.m_data->m_msecs += msecs;
      }
   }
   return dt;
}

pdk::pint64 DateTime::daysTo(const DateTime &other) const
{
   return getDate().daysTo(other.getDate());
}

pdk::pint64 DateTime::secsTo(const DateTime &other) const
{
   return (msecsTo(other) / 1000);
}

pdk::pint64 DateTime::msecsTo(const DateTime &other) const
{
   if (!isValid() || !other.isValid())
      return 0;
   
   return other.toMSecsSinceEpoch() - toMSecsSinceEpoch();
}

DateTime DateTime::toTimeSpec(pdk::TimeSpec spec) const
{
   if (get_spec(m_data) == spec && (spec == pdk::TimeSpec::UTC || spec == pdk::TimeSpec::LocalTime)) {
      return *this;
   }
   if (!isValid()) {
      DateTime ret = *this;
      ret.setTimeSpec(spec);
      return ret;
   }
   return fromMSecsSinceEpoch(toMSecsSinceEpoch(), spec, 0);
}

DateTime DateTime::toOffsetFromUtc(int offsetSeconds) const
{
   if (get_spec(m_data) == pdk::TimeSpec::OffsetFromUTC
       && m_data->m_offsetFromUtc == offsetSeconds) {
      return *this;
   }
   if (!isValid()) {
      DateTime ret = *this;
      ret.setOffsetFromUtc(offsetSeconds);
      return ret;
   }
   return fromMSecsSinceEpoch(toMSecsSinceEpoch(), pdk::TimeSpec::OffsetFromUTC, offsetSeconds);
}

#if PDK_CONFIG(timezone)
DateTime DateTime::toTimeZone(const TimeZone &timeZone) const
{
   if (get_spec(m_data) == pdk::TimeSpec::TimeZone && m_data->m_timeZone == timeZone) {
      return *this;
   }
   if (!isValid()) {
      DateTime ret = *this;
      ret.setTimeZone(timeZone);
      return ret;
   }
   return fromMSecsSinceEpoch(toMSecsSinceEpoch(), timeZone);
}
#endif // timezone

bool DateTime::operator==(const DateTime &other) const
{
   if (get_spec(m_data) == pdk::TimeSpec::LocalTime
       && get_status(m_data) == get_status(other.m_data)) {
      return get_msecs(m_data) == get_msecs(other.m_data);
   }
   // Convert to UTC and compare
   return (toMSecsSinceEpoch() == other.toMSecsSinceEpoch());
}

bool DateTime::operator<(const DateTime &other) const
{
   if (get_spec(m_data) == pdk::TimeSpec::LocalTime
       && get_status(m_data) == get_status(other.m_data)) {
      return get_msecs(m_data) < get_msecs(other.m_data);
   }
   // Convert to UTC and compare
   return (toMSecsSinceEpoch() < other.toMSecsSinceEpoch());
}

#if defined(PDK_OS_WIN)
namespace {
inline uint msecs_from_decomposed(int hour, int minute, int sec, int msec = 0)
{
   return MSECS_PER_HOUR * hour + MSECS_PER_MIN * minute + 1000 * sec + msec;
}

} // anonymous namespace

Date Date::getCurrentDate()
{
   Date d;
   SYSTEMTIME st;
   memset(&st, 0, sizeof(SYSTEMTIME));
   GetLocalTime(&st);
   d.jd = julian_day_from_date(st.wYear, st.wMonth, st.wDay);
   return d;
}

Time Time::getCurrentTime()
{
   Time ct;
   SYSTEMTIME st;
   memset(&st, 0, sizeof(SYSTEMTIME));
   GetLocalTime(&st);
   ct.setHMS(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
   return ct;
}

DateTime DateTime::getCurrentDateTime()
{
   Date d;
   Time t;
   SYSTEMTIME st;
   memset(&st, 0, sizeof(SYSTEMTIME));
   GetLocalTime(&st);
   d.jd = julian_day_from_date(st.wYear, st.wMonth, st.wDay);
   t.mds = msecs_from_decomposed(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
   return DateTime(d, t);
}

DateTime DateTime::getCurrentDateTimeUtc()
{
   Date d;
   Time t;
   SYSTEMTIME st;
   memset(&st, 0, sizeof(SYSTEMTIME));
   GetSystemTime(&st);
   d.jd = julian_day_from_date(st.wYear, st.wMonth, st.wDay);
   t.mds = msecs_from_decomposed(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
   return DateTime(d, t, pdk::TimeSpec::UTC);
}

pdk::pint64 DateTime::getCurrentMSecsSinceEpoch() noexcept
{
   SYSTEMTIME st;
   memset(&st, 0, sizeof(SYSTEMTIME));
   GetSystemTime(&st);
   return msecs_from_decomposed(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds) +
         pdk::pint64(julian_day_from_date(st.wYear, st.wMonth, st.wDay)
                     - julian_day_from_date(1970, 1, 1)) * PDK_INT64_C(86400000);
}

pdk::pint64 DateTime::getCurrentSecsSinceEpoch() noexcept
{
   SYSTEMTIME st;
   memset(&st, 0, sizeof(SYSTEMTIME));
   GetSystemTime(&st);
   return st.wHour * SECS_PER_HOUR + st.wMinute * SECS_PER_MIN + st.wSecond +
         pdk::pint64(julian_day_from_date(st.wYear, st.wMonth, st.wDay)
                     - julian_day_from_date(1970, 1, 1)) * PDK_INT64_C(86400);
}

#elif defined(PDK_OS_UNIX)
Date Date::getCurrentDate()
{
   return DateTime::getCurrentDateTime().getDate();
}

Time Time::getCurrentTime()
{
   return DateTime::getCurrentDateTime().getTime();
}

DateTime DateTime::getCurrentDateTime()
{
   return fromMSecsSinceEpoch(getCurrentMSecsSinceEpoch(), pdk::TimeSpec::LocalTime);
}

DateTime DateTime::getCurrentDateTimeUtc()
{
   return fromMSecsSinceEpoch(getCurrentMSecsSinceEpoch(), pdk::TimeSpec::UTC);
}

pdk::pint64 DateTime::getCurrentMSecsSinceEpoch() noexcept
{
   // posix compliant system
   // we have milliseconds
   struct timeval tv;
   gettimeofday(&tv, 0);
   return pdk::pint64(tv.tv_sec) * PDK_INT64_C(1000) + tv.tv_usec / 1000;
}

pdk::pint64 DateTime::getCurrentSecsSinceEpoch() noexcept
{
   struct timeval tv;
   gettimeofday(&tv, 0);
   return pdk::pint64(tv.tv_sec);
}
#else
#error "What system is this?"
#endif

DateTime DateTime::fromMSecsSinceEpoch(pdk::pint64 msecs, pdk::TimeSpec spec, int offsetSeconds)
{
   DateTime dt;
   set_time_spec(dt.m_data, spec, offsetSeconds);
   dt.setMSecsSinceEpoch(msecs);
   return dt;
}

DateTime DateTime::fromSecsSinceEpoch(pdk::pint64 secs, pdk::TimeSpec spec, int offsetSeconds)
{
   return fromMSecsSinceEpoch(secs * 1000, spec, offsetSeconds);
}

#if PDK_CONFIG(timezone)

DateTime DateTime::fromMSecsSinceEpoch(pdk::pint64 msecs, const TimeZone &timeZone)
{
   DateTime dt;
   dt.setTimeZone(timeZone);
   dt.setMSecsSinceEpoch(msecs);
   return dt;
}

DateTime DateTime::fromSecsSinceEpoch(pdk::pint64 secs, const TimeZone &timeZone)
{
   return fromMSecsSinceEpoch(secs * 1000, timeZone);
}
#endif

#ifndef PDK_NO_DATESTRING
DateTime DateTime::fromString(const String& string, pdk::DateFormat format)
{
   if (string.isEmpty()) {
      return DateTime();
   }
   switch (format) {
   case pdk::DateFormat::SystemLocaleShortDate:
      return Locale::system().toDateTime(string, Locale::FormatType::ShortFormat);
   case pdk::DateFormat::SystemLocaleLongDate:
      return Locale::system().toDateTime(string, Locale::FormatType::LongFormat);
   case pdk::DateFormat::DefaultLocaleShortDate:
      return Locale().toDateTime(string, Locale::FormatType::ShortFormat);
   case pdk::DateFormat::DefaultLocaleLongDate:
      return Locale().toDateTime(string, Locale::FormatType::LongFormat);
   case pdk::DateFormat::RFC2822Date: {
      const ParsedRfcDateTime rfc = rfc_date_impl(string);
      if (!rfc.m_date.isValid() || !rfc.m_time.isValid()) {
         return DateTime();
      }
      DateTime dateTime(rfc.m_date, rfc.m_time, pdk::TimeSpec::UTC);
      dateTime.setOffsetFromUtc(rfc.m_utcOffset);
      return dateTime;
   }
   case pdk::DateFormat::ISODate:
   case pdk::DateFormat::ISODateWithMs: {
      const int size = string.size();
      if (size < 10) {
         return DateTime();
      }
      Date date = Date::fromString(string.left(10), pdk::DateFormat::ISODate);
      if (!date.isValid()) {
         return DateTime();
      }
      if (size == 10) {
         return DateTime(date);
      }
      pdk::TimeSpec spec = pdk::TimeSpec::LocalTime;
      StringRef isoString(&string);
      isoString = isoString.substring(10); // trim "yyyy-MM-dd"
      
      // Must be left with T and at least one digit for the hour:
      if (isoString.size() < 2
          || !(isoString.startsWith(Latin1Character('T'))
               // FIXME: QSql relies on QVariant::toDateTime() accepting a space here:
               || isoString.startsWith(Latin1Character(' ')))) {
         return DateTime();
      }
      isoString = isoString.substring(1); // trim 'T' (or space)
      
      int offset = 0;
      // Check end of string for Time Zone definition, either Z for UTC or [+-]HH:mm for Offset
      if (isoString.endsWith(Latin1Character('Z'))) {
         spec = pdk::TimeSpec::UTC;
         isoString.chop(1); // trim 'Z'
      } else {
         // the loop below is faster but functionally equal to:
         // const int signIndex = isoString.indexOf(QRegExp(StringLiteral("[+-]")));
         int signIndex = isoString.size() - 1;
         PDK_ASSERT(signIndex >= 0);
         bool found = false;
         {
            const Character plus = Latin1Character('+');
            const Character minus = Latin1Character('-');
            do {
               Character character(isoString.at(signIndex));
               found = character == plus || character == minus;
            } while (!found && --signIndex >= 0);
         }
         
         if (found) {
            bool ok;
            offset = from_offset_string(isoString.substring(signIndex), &ok);
            if (!ok) {
               return DateTime();
            }
            isoString = isoString.left(signIndex);
            spec = pdk::TimeSpec::OffsetFromUTC;
         }
      }
      
      // Might be end of day (24:00, including variants), which Time considers invalid.
      // ISO 8601 (section 4.2.3) says that 24:00 is equivalent to 00:00 the next day.
      bool isMidnight24 = false;
      Time time = fromIsoTimeString(isoString, format, &isMidnight24);
      if (!time.isValid()) {
         return DateTime();
      }
      if (isMidnight24) {
         date = date.addDays(1);
      }
      return DateTime(date, time, spec, offset);
   }
#if !defined(PDK_NO_TEXTDATE)
   case pdk::DateFormat::TextDate: {
      std::vector<StringRef> parts = string.splitRef(Latin1Character(' '), String::SplitBehavior::SkipEmptyParts);
      if ((parts.size() < 5) || (parts.size() > 6)) {
         return DateTime();
      }
      // Accept "Sun Dec 1 13:02:00 1974" and "Sun 1. Dec 13:02:00 1974"
      int month = 0;
      int day = 0;
      bool ok = false;      
      // First try month then day
      month = from_short_month_name(parts.at(1));
      if (month) {
         day = parts.at(2).toInt();
      }
      // If failed try day then month
      if (!month || !day) {
         month = from_short_month_name(parts.at(2));
         if (month) {
            StringRef dayStr = parts.at(1);
            if (dayStr.endsWith(Latin1Character('.'))) {
               dayStr = dayStr.left(dayStr.size() - 1);
               day = dayStr.toInt();
            }
         }
      }
      // If both failed, give up
      if (!month || !day) {
         return DateTime();
      }
      // Year can be before or after time, "Sun Dec 1 1974 13:02:00" or "Sun Dec 1 13:02:00 1974"
      // Guess which by looking for ':' in the time
      int year = 0;
      int yearPart = 0;
      int timePart = 0;
      if (parts.at(3).contains(Latin1Character(':'))) {
         yearPart = 4;
         timePart = 3;
      } else if (parts.at(4).contains(Latin1Character(':'))) {
         yearPart = 3;
         timePart = 4;
      } else {
         return DateTime();
      }
      
      year = parts.at(yearPart).toInt(&ok);
      if (!ok) {
         return DateTime();
      }
      Date date(year, month, day);
      if (!date.isValid()) {
         return DateTime();
      }
      std::vector<StringRef> timeParts = parts.at(timePart).split(Latin1Character(':'));
      if (timeParts.size() < 2 || timeParts.size() > 3) {
         return DateTime();
      }
      int hour = timeParts.at(0).toInt(&ok);
      if (!ok) {
         return DateTime();
      }
      int minute = timeParts.at(1).toInt(&ok);
      if (!ok) {
         return DateTime();
      }
      int second = 0;
      int millisecond = 0;
      if (timeParts.size() > 2) {
         const std::vector<StringRef> secondParts = timeParts.at(2).split(Latin1Character('.'));
         if (secondParts.size() > 2) {
            return DateTime();
         }
         second = secondParts.front().toInt(&ok);
         if (!ok) {
            return DateTime();
         }
         if (secondParts.size() > 1) {
            millisecond = secondParts.back().toInt(&ok);
            if (!ok) {
               return DateTime();
            }
         }
      }
      
      Time time(hour, minute, second, millisecond);
      if (!time.isValid()) {
         return DateTime();
      }
      if (parts.size() == 5) {
         return DateTime(date, time, pdk::TimeSpec::LocalTime);
      }
      StringRef tz = parts.at(5);
      if (!tz.startsWith(Latin1String("GMT"), pdk::CaseSensitivity::Insensitive)) {
         return DateTime();
      }
      tz = tz.substring(3);
      if (!tz.isEmpty()) {
         int offset = from_offset_string(tz, &ok);
         if (!ok) {
            return DateTime();
         }
         return DateTime(date, time, pdk::TimeSpec::OffsetFromUTC, offset);
      } else {
         return DateTime(date, time, pdk::TimeSpec::UTC);
      }
   }
#endif //PDK_NO_TEXTDATE
   }
   return DateTime();
}

DateTime DateTime::fromString(const String &string, const String &format)
{
#if PDK_CONFIG(datetimeparser)
   //   Time time;
   //   Date date;
   
   //   DateTimeParser dt(QVariant::DateTime, DateTimeParser::FromString);
   //   // dt.setDefaultLocale(Locale::c()); ### Qt 6
   //   if (dt.parseFormat(format) && dt.fromString(string, &date, &time))
   //      return DateTime(date, time);
#else
   PDK_UNUSED(string);
   PDK_UNUSED(format);
#endif
   return DateTime();
}

#endif // PDK_NO_DATESTRING

/*****************************************************************************
  Date/time stream functions
 *****************************************************************************/

#ifndef PDK_NO_DATASTREAM
DataStream &operator<<(DataStream &out, const Date &date)
{
   return out << pdk::pint64(date.m_jd);
}

DataStream &operator>>(DataStream &in, Date &date)
{
   pdk::pint64 jd;
   in >> jd;
   date.m_jd = jd;
   return in;
}

DataStream &operator<<(DataStream &out, const Time &time)
{
   return out << pdk::puint32(time.m_mds);
}

DataStream &operator>>(DataStream &in, Time &time)
{
   pdk::puint32 ds;
   in >> ds;
   time.m_mds = int(ds);
   return in;
}

DataStream &operator<<(DataStream &out, const DateTime &dateTime)
{
   std::pair<Date, Time> dateAndTime;
   // In 5.2 we switched to using Qt::TimeSpec and added offset support
   dateAndTime = get_datetime(dateTime.m_data);
   out << dateAndTime << pdk::pint8(dateTime.getTimeSpec());
   if (dateTime.getTimeSpec() == pdk::TimeSpec::OffsetFromUTC) {
      out << pdk::pint32(dateTime.getOffsetFromUtc());
   }
#if PDK_CONFIG(timezone)
   else if (dateTime.getTimeSpec() == pdk::TimeSpec::TimeZone)
      out << dateTime.getTimeZone();
#endif // timezone
   return out;
}

DataStream &operator>>(DataStream &in, DateTime &dateTime)
{
   Date dt;
   Time tm;
   pdk::pint8 ts = 0;
   pdk::TimeSpec spec = pdk::TimeSpec::LocalTime;
   pdk::pint32 offset = 0;
#if PDK_CONFIG(timezone)
   TimeZone tz;
#endif // timezone
   // In 5.2 we switched to using pdk::TimeSpec and added offset support
   in >> dt >> tm >> ts;
   spec = static_cast<pdk::TimeSpec>(ts);
   if (spec == pdk::TimeSpec::OffsetFromUTC) {
      in >> offset;
      dateTime = DateTime(dt, tm, spec, offset);
#if PDK_CONFIG(timezone)
   } else if (spec == pdk::TimeSpec::TimeZone) {
      in >> tz;
      dateTime = DateTime(dt, tm, tz);
#endif // timezone
   } else {
      dateTime = DateTime(dt, tm, spec);
   }
   return in;
}
#endif // PDK_NO_DATASTREAM

/*****************************************************************************
  Date / Time Debug Streams
*****************************************************************************/

#if !defined(PDK_NO_DEBUG_STREAM) && !defined(PDK_NO_DATESTRING)
Debug operator<<(Debug dbg, const Date &date)
{
   DebugStateSaver saver(dbg);
   dbg.nospace() << "Date(";
   if (date.isValid()) {
      dbg.nospace() << date.toString(pdk::DateFormat::ISODate);
   } else {
      dbg.nospace() << "Invalid";
   }
   dbg.nospace() << ')';
   return dbg;
}

Debug operator<<(Debug dbg, const Time &time)
{
   DebugStateSaver saver(dbg);
   dbg.nospace() << "Time(";
   if (time.isValid()) {
      dbg.nospace() << time.toString(StringViewLiteral("HH:mm:ss.zzz"));
   } else {
      dbg.nospace() << "Invalid";
   }
   dbg.nospace() << ')';
   return dbg;
}

Debug operator<<(Debug dbg, const DateTime &date)
{
   DebugStateSaver saver(dbg);
   dbg.nospace() << "DateTime(";
   if (date.isValid()) {
      const pdk::TimeSpec ts = date.getTimeSpec();
      dbg.noquote() << date.toString(StringViewLiteral("yyyy-MM-dd HH:mm:ss.zzz t"))
                    << ' ' << pdk::as_integer<pdk::TimeSpec>(ts);
      switch (ts) {
      case pdk::TimeSpec::UTC:
         break;
      case pdk::TimeSpec::OffsetFromUTC:
         dbg.space() << date.getOffsetFromUtc() << 's';
         break;
      case pdk::TimeSpec::TimeZone:
#if PDK_CONFIG(timezone)
         dbg.space() << date.getTimeZone().getId();
#endif // timezone
         break;
      case pdk::TimeSpec::LocalTime:
         break;
      }
   } else {
      dbg.nospace() << "Invalid";
   }
   return dbg.nospace() << ')';
}
#endif

uint pdk_hash(const DateTime &key, uint seed)
{
   // Use to toMSecsSinceEpoch instead of individual pdk_hash functions for
   // Date/Time/spec/offset because DateTime::operator== converts both arguments
   // to the same timezone. If we don't, pdk_hash would return different hashes for
   // two DateTimes that are equivalent once converted to the same timezone.
   return pdk::pdk_hash(key.toMSecsSinceEpoch(), seed);
}

uint pdk_hash(const Date &key, uint seed) noexcept
{
   return pdk::pdk_hash(key.toJulianDay(), seed);
}

uint pdk_hash(const Time &key, uint seed) noexcept
{
   return pdk::pdk_hash(key.msecsSinceStartOfDay(), seed);
}

} // time
} // pdk
