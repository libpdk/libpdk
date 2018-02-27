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
// Created by softboy on 2018/01/31.

#ifndef PDK_UTILS_INTERNAL_LOCALE_PRIVATE_H
#define PDK_UTILS_INTERNAL_LOCALE_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/base/os/thread/Atomic.h"

#include "pdk/utils/Locale.h"
#include <limits>
#include <cmath>
#include <any>

namespace pdk {
namespace utils {
namespace internal {

using pdk::lang::String;
using pdk::lang::Character;
using pdk::lang::Latin1Character;
using pdk::lang::Latin1String;
using pdk::lang::StringView;
using pdk::ds::ByteArray;
using pdk::ds::VarLengthArray;
using pdk::os::thread::BasicAtomicInt;

#ifndef PDK_NO_SYSTEMLOCALE

class PDK_CORE_EXPORT SystemLocale
{
public:
   SystemLocale();
   virtual ~SystemLocale();
   
   struct CurrencyToStringArgument
   {
      CurrencyToStringArgument()
      {}
      
      CurrencyToStringArgument(const std::any &value, const String &symbol)
         : m_value(value),
           m_symbol(symbol)
      {}
      
      std::any m_value;
      String m_symbol;
   };
   
   enum class QueryType {
      LanguageId, // uint
      CountryId, // uint
      DecimalPoint, // String
      GroupSeparator, // String
      ZeroDigit, // String
      NegativeSign, // String
      DateFormatLong, // String
      DateFormatShort, // String
      TimeFormatLong, // String
      TimeFormatShort, // String
      DayNameLong, // String, in: int
      DayNameShort, // String, in: int
      MonthNameLong, // String, in: int
      MonthNameShort, // String, in: int
      DateToStringLong, // String, in: Date
      DateToStringShort, // String in: Date
      TimeToStringLong, // String in: Time
      TimeToStringShort, // String in: Time
      DateTimeFormatLong, // String
      DateTimeFormatShort, // String
      DateTimeToStringLong, // String in: DateTime
      DateTimeToStringShort, // String in: DateTime
      MeasurementSystem, // uint
      PositiveSign, // String
      AMText, // String
      PMText, // String
      FirstDayOfWeek, // pdk::DayOfWeek
      Weekdays, // std::list<pdk::DayOfWeek>
      CurrencySymbol, // String in: CurrencyToStringArgument
      CurrencyToString, // String in: qlonglong, qulonglong or double
      UILanguages, // StringList
      StringToStandardQuotation, // String in: StringRef to quote
      StringToAlternateQuotation, // String in: StringRef to quote
      ScriptId, // uint
      ListToSeparatedString, // String
      LocaleChanged, // system locale changed
      NativeLanguageName, // String
      NativeCountryName, // String
      StandaloneMonthNameLong, // String, in: int
      StandaloneMonthNameShort // String, in: int
   };
   virtual std::any query(QueryType type, std::any in) const;
   virtual Locale fallbackUiLocale() const;
   
private:
   SystemLocale(bool);
   friend class SystemLocaleSingleton;
};



#endif

#if PDK_CONFIG(ICU)

namespace icu {

String toUpper(const ByteArray &localeId, const String &str, bool *ok);
String toLower(const ByteArray &localeId, const String &str, bool *ok);

} // icu

#endif

struct LocaleId
{
   // bypass constructors
   static inline LocaleId fromIds(ushort language, ushort script, ushort country)
   {
      const LocaleId localeId = { language, script, country };
      return localeId;
   }
   
   inline bool operator==(LocaleId other) const
   {
      return m_languageId == other.m_languageId && 
            m_scriptId == other.m_scriptId && 
            m_countryId == other.m_countryId;
   }
   
   inline bool operator!=(LocaleId other) const
   {
      return !operator==(other);
   }
   
   LocaleId withLikelySubtagsAdded() const;
   LocaleId withLikelySubtagsRemoved() const;
   
   ByteArray name(char separator = '-') const;
   
   ushort m_languageId;
   ushort m_scriptId;
   ushort m_countryId;
};

struct LocaleData
{
public:
   static const LocaleData *findLocaleData(Locale::Language language,
                                           Locale::Script script,
                                           Locale::Country country);
   static const LocaleData *c();
   // Maximum number of significant digits needed to represent a double.
   // We cannot use std::numeric_limits here without constexpr.
   static const int DoubleMantissaBits = 53;
   static const int Log10_2_100000 = 30103;    // log10(2) * 100000
   // same as C++11 std::numeric_limits<T>::max_digits10
   static const int DoubleMaxSignificant = (DoubleMantissaBits * Log10_2_100000) / 100000 + 2;
   
