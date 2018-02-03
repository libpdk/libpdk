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

#include "pdk/utils/internal/LocaleToolsPrivate.h"
#include "pdk/utils/internal/LocalePrivate.h"
#include "pdk/base/lang/String.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#if defined(PDK_OS_LINUX) && !defined(__UCLIBC__)
#    include <fenv.h>
#endif

// Sizes as defined by the ISO C99 standard - fallback
#ifndef LLONG_MAX
#   define LLONG_MAX PDK_INT64_C(0x7fffffffffffffff)
#endif
#ifndef LLONG_MIN
#   define LLONG_MIN (-LLONG_MAX - PDK_INT64_C(1))
#endif
#ifndef ULLONG_MAX
#   define ULLONG_MAX PDK_UINT64_C(0xffffffffffffffff)
#endif


namespace pdk {
namespace utils {
namespace internal {

} // internal
} // utils
} // pdk
