// @copyright 2017-2018 zzu_softboy <zzu_softboy@163.com>
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIEstr, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
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

#include "pdk/global/Global.h"
#include "pdk/global/PlatformDefs.h"
#include "pdk/global/GlobalStatic.h"
#include "pdk/global/internal/NumericPrivate.h"
#include "pdk/kernel/HashFuncs.h"
#include "pdk/kernel/Algorithms.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/lang/StringView.h"
#include "pdk/base/lang/StringBuilder.h"
#include "pdk/utils/Locale.h"
#include "pdk/utils/SharedData.h"
#include "pdk/utils/internal/LocalePrivate.h"
#include "pdk/utils/internal/LocaleToolsPrivate.h"
#include "pdk/utils/internal/LocaleDataPrivate.h"
#include "pdk/base/time/Date.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/time/Time.h"
#include "pdk/base/ds/StringList.h"

#include <cmath>

#if defined(PDK_OS_MAC)
#   include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef PDK_OS_WIN
#   include "pdk/global/Windows.h"
#   include <time.h>
#endif

namespace pdk {
namespace utils {

using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::lang::StringView;
using pdk::utils::SharedDataPointer;
using pdk::ds::VarLengthArray;
using pdk::ds::ByteArray;
using internal::LocaleData;
using internal::SystemLocale;
using internal::LocaleId;

namespace internal {

#ifndef PDK_NO_SYSTEMLOCALE

static SystemLocale *sg_systemLocale = nullptr;

class SystemLocaleSingleton : public SystemLocale
{
public:
   SystemLocaleSingleton() : SystemLocale(true) {}
};

PDK_GLOBAL_STATIC(SystemLocaleSingleton, sg_globalSystemLocale);
static LocaleData *sg_systemData = 0;
static LocaleData sg_globalLocaleData;

#endif // PDK_NO_SYSTEMLOCALE

Locale::Language LocalePrivate::codeToLanguage(StringView code) noexcept
{
   const auto len = code.size();
   if (len != 2 && len != 3) {
      return Locale::Language::C;
   }
   ushort uc1 = code[0].toLower().unicode();
   ushort uc2 = code[1].toLower().unicode();
   ushort uc3 = len > 2 ? code[2].toLower().unicode() : 0;
   const unsigned char *dptr = sg_languageCodeList;
   for (; *dptr != 0; dptr += 3) {
      if (uc1 == dptr[0] && uc2 == dptr[1] && uc3 == dptr[2]) {
         return Locale::Language((dptr - sg_languageCodeList)/3);
      }
   }
   // legacy codes
   if (uc1 == 'n' && uc2 == 'o' && uc3 == 0) { // no -> nb
      PDK_STATIC_ASSERT(Locale::Language::Norwegian == Locale::Language::NorwegianBokmal);
      return Locale::Language::Norwegian;
   }
   if (uc1 == 't' && uc2 == 'l' && uc3 == 0) { // tl -> fil
      PDK_STATIC_ASSERT(Locale::Language::Tagalog == Locale::Language::Filipino);
      return Locale::Language::Tagalog;
   }
   if (uc1 == 's' && uc2 == 'h' && uc3 == 0) { // sh -> sr[_Latn]
      PDK_STATIC_ASSERT(Locale::Language::SerboCroatian == Locale::Language::Serbian);
      return Locale::Language::SerboCroatian;
   }
   if (uc1 == 'm' && uc2 == 'o' && uc3 == 0) { // mo -> ro
      PDK_STATIC_ASSERT(Locale::Language::Moldavian == Locale::Language::Romanian);
      return Locale::Language::Moldavian;
   }
   // Android uses the following deprecated codes
   if (uc1 == 'i' && uc2 == 'w' && uc3 == 0) {// iw -> he
      return Locale::Language::Hebrew;
   }
   if (uc1 == 'i' && uc2 == 'n' && uc3 == 0) { // in -> id
      return Locale::Language::Indonesian;
   }
   if (uc1 == 'j' && uc2 == 'i' && uc3 == 0) {// ji -> yi
      return Locale::Language::Yiddish;
   }
   return Locale::Language::C;
}

Locale::Script LocalePrivate::codeToScript(StringView code) noexcept
{
   const auto len = code.size();
   if (len != 4) {
      return Locale::Script::AnyScript;
   }
   // script is titlecased in our data
   unsigned char c0 = code[0].toUpper().toLatin1();
   unsigned char c1 = code[1].toLower().toLatin1();
   unsigned char c2 = code[2].toLower().toLatin1();
   unsigned char c3 = code[3].toLower().toLatin1();
   const unsigned char *dptr = sg_scriptCodeList;
   int lastScript = static_cast<int>(Locale::Script::LastScript);
   for (int i = 0; i < lastScript; ++i, dptr += 4) {
      if (c0 == dptr[0] && c1 == dptr[1] && c2 == dptr[2] && c3 == dptr[3]) {
         return Locale::Script(i);
      }
   }
   return Locale::Script::AnyScript;
}

Locale::Country LocalePrivate::codeToCountry(StringView code) noexcept
{
   const auto len = code.size();
   if (len != 2 && len != 3) {
      return Locale::Country::AnyCountry;
   }
   ushort uc1 = code[0].toUpper().unicode();
   ushort uc2 = code[1].toUpper().unicode();
   ushort uc3 = len > 2 ? code[2].toUpper().unicode() : 0;
   const unsigned char *dptr = sg_countryCodeList;
   for (; *dptr != 0; dptr += 3) {
      if (uc1 == dptr[0] && uc2 == dptr[1] && uc3 == dptr[2]) {
         return Locale::Country((dptr - sg_countryCodeList)/3);
      }
   }
   return Locale::Country::AnyCountry;
}

Latin1String LocalePrivate::languageToCode(Locale::Language language)
{
   if (language == Locale::Language::AnyLanguage) {
      return Latin1String();
   }
   if (language == Locale::Language::C) {
      return Latin1String("C");
   }
   const unsigned char *dptr = sg_languageCodeList + 3 * pdk::as_integer<Locale::Language>(language);
   return Latin1String(reinterpret_cast<const char *>(dptr), dptr[2] == 0 ? 2 : 3);
}

Latin1String LocalePrivate::scriptToCode(Locale::Script script)
{
   if (script == Locale::Script::AnyScript || script > Locale::Script::LastScript) {
      return Latin1String();
   }
   const unsigned char *dptr = sg_scriptCodeList + 4 * pdk::as_integer<Locale::Script>(script);
   return Latin1String(reinterpret_cast<const char *>(dptr), 4);
}

Latin1String LocalePrivate::countryToCode(Locale::Country country)
{
   if (country == Locale::Country::AnyCountry) {
      return Latin1String();
   }
   const unsigned char *dptr = sg_countryCodeList + 3 * pdk::as_integer<Locale::Country>(country);
   return Latin1String(reinterpret_cast<const char*>(dptr), dptr[2] == 0 ? 2 : 3);
}

namespace {
// http://www.unicode.org/reports/tr35/#Likely_Subtags
bool add_likely_subtags(LocaleId &localeId)
{
   // ### optimize with bsearch
   const int likelySubtagsCount = sizeof(sg_likelySubtags) / sizeof(sg_likelySubtags[0]);
   const LocaleId *p = sg_likelySubtags;
   const LocaleId *const e = p + likelySubtagsCount;
   for ( ; p < e; p += 2) {
      if (localeId == p[0]) {
         localeId = p[1];
         return true;
      }
   }
   return false;
}
} // anonymous namespace

LocaleId LocaleId::withLikelySubtagsAdded() const
{
   // language_script_region
   if (m_languageId || m_scriptId || m_countryId) {
      LocaleId id = LocaleId::fromIds(m_languageId, m_scriptId, m_countryId);
      if (add_likely_subtags(id)) {
         return id;
      }
   }
   // language_script
   if (m_countryId) {
      LocaleId id = LocaleId::fromIds(m_languageId, m_scriptId, 0);
      if (add_likely_subtags(id)) {
         id.m_countryId = m_countryId;
         return id;
      }
   }
   // language_region
   if (m_scriptId) {
      LocaleId id = LocaleId::fromIds(m_languageId, 0, m_countryId);
      if (add_likely_subtags(id)) {
         id.m_scriptId = m_scriptId;
         return id;
      }
   }
   // language
   if (m_scriptId && m_countryId) {
      LocaleId id = LocaleId::fromIds(m_languageId, 0, 0);
      if (add_likely_subtags(id)) {
         id.m_scriptId = m_scriptId;
         id.m_countryId = m_countryId;
         return id;
      }
   }
   return *this;
}

LocaleId LocaleId::withLikelySubtagsRemoved() const
{
   LocaleId max = withLikelySubtagsAdded();
   // language
   {
      LocaleId id = LocaleId::fromIds(m_languageId, 0, 0);
      if (id.withLikelySubtagsAdded() == max)
         return id;
   }
   // language_region
   if (m_countryId) {
      LocaleId id = LocaleId::fromIds(m_languageId, 0, m_countryId);
      if (id.withLikelySubtagsAdded() == max)
         return id;
   }
   // language_script
   if (m_scriptId) {
      LocaleId id = LocaleId::fromIds(m_languageId, m_scriptId, 0);
      if (id.withLikelySubtagsAdded() == max)
         return id;
   }
   return max;
}

ByteArray LocaleId::name(char separator) const
{
   if (m_languageId == pdk::as_integer<Locale::Language>(Locale::Language::AnyLanguage)) {
      return ByteArray();
   }
   if (m_languageId == pdk::as_integer<Locale::Language>(Locale::Language::C)) {
      return ByteArrayLiteral("C");
   }
   const unsigned char *lang = sg_languageCodeList + 3 * m_languageId;
   const unsigned char *script =
         (m_scriptId != pdk::as_integer<Locale::Script>( Locale::Script::AnyScript) ? sg_scriptCodeList + 4 * m_scriptId : 0);
   const unsigned char *country =
         (m_countryId != pdk::as_integer<Locale::Country>(Locale::Country::AnyCountry) ? sg_countryCodeList + 3 * m_countryId : 0);
   char len = (lang[2] != 0 ? 3 : 2) + (script ? 4+1 : 0) + (country ? (country[2] != 0 ? 3 : 2)+1 : 0);
   ByteArray name(len, pdk::Uninitialized);
   char *uc = name.getRawData();
   *uc++ = lang[0];
   *uc++ = lang[1];
   if (lang[2] != 0) {
      *uc++ = lang[2];
   }
   if (script) {
      *uc++ = separator;
      *uc++ = script[0];
      *uc++ = script[1];
      *uc++ = script[2];
      *uc++ = script[3];
   }
   if (country) {
      *uc++ = separator;
      *uc++ = country[0];
      *uc++ = country[1];
      if (country[2] != 0) {
         *uc++ = country[2];
      }
   }
   return name;
}

ByteArray LocalePrivate::bcp47Name(char separator) const
{
   if (m_data->m_languageId == pdk::as_integer<Locale::Language>(Locale::Language::AnyLanguage)) {
      return ByteArray();
   }
   if (m_data->m_languageId == pdk::as_integer<Locale::Language>(Locale::Language::C)) {
      return ByteArrayLiteral("en");
   }
   LocaleId localeId = LocaleId::fromIds(m_data->m_languageId, m_data->m_scriptId, m_data->m_countryId);
   return localeId.withLikelySubtagsRemoved().name(separator);
}

namespace {

const LocaleData *find_locale_data_by_id(const LocaleId &localeId)
{
   const uint idx = sg_localeIndex[localeId.m_languageId];
   const LocaleData *data = sg_localeData + idx;
   if (idx == 0) { // default language has no associated script or country
      return data;
   }
   PDK_ASSERT(data->m_languageId == localeId.m_languageId);
   if (localeId.m_scriptId == pdk::as_integer<Locale::Script>(Locale::Script::AnyScript) && 
       localeId.m_countryId == pdk::as_integer<Locale::Country>(Locale::Country::AnyCountry)) {
      return data;
   }
   if (localeId.m_scriptId == pdk::as_integer<Locale::Script>(Locale::Script::AnyScript)) {
      do {
         if (data->m_countryId == localeId.m_countryId) {
            return data;
         }
         ++data;
      } while (data->m_languageId && data->m_languageId == localeId.m_languageId);
   } else if (localeId.m_countryId == pdk::as_integer<Locale::Country>(Locale::Country::AnyCountry)) {
      do {
         if (data->m_scriptId == localeId.m_scriptId) {
            return data;
         }
         ++data;
      } while (data->m_languageId && data->m_languageId == localeId.m_languageId);
   } else {
      do {
         if (data->m_scriptId == localeId.m_scriptId && data->m_countryId == localeId.m_countryId) {
            return data;
         }
         ++data;
      } while (data->m_languageId && data->m_languageId == localeId.m_languageId);
   }
   return 0;
}

} // anonymous namespace

const LocaleData *LocaleData::findLocaleData(Locale::Language language, Locale::Script script, Locale::Country country)
{
   ushort languageId = pdk::as_integer<Locale::Language>(language);
   ushort scriptId = pdk::as_integer<Locale::Script>(script);
   ushort countryId = pdk::as_integer<Locale::Country>(country);
   ushort anyScriptId = pdk::as_integer<Locale::Script>(Locale::Script::AnyScript);
   ushort anyCountryId = pdk::as_integer<Locale::Country>(Locale::Country::AnyCountry);
   
   LocaleId localeId = LocaleId::fromIds(languageId, scriptId, countryId);
   localeId = localeId.withLikelySubtagsAdded();
   const uint idx = sg_localeIndex[localeId.m_languageId];
   // Try a straight match
   if (const LocaleData *const data = find_locale_data_by_id(localeId)) {
      return data;
   }
   std::list<LocaleId> tried;
   tried.push_back(localeId);
   // No match; try again with likely country
   localeId = LocaleId::fromIds(languageId, scriptId, anyCountryId);
   localeId = localeId.withLikelySubtagsAdded();
   
   auto end = tried.end();
   auto iter = std::find(tried.begin(), tried.end(), localeId);
   if (iter == end) {
      if (const LocaleData *const data = find_locale_data_by_id(localeId)) {
         return data;
      }
      tried.push_back(localeId);
   }
   // No match; try again with any country
   localeId = LocaleId::fromIds(languageId, scriptId, anyCountryId);
   iter = std::find(tried.begin(), tried.end(), localeId);
   
   if (iter == end) {
      if (const LocaleData *const data = find_locale_data_by_id(localeId)) {
         return data;
      }
      tried.push_back(localeId);
   }
   // No match; try again with likely script
   localeId = LocaleId::fromIds(languageId, anyScriptId, countryId);
   localeId = localeId.withLikelySubtagsAdded();
   iter = std::find(tried.begin(), tried.end(), localeId);
   if (iter == end) {
      if (const LocaleData *const data = find_locale_data_by_id(localeId)) {
         return data;
      }
      tried.push_back(localeId);
   }
   // No match; try again with any script
   localeId = LocaleId::fromIds(languageId, anyScriptId, countryId);
   iter = std::find(tried.begin(), tried.end(), localeId);
   if (iter == end) {
      if (const LocaleData *const data = find_locale_data_by_id(localeId)) {
         return data;
      }
      tried.push_back(localeId);
   }
   // No match; return data at original index
   return sg_localeData + idx;
}

namespace {

bool parse_locale_tag(const String &input, int &i, String *result, const String &separators)
{
   *result = String(8, pdk::Uninitialized); // worst case according to BCP47
   Character *pch = result->getRawData();
   const Character *uc = input.getRawData() + i;
   const int l = input.length();
   int size = 0;
   for (; i < l && size < 8; ++i, ++size) {
      if (separators.contains(*uc)) {
         break;
      }
      if (! ((uc->unicode() >= 'a' && uc->unicode() <= 'z') ||
             (uc->unicode() >= 'A' && uc->unicode() <= 'Z') ||
             (uc->unicode() >= '0' && uc->unicode() <= '9')) ) {// latin only
         return false;
      }
      *pch++ = *uc++;
   }
   result->truncate(size);
   return true;
}

} // anonymous namespace

bool split_locale_name(const String &name, String &lang, String &script, String &cntry)
{
   const int length = name.length();
   lang = script = cntry = String(); 
   const String separators = StringLiteral("_-.@");
   enum ParserState { NoState, LangState, ScriptState, CountryState};
   ParserState state = LangState;
   for (int i = 0; i < length && state != NoState; ) {
      String value;
      if (!parse_locale_tag(name, i, &value, separators) ||value.isEmpty()) {
         break;
      } 
      Character sep = i < length ? name.at(i) : Character();
      switch (state) {
      case LangState:
         if (!sep.isNull() && !separators.contains(sep)) {
            state = NoState;
            break;
         }
         lang = value;
         if (i == length) {
            // just language was specified
            state = NoState;
            break;
         }
         state = ScriptState;
         break;
      case ScriptState: {
         String scripts = String::fromLatin1(reinterpret_cast<const char *>(sg_scriptCodeList), sizeof(sg_scriptCodeList) - 1);
         if (value.length() == 4 && scripts.indexOf(value) % 4 == 0) {
            // script name is always 4 characters
            script = value;
            state = CountryState;
         } else {
            // it wasn't a script, maybe it is a country then?
            cntry = value;
            state = NoState;
         }
         break;
      }
      case CountryState:
         cntry = value;
         state = NoState;
         break;
      case NoState:
         // shouldn't happen
         //warning_stream("Locale: This should never happen");
         break;
      }
      ++i;
   }
   return lang.length() == 2 || lang.length() == 3;
}

void LocalePrivate::getLangAndCountry(const String &name, Locale::Language &lang,
                                      Locale::Script &script, Locale::Country &cntry)
{
   lang = Locale::Language::C;
   script = Locale::Script::AnyScript;
   cntry = Locale::Country::AnyCountry;
   
   String lang_code;
   String script_code;
   String cntry_code;
   if (!internal::split_locale_name(name, lang_code, script_code, cntry_code))
      return;
   
   lang = LocalePrivate::codeToLanguage(lang_code);
   if (lang == Locale::Language::C)
      return;
   script = LocalePrivate::codeToScript(script_code);
   cntry = LocalePrivate::codeToCountry(cntry_code);
}

namespace {

const LocaleData *find_locale_data(const String &name)
{
   Locale::Language lang;
   Locale::Script script;
   Locale::Country cntry;
   LocalePrivate::getLangAndCountry(name, lang, script, cntry);
   return LocaleData::findLocaleData(lang, script, cntry);
}

} // anonymous namespace

String read_escaped_format_string(StringView format, int *idx)
{
   int &i = *idx;
   int formatSize = format.size();
   PDK_ASSERT(format.at(i) == Latin1Character('\''));
   ++i;
   if (i == formatSize) {
      return String();
   }
   if (format.at(i).unicode() == '\'') { // "''" outside of a quoted stirng
      ++i;
      return Latin1String("'");
   }
   String result;
   while (i < formatSize) {
      if (format.at(i).unicode() == '\'') {
         if (i + 1 < formatSize && format.at(i + 1).unicode() == '\'') {
            // "''" inside of a quoted string
            result.append(Latin1Character('\''));
            i += 2;
         } else {
            break;
         }
      } else {
         result.append(format.at(i++));
      }
   }
   if (i < formatSize) {
      ++i;
   }
   return result;
}

int repeat_count(StringView s)
{
   if (s.isEmpty()) {
      return 0;
   }
   const Character c = s.front();
   pdk::sizetype j = 1;
   while (j < s.size() && s.at(j) == c) {
      ++j;
   }
   return static_cast<int>(j);
}

namespace {

static const LocaleData *sg_defaultData = 0;
static Locale::NumberOptions sg_defaultNumberOptions = Locale::NumberOption::DefaultNumberOptions;

static const LocaleData *const sg_cdata = sg_localeData;
static LocalePrivate *c_private()
{
   static LocalePrivate c_locale = { sg_cdata, PDK_BASIC_ATOMIC_INITIALIZER(1), Locale::NumberOption::OmitGroupSeparator };
   return &c_locale;
}

} // anonymous namespace

#ifndef PDK_NO_SYSTEMLOCALE

SystemLocale::SystemLocale()
{
   sg_systemLocale = this;
   
   if (sg_systemData){
      sg_systemData->m_languageId = 0;
   }
}

SystemLocale::SystemLocale(bool)
{}

/*!
  Deletes the object.
*/
SystemLocale::~SystemLocale()
{
   if (sg_systemLocale == this) {
      sg_systemLocale = 0;
      if (sg_systemData)
         sg_systemData->m_languageId = 0;
   }
}

namespace {
const SystemLocale *system_locale()
{
   if (sg_systemLocale) {
      return sg_systemLocale;
   }
   return sg_globalSystemLocale();
}
} // anonymous namespace

void LocalePrivate::updateSystemPrivate()
{
   const SystemLocale *sys_locale = system_locale();
   if (!sg_systemData) {
      sg_systemData = &sg_globalLocaleData;
   }
   // tell the object that the system locale has changed.
   sys_locale->query(SystemLocale::QueryType::LocaleChanged, std::any());
   *sg_systemData = *sys_locale->fallbackUiLocale().m_implPtr->m_data;
   std::any res = sys_locale->query(SystemLocale::QueryType::LanguageId, std::any());
   if (res.has_value()) {
      sg_systemData->m_languageId = std::any_cast<int>(res);
      sg_systemData->m_scriptId = pdk::as_integer<Locale::Script>(Locale::Script::AnyScript); // default for compatibility
   }
   res = sys_locale->query(SystemLocale::QueryType::CountryId, std::any());
   if (res.has_value()) {
      sg_systemData->m_countryId = std::any_cast<int>(res);
      sg_systemData->m_scriptId = pdk::as_integer<Locale::Script>(Locale::Script::AnyScript); // default for compatibility
   }
   res = sys_locale->query(SystemLocale::QueryType::ScriptId, std::any());
   if (res.has_value()) {
      sg_systemData->m_scriptId = std::any_cast<int>(res);
   }
   res = sys_locale->query(SystemLocale::QueryType::DecimalPoint, std::any());
   if (res.has_value()) {
      sg_systemData->m_decimal = std::any_cast<String>(res).at(0).unicode();
   }
   res = sys_locale->query(SystemLocale::QueryType::GroupSeparator, std::any());
   if (res.has_value()) {
      sg_systemData->m_group = std::any_cast<String>(res).at(0).unicode();
   }
   res = sys_locale->query(SystemLocale::QueryType::ZeroDigit, std::any());
   if (res.has_value()) {
      sg_systemData->m_zero = std::any_cast<String>(res).at(0).unicode();
   }
   res = sys_locale->query(SystemLocale::QueryType::NegativeSign, std::any());
   if (res.has_value()) {
      sg_systemData->m_minus = std::any_cast<String>(res).at(0).unicode();
   }
   res = sys_locale->query(SystemLocale::QueryType::PositiveSign, std::any());
   if (res.has_value()) {
      sg_systemData->m_plus = std::any_cast<String>(res).at(0).unicode();
   }   
}

#endif // PDK_NO_SYSTEMLOCALE

const LocaleData *LocaleData::c()
{
   PDK_ASSERT(sg_localeIndex[pdk::as_integer<Locale::Language>(Locale::Language::C)] == 0);
   return sg_cdata;
}

namespace {
const LocaleData *default_data();
} // anonymous

static const int sg_localeDataSize = sizeof(sg_localeData) / sizeof(LocaleData) - 1;

PDK_GLOBAL_STATIC_WITH_ARGS(SharedDataPointer<LocalePrivate>, sg_defaultLocalePrivate,
                            (LocalePrivate::create(default_data(), sg_defaultNumberOptions)));

namespace {

const LocaleData *system_data()
{
#ifndef PDK_NO_SYSTEMLOCALE
   // copy over the information from the fallback locale and modify
   if (!sg_systemData || sg_systemData->m_languageId == 0) {
      LocalePrivate::updateSystemPrivate();
   }
   return sg_systemData;
#else
   return sg_localeData;
#endif
}

const LocaleData *default_data()
{
   if (!sg_defaultData) {
      sg_defaultData = system_data();
   }
   return sg_defaultData;
}

inline String get_locale_data(const ushort *data, int size)
{
   return size > 0 ? String::fromRawData(reinterpret_cast<const Character *>(data), size) : String();
}

String get_locale_list_data(const ushort *data, int size, int index)
{
   static const ushort separator = ';';
   while (index && size > 0) {
      while (*data != separator) {
         ++data, --size;
      }
      --index;
      ++data;
      --size;
   }
   const ushort *end = data;
   while (size > 0 && *end != separator){
      ++end;
      --size;
   }
   return get_locale_data(data, end - data);
}

LocalePrivate *locale_private_by_name(const String &name)
{
   if (name == Latin1String("C")) {
      return c_private();
   }  
   const LocaleData *data = find_locale_data(name);
   return LocalePrivate::create(data, data->m_languageId == pdk::as_integer<Locale::Language>(Locale::Language::C) ?
                                   Locale::NumberOption::OmitGroupSeparator : Locale::NumberOption::DefaultNumberOptions);
}

LocalePrivate *find_locale_private(Locale::Language language, Locale::Script script,
                                   Locale::Country country)
{
   if (language == Locale::Language::C) {
      return c_private();
   }
   const LocaleData *data = LocaleData::findLocaleData(language, script, country);
   Locale::NumberOptions numberOptions = Locale::NumberOption::DefaultNumberOptions;
   // If not found, should default to system
   const ushort languageId = pdk::as_integer<Locale::Language>(Locale::Language::C);
   if (data->m_languageId == languageId && pdk::as_integer<Locale::Language>(language) != languageId) {
      numberOptions = sg_defaultNumberOptions;
      data = default_data();
   }
   return LocalePrivate::create(data, numberOptions);
}

} // anonymous namespace


} // internal

Locale::Locale(LocalePrivate &dd)
   : m_implPtr(&dd)
{}

Locale::Locale(const String &name)
   : m_implPtr(internal::locale_private_by_name(name))
{
}

Locale::Locale()
   : m_implPtr(*internal::sg_defaultLocalePrivate)
{
}

Locale::Locale(Language language, Country country)
   : m_implPtr(internal::find_locale_private(language, Locale::Script::AnyScript, country))
{
}

Locale::Locale(Language language, Script script, Country country)
   : m_implPtr(internal::find_locale_private(language, script, country))
{
}

Locale::Locale(const Locale &other)
{
   m_implPtr = other.m_implPtr;
}

Locale::~Locale()
{
}

Locale &Locale::operator=(const Locale &other)
{
   m_implPtr = other.m_implPtr;
   return *this;
}

bool Locale::operator==(const Locale &other) const
{
   return m_implPtr->m_data == other.m_implPtr->m_data && 
         m_implPtr->m_numberOptions.getUnderData() == other.m_implPtr->m_numberOptions.getUnderData();
}

bool Locale::operator!=(const Locale &other) const
{
   return m_implPtr->m_data != other.m_implPtr->m_data || 
         m_implPtr->m_numberOptions.getUnderData() != other.m_implPtr->m_numberOptions.getUnderData();
}

// @TODO review locale hash
//uint hash(const Locale &key, uint seed) noexcept
//{
//    ::pdk::internal::HashCombine chash;
//    seed = chash(key.m_implPtr->m_data, seed);
//    seed = chash(key.m_implPtr->m_numberOptionstr, seed);
//    return seed;
//}

void Locale::setNumberOptions(NumberOptions options)
{
   m_implPtr->m_numberOptions = options;
}

Locale::NumberOptions Locale::numberOptions() const
{
   return static_cast<NumberOptions>(m_implPtr->m_numberOptions);
}

String Locale::quoteString(const String &str, QuotationStyle style) const
{
   return quoteString(StringRef(&str), style);
}


String Locale::quoteString(const StringRef &str, QuotationStyle style) const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res;
      if (style == Locale::QuotationStyle::AlternateQuotation) {
         res = internal::system_locale()->query(SystemLocale::QueryType::StringToAlternateQuotation, std::any(str));
      }
      if (!res.has_value() || style == Locale::QuotationStyle::StandardQuotation) {
         res = internal::system_locale()->query(SystemLocale::QueryType::StringToStandardQuotation, std::any(str));
      }
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   if (style == Locale::QuotationStyle::StandardQuotation) {
      return Character(m_implPtr->m_data->m_quotationStart) % str % Character(m_implPtr->m_data->m_quotationEnd);
   } else {
      return Character(m_implPtr->m_data->m_alternateQuotationStart) % str % Character(m_implPtr->m_data->m_alternateQuotationEnd);
   }
}

String Locale::createSeparatedList(const StringList &list) const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res;
      res = internal::system_locale()->query(SystemLocale::QueryType::ListToSeparatedString, std::any(list));
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   const int size = list.size();
   if (size == 1) {
      return list.at(0);
   } else if (size == 2) {
      String format = internal::get_locale_data(internal::sg_listPatternPartData + m_implPtr->m_data->m_listPatternPartTwoIdx, 
                                                m_implPtr->m_data->m_listPatternPartTwoSize);
      return format.arg(list.at(0), list.at(1));
   } else if (size > 2) {
      String formatStart = internal::get_locale_data(internal::sg_listPatternPartData + m_implPtr->m_data->m_listPatternPartStartIdx,
                                                     m_implPtr->m_data->m_listPatternPartStartSize);
      String formatMid = internal::get_locale_data(internal::sg_listPatternPartData + m_implPtr->m_data->m_listPatternPartMidIdx, 
                                                   m_implPtr->m_data->m_listPatternPartMidSize);
      String formatEnd = internal::get_locale_data(internal::sg_listPatternPartData + m_implPtr->m_data->m_listPatternPartEndIdx, 
                                                   m_implPtr->m_data->m_listPatternPartEndSize);
      String result = formatStart.arg(list.at(0), list.at(1));
      for (int i = 2; i < size - 1; ++i)
         result = formatMid.arg(result, list.at(i));
      result = formatEnd.arg(result, list.at(size - 1));
      return result;
   }
   
