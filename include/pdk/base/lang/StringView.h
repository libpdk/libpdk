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

#ifndef PDK_M_BASE_LANG_STRING_VIEW_H
#define PDK_M_BASE_LANG_STRING_VIEW_H

#ifndef PDK_STRINGVIEW_LEVEL
#  define PDK_STRINGVIEW_LEVEL 1
#endif

#include "pdk/base/lang/Character.h"
#include "pdk/base/lang/StringLiteral.h"
#include "pdk/base/lang/StringAlgorithms.h"
#include "pdk/base/ds/ByteArray.h"
#include <cstddef>

namespace pdk {
namespace lang {

class String;
class StringRef;

namespace internal {

template <typename CharType>
struct IsCompatibleCharTypeHelper
      : std::integral_constant<bool,
      std::is_same<CharType, Character>::value ||
      std::is_same<CharType, ushort>::value ||
      #ifdef PDK_COMPILER_UNICODE_STRINGS
      std::is_same<CharType, char16_t>::value ||
      #endif
      (std::is_same<CharType, wchar_t>::value && sizeof(wchar_t) == sizeof(CharType))>
{};

template <typename CharType>
struct IsCompatibleCharType
      : IsCompatibleCharTypeHelper<typename std::remove_cv<typename std::remove_reference<CharType>::type>::type>
{};

template <typename Array>
struct IsCompatibleArrayHelper
      : std::false_type
{};

template <typename Char, size_t N>
struct IsCompatibleArrayHelper<Char[N]>
      : IsCompatibleCharType<Char>
{};

template <typename Array>
struct IsCompatibleArray
      : IsCompatibleArrayHelper<typename std::remove_cv<typename std::remove_reference<Array>::type>::type>
{};

template <typename Pointer>
struct IsCompatiblePointerHelper
      : std::false_type
{};

template <typename CharType>
struct IsCompatiblePointerHelper<CharType*>
      : IsCompatibleCharType<CharType>
{};

template <typename Pointer>
struct IsCompatiblePointer
      : IsCompatiblePointerHelper<typename std::remove_cv<typename std::remove_reference<Pointer>::type>::type>
{};

template <typename T>
struct IsCompatibleStdBasicStringHelper
      : std::false_type
{};

template <typename Char, typename...Args>
struct IsCompatibleStdBasicStringHelper<std::basic_string<Char, Args...> >
      : IsCompatibleCharType<Char> {};

template <typename T>
struct IsCompatibleStdBasicString
      : IsCompatibleStdBasicStringHelper<
      typename std::remove_cv<typename std::remove_reference<T>::type>::type
      >
{};

} // internal

class StringView
{
#if defined(PDK_OS_WIN) && !defined(PDK_COMPILER_UNICODE_STRINGS)
   using storage_type = wchar_t;
#else
   using storage_type = char16_t;
#endif
   typedef const Character value_type;
   typedef std::ptrdiff_t difference_type;
   typedef pdk::sizetype size_type;
   typedef value_type &reference;
   typedef value_type &const_reference;
   typedef value_type *pointer;
   typedef value_type *const_pointer;
   
   typedef pointer iterator;
   typedef const_pointer const_iterator;
   typedef std::reverse_iterator<iterator> reverse_iterator;
   typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
private:
   template <typename Char>
   using if_compatible_char = typename std::enable_if<internal::IsCompatibleCharType<Char>::value, bool>::type;
   
   template <typename Array>
   using if_compatible_array = typename std::enable_if<internal::IsCompatibleArray<Array>::value, bool>::type;
   
   template <typename Pointer>
   using if_compatible_pointer = typename std::enable_if<internal::IsCompatiblePointer<Pointer>::value, bool>::type;
   
   template <typename T>
   using if_compatible_string = typename std::enable_if<internal::IsCompatibleStdBasicString<T>::value, bool>::type;
   
   template <typename T>
   using if_compatible_qstring_like = typename std::enable_if<std::is_same<T, String>::value || std::is_same<T, StringRef>::value, bool>::type;
   
   template <typename CharType, size_t N>
   static constexpr pdk::sizetype lengthHelperArray(const CharType (&)[N]) noexcept
   {
      return pdk::sizetype(N - 1);
   }
   
   template <typename CharType>
   static pdk::sizetype lengthHelperPointer(const CharType *str) noexcept
   {
#if defined(PDK_CC_GNU) && !defined(PDK_CC_CLANG) && !defined(PDK_CC_INTEL)
      if (__builtin_constant_p(*str)) {
         pdk::sizetype result = 0;
         while (*str++)
            ++result;
         return result;
      }
#endif
      return stringprivate::ustrlen(reinterpret_cast<const char16_t *>(str));
   }
   
