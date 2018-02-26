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

#if PDK_CONFIG(DATETIME_PARSER)
#include "pdk/base/time/internal/DateTimeParserPrivate.h"
#endif

#include "pdk/utils/Locale.h"
#include "pdk/base/time/Date.h"
#include "pdk/base/time/Time.h"
#include "pdk/base/time/DateTime.h"
#if PDK_CONFIG(timezone)
#include "pdk/base/time/internal/TimeZonePrivate.h"
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
   //         result.time = QTime(cap[4].toInt(), cap[5].toInt(), cap[6].toInt());
   //      const bool positiveOffset = (cap[7] == QLatin1String("+"));
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
   //            result.time = QTime(cap[3].toInt(), cap[4].toInt(), cap[5].toInt());
   //         const bool positiveOffset = (cap[7] == QLatin1String("+"));
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

} // time
} // pdk