   return String();
}

void Locale::setDefault(const Locale &locale)
{
   internal::sg_defaultData = locale.m_implPtr->m_data;
   internal::sg_defaultNumberOptions = locale.numberOptions();
   if (internal::sg_defaultLocalePrivate.exists()) {
      // update the cached private
      *internal::sg_defaultLocalePrivate = locale.m_implPtr;
   }
}

Locale::Language Locale::getLanguage() const
{
   return Language(m_implPtr->languageId());
}

Locale::Script Locale::getScript() const
{
   return Script(m_implPtr->m_data->m_scriptId);
}

Locale::Country Locale::getCountry() const
{
   return Country(m_implPtr->countryId());
}

String Locale::getName() const
{
   Language l = getLanguage();
   if (l == Language::C) {
      return m_implPtr->languageCode();
   }
   Country c = getCountry();
   if (c == Country::AnyCountry) {
      return m_implPtr->languageCode();
   }
   return m_implPtr->languageCode() + Latin1Character('_') + m_implPtr->countryCode();
}

namespace {

pdk::plonglong to_integral_helper(const LocaleData *d, StringView str, bool *ok,
                                  Locale::NumberOptions mode, pdk::plonglong)
{
   return d->stringToLongLong(str, 10, ok, mode);
}

pdk::pulonglong to_integral_helper(const LocaleData *d, StringView str, bool *ok,
                                   Locale::NumberOptions mode, pdk::pulonglong)
{
   return d->stringToUnsLongLong(str, 10, ok, mode);
}

template <typename T> static inline
T to_integral_helper(const LocalePrivate *d, StringView str, bool *ok)
{
   using Int64 = typename std::conditional<std::is_unsigned<T>::value, pdk::pulonglong, pdk::plonglong>::type;
   // we select the right overload by the last, unused parameter
   Int64 val = to_integral_helper(d->m_data, str, ok, d->m_numberOptions, Int64());
   if (T(val) != val) {
      if (ok) {
         *ok = false;
      }
      val = 0;
   }
   return T(val);
}

} // anonymous namespace

