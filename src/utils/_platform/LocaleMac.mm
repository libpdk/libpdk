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

#include "pdk/utils/internal/LocalePrivate.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/time/Time.h"
#include "pdk/base/time/Date.h"
#include "pdk/base/lang/StringBuilder.h"
#include "pdk/global/Logging.h"

#ifdef PDK_OS_DARWIN
#include "pdk/base/time/TimeZone.h"
#include "pdk/kernel/internal/CoreMacPrivate.h"
#include <CoreFoundation/CoreFoundation.h>
PDK_REQUIRE_CONFIG(timezone);
#endif

#include <any>

/******************************************************************************
** Wrappers for Mac locale system functions
*/
namespace pdk {
namespace utils {

using pdk::kernel::CFType;
using pdk::kernel::CFString;
using pdk::kernel::MacAutoReleasePool;
using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::lang::Character;
using internal::SystemLocale;

namespace {

ByteArray env_var_locale()
{
   static ByteArray lang = 0;
#ifdef PDK_OS_UNIX
   lang = pdk::pdk_getenv("LC_ALL");
   if (lang.isEmpty()) {
      lang = pdk::pdk_getenv("LC_NUMERIC");
   }
   if (lang.isEmpty())
#endif
      lang = pdk::pdk_getenv("LANG");
   return lang;
}

ByteArray get_mac_locale_name()
{
   ByteArray result = env_var_locale();
   
   String lang, script, cntry;
   if (result.isEmpty()
       || (result != "C" && !internal::split_locale_name(String::fromLocal8Bit(result), lang, script, cntry))) {
      CFType<CFLocaleRef> l = CFLocaleCopyCurrent();
      CFStringRef locale = CFLocaleGetIdentifier(l);
      result = String::fromCFString(locale).toUtf8();
   }
   return result;
}

String mac_month_name(int month, bool shortFormat)
{
   month -= 1;
   if (month < 0 || month > 11) {
      return String();
   }
   CFType<CFDateFormatterRef> formatter
         = CFDateFormatterCreate(0, CFType<CFLocaleRef>(CFLocaleCopyCurrent()),
                                 kCFDateFormatterNoStyle,  kCFDateFormatterNoStyle);
   CFType<CFArrayRef> values
         = static_cast<CFArrayRef>(CFDateFormatterCopyProperty(formatter,
                                                               shortFormat ? kCFDateFormatterShortMonthSymbols
                                                                           : kCFDateFormatterMonthSymbols));
   if (values != 0) {
      CFStringRef cfstring = static_cast<CFStringRef>(CFArrayGetValueAtIndex(values, month));
      return String::fromCFString(cfstring);
   }
   return String();
}

String mac_day_name(int day, bool shortFormat)
{
   if (day < 1 || day > 7)
      return String();
   
   CFType<CFDateFormatterRef> formatter
         = CFDateFormatterCreate(0, CFType<CFLocaleRef>(CFLocaleCopyCurrent()),
                                 kCFDateFormatterNoStyle,  kCFDateFormatterNoStyle);
   CFType<CFArrayRef> values = static_cast<CFArrayRef>(CFDateFormatterCopyProperty(formatter,
                                                                                   shortFormat ? kCFDateFormatterShortWeekdaySymbols
                                                                                               : kCFDateFormatterWeekdaySymbols));
   if (values != 0) {
      CFStringRef cfstring = static_cast<CFStringRef>(CFArrayGetValueAtIndex(values, day % 7));
      return String::fromCFString(cfstring);
   }
   return String();
}

String mac_date_to_string(const Date &date, bool shortFormat)
{
   CFType<CFDateRef> myDate = DateTime(date, Time()).toCFDate();
   CFType<CFLocaleRef> mylocale = CFLocaleCopyCurrent();
   CFDateFormatterStyle style = shortFormat ? kCFDateFormatterShortStyle : kCFDateFormatterLongStyle;
   CFType<CFDateFormatterRef> myFormatter
         = CFDateFormatterCreate(kCFAllocatorDefault,
                                 mylocale, style,
                                 kCFDateFormatterNoStyle);
   return CFString(CFDateFormatterCreateStringWithDate(0, myFormatter, myDate));
}

static String mac_time_to_string(const Time &time, bool shortFormat)
{
   CFType<CFDateRef> myDate = DateTime(Date::getCurrentDate(), time).toCFDate();
   CFType<CFLocaleRef> mylocale = CFLocaleCopyCurrent();
   CFDateFormatterStyle style = shortFormat ? kCFDateFormatterShortStyle :  kCFDateFormatterLongStyle;
   CFType<CFDateFormatterRef> myFormatter = CFDateFormatterCreate(kCFAllocatorDefault,
                                                                  mylocale,
                                                                  kCFDateFormatterNoStyle,
                                                                  style);
   return CFString(CFDateFormatterCreateStringWithDate(0, myFormatter, myDate));
}

// Mac uses the Unicode CLDR format codes
// http://www.unicode.org/reports/tr35/tr35-dates.html#Date_Field_Symbol_Table
// See also scripts/local_database/dateconverter.py
// Makes the assumption that input formats are always well formed and consecutive letters
// never exceed the maximum for the format code.
static String mac_to_pdk_format(StringView sysFmt)
{
   String result;
   int i = 0;
   
   while (static_cast<size_t>(i) < sysFmt.size()) {
      if (sysFmt.at(i).unicode() == '\'') {
         String text = internal::read_escaped_format_string(sysFmt, &i);
         if (text == Latin1String("'")) {
            result += Latin1String("''");
         } else {
            result += Latin1Character('\'') + text + Latin1Character('\'');
         }
         continue;
      }
      
      Character c = sysFmt.at(i);
      int repeat = internal::repeat_count(sysFmt.substring(i));
      
      switch (c.unicode()) {
      // pdk does not support the following options
      case 'G': // Era (1..5): 4 = long, 1..3 = short, 5 = narrow
      case 'Y': // Year of Week (1..n): 1..n = padded number
      case 'U': // Cyclic Year Name (1..5): 4 = long, 1..3 = short, 5 = narrow
      case 'Q': // Quarter (1..4): 4 = long, 3 = short, 1..2 = padded number
      case 'q': // Standalone Quarter (1..4): 4 = long, 3 = short, 1..2 = padded number
      case 'w': // Week of Year (1..2): 1..2 = padded number
      case 'W': // Week of Month (1): 1 = number
      case 'D': // Day of Year (1..3): 1..3 = padded number
      case 'F': // Day of Week in Month (1): 1 = number
      case 'g': // Modified Julian Day (1..n): 1..n = padded number
      case 'A': // Milliseconds in Day (1..n): 1..n = padded number
         break;
         
      case 'y': // Year (1..n): 2 = short year, 1 & 3..n = padded number
      case 'u': // Extended Year (1..n): 2 = short year, 1 & 3..n = padded number
         // pdk only supports long (4) or short (2) year, use long for all others
         if (repeat == 2) {
            result += Latin1String("yy");
         } else {
            result += Latin1String("yyyy");
         }
         break;
      case 'M': // Month (1..5): 4 = long, 3 = short, 1..2 = number, 5 = narrow
      case 'L': // Standalone Month (1..5): 4 = long, 3 = short, 1..2 = number, 5 = narrow
         // Qt only supports long, short and number, use short for narrow
         if (repeat == 5) {
            result += Latin1String("MMM");
         } else {
            result += String(repeat, Latin1Character('M'));
         }
         break;
      case 'd': // Day of Month (1..2): 1..2 padded number
         result += String(repeat, c);
         break;
      case 'E': // Day of Week (1..6): 4 = long, 1..3 = short, 5..6 = narrow
         // pdk only supports long, short and padded number, use short for narrow
         if (repeat == 4) {
            result += Latin1String("dddd");
         } else {
            result += Latin1String("ddd");
         } 
         break;
      case 'e': // Local Day of Week (1..6): 4 = long, 3 = short, 5..6 = narrow, 1..2 padded number
      case 'c': // Standalone Local Day of Week (1..6): 4 = long, 3 = short, 5..6 = narrow, 1..2 padded number
         // pdk only supports long, short and padded number, use short for narrow
         if (repeat >= 5) {
            result += Latin1String("ddd");
         } else {
            result += String(repeat, Latin1Character('d'));
         }            
         break;
      case 'a': // AM/PM (1): 1 = short
         // Translate to pdk uppercase AM/PM
         result += Latin1String("AP");
         break;
      case 'h': // Hour [1..12] (1..2): 1..2 = padded number
      case 'K': // Hour [0..11] (1..2): 1..2 = padded number
      case 'j': // Local Hour [12 or 24] (1..2): 1..2 = padded number
         // pdk h is local hour
         result += String(repeat, Latin1Character('h'));
         break;
      case 'H': // Hour [0..23] (1..2): 1..2 = padded number
      case 'k': // Hour [1..24] (1..2): 1..2 = padded number
         // pdk H is 0..23 hour
         result += String(repeat, Latin1Character('H'));
         break;
      case 'm': // Minutes (1..2): 1..2 = padded number
      case 's': // Seconds (1..2): 1..2 = padded number
         result += String(repeat, c);
         break;
      case 'S': // Fractional second (1..n): 1..n = truncates to decimal places
         // pdk uses msecs either unpadded or padded to 3 places
         if (repeat < 3) {
            result += Latin1Character('z');
         } else {
            result += Latin1String("zzz");
         }
         break;
      case 'z': // Time Zone (1..4)
      case 'Z': // Time Zone (1..5)
      case 'O': // Time Zone (1, 4)
      case 'v': // Time Zone (1, 4)
      case 'V': // Time Zone (1..4)
      case 'X': // Time Zone (1..5)
      case 'x': // Time Zone (1..5)
         result += Latin1Character('t');
         break;
      default:
         // a..z and A..Z are reserved for format codes, so any occurrence of these not
         // already processed are not known and so unsupported formats to be ignored.
         // All other chars are allowed as literals.
         if (c < Latin1Character('A') || c > Latin1Character('z') ||
             (c > Latin1Character('Z') && c < Latin1Character('a'))) {
            result += String(repeat, c);
         }
         break;
      }
      
      i += repeat;
   }
   
   return result;
}

String get_mac_date_format(CFDateFormatterStyle style)
{
   CFType<CFLocaleRef> l = CFLocaleCopyCurrent();
   CFType<CFDateFormatterRef> formatter = CFDateFormatterCreate(kCFAllocatorDefault,
                                                                l, style, kCFDateFormatterNoStyle);
   return mac_to_pdk_format(String::fromCFString(CFDateFormatterGetFormat(formatter)));
}

String get_mac_time_format(CFDateFormatterStyle style)
{
   CFType<CFLocaleRef> l = CFLocaleCopyCurrent();
   CFType<CFDateFormatterRef> formatter = CFDateFormatterCreate(kCFAllocatorDefault,
                                                                l, kCFDateFormatterNoStyle, style);
   return mac_to_pdk_format(String::fromCFString(CFDateFormatterGetFormat(formatter)));
}

String get_cf_locale_value(CFStringRef key)
{
   CFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
   CFTypeRef value = CFLocaleGetValue(locale, key);
   return String::fromCFString(CFStringRef(static_cast<CFTypeRef>(value)));
}

Locale::MeasurementSystem mac_measurement_system()
{
   CFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
   CFStringRef system = static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleMeasurementSystem));
   if (String::fromCFString(system) == Latin1String("Metric")) {
      return Locale::MeasurementSystem::MetricSystem;
   } else {
      return Locale::MeasurementSystem::ImperialUSSystem;
   }
}


pdk::puint8 mac_first_day_of_week()
{
   CFType<CFCalendarRef> calendar = CFCalendarCopyCurrent();
   pdk::puint8 day = static_cast<pdk::puint8>(CFCalendarGetFirstWeekday(calendar))-1;
   if (day == 0) {
      day = 7;
   }
   return day;
}

String mac_currency_symbol(Locale::CurrencySymbolFormat format)
{
   CFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
   switch (format) {
   case Locale::CurrencyIsoCode:
      return String::fromCFString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleCurrencyCode)));
   case Locale::CurrencySymbol:
      return String::fromCFString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleCurrencySymbol)));
   case Locale::CurrencyDisplayName: {
      CFStringRef code = static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleCurrencyCode));
      CFType<CFStringRef> value = CFLocaleCopyDisplayNameForPropertyValue(locale, kCFLocaleCurrencyCode, code);
      return String::fromCFString(value);
   }
   }
   return String();
}