   // Maximum number of digits before decimal point to represent a double
   // Same as std::numeric_limits<double>::max_exponent10 + 1
   static const int DoubleMaxDigitsBeforeDecimal = 309;
   
   enum class DoubleForm : uint
   {
      DFExponent = 0,
      DFDecimal,
      DFSignificantDigits,
      _DFMax = DFSignificantDigits
   };
   
   enum class Flags : uint
   {
      NoFlags             = 0,
      AddTrailingZeroes   = 0x01,
      ZeroPadded          = 0x02,
      LeftAdjusted        = 0x04,
      BlankBeforePositive = 0x08,
      AlwaysShowSign      = 0x10,
      ThousandsGroup      = 0x20,
      CapitalEorX         = 0x40,
      
      ShowBase            = 0x80,
      UppercaseBase       = 0x100,
      ZeroPadExponent     = 0x200,
      ForcePoint          = 0x400
   };
   
   enum class NumberMode
   { 
      IntegerMode,
      DoubleStandardMode,
      DoubleScientificMode
   };
   
   using CharBuff = VarLengthArray<char, 256>;
   
   static String doubleToString(const Character zero, const Character plus,
                                const Character minus, const Character exponent,
                                const Character group, const Character decimal,
                                double d, int precision,
                                DoubleForm form,
                                int width, unsigned flags);
   
   static String longLongToString(const Character zero, const Character group,
                                  const Character plus, const Character minus,
                                  pdk::pint64 l, int precision, int base,
                                  int width, unsigned flags);
   
   static String unsLongLongToString(const Character zero, const Character group,
                                     const Character plus,
                                     pdk::puint64 l, int precision,
                                     int base, int width,
                                     unsigned flags);
   
   String doubleToString(double d,
                         int precision = -1,
                         DoubleForm form = DoubleForm::DFSignificantDigits,
                         int width = -1,
                         unsigned flags = (unsigned)Flags::NoFlags) const;
   
   String longLongToString(pdk::pint64 l, int precision = -1,
                           int base = 10,
                           int width = -1,
                           unsigned flags = (unsigned)Flags::NoFlags) const;
   
   String unsLongLongToString(pdk::puint64 l, int precision = -1,
                              int base = 10,
                              int width = -1,
                              unsigned flags = (unsigned)Flags::NoFlags) const;
   
   // this function is meant to be called with the result of stringToDouble or bytearrayToDouble
   static float convertDoubleToFloat(double d, bool *ok)
   {
      if (std::isinf(d))
         return float(d);
      if (std::fabs(d) > std::numeric_limits<float>::max()) {
         if (ok != 0) {
            *ok = false;
         }
         return 0.0f;
      }
      return float(d);
   }
   
   double stringToDouble(StringView str, bool *ok,
                         Locale::NumberOptions numberOptions) const;
   pdk::pint64 stringToLongLong(StringView str, int base, bool *ok,
                                Locale::NumberOptions numberOptions) const;
   pdk::puint64 stringToUnsLongLong(StringView str, int base, bool *ok,
                                    Locale::NumberOptions numberOptions) const;
   
   // these functions are used in IntValidator (Gui)
   static double bytearrayToDouble(const char *num, bool *ok);
   PDK_CORE_EXPORT static pdk::pint64 bytearrayToLongLong(const char *num, int base, bool *ok);
   static pdk::puint64 bytearrayToUnsLongLong(const char *num, int base, bool *ok);
   
   bool numberToCLocale(StringView str, Locale::NumberOptions number_options,
                        CharBuff *result) const;
   inline char digitToCLocale(Character c) const;
   
   // this function is used in QIntValidator (QtGui)
   PDK_CORE_EXPORT bool validateChars(
         StringView str, NumberMode numMode, ByteArray *buff, int decDigits = -1,
         Locale::NumberOptions numberOptions = Locale::NumberOption::DefaultNumberOptions) const;
   
public:
   pdk::puint16 m_languageId;
   pdk::puint16 m_scriptId;
   pdk::puint16 m_countryId;
   