String Locale::getBcp47Name() const
{
   return String::fromLatin1(m_implPtr->bcp47Name());
}

String Locale::languageToString(Language language)
{
   if (uint(language) > uint(Locale::Language::LastLanguage)) {
      return Latin1String("Unknown");
   }
   return Latin1String(internal::sg_languageNameList + 
                       internal::sg_languageNameIndex[pdk::as_integer<Locale::Language>(language)]);
}

String Locale::countryToString(Country country)
{
   if (uint(country) > uint(Locale::Country::LastCountry)) {
      return Latin1String("Unknown");
   }
   return Latin1String(internal::sg_countryNameList + 
                       internal::sg_countryNameIndex[pdk::as_integer<Locale::Country>(country)]);
}

String Locale::scriptToString(Locale::Script script)
{
   if (uint(script) > uint(Locale::Script::LastScript)) {
      return Latin1String("Unknown");
   }
   return Latin1String(internal::sg_scriptNameList + 
                       internal::sg_scriptNameIndex[pdk::as_integer<Locale::Script>(script)]);
}

#if PDK_STRINGVIEW_LEVEL < 2

short Locale::toShort(const String &str, bool *ok) const
{
   return to_integral_helper<short>(m_implPtr, str, ok);
}

ushort Locale::toUShort(const String &str, bool *ok) const
{
   return to_integral_helper<ushort>(m_implPtr, str, ok);
}

int Locale::toInt(const String &str, bool *ok) const
{
   return to_integral_helper<int>(m_implPtr, str, ok);
}

uint Locale::toUInt(const String &str, bool *ok) const
{
   return to_integral_helper<uint>(m_implPtr, str, ok);
}

pdk::plonglong Locale::toLongLong(const String &str, bool *ok) const
{
   return to_integral_helper<pdk::plonglong>(m_implPtr, str, ok);
}

pdk::pulonglong Locale::toULongLong(const String &str, bool *ok) const
{
   return to_integral_helper<pdk::pulonglong>(m_implPtr, str, ok);
}

float Locale::toFloat(const String &str, bool *ok) const
{
   return LocaleData::convertDoubleToFloat(toDouble(str, ok), ok);
}

double Locale::toDouble(const String &str, bool *ok) const
{
   return m_implPtr->m_data->stringToDouble(str, ok, m_implPtr->m_numberOptions);
}

short Locale::toShort(const StringRef &str, bool *ok) const
{
   return to_integral_helper<short>(m_implPtr, str, ok);
}

ushort Locale::toUShort(const StringRef &str, bool *ok) const
{
   return to_integral_helper<ushort>(m_implPtr, str, ok);
}

int Locale::toInt(const StringRef &str, bool *ok) const
{
   return to_integral_helper<int>(m_implPtr, str, ok);
}

uint Locale::toUInt(const StringRef &str, bool *ok) const
{
   return to_integral_helper<uint>(m_implPtr, str, ok);
}

pdk::plonglong Locale::toLongLong(const StringRef &str, bool *ok) const
{
   return to_integral_helper<pdk::plonglong>(m_implPtr, str, ok);
}

pdk::pulonglong Locale::toULongLong(const StringRef &str, bool *ok) const
{
   return to_integral_helper<pdk::pulonglong>(m_implPtr, str, ok);
}