#ifndef PDK_NO_SYSTEMLOCALE
static String macFormatCurrency(const SystemLocale::CurrencyToStringArgument &arg)
{
   CFType<CFNumberRef> value;
   size_t vhashCode = arg.m_value.type().hash_code();
   if (vhashCode == typeid(int).hash_code() ||
       vhashCode == typeid(uint).hash_code()) {
      int v = std::any_cast<int>(arg.m_value);
      value = CFNumberCreate(NULL, kCFNumberIntType, &v);
   } else if (vhashCode == typeid(double).hash_code()) {
      double v = std::any_cast<double>(arg.m_value);
      value = CFNumberCreate(NULL, kCFNumberDoubleType, &v);
   } else if (vhashCode == typeid(pdk::plonglong).hash_code() ||
              vhashCode == typeid(pdk::pulonglong).hash_code()) {
      pdk::pint64 v = std::any_cast<pdk::plonglong>(arg.m_value);
      value = CFNumberCreate(NULL, kCFNumberLongLongType, &v);
   } else {
      return String();
   }
   
   CFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
   CFType<CFNumberFormatterRef> currencyFormatter =
         CFNumberFormatterCreate(NULL, locale, kCFNumberFormatterCurrencyStyle);
   if (!arg.m_symbol.isEmpty()) {
      CFNumberFormatterSetProperty(currencyFormatter, kCFNumberFormatterCurrencySymbol,
                                   arg.m_symbol.toCFString());
   }
   CFType<CFStringRef> result = CFNumberFormatterCreateStringWithNumber(NULL, currencyFormatter, value);
   return String::fromCFString(result);
}