   static pdk::sizetype lengthHelperPointer(const Character *str) noexcept
   {
      return stringprivate::ustrlen(reinterpret_cast<const char16_t *>(str));
   }
   
   template <typename CharType>
   static const storage_type *castHelper(const CharType *str) noexcept
   {
      return reinterpret_cast<const storage_type*>(str);
   }
   
   static constexpr const storage_type *castHelper(const storage_type *str) noexcept
   {
      return str;
   }
   
public:
   constexpr StringView() noexcept
      : m_size(0),
        m_data(nullptr)
   {}
   
   constexpr StringView(std::nullptr_t) noexcept
      : StringView()
   {}
   
   template <typename Char, if_compatible_char<Char> = true>
   constexpr StringView(const Char *str, pdk::sizetype len)
      : m_size((PDK_ASSERT(len >= 0), PDK_ASSERT(str || !len), len)),
        m_data(castHelper(str)) 
   {}
   
   template <typename Char, if_compatible_char<Char> = true>
   constexpr StringView(const Char *f, const Char *l)
      : StringView(f, l - f)
   {}
   
   template <typename Array, if_compatible_array<Array> = true>
   constexpr StringView(const Array &str) noexcept
      : StringView(str, lengthHelperArray(str))
   {}
   
   template <typename Pointer, if_compatible_pointer<Pointer> = true>
   constexpr StringView(const Pointer &str) noexcept
      : StringView(str, str ? lengthHelperPointer(str) : 0)
   {}
   
   template <typename String, if_compatible_qstring_like<String> = true>
   StringView(const String &str) noexcept
      : StringView(str.isNull() ? nullptr : str.getRawData(), pdk::sizetype(str.size()))
   {}
   
   template <typename StdBasicString, if_compatible_string<StdBasicString> = true>
   StringView(const StdBasicString &str) noexcept
      : StringView(str.getRawData(), pdk::sizetype(str.size())) {}
   
   PDK_REQUIRED_RESULT inline String toString() const; // defined in qstring.h
   
   PDK_REQUIRED_RESULT constexpr pdk::sizetype size() const noexcept
   {
      return m_size;
   }
   
   PDK_REQUIRED_RESULT const_pointer data() const noexcept
   {
      return reinterpret_cast<const_pointer>(m_data);
   }
   
   PDK_REQUIRED_RESULT constexpr const storage_type *utf16() const noexcept
   {
      return m_data;
   }
   
   PDK_REQUIRED_RESULT constexpr Character operator[](pdk::sizetype n) const
   {
      return PDK_ASSERT(n >= 0), PDK_ASSERT(n < size()), Character(m_data[n]);
   }
   
   //
   // QString API
   //
   
   PDK_REQUIRED_RESULT ByteArray toLatin1() const
   {
      return stringprivate::convert_to_latin1(*this);
   }
   
   PDK_REQUIRED_RESULT ByteArray toUtf8() const
   {
      return stringprivate::convert_to_utf8(*this);
   }
   
   PDK_REQUIRED_RESULT ByteArray toLocal8Bit() const
   {
      return stringprivate::convert_to_local_8bit(*this);
   }
   
   PDK_REQUIRED_RESULT inline std::vector<uint> toUcs4() const; // defined in qvector.h
   
   PDK_REQUIRED_RESULT PDK_DECL_CONSTEXPR Character at(pdk::sizetype n) const
   {
      return (*this)[n];
   }
   
   PDK_REQUIRED_RESULT PDK_DECL_CONSTEXPR StringView mid(pdk::sizetype pos) const
   {
      return PDK_ASSERT(pos >= 0), PDK_ASSERT(pos <= size()), StringView(m_data + pos, m_size - pos);
   }
   
   PDK_REQUIRED_RESULT PDK_DECL_CONSTEXPR StringView mid(pdk::sizetype pos, pdk::sizetype n) const
   {
      return PDK_ASSERT(pos >= 0), PDK_ASSERT(n >= 0), PDK_ASSERT(pos + n <= size()), StringView(m_data + pos, n);
   }
   
   PDK_REQUIRED_RESULT PDK_DECL_CONSTEXPR StringView left(pdk::sizetype n) const
   {
      return PDK_ASSERT(n >= 0), PDK_ASSERT(n <= size()), StringView(m_data, n);
   }
   
   PDK_REQUIRED_RESULT PDK_DECL_CONSTEXPR StringView right(pdk::sizetype n) const
   {
      return PDK_ASSERT(n >= 0), PDK_ASSERT(n <= size()), StringView(m_data + m_size - n, n);
   }
   
   PDK_REQUIRED_RESULT PDK_DECL_CONSTEXPR StringView chopped(pdk::sizetype n) const
   {
      return PDK_ASSERT(n >= 0), PDK_ASSERT(n <= size()), StringView(m_data, m_size - n); 
   }
   