float Locale::toFloat(const StringRef &str, bool *ok) const
{
   return LocaleData::convertDoubleToFloat(toDouble(str, ok), ok);
}

double Locale::toDouble(const StringRef &str, bool *ok) const
{
   return m_implPtr->m_data->stringToDouble(str, ok, m_implPtr->m_numberOptions);
}
#endif // QT_STRINGVIEW_LEVEL < 2

short Locale::toShort(StringView str, bool *ok) const
{
   return to_integral_helper<short>(m_implPtr, str, ok);
}

ushort Locale::toUShort(StringView str, bool *ok) const
{
   return to_integral_helper<ushort>(m_implPtr, str, ok);
}

int Locale::toInt(StringView str, bool *ok) const
{
   return to_integral_helper<int>(m_implPtr, str, ok);
}

uint Locale::toUInt(StringView str, bool *ok) const
{
   return to_integral_helper<uint>(m_implPtr, str, ok);
}

pdk::plonglong Locale::toLongLong(StringView str, bool *ok) const
{
   return to_integral_helper<pdk::plonglong>(m_implPtr, str, ok);
}

pdk::pulonglong Locale::toULongLong(StringView str, bool *ok) const
{
   return to_integral_helper<pdk::pulonglong>(m_implPtr, str, ok);
}

float Locale::toFloat(StringView str, bool *ok) const
{
   return LocaleData::convertDoubleToFloat(toDouble(str, ok), ok);
}

double Locale::toDouble(StringView str, bool *ok) const
{
   return m_implPtr->m_data->stringToDouble(str, ok, m_implPtr->m_numberOptions);
}

String Locale::toString(pdk::plonglong i) const
{
   unsigned flags = pdk::as_integer<LocaleData::Flags>
         (m_implPtr->m_numberOptions & NumberOption::OmitGroupSeparator
          ? LocaleData::Flags::NoFlags
          : LocaleData::Flags::ThousandsGroup);
   
   return m_implPtr->m_data->longLongToString(i, -1, 10, -1, flags);
}

String Locale::toString(pdk::pulonglong i) const
{
   int flags = pdk::as_integer<LocaleData::Flags>
         (m_implPtr->m_numberOptions & NumberOption::OmitGroupSeparator
          ? LocaleData::Flags::NoFlags
          : LocaleData::Flags::ThousandsGroup);
   
   return m_implPtr->m_data->unsLongLongToString(i, -1, 10, -1, flags);
}

#if PDK_STRINGVIEW_LEVEL < 2

String Locale::toString(const Date &date, const String &format) const
{
   return m_implPtr->dateTimeToString(format, DateTime(), date, Time(), this);
}

#endif

String Locale::toString(const Date &date, StringView format) const
{
   return m_implPtr->dateTimeToString(format, DateTime(), date, Time(), this);
}

String Locale::toString(const Date &date, FormatType format) const
{
   if (!date.isValid()) {
      return String();
   }
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(format == FormatType::LongFormat
                                                      ? SystemLocale::QueryType::DateToStringLong 
                                                      : SystemLocale::QueryType::DateToStringShort,
                                                      date);
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   String formatStr = dateFormat(format);
   return toString(date, formatStr);
}

namespace {

bool time_format_containsAP(StringView format)
{
   int i = 0;
   while (static_cast<StringView::size_type>(i) < format.size()) {
      if (format.at(i).unicode() == '\'') {
         internal::read_escaped_format_string(format, &i);
         continue;
      }
      if (format.at(i).toLower().unicode() == 'a') {
         return true;
      }
      ++i;
   }
   return false;
}

} // anonymous namespace

#if PDK_STRINGVIEW_LEVEL < 2

String Locale::toString(const Time &time, const String &format) const
{
   return m_implPtr->dateTimeToString(format, DateTime(), Date(), time, this);
}

#endif

String Locale::toString(const Time &time, StringView format) const
{
   return m_implPtr->dateTimeToString(format, DateTime(), Date(), time, this);
}

#if QT_STRINGVIEW_LEVEL < 2

String Locale::toString(const DateTime &dateTime, const String &format) const
{
   return m_implPtr->dateTimeToString(format, dateTime, Date(), Time(), this);
}

#endif

String Locale::toString(const DateTime &dateTime, StringView format) const
{
   return m_implPtr->dateTimeToString(format, dateTime, Date(), Time(), this);
}

String Locale::toString(const DateTime &dateTime, FormatType format) const
{
   if (!dateTime.isValid()) {
      return String();
   }
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(format == FormatType::LongFormat
                                                      ? SystemLocale::QueryType::DateTimeToStringLong
                                                      : SystemLocale::QueryType::DateTimeToStringShort,
                                                      dateTime);
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   
   const String formatStr = dateTimeFormat(format);
   return toString(dateTime, formatStr);
}

String Locale::toString(const Time &time, FormatType format) const
{
   if (!time.isValid())
      return String();
   
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(format == FormatType::LongFormat
                                                      ? SystemLocale::QueryType::TimeToStringLong 
                                                      : SystemLocale::QueryType::TimeToStringShort,
                                                      time);
      if (res.has_value()) {
         std::any_cast<String>(res);
      }
   }
#endif
   
   String format_str = timeFormat(format);
   return toString(time, format_str);
}

String Locale::dateFormat(FormatType format) const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(format == FormatType::LongFormat
                                                      ? SystemLocale::QueryType::DateFormatLong 
                                                      : SystemLocale::QueryType::DateFormatShort,
                                                      std::any());
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   pdk::puint32 idx;
   pdk::puint32 size;
   switch (format) {
   case FormatType::LongFormat:
      idx = m_implPtr->m_data->m_longDateFormatIdx;
      size = m_implPtr->m_data->m_longDateFormatSize;
      break;
   default:
      idx = m_implPtr->m_data->m_shortDateFormatIdx;
      size = m_implPtr->m_data->m_shortDateFormatSize;
      break;
   }
   return internal::get_locale_data(internal::sg_dateFormatData + idx, size);
}

String Locale::timeFormat(FormatType format) const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(format == FormatType::LongFormat
                                                      ? SystemLocale::QueryType::TimeFormatLong 
                                                      : SystemLocale::QueryType::TimeFormatShort,
                                                      std::any());
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   
   pdk::puint32 idx;
   pdk::puint32 size;
   switch (format) {
   case FormatType::LongFormat:
      idx = m_implPtr->m_data->m_longTimeFormatIdx;
      size = m_implPtr->m_data->m_longTimeFormatSize;
      break;
   default:
      idx = m_implPtr->m_data->m_shortTimeFormatIdx;
      size = m_implPtr->m_data->m_shortTimeFormatSize;
      break;
   }
   return internal::get_locale_data(internal::sg_timeFormatData + idx, size);
}

String Locale::dateTimeFormat(FormatType format) const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(format == FormatType::LongFormat
                                                      ? SystemLocale::QueryType::DateTimeFormatLong
                                                      : SystemLocale::QueryType::DateTimeFormatShort,
                                                      std::any());
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   return dateFormat(format) + Latin1Character(' ') + timeFormat(format);
}

#ifndef PDK_NO_DATESTRING
Time Locale::toTime(const String &string, FormatType format) const
{
   return toTime(string, timeFormat(format));
}

Date Locale::toDate(const String &string, FormatType format) const
{
   return toDate(string, dateFormat(format));
}

DateTime Locale::toDateTime(const String &string, FormatType format) const
{
   return toDateTime(string, dateTimeFormat(format));
}

Time Locale::toTime(const String &string, const String &format) const
{
   Time time;
#if PDK_CONFIG(DATETIME_PARSER)
   //    DateTimeParser dt(Variant::Time, DateTimeParser::FromString);
   //    dt.setDefaultLocale(*this);
   //    if (dt.parseFormat(format))
   //        dt.fromString(string, 0, &time);
#else
   PDK_UNUSED(string);
   PDK_UNUSED(format);
#endif
   return time;
}

Date Locale::toDate(const String &string, const String &format) const
{
   Date date;
#if PDK_CONFIG(DATETIME_PARSER)
   //    DateTimeParser dt(Variant::Date, DateTimeParser::FromString);
   //    dt.setDefaultLocale(*this);
   //    if (dt.parseFormat(format))
   //        dt.fromString(string, &date, 0);
#else
   PDK_UNUSED(string);
   PDK_UNUSED(format);
#endif
   return date;
}

DateTime Locale::toDateTime(const String &string, const String &format) const
{
#if PDK_CONFIG(DATETIME_PARSER)
   //    Time time;
   //    Date date;
   
   //    DateTimeParser dt(QVariant::DateTime, DateTimeParser::FromString);
   //    dt.setDefaultLocale(*this);
   //    if (dt.parseFormat(format) && dt.fromString(string, &date, &time))
   //        return DateTime(date, time);
#else
   PDK_UNUSED(string);
   PDK_UNUSED(format);
#endif
   return DateTime(Date(), Time(-1, -1, -1));
}

#endif

Character Locale::decimalPoint() const
{
   return m_implPtr->decimal();
}

Character Locale::groupSeparator() const
{
   return m_implPtr->group();
}

Character Locale::percent() const
{
   return m_implPtr->percent();
}

Character Locale::zeroDigit() const
{
   return m_implPtr->zero();
}

Character Locale::negativeSign() const
{
   return m_implPtr->minus();
}

Character Locale::positiveSign() const
{
   return m_implPtr->plus();
}

Character Locale::exponential() const
{
   return m_implPtr->exponential();
}

namespace {

bool pdk_is_upper(char c)
{
   return c >= 'A' && c <= 'Z';
}

char pdk_to_lower(char c)
{
   if (c >= 'A' && c <= 'Z') {
      return c - 'A' + 'a';
   } else {
      return c;
   }
}

} // anonymous namespace

String Locale::toString(double i, char f, int prec) const
{
   LocaleData::DoubleForm form = LocaleData::DoubleForm::DFDecimal;
   uint flags = 0;
   if (pdk_is_upper(f)) {
      flags = pdk::as_integer<LocaleData::Flags>(LocaleData::Flags::CapitalEorX);
   }
   f = pdk_to_lower(f);
   
   switch (f) {
   case 'f':
      form = LocaleData::DoubleForm::DFDecimal;
      break;
   case 'e':
      form = LocaleData::DoubleForm::DFExponent;
      break;
   case 'g':
      form = LocaleData::DoubleForm::DFSignificantDigits;
      break;
   default:
      break;
   }
   if (!(m_implPtr->m_numberOptions & NumberOption::OmitGroupSeparator)) {
      flags |= pdk::as_integer<LocaleData::Flags>(LocaleData::Flags::ThousandsGroup);
   }
   if (!(m_implPtr->m_numberOptions & NumberOption::OmitLeadingZeroInExponent)) {
      flags |= pdk::as_integer<LocaleData::Flags>(LocaleData::Flags::ZeroPadExponent);
   }
   if (m_implPtr->m_numberOptions & NumberOption::IncludeTrailingZeroesAfterDot) {
      flags |= pdk::as_integer<LocaleData::Flags>(LocaleData::Flags::AddTrailingZeroes);
   }
   return m_implPtr->m_data->doubleToString(i, prec, form, -1, flags);
}

Locale Locale::system()
{
   return Locale(*LocalePrivate::create(internal::system_data()));
}

