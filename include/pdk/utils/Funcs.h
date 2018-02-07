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

#ifndef PDK_UTILS_FUNCS_H
#define PDK_UTILS_FUNCS_H

#include "pdk/global/Global.h"
#include <type_traits>

namespace pdk {

template <typename Enumeration>
constexpr auto as_integer(Enumeration const value)
-> typename std::underlying_type<Enumeration>::type
{
   return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

// forward declare class with namespace
namespace ds {
class ByteArray;
} // ds

// forward declare class with namespace
namespace lang {
class String;
} // lang

PDK_CORE_EXPORT ds::ByteArray pdk_getenv(const char *varName);
PDK_CORE_EXPORT lang::String pdk_env_var(const char *varName);
PDK_CORE_EXPORT lang::String pdk_env_var(const char *varName, const lang::String &defaultValue);
PDK_CORE_EXPORT bool pdk_putenv(const char *varName, const ds::ByteArray &value);
PDK_CORE_EXPORT bool pdk_unsetenv(const char *varName);

PDK_CORE_EXPORT bool env_var_is_empty(const char *varName) noexcept;
PDK_CORE_EXPORT bool env_var_isset(const char *varName) noexcept;
PDK_CORE_EXPORT int  env_var_intval(const char *varName, bool *ok = nullptr) noexcept;

} // pdk

#endif // PDK_UTILS_FUNCS_H