   pdk::puint16 m_decimal;
   pdk::puint16 m_group;
   pdk::puint16 m_list;
   pdk::puint16 m_percent;
   pdk::puint16 m_zero;
   pdk::puint16 m_minus;
   pdk::puint16 m_plus;
   pdk::puint16 m_exponential;
   pdk::puint16 m_quotationStart;
   pdk::puint16 m_quotationEnd;
   pdk::puint16 m_alternateQuotationStart;
   pdk::puint16 m_alternateQuotationEnd;
   
   pdk::puint16 m_listPatternPartStartIdx;
   pdk::puint16 m_listPatternPartStartSize;
   pdk::puint16 m_listPatternPartMidIdx;
   pdk::puint16 m_listPatternPartMidSize;
   pdk::puint16 m_listPatternPartEndIdx;
   pdk::puint16 m_listPatternPartEndSize;
   pdk::puint16 m_listPatternPartTwoIdx;
   pdk::puint16 m_listPatternPartTwoSize;
   pdk::puint16 m_shortDateFormatIdx;
   pdk::puint16 m_shortDateFormatSize;
   pdk::puint16 m_longDateFormatIdx;
   pdk::puint16 m_longDateFormatSize;
   pdk::puint16 m_shortTimeFormatIdx;
   pdk::puint16 m_shortTimeFormatSize;
   pdk::puint16 m_longTimeFormatIdx;
   pdk::puint16 m_longTimeFormatSize;
   pdk::puint16 m_standaloneShortMonthNamesIdx;
   pdk::puint16 m_standaloneShortMonthNamesSize;
   pdk::puint16 m_standaloneLongMonthNamesIdx;
   pdk::puint16 m_standaloneLongMonthNamesSize;
   pdk::puint16 m_standaloneNarrowMonthNamesIdx;
   pdk::puint16 m_standaloneNarrowMonthNamesSize;
   pdk::puint16 m_shortMonthNamesIdx;
   pdk::puint16 m_shortMonthNamesSize;
   pdk::puint16 m_longMonthNamesIdx;
   pdk::puint16 m_longMonthNamesSize;
   pdk::puint16 m_narrowMonthNamesIdx;
   pdk::puint16 m_narrowMonthNamesSize;
   pdk::puint16 m_standaloneShortDayNamesIdx;
   pdk::puint16 m_standaloneShortDayNamesSize;
   pdk::puint16 m_standaloneLongDayNamesIdx;
   pdk::puint16 m_standaloneLongDayNamesSize;
   pdk::puint16 m_standaloneNarrowDayNamesIdx;
   pdk::puint16 m_standaloneNarrowDayNamesSize;
   pdk::puint16 m_shortDayNamesIdx;
   pdk::puint16 m_shortDayNamesSize;
   pdk::puint16 m_longDayNamesIdx;
   pdk::puint16 m_longDayNamesSize;
   pdk::puint16 m_narrowDayNamesIdx;
   pdk::puint16 m_narrowDayNamesSize;
   pdk::puint16 m_amIdx;
   pdk::puint16 m_amSize;
   pdk::puint16 m_pmIdx;
   pdk::puint16 m_pmSize;
   pdk::puint16 m_byteIdx;
   pdk::puint16 m_byteSize;
   pdk::puint16 m_byteSiQuantifiedIdx;
   pdk::puint16 m_byteSiQuantifiedSize;
   pdk::puint16 m_byteIecQuantifiedIdx;
   pdk::puint16 m_byteIecQuantifiedSize;
   char    m_currencyIsoCode[3];
   pdk::puint16 m_currencySymbolIdx;
   pdk::puint16 m_currencySymbolSize;
   pdk::puint16 m_currencyDisplayNameIdx;
   pdk::puint16 m_currencyDisplayNameSize;
   pdk::puint8  m_currencyFormatIdx;
   pdk::puint8  m_currencyFormatSize;
   pdk::puint8  m_currencyNegativeFormatIdx;
   pdk::puint8  m_currencyNegativeFormatSize;
   pdk::puint16 m_languageEndonymIdx;
   pdk::puint16 m_languageEndonymSize;
   pdk::puint16 m_countryEndonymIdx;
   pdk::puint16 m_countryEndonymSize;
   pdk::puint16 m_currencyDigits : 2;
   pdk::puint16 m_currencyRounding : 3;
   pdk::puint16 m_firstDayOfWeek : 3;
   pdk::puint16 m_weekendStart : 3;
   pdk::puint16 m_weekendEnd : 3;
   
};

class PDK_CORE_EXPORT LocalePrivate
{
public:
   static LocalePrivate *create(
         const LocaleData *data,
         Locale::NumberOptions numberOptions = Locale::NumberOption::DefaultNumberOptions)
   {
      LocalePrivate *retval = new LocalePrivate;
      retval->m_data = data;
      retval->m_ref.store(0);
      retval->m_numberOptions = numberOptions;
      return retval;
   }
   