std::list<Locale> Locale::matchingLocales(Language language,
                                          Script script,
                                          Country country)
{
   if (pdk::as_integer<Language>(language) > pdk::as_integer<Language>(Language::LastLanguage) || 
       pdk::as_integer<Script>(script) >  pdk::as_integer<Script>(Script::LastScript) ||
       pdk::as_integer<Country>(country) > pdk::as_integer<Country>(Country::LastCountry)) {
      return std::list<Locale>();
   }
   
   if (language == Language::C) {
      std::list<Locale> ret;
      ret.emplace_back(Language::C);
      return ret;
   }
   std::list<Locale> result;
   if (language == Language::AnyLanguage && 
       script == Script::AnyScript && 
       country == Country::AnyCountry) {
      result.resize(internal::sg_localeDataSize);
   }
   
   const LocaleData *data = internal::sg_localeData + internal::sg_localeIndex[pdk::as_integer<Language>(language)];
   while ( (data != internal::sg_localeData + internal::sg_localeDataSize)
           && (language == Language::AnyLanguage || data->m_languageId == pdk::as_integer<Language>(language))) {
      if ((script == Script::AnyScript || data->m_scriptId == pdk::as_integer<Script>(script))
          && (country == Country::AnyCountry || data->m_countryId == pdk::as_integer<Country>(country))) {
         result.push_back(Locale(*(data->m_languageId == pdk::as_integer<Language>(Language::C) 
                                   ? internal::c_private()
                                   : LocalePrivate::create(data))));
      }
      ++data;
   }
   return result;
}

String Locale::monthName(int month, FormatType type) const
{
   if (month < 1 || month > 12)
      return String();
   
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(type == FormatType::LongFormat
                                                      ? SystemLocale::QueryType::MonthNameLong
                                                      : SystemLocale::QueryType::MonthNameShort,
                                                      month);
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   
   pdk::puint32 idx;
   pdk::puint32 size;
   switch (type) {
   case FormatType::LongFormat:
      idx = m_implPtr->m_data->m_longMonthNamesIdx;
      size = m_implPtr->m_data->m_longMonthNamesSize;
      break;
   case FormatType::ShortFormat:
      idx = m_implPtr->m_data->m_shortMonthNamesIdx;
      size = m_implPtr->m_data->m_shortMonthNamesSize;
      break;
   case FormatType::NarrowFormat:
      idx = m_implPtr->m_data->m_narrowMonthNamesIdx;
      size = m_implPtr->m_data->m_narrowMonthNamesSize;
      break;
   }
   return internal::get_locale_list_data(internal::sg_monthsData + idx, size, month - 1);
}

String Locale::standaloneMonthName(int month, FormatType type) const
{
   if (month < 1 || month > 12) {
      return String();
   }
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(type == FormatType::LongFormat
                                                      ? SystemLocale::QueryType::StandaloneMonthNameLong 
                                                      : SystemLocale::QueryType::StandaloneMonthNameShort,
                                                      month);
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   
   pdk::puint32 idx;
   pdk::puint32 size;
   switch (type) {
   case FormatType::LongFormat:
      idx = m_implPtr->m_data->m_standaloneLongMonthNamesIdx;
      size = m_implPtr->m_data->m_standaloneLongMonthNamesSize;
      break;
   case FormatType::ShortFormat:
      idx = m_implPtr->m_data->m_standaloneShortMonthNamesIdx;
      size = m_implPtr->m_data->m_standaloneShortMonthNamesSize;
      break;
   case FormatType::NarrowFormat:
      idx = m_implPtr->m_data->m_standaloneNarrowMonthNamesIdx;
      size = m_implPtr->m_data->m_standaloneNarrowMonthNamesSize;
      break;
   }
   String name = internal::get_locale_list_data(internal::sg_monthsData + idx, size, month - 1);
   if (name.isEmpty()) {
      return monthName(month, type);
   }
   return name;
}

String Locale::dayName(int day, FormatType type) const
{
   if (day < 1 || day > 7) {
      return String();
   }
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(type == FormatType::LongFormat
                                                      ? SystemLocale::QueryType::DayNameLong 
                                                      : SystemLocale::QueryType::DayNameShort,
                                                      day);
      if (res.has_value()) {
         std::any_cast<String>(res);
      }
   }
#endif
   if (day == 7)
      day = 0;
   
   pdk::puint32 idx;
   pdk::puint32 size;
   switch (type) {
   case FormatType::LongFormat:
      idx = m_implPtr->m_data->m_longDayNamesIdx;
      size = m_implPtr->m_data->m_longDayNamesSize;
      break;
   case FormatType::ShortFormat:
      idx = m_implPtr->m_data->m_shortDayNamesIdx;
      size = m_implPtr->m_data->m_shortDayNamesSize;
      break;
   case FormatType::NarrowFormat:
      idx = m_implPtr->m_data->m_narrowDayNamesIdx;
      size = m_implPtr->m_data->m_narrowDayNamesSize;
      break;
   }
   return internal::get_locale_list_data(internal::sg_daysData + idx, size, day);
}

String Locale::standaloneDayName(int day, FormatType type) const
{
   if (day < 1 || day > 7) {
      return String();
   }
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(type == FormatType::LongFormat
                                                      ? SystemLocale::QueryType::DayNameLong
                                                      : SystemLocale::QueryType::DayNameShort,
                                                      day);
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   if (day == 7) {
      day = 0;
   }
   pdk::puint32 idx, size;
   switch (type) {
   case FormatType::LongFormat:
      idx = m_implPtr->m_data->m_standaloneLongDayNamesIdx;
      size = m_implPtr->m_data->m_standaloneLongDayNamesSize;
      break;
   case FormatType::ShortFormat:
      idx = m_implPtr->m_data->m_standaloneShortDayNamesIdx;
      size = m_implPtr->m_data->m_standaloneShortDayNamesSize;
      break;
   case FormatType::NarrowFormat:
      idx = m_implPtr->m_data->m_standaloneNarrowDayNamesIdx;
      size = m_implPtr->m_data->m_standaloneNarrowDayNamesSize;
      break;
   }
   String name = internal::get_locale_list_data(internal::sg_daysData + idx, size, day);
   if (name.isEmpty()) {
      return dayName(day == 0 ? 7 : day, type);
   }
   return name;
}

pdk::DayOfWeek Locale::firstDayOfWeek() const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(SystemLocale::QueryType::FirstDayOfWeek, std::any());
      if (res.has_value()) {
         return static_cast<pdk::DayOfWeek>(std::any_cast<uint>(res));
      }
   }
#endif
   return static_cast<pdk::DayOfWeek>(m_implPtr->m_data->m_firstDayOfWeek);
}

namespace internal {

Locale::MeasurementSystem LocalePrivate::measurementSystem() const
{
   for (int i = 0; i < sg_imperialMeasurementSystemsCount; ++i) {
      if (sg_imperialMeasurementSystems[i].languageId == m_data->m_languageId
          && sg_imperialMeasurementSystems[i].countryId == m_data->m_countryId) {
         return sg_imperialMeasurementSystems[i].system;
      }
   }
   return Locale::MeasurementSystem::MetricSystem;
}

} // internal

std::list<pdk::DayOfWeek> Locale::weekdays() const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(SystemLocale::QueryType::Weekdays, std::any());
      if (res.has_value()) {
         return std::any_cast<std::list<pdk::DayOfWeek>>(res);
      }
   }
#endif
   std::list<pdk::DayOfWeek> weekdays;
   pdk::puint16 weekendStart = m_implPtr->m_data->m_weekendStart;
   pdk::puint16 weekendEnd = m_implPtr->m_data->m_weekendEnd;
   for (int day = pdk::as_integer<pdk::DayOfWeek>(pdk::DayOfWeek::Monday);
        day <= pdk::as_integer<pdk::DayOfWeek>(pdk::DayOfWeek::Sunday); day++) {
      if ((weekendEnd >= weekendStart && (day < weekendStart || day > weekendEnd)) ||
          (weekendEnd < weekendStart && (day > weekendEnd && day < weekendStart)))
         weekdays.push_back(static_cast<pdk::DayOfWeek>(day));
   }
   return weekdays;
}

Locale::MeasurementSystem Locale::measurementSystem() const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(SystemLocale::QueryType::MeasurementSystem, std::any());
      if (res.has_value()) {
         return MeasurementSystem(std::any_cast<int>(res));
      }
   }
#endif
   return m_implPtr->measurementSystem();
}

pdk::LayoutDirection Locale::textDirection() const
{
   switch (getScript()) {
   case Script::AdlamScript:
   case Script::ArabicScript:
   case Script::AvestanScript:
   case Script::CypriotScript:
   case Script::HatranScript:
   case Script::HebrewScript:
   case Script::ImperialAramaicScript:
   case Script::InscriptionalPahlaviScript:
   case Script::InscriptionalParthianScript:
   case Script::KharoshthiScript:
   case Script::LydianScript:
   case Script::MandaeanScript:
   case Script::ManichaeanScript:
   case Script::MendeKikakuiScript:
   case Script::MeroiticCursiveScript:
   case Script::MeroiticScript:
   case Script::NabataeanScript:
   case Script::NkoScript:
   case Script::OldHungarianScript:
   case Script::OldNorthArabianScript:
   case Script::OldSouthArabianScript:
   case Script::OrkhonScript:
   case Script::PalmyreneScript:
   case Script::PhoenicianScript:
   case Script::PsalterPahlaviScript:
   case Script::SamaritanScript:
   case Script::SyriacScript:
   case Script::ThaanaScript:
      return pdk::LayoutDirection::RightToLeft;
   default:
      break;
   }
   return pdk::LayoutDirection::LeftToRight;
}

String Locale::toUpper(const String &str) const
{
#if PDK_CONFIG(ICU)
   bool ok = true;
   String result = internal::Icu::toUpper(m_implPtr->bcp47Name('_'), str, &ok);
   if (ok) {
      return result;
   }
   // else fall through and use Qt's toUpper
#endif
   return str.toUpper();
}

String Locale::toLower(const String &str) const
{
#if PDK_CONFIG(ICU)
   bool ok = true;
   const String result = internal::Icu::toLower(m_implPtr->bcp47Name('_'), str, &ok);
   if (ok) {
      return result;
   }
   // else fall through and use Qt's toUpper
#endif
   return str.toLower();
}

String Locale::amText() const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(SystemLocale::QueryType::AMText, std::any());
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   return internal::get_locale_data(internal::sg_amData + m_implPtr->m_data->m_amIdx, m_implPtr->m_data->m_amSize);
}

String Locale::pmText() const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(SystemLocale::QueryType::PMText, std::any());
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   return internal::get_locale_data(internal::sg_pmData + m_implPtr->m_data->m_pmIdx, m_implPtr->m_data->m_pmSize);
}