   PDK_DECL_RELAXED_CONSTEXPR void truncate(pdk::sizetype n)
   {
      PDK_ASSERT(n >= 0); PDK_ASSERT(n <= size()); m_size = n;
   }
   
   PDK_DECL_RELAXED_CONSTEXPR void chop(pdk::sizetype n)
   {
      PDK_ASSERT(n >= 0); PDK_ASSERT(n <= size()); m_size -= n;
   }
   
   PDK_REQUIRED_RESULT StringView trimmed() const noexcept
   {
      return stringprivate::trimmed(*this);
   }
   
   PDK_REQUIRED_RESULT bool startsWith(StringView s, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept
   {
      return stringprivate::starts_with(*this, s, cs);
   }
   
   PDK_REQUIRED_RESULT inline bool startsWith(Latin1String s, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept;
   PDK_REQUIRED_RESULT bool startsWith(Character c) const noexcept
   {
      return !empty() && front() == c;
   }
   
   PDK_REQUIRED_RESULT bool startsWith(Character c, pdk::CaseSensitivity cs) const noexcept
   {
      return stringprivate::starts_with(*this, StringView(&c, 1), cs);
   }
   
   PDK_REQUIRED_RESULT bool endsWith(StringView s, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept
   {
      return stringprivate::ends_with(*this, s, cs);
   }
   
   PDK_REQUIRED_RESULT inline bool endsWith(Latin1String s, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept;
   PDK_REQUIRED_RESULT bool endsWith(Character c) const noexcept
   { 
      return !empty() && back() == c;
   }
   
   PDK_REQUIRED_RESULT bool endsWith(Character c, pdk::CaseSensitivity cs) const noexcept
   {
      return stringprivate::ends_with(*this, StringView(&c, 1), cs);
   }
   
   //
   // STL compatibility API:
   //
   PDK_REQUIRED_RESULT const_iterator begin()   const noexcept
   {
      return data();
   }
   PDK_REQUIRED_RESULT const_iterator end()     const noexcept
   {
      return data() + size();
   }
   
   PDK_REQUIRED_RESULT const_iterator cbegin()  const noexcept
   {
      return begin();
   }
   
   PDK_REQUIRED_RESULT const_iterator cend()    const noexcept
   {
      return end();
   }
   
   PDK_REQUIRED_RESULT const_reverse_iterator rbegin()  const noexcept
   {
      return const_reverse_iterator(end());
   }
   
   PDK_REQUIRED_RESULT const_reverse_iterator rend()    const noexcept
   {
      return const_reverse_iterator(begin());
   }
   
   PDK_REQUIRED_RESULT const_reverse_iterator crbegin() const noexcept
   {
      return rbegin();
   }
   
   PDK_REQUIRED_RESULT const_reverse_iterator crend()   const noexcept
   {
      return rend();
   }
   
   PDK_REQUIRED_RESULT PDK_DECL_CONSTEXPR bool empty() const noexcept
   {
      return size() == 0;
   }
   
   PDK_REQUIRED_RESULT PDK_DECL_CONSTEXPR Character front() const
   {
      return PDK_ASSERT(!empty()), Character(m_data[0]);
   }
   
   PDK_REQUIRED_RESULT PDK_DECL_CONSTEXPR Character back()  const
   {
      return PDK_ASSERT(!empty()), Character(m_data[m_size - 1]);
   }
   
   //
   // pdk compatibility API:
   //
   PDK_REQUIRED_RESULT PDK_DECL_CONSTEXPR bool isNull() const noexcept
   {
      return !m_data;
   }
   
   PDK_REQUIRED_RESULT PDK_DECL_CONSTEXPR bool isEmpty() const noexcept
   {
      return empty();
   }
   
   PDK_REQUIRED_RESULT PDK_DECL_CONSTEXPR int length() const /* not nothrow! */
   {
      return static_cast<int>(size());
   }
   
   PDK_REQUIRED_RESULT PDK_DECL_CONSTEXPR Character first() const
   {
      return front();
   }
   
   PDK_REQUIRED_RESULT PDK_DECL_CONSTEXPR Character last()  const
   {
      return back();
   }
private:
   pdk::sizetype m_size;
   const storage_type *m_data;
};

template <typename StringLike, typename std::enable_if<
    std::is_same<StringLike, String>::value || std::is_same<StringLike, StringRef>::value,
    bool>::type = true>
inline StringView to_string_view_ignoring_null(const StringLike &s) noexcept
{
   return StringView(s.data(), s.size()); 
}

} // lang
} // pdk

PDK_DECLARE_TYPEINFO(pdk::lang::StringView, PDK_PRIMITIVE_TYPE);

#endif // PDK_M_BASE_LANG_STRING_VIEW_H