   static LocalePrivate *get(Locale &locale)
   {
      return locale.m_implPtr;
   }
   
   static const LocalePrivate *get(const Locale &locale)
   {
      return locale.m_implPtr;
   }
   
   Character decimal() const
   {
      return Character(m_data->m_decimal);
   }
   
   Character group() const 
   {
      return Character(m_data->m_group);
   }
   
   Character list() const
   {
      return Character(m_data->m_list);
   }
   
   Character percent() const
   {
      return Character(m_data->m_percent);
   }
   
   Character zero() const
   {
      return Character(m_data->m_zero);
   }
   
   Character plus() const
   {
      return Character(m_data->m_plus);
   }
   
   Character minus() const
   {
      return Character(m_data->m_minus);
   }
   
   Character exponential() const
   {
      return Character(m_data->m_exponential);
   }
   
   pdk::puint16 languageId() const 
   {
      return m_data->m_languageId;
   }
   
   pdk::puint16 countryId() const
   {
      return m_data->m_countryId;
   }
   
   ByteArray bcp47Name(char separator = '-') const;
   
   inline Latin1String languageCode() const 
   {
      return LocalePrivate::languageToCode(Locale::Language(m_data->m_languageId));
   }
   
   inline Latin1String scriptCode() const
   {
      return LocalePrivate::scriptToCode(Locale::Script(m_data->m_scriptId));
   }
   
   inline Latin1String countryCode() const
   {
      return LocalePrivate::countryToCode(Locale::Country(m_data->m_countryId));
   }
   
   static Latin1String languageToCode(Locale::Language language);
   static Latin1String scriptToCode(Locale::Script script);
   static Latin1String countryToCode(Locale::Country country);
   
   static Locale::Language codeToLanguage(StringView code) noexcept;
   static Locale::Script codeToScript(StringView code) noexcept;
   static Locale::Country codeToCountry(StringView code) noexcept;
   
   static void getLangAndCountry(const String &name, Locale::Language &lang,
                                 Locale::Script &script, Locale::Country &cntry);
   
   Locale::MeasurementSystem measurementSystem() const;
   
   static void updateSystemPrivate();
   
   String dateTimeToString(StringView format, const DateTime &datetime,
                           const Date &dateOnly, const Time &timeOnly,
                           const Locale *q) const;
   
   const LocaleData *m_data;
   BasicAtomicInt m_ref;
   Locale::NumberOptions m_numberOptions;
};

inline char LocaleData::digitToCLocale(Character in) const
{
   const ushort tenUnicode = m_zero + 10;
   
   if (in.unicode() >= m_zero && in.unicode() < tenUnicode) {
      return '0' + in.unicode() - m_zero;
   }
   if (in.unicode() >= '0' && in.unicode() <= '9') {
      return in.toLatin1();
   }
   if (in == m_plus || in == Latin1Character('+')) {
      return '+';
   }
   if (in == m_minus || in == Latin1Character('-') || in == Character(0x2212)) {
      return '-';
   }
   if (in == m_decimal) {
      return '.';
   }
   if (in == m_group) {
      return ',';
   }
   if (in == m_exponential || in == Character::toUpper(m_exponential)) {
      return 'e';
   }
   // In several languages group() is the char 0xA0, which looks like a space.
   // People use a regular space instead of it and complain it doesn't work.
   if (m_group == 0xA0 && in.unicode() == ' ') {
      return ',';
   }
   return 0;
}

String read_escaped_format_string(StringView format, int *idx);
bool split_locale_name(const String &name, String &lang, String &script, String &cntry);
int repeat_count(StringView str);

} // internal

template <>
inline LocalePrivate *SharedDataPointer<internal::LocalePrivate>::clone()
{
   // cannot use LocalePrivate's copy constructor
   // since it is deleted in C++11
   return internal::LocalePrivate::create(m_implPtr->m_data, m_implPtr->m_numberOptions);
}

} // utils
} // pdk

PDK_DECLARE_TYPEINFO(pdk::utils::internal::SystemLocale::QueryType, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(pdk::utils::internal::SystemLocale::CurrencyToStringArgument, PDK_MOVABLE_TYPE);

#endif // PDK_UTILS_INTERNAL_LOCALE_PRIVATE_H