std::any mac_quote_string(SystemLocale::QueryType type, const StringRef &str)
{
   String begin, end;
   CFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
   switch (type) {
   case SystemLocale::QueryType::StringToStandardQuotation:
      begin = String::fromCFString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleQuotationBeginDelimiterKey)));
      end = String::fromCFString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleQuotationEndDelimiterKey)));
      return String(begin % str % end);
   case SystemLocale::QueryType::StringToAlternateQuotation:
      begin = String::fromCFString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleAlternateQuotationBeginDelimiterKey)));
      end = String::fromCFString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleAlternateQuotationEndDelimiterKey)));
      return String(begin % str % end);
   default:
      break;
   }
   return std::any();
}
#endif //PDK_NO_SYSTEMLOCALE

} // anonymous namespace


#ifndef PDK_NO_SYSTEMLOCALE

Locale SystemLocale::fallbackUiLocale() const
{
   return Locale(String::fromUtf8(get_mac_locale_name().getConstRawData()));
}

std::any SystemLocale::query(QueryType type, std::any in = std::any()) const
{
   MacAutoReleasePool pool;
   switch(type) {
   //     case Name:
   //         return getMacLocaleName();
   case QueryType::DecimalPoint: {
      String value = get_cf_locale_value(kCFLocaleDecimalSeparator);
      return value.isEmpty() ? std::any() : value;
   }
   case QueryType::GroupSeparator: {
      String value = get_cf_locale_value(kCFLocaleGroupingSeparator);
      return value.isEmpty() ? std::any() : value;
   }
   case QueryType::DateFormatLong:
   case QueryType::DateFormatShort:
      return get_mac_date_format(type == QueryType::DateFormatShort
                              ? kCFDateFormatterShortStyle
                              : kCFDateFormatterLongStyle);
   case QueryType::TimeFormatLong:
   case QueryType::TimeFormatShort:
      return get_mac_time_format(type == QueryType::TimeFormatShort
                              ? kCFDateFormatterShortStyle
                              : kCFDateFormatterLongStyle);
   case QueryType::DayNameLong:
   case QueryType::DayNameShort:
      return mac_day_name(std::any_cast<int>(in), (type == QueryType::DayNameShort));
   case QueryType::MonthNameLong:
   case QueryType::MonthNameShort:
   case QueryType::StandaloneMonthNameLong:
   case QueryType::StandaloneMonthNameShort:
      return mac_month_name(std::any_cast<int>(in), (type == QueryType::MonthNameShort || type == QueryType::StandaloneMonthNameShort));
   case QueryType::DateToStringShort:
   case QueryType::DateToStringLong:
      return mac_date_to_string(std::any_cast<Date>(in), (type == QueryType::DateToStringShort));
   case QueryType::TimeToStringShort:
   case QueryType::TimeToStringLong:
      return mac_time_to_string(std::any_cast<Time>(in), (type == QueryType::TimeToStringShort));
      
   case QueryType::NegativeSign:
   case QueryType::PositiveSign:
   case QueryType::ZeroDigit:
      break;
      
   case QueryType::MeasurementSystem:
      return std::any(static_cast<int>(mac_measurement_system()));
      
   case QueryType::AMText:
   case QueryType::PMText: {
      CFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
      CFType<CFDateFormatterRef> formatter = CFDateFormatterCreate(NULL, locale, kCFDateFormatterLongStyle, kCFDateFormatterLongStyle);
      CFType<CFStringRef> value = static_cast<CFStringRef>(CFDateFormatterCopyProperty(formatter,
                                                                                        (type == QueryType::AMText ? kCFDateFormatterAMSymbol : kCFDateFormatterPMSymbol)));
      return String::fromCFString(value);
   }
   case QueryType::FirstDayOfWeek:
      return std::any(mac_first_day_of_week());
   case QueryType::CurrencySymbol:
      return std::any(mac_currency_symbol(Locale::CurrencySymbolFormat(std::any_cast<uint>(in))));
   case QueryType::CurrencyToString:
      return macFormatCurrency(std::any_cast<SystemLocale::CurrencyToStringArgument>(in));
   case QueryType::UILanguages: {
      CFType<CFPropertyListRef> languages = CFPreferencesCopyValue(
               CFSTR("AppleLanguages"),
               kCFPreferencesAnyApplication,
               kCFPreferencesCurrentUser,
               kCFPreferencesAnyHost);
      StringList result;
      if (!languages) {
         return std::any(result);
      }
      CFTypeID typeId = CFGetTypeID(languages);
      if (typeId == CFArrayGetTypeID()) {
         const int cnt = CFArrayGetCount(languages.as<CFArrayRef>());
         result.reserve(cnt);
         for (int i = 0; i < cnt; ++i) {
            const String lang = String::fromCFString(
                     static_cast<CFStringRef>(CFArrayGetValueAtIndex(languages.as<CFArrayRef>(), i)));
            result.push_back(lang);
         }
      } else if (typeId == CFStringGetTypeID()) {
         result = StringList(String::fromCFString(languages.as<CFStringRef>()));
      } else {
         warning_stream("Locale::uiLanguages(): CFPreferencesCopyValue returned unhandled type \"%s\"; please report to http://bugreports.libpdk.org",
                  pdk_printable(String::fromCFString(CFCopyTypeIDDescription(typeId))));
      }
      return std::any(result);
   }
   case QueryType::StringToStandardQuotation:
   case QueryType::StringToAlternateQuotation:
      return mac_quote_string(type, std::any_cast<StringRef>(in));
   default:
      break;
   }
   return std::any();
}

#endif // PDK_NO_SYSTEMLOCALE

} // utils
} // pdk