namespace internal {

String LocalePrivate::dateTimeToString(StringView format, const DateTime &datetime,
                                       const Date &dateOnly, const Time &timeOnly,
                                       const Locale *q) const
{
   Date date;
   Time time;
   bool formatDate = false;
   bool formatTime = false;
   if (datetime.isValid()) {
      date = datetime.getDate();
      time = datetime.getTime();
      formatDate = true;
      formatTime = true;
   } else if (dateOnly.isValid()) {
      date = dateOnly;
      formatDate = true;
   } else if (timeOnly.isValid()) {
      time = timeOnly;
      formatTime = true;
   } else {
      return String();
   }
   
   String result;
   
   int i = 0;
   while (static_cast<StringView::size_type>(i) < format.size()) {
      if (format.at(i).unicode() == '\'') {
         result.append(internal::read_escaped_format_string(format, &i));
         continue;
      }
      
      const Character c = format.at(i);
      int repeat = internal::repeat_count(format.mid(i));
      bool used = false;
      if (formatDate) {
         switch (c.unicode()) {
         case 'y':
            used = true;
            if (repeat >= 4) {
               repeat = 4;
            } else if (repeat >= 2) {
               repeat = 2;
            }
            switch (repeat) {
            case 4: {
               const int yr = date.getYear();
               const int len = (yr < 0) ? 5 : 4;
               result.append(m_data->longLongToString(yr, -1, 10, len,
                                                      pdk::as_integer<LocaleData::Flags>(LocaleData::Flags::ZeroPadded)));
               break;
            }
            case 2:
               result.append(m_data->longLongToString(date.getYear() % 100, -1, 10, 2,
                                                      pdk::as_integer<LocaleData::Flags>(LocaleData::Flags::ZeroPadded)));
               break;
            default:
               repeat = 1;
               result.append(c);
               break;
            }
            break;
            
         case 'M':
            used = true;
            repeat = std::min(repeat, 4);
            switch (repeat) {
            case 1:
               result.append(m_data->longLongToString(date.getMonth()));
               break;
            case 2:
               result.append(m_data->longLongToString(date.getMonth(), -1, 10, 2, 
                                                      pdk::as_integer<LocaleData::Flags>(LocaleData::Flags::ZeroPadded)));
               break;
            case 3:
               result.append(q->monthName(date.getMonth(), Locale::FormatType::ShortFormat));
               break;
            case 4:
               result.append(q->monthName(date.getMonth(), Locale::FormatType::LongFormat));
               break;
            }
            break;
            
         case 'd':
            used = true;
            repeat = std::min(repeat, 4);
            switch (repeat) {
            case 1:
               result.append(m_data->longLongToString(date.getDay()));
               break;
            case 2:
               result.append(m_data->longLongToString(date.getDay(), -1, 10, 2, 
                                                      pdk::as_integer<LocaleData::Flags>(LocaleData::Flags::ZeroPadded)));
               break;
            case 3:
               result.append(q->dayName(date.getDayOfWeek(), Locale::FormatType::ShortFormat));
               break;
            case 4:
               result.append(q->dayName(date.getDayOfWeek(), Locale::FormatType::LongFormat));
               break;
            }
            break;
            
         default:
            break;
         }
      }
      if (!used && formatTime) {
         switch (c.unicode()) {
         case 'h': {
            used = true;
            repeat = std::min(repeat, 2);
            int hour = time.getHour();
            if (time_format_containsAP(format)) {
               if (hour > 12) {
                  hour -= 12;
               } else if (hour == 0) {
                  hour = 12;
               }
            }
            
            switch (repeat) {
            case 1:
               result.append(m_data->longLongToString(hour));
               break;
            case 2:
               result.append(m_data->longLongToString(hour, -1, 10, 2, 
                                                      pdk::as_integer<LocaleData::Flags>(LocaleData::Flags::ZeroPadded)));
               break;
            }
            break;
         }
         case 'H':
            used = true;
            repeat = std::min(repeat, 2);
            switch (repeat) {
            case 1:
               result.append(m_data->longLongToString(time.getHour()));
               break;
            case 2:
               result.append(m_data->longLongToString(time.getHour(), -1, 10, 2, 
                                                      pdk::as_integer<LocaleData::Flags>(LocaleData::Flags::ZeroPadded)));
               break;
            }
            break;
            
         case 'm':
            used = true;
            repeat = std::min(repeat, 2);
            switch (repeat) {
            case 1:
               result.append(m_data->longLongToString(time.getMinute()));
               break;
            case 2:
               result.append(m_data->longLongToString(time.getMinute(), -1, 10, 2, 
                                                      pdk::as_integer<LocaleData::Flags>(LocaleData::Flags::ZeroPadded)));
               break;
            }
            break;
            
         case 's':
            used = true;
            repeat = std::min(repeat, 2);
            switch (repeat) {
            case 1:
               result.append(m_data->longLongToString(time.getSecond()));
               break;
            case 2:
               result.append(m_data->longLongToString(time.getSecond(), -1, 10, 2, 
                                                      pdk::as_integer<LocaleData::Flags>(LocaleData::Flags::ZeroPadded)));
               break;
            }
            break;
            
         case 'a':
            used = true;
            if (static_cast<StringView::size_type>(i + 1) < format.size() && format.at(i + 1).unicode() == 'p') {
               repeat = 2;
            } else {
               repeat = 1;
            }
            result.append(time.getHour() < 12 ? q->amText().toLower() : q->pmText().toLower());
            break;
            
         case 'A':
            used = true;
            if (static_cast<StringView::size_type>(i + 1) < format.size() && format.at(i + 1).unicode() == 'P') {
               repeat = 2;
            } else {
               repeat = 1;
            }
            result.append(time.getHour() < 12 ? q->amText().toUpper() : q->pmText().toUpper());
            break;
            
         case 'z':
            used = true;
            if (repeat >= 3) {
               repeat = 3;
            } else {
               repeat = 1;
            }
            
            // note: the millisecond component is treated like the decimal part of the seconds
            // so ms == 2 is always printed as "002", but ms == 200 can be either "2" or "200"
            result.append(m_data->longLongToString(time.getMsec(), -1, 10, 3, 
                                                   pdk::as_integer<LocaleData::Flags>(LocaleData::Flags::ZeroPadded)));
            if (repeat == 1) {
               if (result.endsWith(zero())) {
                  result.chop(1);
               }
               if (result.endsWith(zero())) {
                  result.chop(1);
               }
            }
            
            break;
            
         case 't':
            used = true;
            repeat = 1;
            // If we have a DateTime use the time spec otherwise use the current system tzname
            if (formatDate) {
               result.append(datetime.timeZoneAbbreviation());
            } else {
               result.append(DateTime::getCurrentDateTime().timeZoneAbbreviation());
            }
            break;
            
         default:
            break;
         }
      }
      if (!used) {
         result.append(String(repeat, c));
      }
      i += repeat;
   }
   
   return result;
}

} // internal

String LocaleData::doubleToString(double d, int precision, DoubleForm form,
                                  int width, unsigned flags) const
{
   return doubleToString(m_zero, m_plus, m_minus, m_exponential, m_group, m_decimal,
                         d, precision, form, width, flags);
}

String LocaleData::doubleToString(const Character _zero, const Character plus, const Character minus,
                                  const Character exponential, const Character group, const Character decimal,
                                  double d, int precision, DoubleForm form, int width, unsigned flags)
{
   if (precision != Locale::FloatingPointShortest && precision < 0)
      precision = 6;
   if (width < 0)
      width = 0;
   
   bool negative = false;
   String num_str;
   
   int decpt;
   int bufSize = 1;
   if (precision == Locale::FloatingPointShortest)
      bufSize += DoubleMaxSignificant;
   else if (form == DoubleForm::DFDecimal) {// optimize for numbers between -512k and 512k
      bufSize += ((d > (1 << 19) || d < -(1 << 19)) ? DoubleMaxDigitsBeforeDecimal : 6) +
            precision;  
   } else {// Add extra digit due to different interpretations of precision. Also, "nan" has to fit.
      bufSize += std::max(2, precision) + 1;  
   }
   VarLengthArray<char> buf(bufSize);
   int length;
   internal::double_to_ascii(d, form, precision, buf.getRawData(), bufSize, negative, length, decpt);
   if (pdk::strncmp(buf.getRawData(), "inf", 3) == 0 || pdk::strncmp(buf.getRawData(), "nan", 3) == 0) {
      num_str = String::fromLatin1(buf.getRawData(), length);
   } else { // Handle normal numbers
      String digits = String::fromLatin1(buf.getRawData(), length);
      
      if (_zero.unicode() != '0') {
         ushort z = _zero.unicode() - '0';
         for (int i = 0; i < digits.length(); ++i)
            reinterpret_cast<ushort *>(digits.getRawData())[i] += z;
      }
      
      bool always_show_decpt = (flags & pdk::as_integer<Flags>(Flags::ForcePoint));
      switch (form) {
      case DoubleForm::DFExponent: {
         num_str = internal::exponent_form(_zero, decimal, exponential, group, plus, minus,
                                           digits, decpt, precision, internal::PrecisionMode::PMDecimalDigits,
                                           always_show_decpt, flags & pdk::as_integer<Flags>(Flags::ZeroPadExponent));
         break;
      }
      case DoubleForm::DFDecimal: {
         num_str = internal::decimal_form(_zero, decimal, group,
                                          digits, decpt, precision, internal::PrecisionMode::PMDecimalDigits,
                                          always_show_decpt, flags & pdk::as_integer<Flags>(Flags::ThousandsGroup));
         break;
      }
      case DoubleForm::DFSignificantDigits: {
         internal::PrecisionMode mode = (flags & pdk::as_integer<Flags>(Flags::AddTrailingZeroes)) ?
                  internal::PrecisionMode::PMSignificantDigits : internal::PrecisionMode::PMChopTrailingZeros;
         
         int cutoff = precision < 0 ? 6 : precision;
         // Find out which representation is shorter
         if (precision == Locale::FloatingPointShortest && decpt > 0) {
            cutoff = digits.length() + 4; // 'e', '+'/'-', one digit exponent
            if (decpt <= 10) {
               ++cutoff;
            } else {
               cutoff += decpt > 100 ? 2 : 1;
            }
            if (!always_show_decpt && digits.length() > decpt) {
               ++cutoff; // decpt shown in exponent form, but not in decimal form
            }
         }
         
         if (decpt != digits.length() && (decpt <= -4 || decpt > cutoff)) {
            num_str = internal::exponent_form(_zero, decimal, exponential, group, plus, minus,
                                              digits, decpt, precision, mode,
                                              always_show_decpt, flags & pdk::as_integer<Flags>(Flags::ZeroPadExponent));
         } else {
            num_str = internal::decimal_form(_zero, decimal, group,
                                             digits, decpt, precision, mode,
                                             always_show_decpt, flags & pdk::as_integer<Flags>(Flags::ThousandsGroup));
         }
         break;
      }
      }
      
      if (internal::is_zero(d)) {
         negative = false;
      }
      // pad with zeros. LeftAdjusted overrides this flag). Also, we don't
      // pad special numbers
      if (flags & pdk::as_integer<Flags>(Flags::ZeroPadded) && !(flags & pdk::as_integer<Flags>(Flags::LeftAdjusted))) {
         int num_pad_chars = width - num_str.length();
         // leave space for the sign
         if (negative
             || flags & pdk::as_integer<Flags>(Flags::AlwaysShowSign)
             || flags & pdk::as_integer<Flags>(Flags::BlankBeforePositive)) {
            --num_pad_chars;
         }
         for (int i = 0; i < num_pad_chars; ++i) {
            num_str.prepend(_zero);
         }  
      }
   }
   
   // add sign
   if (negative) {
      num_str.prepend(minus);
   } else if (flags & pdk::as_integer<Flags>(Flags::AlwaysShowSign)) {
      num_str.prepend(plus);
   } else if (flags & pdk::as_integer<Flags>(Flags::BlankBeforePositive)) {
      num_str.prepend(Latin1Character(' '));
   }
   if (flags & pdk::as_integer<Flags>(Flags::CapitalEorX)) {
      num_str = std::move(num_str).toUpper();
   }
   return num_str;
}

String LocaleData::longLongToString(pdk::plonglong l, int precision,
                                    int base, int width,
                                    unsigned flags) const
{
   return longLongToString(m_zero, m_group, m_plus, m_minus,
                           l, precision, base, width, flags);
}

