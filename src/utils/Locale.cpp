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
#include "pdk/kernel/HashFuncs.h"
#include "pdk/base/lang/String.h"
#include "pdk/utils/Locale.h"
#include "pdk/utils/internal/LocalePrivate.h"
#include "pdk/utils/internal/LocaleDataPrivate.h"
#include "pdk/base/time/Date.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/time/Time.h"

namespace pdk {
namespace utils {

using pdk::lang::Latin1String;

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

Locale::Language LocalePrivate::codeToLanguage(const lang::Character *code, int len) noexcept
{
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

Locale::Script LocalePrivate::codeToScript(const lang::Character *code, int len) noexcept
{
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

Locale::Country LocalePrivate::codeToCountry(const lang::Character *code, int len) noexcept
{
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

String LocalePrivate::languageToCode(Locale::Language language)
{
   if (language == Locale::Language::AnyLanguage) {
      return String();
   }
   if (language == Locale::Language::C) {
      return Latin1String("C");
   }
   const unsigned char *dptr = sg_languageCodeList + 3 * pdk::as_integer<Locale::Language>(language);
   String code(dptr[2] == 0 ? 2 : 3, pdk::Uninitialized);
   code[0] = static_cast<ushort>(dptr[0]);
   code[1] = static_cast<ushort>(dptr[1]);
   if (dptr[2] != 0) {
      code[2] = static_cast<ushort>(dptr[2]);
   }
   return code; 
}

String LocalePrivate::scriptToCode(Locale::Script script)
{
   
}

String LocalePrivate::countryToCode(Locale::Country country)
{
   
}

} // internal

} // utils
} // pdk
