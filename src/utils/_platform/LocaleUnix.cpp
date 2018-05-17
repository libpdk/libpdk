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
#include "pdk/base/lang/StringBuilder.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/time/Time.h"
#include "pdk/base/time/Date.h"
#include "pdk/base/os/thread/ReadWriteLock.h"
#include "pdk/global/GlobalStatic.h"
#include <any>

namespace pdk {
namespace utils {
namespace internal {

using pdk::os::thread::ReadWriteLock;
using pdk::os::thread::WriteLocker;
using pdk::os::thread::ReadLocker;
using pdk::ds::ByteArray;
using pdk::ds::StringList;
using pdk::ds::String;
using pdk::time::Date;
using pdk::time::Time;
using pdk::time::DateTime;

#ifndef PDK_NO_SYSTEMLOCALE

struct SystemLocaleData
{
   SystemLocaleData()
      : m_lcnumeric(Locale::Language::C),
        m_lctime(Locale::Language::C),
        m_lcmonetary(Locale::Language::C),
        m_lcmessages(Locale::Language::C)
   {
      readEnvironment();
   }
   
   void readEnvironment();
   
   ReadWriteLock m_lock;
   
   Locale m_lcnumeric;
   Locale m_lctime;
   Locale m_lcmonetary;
   Locale m_lcmessages;
   ByteArray m_lcmessagesVar;
   ByteArray m_lcmeasurementVar;
   StringList m_uiLanguages;
};

void SystemLocaleData::readEnvironment()
{
   WriteLocker locker(&m_lock);
   ByteArray all = pdk::get_env("LC_ALL");
   ByteArray numeric  = all.isEmpty() ? pdk::get_env("m_lcnumeric") : all;
   ByteArray time     = all.isEmpty() ? pdk::get_env("m_lctime") : all;
   ByteArray monetary = all.isEmpty() ? pdk::get_env("m_lcmonetary") : all;
   m_lcmessagesVar     = all.isEmpty() ? pdk::get_env("m_lcmessages") : all;
   m_lcmeasurementVar  = all.isEmpty() ? pdk::get_env("LC_MEASUREMENT") : all;
   ByteArray lang = pdk::get_env("LANG");
   if (lang.isEmpty()) {
      lang = ByteArray("C");
   }  
   if (numeric.isEmpty()) {
      numeric = lang;
   }
   if (time.isEmpty()) {
      time = lang;
   }  
   if (monetary.isEmpty()) {
      monetary = lang;
   }
   if (m_lcmessagesVar.isEmpty()) {
      m_lcmessagesVar = lang;
   }
   if (m_lcmeasurementVar.isEmpty()) {
      m_lcmeasurementVar = lang;
   }
   m_lcnumeric = Locale(String::fromLatin1(numeric));
   m_lctime = Locale(String::fromLatin1(time));
   m_lcmonetary = Locale(String::fromLatin1(monetary));
   m_lcmessages = Locale(String::fromLatin1(m_lcmessagesVar));
}

PDK_GLOBAL_STATIC(SystemLocaleData, sg_systemLocaleData);

Locale SystemLocale::fallbackUiLocale() const
{
   ByteArray lang = pdk::get_env("LC_ALL");
   if (lang.isEmpty()) {
      lang = pdk::get_env("m_lcmessages");
   }
   if (lang.isEmpty()) {
      lang = pdk::get_env("LANG");
   }
   // if the locale is the "C" locale, then we can return the language we found here:
   if (lang.isEmpty() || lang == ByteArray("C") || lang == ByteArray("POSIX")) {
      return Locale(String::fromLatin1(lang));
   }
   // if the locale is not the "C" locale and LANGUAGE is not empty, return
   // the first part of LANGUAGE if LANGUAGE is set and has a first part:
   ByteArray language = pdk::get_env("LANGUAGE");
   if (!language.isEmpty()) {
      language = language.split(':').front();
      if (!language.isEmpty()) {
         return Locale(String::fromLatin1(language));
      }
   }
   return Locale(String::fromLatin1(lang));
}

std::any SystemLocale::query(QueryType type, std::any in) const
{
   SystemLocaleData *d = sg_systemLocaleData();
   
   if (type == QueryType::LocaleChanged) {
      d->readEnvironment();
      return std::any();
   }
   
   ReadLocker locker(&d->m_lock);
   
   const Locale &m_lcnumeric = d->m_lcnumeric;
   const Locale &m_lctime = d->m_lctime;
   const Locale &m_lcmonetary = d->m_lcmonetary;
   const Locale &m_lcmessages = d->m_lcmessages;
   
   switch (type) {
   case QueryType::DecimalPoint:
      return m_lcnumeric.getDecimalPoint();
   case QueryType::GroupSeparator:
      return m_lcnumeric.getGroupSeparator();
   case QueryType::ZeroDigit:
      return m_lcnumeric.getZeroDigit();
   case QueryType::NegativeSign:
      return m_lcnumeric.getNegativeSign();
   case QueryType::DateFormatLong:
      return m_lctime.getDateFormat(Locale::FormatType::LongFormat);
   case QueryType::DateFormatShort:
      return m_lctime.getDateFormat(Locale::FormatType::ShortFormat);
   case QueryType::TimeFormatLong:
      return m_lctime.getTimeFormat(Locale::FormatType::LongFormat);
   case QueryType::TimeFormatShort:
      return m_lctime.getTimeFormat(Locale::FormatType::ShortFormat);
   case QueryType::DayNameLong:
      return m_lctime.getDayName(std::any_cast<int>(in), Locale::FormatType::LongFormat);
   case QueryType::DayNameShort:
      return m_lctime.getDayName(std::any_cast<int>(in), Locale::FormatType::ShortFormat);
   case QueryType::MonthNameLong:
      return m_lctime.getMonthName(std::any_cast<int>(in), Locale::FormatType::LongFormat);
   case QueryType::MonthNameShort:
      return m_lctime.getMonthName(std::any_cast<int>(in), Locale::FormatType::ShortFormat);
   case QueryType::StandaloneMonthNameLong:
      return m_lctime.getStandaloneMonthName(std::any_cast<int>(in), Locale::FormatType::LongFormat);
   case QueryType::StandaloneMonthNameShort:
      return m_lctime.getStandaloneMonthName(std::any_cast<int>(in), Locale::FormatType::ShortFormat);
   case QueryType::DateToStringLong:
      return m_lctime.toString(std::any_cast<Date>(in), Locale::FormatType::LongFormat);
   case QueryType::DateToStringShort:
      return m_lctime.toString(std::any_cast<Date>(in), Locale::FormatType::ShortFormat);
   case QueryType::TimeToStringLong:
      return m_lctime.toString(std::any_cast<Time>(in), Locale::FormatType::LongFormat);
   case QueryType::TimeToStringShort:
      return m_lctime.toString(std::any_cast<Time>(in), Locale::FormatType::ShortFormat);
   case QueryType::DateTimeFormatLong:
      return m_lctime.getDateTimeFormat(Locale::FormatType::LongFormat);
   case QueryType::DateTimeFormatShort:
      return m_lctime.getDateTimeFormat(Locale::FormatType::ShortFormat);
   case QueryType::DateTimeToStringLong:
      return m_lctime.toString(std::any_cast<DateTime>(in), Locale::FormatType::LongFormat);
   case QueryType::DateTimeToStringShort:
      return m_lctime.toString(std::any_cast<DateTime>(in), Locale::FormatType::ShortFormat);
   case QueryType::PositiveSign:
      return m_lcnumeric.getPositiveSign();
   case QueryType::AMText:
      return m_lctime.getAmText();
   case QueryType::PMText:
      return m_lctime.getPmText();
   case QueryType::FirstDayOfWeek:
      return m_lctime.getFirstDayOfWeek();
   case QueryType::CurrencySymbol:
      return m_lcmonetary.getCurrencySymbol(Locale::CurrencySymbolFormat(std::any_cast<uint>(in)));
   case QueryType::CurrencyToString: {
      size_t ihashcode = in.type().hash_code();
      if (ihashcode == typeid(int).hash_code()) {
         return m_lcmonetary.toCurrencyString(std::any_cast<int>(in));
      } else if (ihashcode == typeid(uint).hash_code()) {
         return m_lcmonetary.toCurrencyString(std::any_cast<uint>(in));
      } else if (ihashcode == typeid(double).hash_code()) {
         return m_lcmonetary.toCurrencyString(std::any_cast<double>(in));
      } else if (ihashcode == typeid(pdk::plonglong).hash_code()) {
         return m_lcmonetary.toCurrencyString(std::any_cast<pdk::plonglong>(in));
      } else if (ihashcode == typeid(pdk::pulonglong).hash_code()) {
         return m_lcmonetary.toCurrencyString(std::any_cast<pdk::pulonglong>(in));
      } else {
         return String();
      }
   }
   case QueryType::MeasurementSystem: {
      const String measLocale = String::fromLatin1(d->m_lcmeasurementVar.getConstRawData(), d->m_lcmeasurementVar.size());
      if (measLocale.compare(Latin1String("Metric"), pdk::CaseSensitivity::Insensitive) == 0)
         return Locale::MeasurementSystem::MetricSystem;
      if (measLocale.compare(Latin1String("Other"), pdk::CaseSensitivity::Insensitive) == 0)
         return Locale::MeasurementSystem::MetricSystem;
      return std::any((int)Locale(measLocale).getMeasurementSystem());
   }
   case QueryType::UILanguages: {
      if (!d->m_uiLanguages.empty())
         return d->m_uiLanguages;
      String languages = String::fromLatin1(pdk::get_env("LANGUAGE"));
      StringList lst;
      if (languages.isEmpty())
         lst.push_back(String::fromLatin1(d->m_lcmessagesVar));
      else
         lst = languages.split(Latin1Character(':'));
      
      for (size_t i = 0; i < lst.size(); ++i) {
         const String &name = lst.at(i);
         String lang, script, cntry;
         if (split_locale_name(name, lang, script, cntry)) {
            if (!cntry.length())
               d->m_uiLanguages.push_back(lang);
            else
               d->m_uiLanguages.push_back(lang % Latin1Character('-') % cntry);
         }
      }
      return d->m_uiLanguages.empty() ? std::any() : std::any(d->m_uiLanguages);
   }
   case QueryType::StringToStandardQuotation:
      return m_lcmessages.quoteString(std::any_cast<StringRef>(in));
   case QueryType::StringToAlternateQuotation:
      return m_lcmessages.quoteString(std::any_cast<StringRef>(in), Locale::QuotationStyle::AlternateQuotation);
   case QueryType::ListToSeparatedString:
      return m_lcmessages.createSeparatedList(std::any_cast<StringList>(in));
   case QueryType::LocaleChanged:
      PDK_ASSERT(false);
   default:
      break;
   }
   return std::any();
}

#endif // PDK_NO_SYSTEMLOCALE

} // internal
} // utils
} // pdk
