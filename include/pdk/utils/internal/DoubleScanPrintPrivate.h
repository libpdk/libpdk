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

#ifndef PDK_UTILS_INTERNAL_DOUBLE_SCAN_PRINT_PRIVATE_H
#define PDK_UTILS_INTERNAL_DOUBLE_SCAN_PRINT_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/global/GlobalStatic.h"

#if defined(PDK_CC_MSVC)
#  include <stdio.h>
#  include <locale.h>

namespace pdk {
namespace utils {
namespace internal {

// We can always use _sscanf_l and _snprintf_l on MSVC as those were introduced in 2005.

// MSVC doesn't document what it will do with a NULL locale passed to _sscanf_l or _snprintf_l.
// The documentation for _create_locale() does not formally document "C" to be valid, but an example
// code snippet in the same documentation shows it.

struct CLocaleT 
{
   CLocaleT() : m_locale(_create_locale(LC_ALL, "C"))
   {
   }
   
   ~CLocaleT()
   {
      _free_locale(m_locale);
   }
   
   const _locale_t m_locale;
};

#  define PDK_CLOCALE_HOLDER PDK_GLOBAL_STATIC(::pdk::utils::internal::CLocaleT, sg_cLocaleT)
#  define PDK_CLOCALE sg_cLocaleT()->m_locale

inline int pdk_double_sscanf(const char *buf, _locale_t locale, const char *format, double *d,
                             int *processed)
{
   return _sscanf_l(buf, format, locale, d, processed);
}

inline int pdk_double_snprintf(char *buf, size_t buflen, _locale_t locale, const char *format, double d)
{
   return _snprintf_l(buf, buflen, format, locale, d);
}

} // internal
} // utils
} // pdk

#else
#  include <stdio.h>

namespace pdk {
namespace utils {
namespace internal {

// When bootstrapping we don't have libdouble-conversion available, yet. We can also not use locale
// aware snprintf and sscanf variants in the general case because those are only available on select
// platforms. We can use the regular snprintf and sscanf because we don't do setlocale(3) when
// bootstrapping and the locale is always "C" then.

#  define PDK_CLOCALE_HOLDER
#  define PDK_CLOCALE 0

inline int pdk_double_sscanf(const char *buf, int, const char *format, double *d, int *processed)
{
   return sscanf(buf, format, d, processed);
}
inline int pdk_double_snprintf(char *buf, size_t buflen, int, const char *format, double d)
{
   return snprintf(buf, buflen, format, d);
}

} // internal
} // utils
} // pdk

#endif // defined(PDK_CC_MSVC)

#endif // PDK_UTILS_INTERNAL_DOUBLE_SCAN_PRINT_PRIVATE_H
