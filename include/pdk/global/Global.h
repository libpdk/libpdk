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
// Created by softboy on 2017/11/13.

#ifndef PDK_GLOBAL_GLOBAL_H
#define PDK_GLOBAL_GLOBAL_H

#include <type_traits>
#include <cstddef>
#include <memory>
#include <algorithm>

#include "pdk/Version.h"
#include "pdk/Config.h"

#define PDK_STRINGIFY2(x) #x
#define PDK_STRINGIFY(x) PDK_STRINGIFY2(x)

#include "pdk/global/SystemDetection.h"
#include "pdk/global/ProcessorDetection.h"
#include "pdk/global/CompilerDetection.h"

#if defined(__ELF__)
#   define PDK_OF_ELF
#endif

#if defined(__MACH__) && defined(__APPLE__)
#   define PDK_OF_MACH_O
#endif

#if defined(PDK_SHARED) || !defined(PDK_STATIC)
#  ifdef PDK_STATIC
#     error "Both PDK_SHARED and PDK_STATIC defined, pelase check up your cmake options!"
#  endif
#  ifndef PDK_SHARED
#     define PDK_SHARED
#  endif
#  define PDK_CORE_EXPORT PDK_DECL_EXPORT
#  define PDK_CORE_IMPORT PDK_DECL_IMPORT
#endif

/*
   Avoid "unused parameter" warnings
*/
#define PDK_UNUSED(x) (void)x;

#if !defined(PDK_NO_DEBUG) && !defined(PDK_DEBUG)
#  define PDK_DEBUG
#endif

namespace pdk 
{

using pint8 = signed char; // 8 bit signed
using puint8 = unsigned char; // 8 bit unsigned
using pint16 = short;// 16 bit signed
using puint16 = unsigned short;// 16 bit unsigned
using pint32 = int;// 32 bit signed
using puint32 = unsigned int;// 32 bit unsigned

#if defined(PDK_OS_WIN) && !defined(PDK_CC_GNU)
#  define PDK_INT64_C(value) value ## i64 // signed 64 bit constant
#  define PDK_UINT64_C(value) value ## ui64 // unsigned 64 bit constant
using pint64 = __int64
using puint64 = unsigned __int64
#else
#  define PDK_INT64_C(value) static_cast<long long>(value ## LL) // signed 64 bit constant
#  define PDK_UINT64_C(value) static_cast<unsigned long long>(value ## ULL) // unsigned 64 bit constant
using pint64 = long long;// 64 bit signed
using puint64 = unsigned long long;// 64 bit unsigned
#endif

using plonglong = pint64;
using pulonglong = puint64;

inline void pdk_noop(void) {}

} // pdk

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;

namespace pdk 
{

#ifndef PDK_CC_MSVC
PDK_NORETURN
#endif
PDK_CORE_EXPORT void pdk_assert(const char *assertion, const char *file, 
                                int line) noexcept;

#if !defined(PDK_ASSERT)
#  if defined(PDK_NO_DEBUG) && !defined(PDK_FORCE_ASSERTS)
#     define PDK_ASSERT(cond) do { } while ((false) && (cond))
#  else
#     define PDK_ASSERT(cond) ((!(cond)) ? pdk_assert(#cond,__FILE__,__LINE__) : pdk_noop())
#  endif
#endif

#ifndef PDK_CC_MSVC
PDK_NORETURN
#endif
PDK_CORE_EXPORT void pdk_assert_x(const char *where, const char *what, 
                                  const char *file, int line) noexcept;

#if !defined(PDK_ASSERT_X)
#  if defined(PDK_NO_DEBUG) && !defined(PDK_FORCE_ASSERTS)
#     define PDK_ASSERT_X(cond, where, what) do {} while ((false) && (cond))
#  else
#     define PDK_ASSERT_X(cond, where, what) ((!(cond)) ? pdk_assert_x(where, what,__FILE__,__LINE__) : pdk_noop())
#  endif
#endif

#define PDK_STATIC_ASSERT(Condition) static_assert(bool(Condition), #Condition)
#define PDK_STATIC_ASSERT_X(Condition, Message) static_assert(bool(Condition), Message)

} // pdk

#include "pdk/global/Flags.h"

#endif // PDK_GLOBAL_GLOBAL_H