String LocaleData::longLongToString(const Character zero, const Character group,
                                    const Character plus, const Character minus,
                                    pdk::plonglong l, int precision,
                                    int base, int width,
                                    unsigned flags)
{
   bool precision_not_specified = false;
   if (precision == -1) {
      precision_not_specified = true;
      precision = 1;
   }
   
   bool negative = l < 0;
   if (base != 10) {
      // these are not supported by sprintf for octal and hex
      flags &= ~pdk::as_integer<Flags>(Flags::AlwaysShowSign);
      flags &= ~pdk::as_integer<Flags>(Flags::BlankBeforePositive);
      negative = false; // neither are negative numbers
   }
   
   String num_str;
   if (base == 10) {
      num_str = internal::pdk_lltoa(l, base, zero);
   } else {
      num_str = internal::pdk_ulltoa(l, base, zero);
   }
   uint cnt_thousand_sep = 0;
   if (flags & pdk::as_integer<Flags>(Flags::ThousandsGroup) && base == 10) {
      for (int i = num_str.length() - 3; i > 0; i -= 3) {
         num_str.insert(i, group);
         ++cnt_thousand_sep;
      }
   }
   
   for (int i = num_str.length()/* - cnt_thousand_sep*/; i < precision; ++i) {
      num_str.prepend(base == 10 ? zero : Character::fromLatin1('0'));
   }
   
   if ((flags & pdk::as_integer<Flags>(Flags::ShowBase))
       && base == 8
       && (num_str.isEmpty() || num_str[0].unicode() != Latin1Character('0'))) {
      num_str.prepend(Latin1Character('0'));
   }
   
   
   // LeftAdjusted overrides this flag ZeroPadded. sprintf only padds
   // when precision is not specified in the format string
   bool zero_padded = flags & pdk::as_integer<Flags>(Flags::ZeroPadded)
         && !(flags & pdk::as_integer<Flags>(Flags::LeftAdjusted))
         && precision_not_specified;
   
   if (zero_padded) {
      int num_pad_chars = width - num_str.length();
      
      // leave space for the sign
      if (negative
          || flags & pdk::as_integer<Flags>(Flags::AlwaysShowSign)
          || flags & pdk::as_integer<Flags>(Flags::BlankBeforePositive)) {
         --num_pad_chars;
      }
      
      
      // leave space for optional '0x' in hex form
      if (base == 16 && (flags & pdk::as_integer<Flags>(Flags::ShowBase))) {
         num_pad_chars -= 2;
      } else if (base == 2 && (flags & pdk::as_integer<Flags>(Flags::ShowBase))) {
         // leave space for optional '0b' in binary form
         num_pad_chars -= 2;
      }
      for (int i = 0; i < num_pad_chars; ++i) {
         num_str.prepend(base == 10 ? zero : Character::fromLatin1('0'));
      }   
   }
   
   if (flags & pdk::as_integer<Flags>(Flags::CapitalEorX)) {
      num_str = std::move(num_str).toUpper();
   }
   
   if (base == 16 && (flags & pdk::as_integer<Flags>(Flags::ShowBase))) {
      num_str.prepend(Latin1String(flags & pdk::as_integer<Flags>(Flags::UppercaseBase) ? "0X" : "0x"));
   }
   
   if (base == 2 && (flags & pdk::as_integer<Flags>(Flags::ShowBase))) {
      num_str.prepend(Latin1String(flags & pdk::as_integer<Flags>(Flags::UppercaseBase) ? "0B" : "0b"));
   }
   
   // add sign
   if (negative) {
      num_str.prepend(minus);
   } else if (flags & pdk::as_integer<Flags>(Flags::AlwaysShowSign)) {
      num_str.prepend(plus);
   } else if (flags & pdk::as_integer<Flags>(Flags::BlankBeforePositive)) {
      num_str.prepend(Latin1Character(' '));
   }
   return num_str;
}

String LocaleData::unsLongLongToString(pdk::pulonglong l, int precision,
                                       int base, int width,
                                       unsigned flags) const
{
   return unsLongLongToString(m_zero, m_group, m_plus,
                              l, precision, base, width, flags);
}

String LocaleData::unsLongLongToString(const Character zero, const Character group,
                                       const Character plus,
                                       pdk::pulonglong l, int precision,
                                       int base, int width,
                                       unsigned flags)
{
   const Character resultZero = base == 10 ? zero : Character(Latin1Character('0'));
   String num_str = l ? internal::pdk_ulltoa(l, base, zero) : String(resultZero);
   
   bool precision_not_specified = false;
   if (precision == -1) {
      if (flags == pdk::as_integer<Flags>(Flags::NoFlags)) {
         return num_str; // fast-path: nothing below applies, so we're done.
      }
      precision_not_specified = true;
      precision = 1;
   }
   
   uint cnt_thousand_sep = 0;
   if (flags & pdk::as_integer<Flags>(Flags::ThousandsGroup) && base == 10) {
      for (int i = num_str.length() - 3; i > 0; i -=3) {
         num_str.insert(i, group);
         ++cnt_thousand_sep;
      }
   }
   
   const int zeroPadding = precision - num_str.length()/* + cnt_thousand_sep*/;
   if (zeroPadding > 0)
      num_str.prepend(String(zeroPadding, resultZero));
   
   if ((flags & pdk::as_integer<Flags>(Flags::ShowBase))
       && base == 8
       && (num_str.isEmpty() || num_str.at(0).unicode() != Latin1Character('0'))) {
      num_str.prepend(Latin1Character('0'));
   }
   // LeftAdjusted overrides this flag ZeroPadded. sprintf only padds
   // when precision is not specified in the format string
   bool zero_padded = flags & pdk::as_integer<Flags>(Flags::ZeroPadded)
         && !(flags & pdk::as_integer<Flags>(Flags::LeftAdjusted))
         && precision_not_specified;
   
   if (zero_padded) {
      int num_pad_chars = width - num_str.length();
      
      // leave space for optional '0x' in hex form
      if (base == 16 && flags & pdk::as_integer<Flags>(Flags::ShowBase)) {
         num_pad_chars -= 2;
      } else if (base == 2 && flags & pdk::as_integer<Flags>(Flags::ShowBase)) {
         // leave space for optional '0b' in binary form
         num_pad_chars -= 2;
      }
      if (num_pad_chars > 0) {
         num_str.prepend(String(num_pad_chars, resultZero));
      }
   }
   
   if (flags & pdk::as_integer<Flags>(Flags::CapitalEorX)) {
      num_str = std::move(num_str).toUpper();
   }
   if (base == 16 && flags & pdk::as_integer<Flags>(Flags::ShowBase)) {
      num_str.prepend(Latin1String(flags & pdk::as_integer<Flags>(Flags::UppercaseBase) ? "0X" : "0x"));
   } else if (base == 2 && flags & pdk::as_integer<Flags>(Flags::ShowBase)) {
      num_str.prepend(Latin1String(flags & pdk::as_integer<Flags>(Flags::UppercaseBase) ? "0B" : "0b"));
   }
   // add sign
   if (flags & pdk::as_integer<Flags>(Flags::AlwaysShowSign)) {
      num_str.prepend(plus);
   } else if (flags & pdk::as_integer<Flags>(Flags::BlankBeforePositive)) {
      num_str.prepend(Latin1Character(' '));
   }
   return num_str;
}

/*
    Converts a number in locale to its representation in the C locale.
    Only has to guarantee that a string that is a correct representation of
    a number will be converted. If junk is passed in, junk will be passed
    out and the error will be detected during the actual conversion to a
    number. We can't detect junk here, since we don't even know the base
    of the number.
*/
bool LocaleData::numberToCLocale(StringView s, Locale::NumberOptions number_options,
                                 CharBuff *result) const
{
   const Character *uc = s.data();
   auto l = s.size();
   StringView::size_type idx = 0;
   // Skip whitespace
   while (idx < l && uc[idx].isSpace()) {
      ++idx;
   }   
   if (idx == l) {
      return false;
   }
   // Check trailing whitespace
   for (; idx < l; --l) {
      if (!uc[l - 1].isSpace()) {
         break;
      }
   }
   
   int group_cnt = 0; // counts number of group chars
   int decpt_idx = -1;
   int last_separator_idx = -1;
   int start_of_digits_idx = -1;
   int exponent_idx = -1;
   
   while (idx < l) {
      const Character in = uc[idx];
      char out = digitToCLocale(in);
      if (out == 0) {
         if (in == m_list) {
            out = ';';
         } else if (in == m_percent) {
            out = '%';
         } else if (in.unicode() >= 'A' && in.unicode() <= 'Z') {
            // for handling base-x numbers
            out = in.toLower().toLatin1();
         } else if (in.unicode() >= 'a' && in.unicode() <= 'z') {
            out = in.toLatin1();
         } else {
            break;
         }
      } else if (out == '.') {
         // Fail if more than one decimal point or point after e
         if (decpt_idx != -1 || exponent_idx != -1) {
            return false;
         }
         decpt_idx = idx;
      } else if (out == 'e' || out == 'E') {
         exponent_idx = idx;
      }
      if (number_options & Locale::NumberOption::RejectLeadingZeroInExponent) {
         if (exponent_idx != -1 && out == '0' && idx < l - 1) {
            // After the exponent there can only be '+', '-' or digits.
            // If we find a '0' directly after some non-digit, then that is a leading zero.
            if (result->last() < '0' || result->last() > '9')
               return false;
         }
      }
      
      if (number_options & Locale::NumberOption::RejectTrailingZeroesAfterDot) {
         // If we've seen a decimal point and the last character after the exponent is 0, then
         // that is a trailing zero.
         if (decpt_idx >= 0 && 
             idx == static_cast<StringView::size_type>(exponent_idx) && 
             result->last() == '0') {
            return false;
         }
      }
      
      if (!(number_options & Locale::NumberOption::RejectGroupSeparator)) {
         if (start_of_digits_idx == -1 && out >= '0' && out <= '9') {
            start_of_digits_idx = idx;
         } else if (out == ',') {
            // Don't allow group chars after the decimal point or exponent
            if (decpt_idx != -1 || exponent_idx != -1) {
               return false;
            }
            // check distance from the last separator or from the beginning of the digits
            // ### FIXME: Some locales allow other groupings! See https://en.wikipedia.org/wiki/Thousands_separator
            if (last_separator_idx != -1 && idx - last_separator_idx != 4) {
               return false;
            }
            
            if (last_separator_idx == -1 && (start_of_digits_idx == -1 || idx - start_of_digits_idx > 3)) {
               return false;
            }
            
            last_separator_idx = idx;
            ++group_cnt;
            
            // don't add the group separator
            ++idx;
            continue;
         } else if (out == '.' || out == 'e' || out == 'E') {
            // check distance from the last separator
            // ### FIXME: Some locales allow other groupings! See https://en.wikipedia.org/wiki/Thousands_separator
            if (last_separator_idx != -1 && idx - last_separator_idx != 4) {
               return false;
            }
            // stop processing separators
            last_separator_idx = -1;
         }
      }
      
      result->append(out);
      ++idx;
   }
   
   if (!(number_options & Locale::NumberOption::RejectGroupSeparator)) {
      // group separator post-processing
      // did we end in a separator?
      if (static_cast<StringView::size_type>(last_separator_idx + 1) == idx) {
         return false;
      }
      
      // were there enough digits since the last separator?
      if (last_separator_idx != -1 && idx - last_separator_idx != 4) {
         return false;
      }
      
   }
   
   if (number_options & Locale::NumberOption::RejectTrailingZeroesAfterDot) {
      // In decimal form, the last character can be a trailing zero if we've seen a decpt.
      if (decpt_idx != -1 && exponent_idx == -1 && result->last() == '0') {
         return false;
      }
      
   }
   
   result->append('\0');
   return idx == l;
}

