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
// Created by softboy on 2018/02/27.

#include "pdk/utils/internal/CollatorPrivate.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/io/Debug.h"
#include "pdk/utils/Locale.h"
#include "pdk/utils/internal/LocalePrivate.h"
#include "pdk/base/ds/VarLengthArray.h"

#include <cstring>
#include <cwchar>

namespace pdk {
namespace utils {

using pdk::ds::VarLengthArray;
using pdk::lang::String;
using pdk::lang::Character;
using pdk::utils::Locale;

namespace internal {

void CollatorPrivate::init()
{
   if (m_locale != Locale()) {
      warning_stream("Only default locale supported with the posix collation implementation");
   }
   if (m_caseSensitivity != pdk::CaseSensitivity::Sensitive) {
      warning_stream("Case insensitive sorting unsupported in the posix collation implementation");
   }
   if (m_numericMode) {
      warning_stream("Numeric mode unsupported in the posix collation implementation");
   }
   if (m_ignorePunctuation) {
      warning_stream("Ignoring punctuation unsupported in the posix collation implementation");
   }
   m_dirty = false;
}

void CollatorPrivate::cleanup()
{}

} // internal

namespace {

void string_to_wchar_array(VarLengthArray<wchar_t> &ret, const String &string)
{
   ret.resize(string.length());
   int len = string.toWCharArray(ret.getRawData());
   ret.resize(len+1);
   ret[len] = 0;
}

} // anonymous namespace

int Collator::compare(const Character *s1, int len1, const Character *s2, int len2) const
{
   VarLengthArray<wchar_t> array1, array2;
   string_to_wchar_array(array1, String(s1, len1));
   string_to_wchar_array(array2, String(s2, len2));
   return std::wcscoll(array1.getConstRawData(), array2.getConstRawData());
}

int Collator::compare(const String &s1, const String &s2) const
{
   VarLengthArray<wchar_t> array1, array2;
   string_to_wchar_array(array1, s1);
   string_to_wchar_array(array2, s2);
   return std::wcscoll(array1.getConstRawData(), array2.getConstRawData());
}

int Collator::compare(const StringRef &s1, const StringRef &s2) const
{
   if (m_implPtr->m_dirty) {
      m_implPtr->init();
   }
   return compare(s1.getConstRawData(), s1.size(), s2.getConstRawData(), s2.size());
}

CollatorSortKey Collator::sortKey(const String &string) const
{
   if (m_implPtr->m_dirty) {
      m_implPtr->init();
   }
   VarLengthArray<wchar_t> original;
   string_to_wchar_array(original, string);
   std::vector<wchar_t> result(string.size());
   size_t size = std::wcsxfrm(result.data(), original.getConstRawData(), string.size());
   if (size > uint(result.size())) {
      result.resize(size+1);
      size = std::wcsxfrm(result.data(), original.getConstRawData(), string.size());
   }
   result.resize(size+1);
   result[size] = 0;
   return CollatorSortKey(new CollatorSortKeyPrivate(std::move(result)));
}

int CollatorSortKey::compare(const CollatorSortKey &otherKey) const
{
   return std::wcscmp(reinterpret_cast<const wchar_t *>(std::as_const(m_implPtr->m_key).data()),
                      reinterpret_cast<const wchar_t *>(std::as_const(otherKey.m_implPtr->m_key).data()));
}

} // utils
} // pdk
