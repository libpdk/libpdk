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
using internal::LocaleData;
using internal::SystemLocale;

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
         //qWarning("Locale: This should never happen");
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


} // utils
} // pdk