bool LocaleData::validateChars(StringView str, NumberMode numMode, ByteArray *buff,
                               int decDigits, Locale::NumberOptions number_options) const
{
   buff->clear();
   buff->reserve(str.length());
   
   const bool scientific = numMode == NumberMode::DoubleScientificMode;
   bool lastWasE = false;
   bool lastWasDigit = false;
   int eCnt = 0;
   int decPointCnt = 0;
   bool dec = false;
   int decDigitCnt = 0;
   
   for (pdk::sizetype i = 0; i < str.size(); ++i) {
      char c = digitToCLocale(str.at(i));
      
      if (c >= '0' && c <= '9') {
         if (numMode != NumberMode::IntegerMode) {
            // If a double has too many digits after decpt, it shall be Invalid.
            if (dec && decDigits != -1 && decDigits < ++decDigitCnt) {
               return false;
            }
         }
         
         // The only non-digit character after the 'e' can be '+' or '-'.
         // If a zero is directly after that, then the exponent is zero-padded.
         if ((number_options & Locale::NumberOption::RejectLeadingZeroInExponent) && c == '0' && eCnt > 0 &&
             !lastWasDigit) {
            return false;
         }
         lastWasDigit = true;
      } else {
         switch (c) {
         case '.':
            if (numMode == NumberMode::IntegerMode) {
               // If an integer has a decimal point, it shall be Invalid.
               return false;
            } else {
               // If a double has more than one decimal point, it shall be Invalid.
               if (++decPointCnt > 1) {
                  return false;
               }
#if 0
               // If a double with no decimal digits has a decimal point, it shall be
               // Invalid.
               if (decDigits == 0)
                  return false;
#endif                  // On second thoughts, it shall be Valid.
               
               dec = true;
            }
            break;
            
         case '+':
         case '-':
            if (scientific) {
               // If a scientific has a sign that's not at the beginning or after
               // an 'e', it shall be Invalid.
               if (i != 0 && !lastWasE)
                  return false;
            } else {
               // If a non-scientific has a sign that's not at the beginning,
               // it shall be Invalid.
               if (i != 0)
                  return false;
            }
            break;
            
         case ',':
            //it can only be placed after a digit which is before the decimal point
            if ((number_options & Locale::NumberOption::RejectGroupSeparator) || !lastWasDigit ||
                decPointCnt > 0)
               return false;
            break;
            
         case 'e':
            if (scientific) {
               // If a scientific has more than one 'e', it shall be Invalid.
               if (++eCnt > 1)
                  return false;
               dec = false;
            } else {
               // If a non-scientific has an 'e', it shall be Invalid.
               return false;
            }
            break;
            
         default:
            // If it's not a valid digit, it shall be Invalid.
            return false;
         }
         lastWasDigit = false;
      }
      
      lastWasE = c == 'e';
      if (c != ',')
         buff->append(c);
   }
   
   return true;
}

double LocaleData::stringToDouble(StringView str, bool *ok,
                                  Locale::NumberOptions number_options) const
{
   CharBuff buff;
   if (!numberToCLocale(str, number_options, &buff)) {
      if (ok != 0)
         *ok = false;
      return 0.0;
   }
   int processed = 0;
   bool nonNullOk = false;
   double d = internal::ascii_to_double(buff.getConstRawData(), buff.length() - 1, nonNullOk, processed);
   if (ok) {
      *ok = nonNullOk;
   }
   return d;
}

pdk::plonglong LocaleData::stringToLongLong(StringView str, int base, bool *ok,
                                            Locale::NumberOptions number_options) const
{
   CharBuff buff;
   if (!numberToCLocale(str, number_options, &buff)) {
      if (ok != 0) {
         *ok = false;
      }
      return 0;
   }
   
   return bytearrayToLongLong(buff.getConstRawData(), base, ok);
}

pdk::pulonglong LocaleData::stringToUnsLongLong(StringView str, int base, bool *ok,
                                                Locale::NumberOptions number_options) const
{
   CharBuff buff;
   if (!numberToCLocale(str, number_options, &buff)) {
      if (ok != 0) {
         *ok = false;
      }
      return 0;
   }
   
   return bytearrayToUnsLongLong(buff.getConstRawData(), base, ok);
}

double LocaleData::bytearrayToDouble(const char *num, bool *ok)
{
   bool nonNullOk = false;
   int len = static_cast<int>(pdk::strlen(num));
   PDK_ASSERT(len >= 0);
   int processed = 0;
   double d = internal::ascii_to_double(num, len, nonNullOk, processed);
   if (ok) {
      *ok = nonNullOk;
   } 
   return d;
}

pdk::plonglong LocaleData::bytearrayToLongLong(const char *num, int base, bool *ok)
{
   bool _ok;
   const char *endptr;
   
   if (*num == '\0') {
      if (ok != 0)
         *ok = false;
      return 0;
   }
   
   pdk::plonglong l = internal::pdk_strtoll(num, &endptr, base, &_ok);
   
   if (!_ok) {
      if (ok != 0) {
         *ok = false;
      }
      return 0;
   }
   
   if (*endptr != '\0') {
      // we stopped at a non-digit character after converting some digits
      if (ok != 0) {
         *ok = false;
      }
      return 0;
   }
   
   if (ok != 0) {
      *ok = true;
   }
   return l;
}

pdk::pulonglong LocaleData::bytearrayToUnsLongLong(const char *num, int base, bool *ok)
{
   bool _ok;
   const char *endptr;
   pdk::pulonglong l = internal::pdk_strtoull(num, &endptr, base, &_ok);
   
   if (!_ok || *endptr != '\0') {
      if (ok != 0) {
         *ok = false;
      }
      return 0;
   }
   
   if (ok != 0) {
      *ok = true;
   }
   return l;
}

String Locale::currencySymbol(Locale::CurrencySymbolFormat format) const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(SystemLocale::QueryType::CurrencySymbol, format);
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   pdk::puint32 idx;
   pdk::puint32 size;
   switch (format) {
   case CurrencySymbol:
      idx = m_implPtr->m_data->m_currencySymbolIdx;
      size = m_implPtr->m_data->m_currencySymbolSize;
      return internal::get_locale_data(internal::sg_currencySymbolData + idx, size);
   case CurrencyDisplayName:
      idx = m_implPtr->m_data->m_currencyDisplayNameIdx;
      size = m_implPtr->m_data->m_currencyDisplayNameSize;
      return internal::get_locale_list_data(internal::sg_currencyDisplayNameData + idx, size, 0);
   case CurrencyIsoCode: {
      int len = 0;
      const LocaleData *data = m_implPtr->m_data;
      for (; len < 3; ++len) {
         if (!data->m_currencyIsoCode[len]) {
            break;
         }
      }
      return len ? String::fromLatin1(data->m_currencyIsoCode, len) : String();
   }
   }
   return String();
}

String Locale::toCurrencyString(pdk::plonglong value, const String &symbol) const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      SystemLocale::CurrencyToStringArgument arg(value, symbol);
      std::any res = internal::system_locale()->query(SystemLocale::QueryType::CurrencyToString, std::any(arg));
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   const LocalePrivate *d = m_implPtr;
   pdk::puint8 idx = d->m_data->m_currencyFormatIdx;
   pdk::puint8 size = d->m_data->m_currencyFormatSize;
   if (d->m_data->m_currencyNegativeFormatSize && value < 0) {
      idx = d->m_data->m_currencyNegativeFormatIdx;
      size = d->m_data->m_currencyNegativeFormatSize;
      value = -value;
   }
   String str = toString(value);
   String sym = symbol.isNull() ? currencySymbol() : symbol;
   if (sym.isEmpty()) {
      sym = currencySymbol(Locale::CurrencyIsoCode);
   }
   String format = internal::get_locale_data(internal::sg_currencyFormatData + idx, size);
   return format.arg(str, sym);
}

String Locale::toCurrencyString(pdk::pulonglong value, const String &symbol) const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      SystemLocale::CurrencyToStringArgument arg(value, symbol);
      std::any res = internal::system_locale()->query(SystemLocale::QueryType::CurrencyToString, std::any(arg));
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   const LocaleData *data = m_implPtr->m_data;
   pdk::puint8 idx = data->m_currencyFormatIdx;
   pdk::puint8 size = data->m_currencyFormatSize;
   String str = toString(value);
   String sym = symbol.isNull() ? currencySymbol() : symbol;
   if (sym.isEmpty()) {
      sym = currencySymbol(Locale::CurrencyIsoCode);
   }
   String format = internal::get_locale_data(internal::sg_currencyFormatData + idx, size);
   return format.arg(str, sym);
}

String Locale::toCurrencyString(double value, const String &symbol, int precision) const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      SystemLocale::CurrencyToStringArgument arg(value, symbol);
      std::any res = internal::system_locale()->query(SystemLocale::QueryType::CurrencyToString, std::any(arg));
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   const LocaleData *data = m_implPtr->m_data;
   pdk::puint8 idx = data->m_currencyFormatIdx;
   pdk::puint8 size = data->m_currencyFormatSize;
   if (data->m_currencyNegativeFormatSize && value < 0) {
      idx = data->m_currencyNegativeFormatIdx;
      size = data->m_currencyNegativeFormatSize;
      value = -value;
   }
   String str = toString(value, 'f', precision == -1 ? m_implPtr->m_data->m_currencyDigits : precision);
   String sym = symbol.isNull() ? currencySymbol() : symbol;
   if (sym.isEmpty()) {
      sym = currencySymbol(Locale::CurrencyIsoCode);
   }
   String format = internal::get_locale_data(internal::sg_currencyFormatData + idx, size);
   return format.arg(str, sym);
}

String Locale::formattedDataSize(pdk::pint64 bytes, int precision, DataSizeFormats format)
{
   int power, base = 1000;
   if (!bytes) {
      power = 0;
   } else if (format & DataSizeBase1000) {
      power = int(std::log10(std::abs(bytes)) / 3);
   } else { // Compute log2(bytes) / 10:
      power = int((63 - pdk::count_leading_zero_bits(pdk::puint64(std::abs(bytes)))) / 10);
      base = 1024;
   }
   // Only go to doubles if we'll be using a quantifier:
   const String number = power
         ? toString(bytes / std::pow(double(base), power), 'f', std::min(precision, 3 * power))
         : toString(bytes);
   
   // We don't support sizes in units larger than exbibytes because
   // the number of bytes would not fit into qint64.
   PDK_ASSERT(power <= 6 && power >= 0);
   String unit;
   if (power > 0) {
      pdk::puint16 index, size;
      if (format & DataSizeSIQuantifiers) {
         index = m_implPtr->m_data->m_byteSiQuantifiedIdx;
         size = m_implPtr->m_data->m_byteSiQuantifiedSize;
      } else {
         index = m_implPtr->m_data->m_byteIecQuantifiedIdx;
         size = m_implPtr->m_data->m_byteIecQuantifiedSize;
      }
      unit = internal::get_locale_list_data(internal::sg_byteUnitData + index, size, power - 1);
   } else {
      unit = internal::get_locale_data(internal::sg_byteUnitData + m_implPtr->m_data->m_byteIdx, m_implPtr->m_data->m_byteSize);
   }
   return number + Latin1Character(' ') + unit;
}

StringList Locale::uiLanguages() const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(SystemLocale::QueryType::UILanguages, std::any());
      if (res.has_value()) {
         StringList result = std::any_cast<StringList>(res);
         if (!result.empty()) {
            return result;
         }
      }
   }
#endif
   LocaleId id = LocaleId::fromIds(m_implPtr->m_data->m_languageId, m_implPtr->m_data->m_scriptId, 
                                   m_implPtr->m_data->m_countryId);
   const LocaleId max = id.withLikelySubtagsAdded();
   const LocaleId min = max.withLikelySubtagsRemoved();
   
   StringList uiLanguages;
   uiLanguages.push_back(String::fromLatin1(min.name()));
   if (id.m_scriptId) {
      id.m_scriptId = 0;
      if (id != min && id.withLikelySubtagsAdded() == max) {
         uiLanguages.push_back(String::fromLatin1(id.name()));
      }
   }
   if (max != min && max != id) {
      uiLanguages.push_back(String::fromLatin1(max.name()));
   }
   return uiLanguages;
}

String Locale::getNativeLanguageName() const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(SystemLocale::QueryType::NativeLanguageName, std::any());
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   return internal::get_locale_data(internal::sg_endonymsData + m_implPtr->m_data->m_languageEndonymIdx,
                                    m_implPtr->m_data->m_languageEndonymSize);
}

String Locale::getNativeCountryName() const
{
#ifndef PDK_NO_SYSTEMLOCALE
   if (m_implPtr->m_data == internal::system_data()) {
      std::any res = internal::system_locale()->query(SystemLocale::QueryType::NativeCountryName, std::any());
      if (res.has_value()) {
         return std::any_cast<String>(res);
      }
   }
#endif
   return internal::get_locale_data(internal::sg_endonymsData + m_implPtr->m_data->m_countryEndonymIdx, 
                                    m_implPtr->m_data->m_countryEndonymSize);
}

} // utils
} // pdk
