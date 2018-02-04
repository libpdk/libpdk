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

#include "pdk/global/Global.h"
#include "pdk/global/PlatformDefs.h"
#include "pdk/global/GlobalStatic.h"
#include "pdk/global/internal/NumericPrivate.h"
#include "pdk/kernel/HashFuncs.h"
#include "pdk/base/lang/String.h"
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
using pdk::utils::SharedDataPointer;

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
} // utils
} // pdk
