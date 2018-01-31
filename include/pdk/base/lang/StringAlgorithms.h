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

#ifndef PDK_M_BASE_LANG_STRING_ALGORITHMNS_H
#define PDK_M_BASE_LANG_STRING_ALGORITHMNS_H

#include "pdk/global/Global.h"
#include <vector>

namespace pdk {

// forwared declared class with namespace
namespace ds {
class ByteArray;
} // ds

namespace lang {

class Latin1String;
class StringView;
using pdk::ds::ByteArray;

namespace stringprivate {

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
pdk::sizetype ustrlen(const char16_t *str) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
int compare_strings(StringView lhs, StringView rhs, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
int compare_strings(StringView lhs, Latin1String rhs, pdk::CaseSensitivity cs =  pdk::CaseSensitivity::Sensitive) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
int compare_strings(Latin1String lhs, StringView rhs, pdk::CaseSensitivity cs =  pdk::CaseSensitivity::Sensitive) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
int compare_strings(Latin1String lhs, Latin1String rhs, pdk::CaseSensitivity cs =  pdk::CaseSensitivity::Sensitive) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
bool starts_with(StringView haystack, StringView needle, pdk::CaseSensitivity cs =  pdk::CaseSensitivity::Sensitive) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
bool starts_with(StringView haystack, Latin1String needle, pdk::CaseSensitivity cs =  pdk::CaseSensitivity::Sensitive) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
bool starts_with(Latin1String haystack, StringView needle, pdk::CaseSensitivity cs =  pdk::CaseSensitivity::Sensitive) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
bool starts_with(Latin1String haystack, Latin1String needle, pdk::CaseSensitivity cs =  pdk::CaseSensitivity::Sensitive) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
bool ends_with(StringView haystack, StringView needle, pdk::CaseSensitivity cs =  pdk::CaseSensitivity::Sensitive) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
bool ends_with(StringView haystack, Latin1String needle, pdk::CaseSensitivity cs =  pdk::CaseSensitivity::Sensitive) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
bool ends_with(Latin1String haystack, StringView needle, pdk::CaseSensitivity cs =  pdk::CaseSensitivity::Sensitive) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
bool ends_with(Latin1String haystack, Latin1String needle, pdk::CaseSensitivity cs =  pdk::CaseSensitivity::Sensitive) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
StringView trimmed(StringView str) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION 
Latin1String trimmed(Latin1String str) noexcept;

PDK_REQUIRED_RESULT PDK_CORE_EXPORT 
ByteArray convert_to_latin1(StringView str);

PDK_REQUIRED_RESULT PDK_CORE_EXPORT 
ByteArray convert_to_utf8(StringView str);

PDK_REQUIRED_RESULT PDK_CORE_EXPORT 
ByteArray convert_to_local_8bit(StringView str);

PDK_REQUIRED_RESULT PDK_CORE_EXPORT 
std::vector<uint> convert_to_ucs4(StringView str);

} // stringprivate
} // lang
} // pdk

#endif // PDK_M_BASE_LANG_STRING_ALGORITHMNS_H
