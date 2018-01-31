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
   const unsigned char *c = sg_languageCodeList;
   for (; *c != 0; c += 3) {
      if (uc1 == c[0] && uc2 == c[1] && uc3 == c[2]) {
         return Locale::Language((c - sg_languageCodeList)/3);
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

} // internal

} // utils
} // pdk
