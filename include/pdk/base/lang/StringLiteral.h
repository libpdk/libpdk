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

#ifndef PDK_M_BASE_LANG_STRING_LITERAL_H
#define PDK_M_BASE_LANG_STRING_LITERAL_H

#include "pdk/base/ds/internal/ArrayData.h"

namespace pdk {
namespace lang {

using pdk::ds::internal::TypedArrayData;
using pdk::ds::internal::ArrayData;

using StringData = TypedArrayData<ushort>;

#if defined(PDK_OS_WIN) && !defined(PDK_COMPILER_UNICODE_STRINGS)
// fall back to wchar_t if the a Windows compiler does not
// support Unicode string literals, assuming wchar_t is 2 bytes
// on that platform (sanity-checked by static_assert further below)

#if defined(PDK_CC_MSVC)
#    define PDK_UNICODE_LITERAL_II(str) L##str
#else
#    define PDK_UNICODE_LITERAL_II(str) L"" str
#endif
using punicodechar = wchar_t;

#else
// all our supported compilers support Unicode string literals,
// even if their PDK_COMPILER_UNICODE_STRING has been revoked due
// to lacking stdlib support. But pdk::lang::StringLiteral only needs the
// core language feature, so just use u"" here unconditionally:

#define PDK_UNICODE_LITERAL_II(str) u"" str
using punicodechar = char16_t;

#endif

PDK_STATIC_ASSERT_X(sizeof(punicodechar) == 2,
                    "punicodechar must typedef an integral type of size 2");

#define PDK_UNICODE_LITERAL(str) PDK_UNICODE_LITERAL_II(str)
#define StringLiteral(str) \
    ([]() noexcept -> ::pdk::lang::String { \
        enum { Size = sizeof(PDK_UNICODE_LITERAL(str))/2 - 1 }; \
        static const ::pdk::lang::StaticStringData<Size> stringLiteral = { \
            PDK_STATIC_STRING_DATA_HEADER_INITIALIZER(Size), \
            PDK_UNICODE_LITERAL(str) }; \
        ::pdk::lang::StringDataPtr holder = { stringLiteral.dataPtr() }; \
        const ::pdk::lang::String stringLiteralTemp(holder); \
        return stringLiteralTemp; \
    }()) \
    /**/

#define PDK_STATIC_STRING_DATA_HEADER_INITIALIZER_WITH_OFFSET(size, offset) \
    { PDK_REFCOUNT_INITIALIZE_STATIC, size, 0, 0, offset } \
    /**/

#define PDK_STATIC_STRING_DATA_HEADER_INITIALIZER(size) \
    PDK_STATIC_STRING_DATA_HEADER_INITIALIZER_WITH_OFFSET(size, sizeof(::pdk::lang::StringData)) \
    /**/

#ifndef PDK_NO_UNICODE_LITERAL
# ifndef PDK_UNICODE_LITERAL
#  error "If you change StringLiteral, please change StringViewLiteral, too"
# endif
# define StringViewLiteral(str) ::pdk::lang::StringView(PDK_UNICODE_LITERAL(str))
#endif

template <int N>
struct StaticStringData
{
   ArrayData m_str;
   punicodechar m_data[N + 1];
   
   StringData *dataPtr() const
   {
      PDK_ASSERT(m_str.m_ref.isStatic());
      return const_cast<StringData *>(static_cast<const StringData*>(&m_str));
   }
};

struct StringDataPtr
{
    StringData *m_ptr;
};

} // lang
} // pdk

#endif // PDK_M_BASE_LANG_STRING_LITERAL_H
