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

#ifndef PDK_UTILS_INTERNAL_LOCALE_DATA_PRIVATE_H
#define PDK_UTILS_INTERNAL_LOCALE_DATA_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/utils/Locale.h"
#include "pdk/utils/internal/LocalePrivate.h"

namespace pdk {
namespace utils {
namespace internal {


/* This part of the file isn't generated, but written by hand since
 * Unicode CLDR doesn't contain measurement system information.
 */
struct CountryLanguage
{
   pdk::puint16 languageId;
   pdk::puint16 countryId;
   Locale::MeasurementSystem system;
};

static const CountryLanguage ImperialMeasurementSystems[] = 
{
   { Locale::Language::English, Locale::Country::UnitedStates, Locale::MeasurementSystem::ImperialUSSystem },
   { Locale::Language::English, Locale::Country::UnitedStatesMinorOutlyingIslands, Locale::MeasurementSystem::ImperialUSSystem },
   { Locale::Language::Spanish, Locale::Country::UnitedStates, Locale::MeasurementSystem::ImperialUSSystem },
   { Locale::Language::Hawaiian, Locale::Country::UnitedStates, Locale::MeasurementSystem::ImperialUSSystem },
   { Locale::Language::English, Locale::Country::UnitedKingdom, Locale::MeasurementSystem::ImperialUKSystem }
};

static const int ImperialMeasurementSystemsCount =
      sizeof(ImperialMeasurementSystems)/sizeof(ImperialMeasurementSystems[0]);

// GENERATED PART STARTS HERE

// This part of the file was generated on 2016-03-19 from the
// Common Locale Data Repository v29
// http://www.unicode.org/cldr/
// Do not change it, instead edit CLDR data and regenerate this file using
// cldr2qlocalexml.py and qlocalexml2cpp.py.


} // internal
} // utils
} // pdk

#endif // PDK_UTILS_INTERNAL_LOCALE_DATA_PRIVATE_H
