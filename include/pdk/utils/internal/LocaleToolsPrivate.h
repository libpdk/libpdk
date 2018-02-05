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
// Created by softboy on 2018/02/03.

#ifndef PDK_UTILS_INTERNAL_LOCALE_TOOLS_PRIVATE_H
#define PDK_UTILS_INTERNAL_LOCALE_TOOLS_PRIVATE_H

#include "pdk/utils/internal/LocalePrivate.h"
#include "pdk/base/lang/String.h"
#include "pdk/global/SysInfo.h"

namespace pdk {
namespace utils {
namespace internal {

#if !defined(PDK_QLOCALE_NEEDS_VOLATILE)
#  if defined(PDK_CC_GNU)
#    if  __GNUC__ == 4
#      define PDK_QLOCALE_NEEDS_VOLATILE
#    elif defined(PDK_OS_WIN)
#      define PDK_QLOCALE_NEEDS_VOLATILE
#    endif
#  endif
#endif

#if defined(PDK_QLOCALE_NEEDS_VOLATILE)
#   define NEEDS_VOLATILE volatile
#else
#   define NEEDS_VOLATILE
#endif

enum class TrailingJunkMode 
{
   TrailingJunkProhibited,
   TrailingJunkAllowed
};

double ascii_to_double(const char *num, int numLen, bool &ok, int &processed,
                       TrailingJunkMode trailingJunkMode = TrailingJunkMode::TrailingJunkProhibited);
void double_to_ascii(double d, LocaleData::DoubleForm form, int precision, char *buf, int bufSize,
                     bool &sign, int &length, int &decpt);

String pdk_ulltoa(pdk::pulonglong l, int base, const Character _zero);
String pdk_lltoa(pdk::plonglong l, int base, const Character zero);
PDK_CORE_EXPORT String pdk_dtoa(double d, int *decpt, int *sign);

enum class PrecisionMode {
   PMDecimalDigits =             0x01,
   PMSignificantDigits =   0x02,
   PMChopTrailingZeros =   0x03
};

String &decimal_form(Character zero, Character decimal, Character group,
                     String &digits, int decpt, int precision,
                     PrecisionMode pm,
                     bool always_show_decpt,
                     bool thousands_group);
String &exponent_form(Character zero, Character decimal, Character exponential,
                      Character group, Character plus, Character minus,
                      String &digits, int decpt, int precision,
                      PrecisionMode pm,
                      bool always_show_decpt,
                      bool leading_zero_in_exponent);

inline bool is_zero(double d)
{
   uchar *ch = (uchar *)&d;
   if (pdk::SysInfo::ByteOrder == pdk::SysInfo::BigEndian) {
      return !(ch[0] & 0x7F || ch[1] || ch[2] || ch[3] || ch[4] || ch[5] || ch[6] || ch[7]);
   } else {
      return !(ch[7] & 0x7F || ch[6] || ch[5] || ch[4] || ch[3] || ch[2] || ch[1] || ch[0]);
   }
}

PDK_CORE_EXPORT double pdk_strtod(const char *s00, char const **se, bool *ok);
PDK_CORE_EXPORT double pdk_strntod(const char *s00, int len, char const **se, bool *ok);
pdk::plonglong pdk_strtoll(const char *nptr, const char **endptr, int base, bool *ok);
pdk::pulonglong pdk_strtoull(const char *nptr, const char **endptr, int base, bool *ok);

} // internal
} // utils
} // pdk

#endif // PDK_UTILS_INTERNAL_LOCALE_TOOLS_PRIVATE_H
