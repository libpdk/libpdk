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
// Created by softboy on 2017/11/14.

#ifndef PDK_M_BASE_LANG_STRING_H
#define PDK_M_BASE_LANG_STRING_H

#include "pdk/global/Global.h"
#include "pdk/base/lang/Character.h"
#include "pdk/base/lang/StringLiteral.h"
#include "pdk/base/lang/StringView.h"
#include "pdk/base/lang/StringAlgorithms.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/utils/RefCount.h"

#include <cstdarg>
#include <string>
#include <iterator>
#include <stdarg.h>
#include <vector>
#include <list>

#ifdef truncate
#error String.h must be included before any header file that defines truncate
#endif

#ifdef PDK_OS_DARWIN
PDK_FORWARD_DECLARE_CF_TYPE(CFString);
PDK_FORWARD_DECLARE_OBJC_CLASS(NSString);
#endif

namespace pdk {

// forward declare class with namespace
namespace ds {
class StringList;
} // ds

namespace lang {

using pdk::ds::ByteArray;

class CharacterRef;
class String;
class StringRef;
using pdk::ds::StringList;

class Latin1String
{
public:
   using value_type = const char;
   using reference = value_type&;
   using const_reference = reference;
   using iterator = value_type*;
   using const_iterator = iterator;
   using difference_type = int; // violates Container concept requirements
   using size_type = int;       // violates Container concept requirements
   using reverse_iterator = std::reverse_iterator<iterator>;
   using const_reverse_iterator = reverse_iterator;
public:
   constexpr inline Latin1String() noexcept
      : m_size(0),
        m_data(nullptr)
   {}
   
   constexpr inline explicit Latin1String(const char *str) noexcept
      : m_size(str ? std::strlen(str) : 0),
        m_data(str)
   {}
   
   constexpr inline explicit Latin1String(const char *str, int size) noexcept
      : m_size(size),
        m_data(str)
   {}
   
   inline explicit Latin1String(const ByteArray &str) noexcept
      : m_size(pdk::strnlen(str.getConstRawData(), str.size())),
        m_data(str.getConstRawData())
   {}
   
   constexpr const char *latin1() const noexcept
   {
      return m_data;
   }
   
   constexpr int size() const noexcept
   {
      return m_size;
   }
   
   constexpr const char *getRawData() const noexcept
   {
      return m_data;
   }
   
   constexpr bool isNull() const noexcept
   {
      return !getRawData();
   }
   
   constexpr bool isEmpty() const noexcept
   {
      return !size();
   }
   
   constexpr Latin1Character at(int i) const
   {
      PDK_ASSERT(i >= 0);
      PDK_ASSERT(i < size());
      return Latin1Character(m_data[i]);
   }
   
   constexpr Latin1Character operator[](int i) const
   {
      return at(i);
   }
   
   PDK_REQUIRED_RESULT constexpr Latin1Character front() const
   {
      return at(0);
   }
   
   PDK_REQUIRED_RESULT constexpr Latin1Character back() const
   {
      return at(size() - 1);
   }
   
   PDK_REQUIRED_RESULT bool startsWith(Latin1String s, 
                                       pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept
   {
      //return QtPrivate::startsWith(*this, s, cs);
   }
   
   PDK_REQUIRED_RESULT constexpr bool startsWith(Character c) const noexcept
   {
      return !isEmpty() && front() == c;
   }
   
   PDK_REQUIRED_RESULT inline bool startsWith(Character c, pdk::CaseSensitivity cs) const noexcept
   {
      //return QtPrivate::startsWith(*this, StringView(&c, 1), cs);
   }
   
   PDK_REQUIRED_RESULT bool endsWith(Latin1String s, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept
   {
      //return QtPrivate::endsWith(*this, s, cs);
   }
   
   PDK_REQUIRED_RESULT constexpr bool endsWith(Character c) const noexcept
   {
      return !isEmpty() && back() == c;
   }
   
   PDK_REQUIRED_RESULT inline bool endsWith(Character c, pdk::CaseSensitivity cs) const noexcept
   {
      //return QtPrivate::endsWith(*this, StringView(&c, 1), cs);
   }
   
   constexpr const_iterator begin() const noexcept
   {
      return getRawData();
   }
   
   constexpr const_iterator cbegin() const noexcept
   {
      return getRawData();
   }
   
   constexpr const_iterator end() const noexcept
   {
      return getRawData() + size();
   }
   
   constexpr const_iterator cend() const noexcept
   {
      return getRawData() + size();
   }
   
   const_reverse_iterator rbegin() const noexcept
   {
      return const_reverse_iterator(end());
   }
   
   const_reverse_iterator crbegin() const noexcept
   {
      return const_reverse_iterator(end());
   }
   
   const_reverse_iterator rend() const noexcept
   {
      return const_reverse_iterator(begin());
   }
   
   const_reverse_iterator crend() const noexcept
   {
      return const_reverse_iterator(begin());
   }
   
   constexpr Latin1String substring(int pos) const
   {
      PDK_ASSERT(pos >= 0);
      PDK_ASSERT(pos <= size()); 
      return Latin1String(m_data + pos, m_size - pos);
   }
   
   constexpr Latin1String substring(int pos, int n) const
   {
      PDK_ASSERT(pos >= 0);
      PDK_ASSERT(n >= 0);
      PDK_ASSERT(pos + n <= size());
      return Latin1String(m_data + pos, n);
   }
   
   constexpr Latin1String left(int n) const
   {
      PDK_ASSERT(n >= 0);
      PDK_ASSERT(n <= size()); 
      return Latin1String(m_data, n);
   }
   
   constexpr Latin1String right(int n) const
   {
      PDK_ASSERT(n >= 0);
      PDK_ASSERT(n <= size()); 
      return Latin1String(m_data + m_size - n, n);
   }
   
   PDK_REQUIRED_RESULT constexpr Latin1String chopped(int n) const
   {
      PDK_ASSERT(n >= 0);
      PDK_ASSERT(n <= size()); 
      return Latin1String(m_data, m_size - n);
   }
   
   PDK_DECL_RELAXED_CONSTEXPR void chop(int n)
   {
      PDK_ASSERT(n >= 0);
      PDK_ASSERT(n <= size());
      m_size -= n;
   }
   
   PDK_DECL_RELAXED_CONSTEXPR void truncate(int n)
   {
      PDK_ASSERT(n >= 0);
      PDK_ASSERT(n <= size());
      m_size = n;
   }
   
   PDK_REQUIRED_RESULT Latin1String trimmed() const noexcept
   {
      //return QtPrivate::trimmed(*this);
   }
   
   inline bool operator==(const String &s) const noexcept;
   inline bool operator!=(const String &s) const noexcept;
   inline bool operator>(const String &s) const noexcept;
   inline bool operator<(const String &s) const noexcept;
   inline bool operator>=(const String &s) const noexcept;
   inline bool operator<=(const String &s) const noexcept;
   
private:
   int m_size;
   const char *m_data;
};

class PDK_CORE_EXPORT String
{
public:
   using Data = StringData;
   using DataPtr = Data *;
   
   using Iterator = Character *;
   using ConstIterator = const Character *;
   using ReverseIterator = std::reverse_iterator<Iterator>;
   using ConstReverseIterator = std::reverse_iterator<ConstIterator>;
   
   using iterator = Iterator;
   using const_iterator = ConstIterator;
   using reverse_iterator = ReverseIterator;
   using const_reverse_iterator = ConstReverseIterator;
   
   // STL compatibility
   using size_type = int;
   using difference_type = pdk::ptrdiff;
   using reference = Character &;
   using const_reference = const Character &;
   using pointer = Character *;
   using const_pointer = const Character *;
   using value_type = Character;
   
   // compatibility
   struct Null
   {};
   
   enum class SectionFlag
   {
      Default             = 0x00,
      SkipEmpty           = 0x01,
      IncludeLeadingSep   = 0x02,
      IncludeTrailingSep  = 0x04,
      CaseInsensitiveSeps = 0x08
   };
   PDK_DECLARE_FLAGS(SectionFlags, SectionFlag);
   
   enum class SplitBehavior
   {
      KeepEmptyParts, 
      SkipEmptyParts
   };
   
   enum class NormalizationForm
   {
      Form_D,
      Form_C,
      Form_KD,
      Form_KC
   };
   
public:
   inline String() noexcept;
   explicit String(const Character *unicode, int size = -1);
   String(Character c);
   String(int size, Character c);
   inline String(Latin1String other);
   template <int N>
   inline String(const char (&str)[N])
      : m_data(fromAsciiHelper(str, N - 1))
   {}
   
   inline String(const String &other) noexcept;
   
   String(const char *str) = delete;
   String(const ByteArray &str) = delete;
   
   
   String(int size, pdk::Initialization);
   constexpr inline String(StringDataPtr dataPtr)
      : m_data(dataPtr.m_ptr)
   {}
   
   inline ~String();
   
   String &operator =(const char *str) = delete;
   String &operator =(const ByteArray &str) = delete;
   
   String &operator =(Character c);
   String &operator =(const String &other) noexcept;
   String &operator =(Latin1String other);
   
   template <int N>
   inline String &operator =(const char (&str)[N])
   {
      return (*this = fromUtf8(str, N - 1));
   }
   
   inline String(String &&other) noexcept
      : m_data(other.m_data)
   {
      other.m_data = Data::getSharedNull();
   }
   
   inline String &operator =(String &&other) noexcept
   {
      std::swap(m_data, other.m_data);
      return *this;
   }
   
   String &operator +=(const char *str) = delete;
   String &operator +=(const ByteArray &str) = delete;
   
   
   inline void swap(String &other) noexcept
   {
      std::swap(m_data, other.m_data);
   }
   
   inline int size() const
   {
      return m_data->m_size;
   }
   
   inline bool isNull() const
   {
      return m_data == Data::getSharedNull();
   }
   
   bool isSimpleText() const;
   bool isRightToLeft() const;
   
   inline int count() const
   {
      return m_data->m_size;
   }
   
   inline int length() const;
   inline bool isEmpty() const;
   void resize(int size);
   void resize(int size, Character fillChar);
   
   String &fill(Character c, int size = -1);
   void truncate(int pos);
   void chop(int n);
   
   int capacity() const;
   inline void reserve(int size);
   inline void squeeze();
   
   inline const Character *unicode() const;
   inline Character *getRawData();
   inline const Character *getRawData() const;
   inline const Character *getConstRawData() const;
   
   inline void detach();
   inline bool isDetached() const;
   inline bool isSharedWith(const String &other) const
   {
      return m_data == other.m_data;
   }
   
   void clear();
   inline const Character at(int i) const;
   const Character operator[](int i) const;
   CharacterRef operator [](int i);
   const Character operator [](uint i) const;
   CharacterRef operator [](uint i);
   
   PDK_REQUIRED_RESULT String arg(pdk::plonglong a, int fieldwidth=0, int base=10,
                                  Character fillChar = Latin1Character(' ')) const;
   PDK_REQUIRED_RESULT String arg(pdk::pulonglong a, int fieldwidth=0, int base=10,
                                  Character fillChar = Latin1Character(' ')) const;
   PDK_REQUIRED_RESULT String arg(long a, int fieldwidth=0, int base=10,
                                  Character fillChar = Latin1Character(' ')) const;
   PDK_REQUIRED_RESULT String arg(ulong a, int fieldwidth=0, int base=10,
                                  Character fillChar = Latin1Character(' ')) const;
   PDK_REQUIRED_RESULT String arg(int a, int fieldWidth = 0, int base = 10,
                                  Character fillChar = Latin1Character(' ')) const;
   PDK_REQUIRED_RESULT String arg(uint a, int fieldWidth = 0, int base = 10,
                                  Character fillChar = Latin1Character(' ')) const;
   PDK_REQUIRED_RESULT String arg(short a, int fieldWidth = 0, int base = 10,
                                  Character fillChar = Latin1Character(' ')) const;
   PDK_REQUIRED_RESULT String arg(ushort a, int fieldWidth = 0, int base = 10,
                                  Character fillChar = Latin1Character(' ')) const;
   PDK_REQUIRED_RESULT String arg(double a, int fieldWidth = 0, char fmt = 'g', int prec = -1,
                                  Character fillChar = Latin1Character(' ')) const;
   PDK_REQUIRED_RESULT String arg(char a, int fieldWidth = 0,
                                  Character fillChar = Latin1Character(' ')) const;
   PDK_REQUIRED_RESULT String arg(Character a, int fieldWidth = 0,
                                  Character fillChar = Latin1Character(' ')) const;
#if PDK_STRINGVIEW_LEVEL < 2
   PDK_REQUIRED_RESULT String arg(const String &a, int fieldWidth = 0,
                                  Character fillChar = Latin1Character(' ')) const;
#endif
   PDK_REQUIRED_RESULT String arg(StringView a, int fieldWidth = 0,
                                  Character fillChar = Latin1Character(' ')) const;
   PDK_REQUIRED_RESULT String arg(Latin1String a, int fieldWidth = 0,
                                  Character fillChar = Latin1Character(' ')) const;
   PDK_REQUIRED_RESULT String arg(const String &a1, const String &a2) const;
   PDK_REQUIRED_RESULT String arg(const String &a1, const String &a2, const String &a3) const;
   PDK_REQUIRED_RESULT String arg(const String &a1, const String &a2, const String &a3,
                                  const String &a4) const;
   PDK_REQUIRED_RESULT String arg(const String &a1, const String &a2, const String &a3,
                                  const String &a4, const String &a5) const;
   PDK_REQUIRED_RESULT String arg(const String &a1, const String &a2, const String &a3,
                                  const String &a4, const String &a5, const String &a6) const;
   PDK_REQUIRED_RESULT String arg(const String &a1, const String &a2, const String &a3,
                                  const String &a4, const String &a5, const String &a6,
                                  const String &a7) const;
   PDK_REQUIRED_RESULT String arg(const String &a1, const String &a2, const String &a3,
                                  const String &a4, const String &a5, const String &a6,
                                  const String &a7, const String &a8) const;
   PDK_REQUIRED_RESULT String arg(const String &a1, const String &a2, const String &a3,
                                  const String &a4, const String &a5, const String &a6,
                                  const String &a7, const String &a8, const String &a9) const;
   
   String &vsprintf(const char *format, va_list ap) PDK_ATTRIBUTE_FORMAT_PRINTF(2, 0);
   String &sprintf(const char *format, ...) PDK_ATTRIBUTE_FORMAT_PRINTF(2, 3);
   static String vasprintf(const char *format, va_list ap) PDK_ATTRIBUTE_FORMAT_PRINTF(1, 0);
   static String asprintf(const char *format, ...) PDK_ATTRIBUTE_FORMAT_PRINTF(1, 2);
   
   int indexOf(Character needle, int from = 0, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int indexOf(const String &needle, int from = 0, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int indexOf(Latin1String needle, int from = 0, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int indexOf(const StringRef &needle, int from = 0, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   
   int lastIndexOf(Character needle, int from = -1, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int lastIndexOf(const String &needle, int from = -1, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int lastIndexOf(Latin1String needle, int from = -1, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int lastIndexOf(const StringRef &needle, int from = -1, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   
   inline bool contains(Character needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   inline bool contains(const String &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   inline bool contains(Latin1String needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   inline bool contains(const StringRef &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   
   int count(Character needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int count(const String &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int count(const StringRef &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   
   String section(Character separator, int start, int end = -1, SectionFlags flags = SectionFlag::Default) const;
   String section(const String &separator, int start, int end = -1, SectionFlags flags = SectionFlag::Default) const;
   
   PDK_REQUIRED_RESULT String left(int n) const;
   PDK_REQUIRED_RESULT String right(int n) const;
   PDK_REQUIRED_RESULT String substring(int pos, int n = -1) const;
   PDK_REQUIRED_RESULT StringRef leftRef(int n) const;
   PDK_REQUIRED_RESULT StringRef rightRef(int n) const;
   PDK_REQUIRED_RESULT StringRef substringRef(int pos, int n = -1) const;
#if PDK_STRINGVIEW_LEVEL < 2
   bool startsWith(const String &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool startsWith(const StringRef &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
#endif
   bool startsWith(Latin1String needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool startsWith(Character needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
#if PDK_STRINGVIEW_LEVEL < 2
   bool endsWith(const String &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool endsWith(const StringRef &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
#endif
   bool endsWith(Latin1String needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool endsWith(Character needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   
   PDK_REQUIRED_RESULT String leftJustified(int width, Character fill = Latin1Character(' '), bool truncate = false) const;
   PDK_REQUIRED_RESULT String rightJustified(int width, Character fill = Latin1Character(' '), bool truncate = false) const;
   
#if defined(PDK_CC_GNU)
   // required due to https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61941
#  pragma push_macro("PDK_REQUIRED_RESULT")
#  undef PDK_REQUIRED_RESULT
#  define PDK_REQUIRED_RESULT
#  define PDK_REQUIRED_RESULT_PUSHED
#endif
   
   PDK_REQUIRED_RESULT PDK_ALWAYS_INLINE String toLower() const &
   {}
   
   PDK_REQUIRED_RESULT PDK_ALWAYS_INLINE String toLower() &&
   {}
   
   PDK_REQUIRED_RESULT PDK_ALWAYS_INLINE String toUpper() const &
   {}
   
   PDK_REQUIRED_RESULT PDK_ALWAYS_INLINE String toUpper() &&
   {}
   
   PDK_REQUIRED_RESULT PDK_ALWAYS_INLINE String toCaseFolded() const &
   {}
   
   PDK_REQUIRED_RESULT PDK_ALWAYS_INLINE String toCaseFolded() &&
   {}
   
   PDK_REQUIRED_RESULT PDK_ALWAYS_INLINE String trimmed() const &
   {}
   
   PDK_REQUIRED_RESULT PDK_ALWAYS_INLINE String trimmed() &&
   {}
   
   PDK_REQUIRED_RESULT PDK_ALWAYS_INLINE String simplified() const &
   {}
   
   PDK_REQUIRED_RESULT PDK_ALWAYS_INLINE String simplified() &&
   {}
   
   PDK_REQUIRED_RESULT String toHtmlEscaped() const;
#ifdef PDK_REQUIRED_RESULT_PUSHED
#  pragma pop_macro("PDK_REQUIRED_RESULT")
#endif
   
   String &insert(int i, Character c);
   String &insert(int i, const Character *str, int length);
   inline String &insert(int i, const String &str)
   {}
   inline String &insert(int i, const StringRef &str);
   String &insert(int i, Latin1String str);
   
   String &append(Character ch);
   String &append(const Character *str, int length);
   String &append(const String &str);
   String &append(const StringRef &str);
   String &append(Latin1String str);
   
   inline String &prepend(Character c);
   inline String &prepend(const Character *str, int length);
   inline String &prepend(const String &str);
   inline String &prepend(const StringRef &str);
   inline String &prepend(Latin1String str);
   
   inline String &operator +=(Character c)
   {
      
   }
   
   inline String &operator +=(Character::SpecialCharacter c)
   {}
   
   inline String &operator +=(const String &str)
   {}
   
   inline String &operator +=(const StringRef &str)
   {}
   
   inline String &operator +=(Latin1String str)
   {}
   
   String &remove(int pos, int length);
   String &remove(Character c, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &remove(const String &str, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   
   String &replace(int pos, int length, Character after);
   String &replace(int pos, int length, const Character *after, int alength);
   String &replace(int pos, int length, const String &after);
   String &replace(Character before, Character after, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &replace(const Character *before, int blength, const Character *after, int alength, 
                   pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &replace(Latin1String before, Latin1String after, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &replace(Latin1String before, const String &after, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &replace(const String &before, Latin1String after, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &replace(const String &before, const String &after, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &replace(Character c, Latin1String after, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &replace(Character c, const String &after, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   
   PDK_REQUIRED_RESULT StringList split(const String &separator, SplitBehavior behavior = SplitBehavior::KeepEmptyParts,
                                        pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   PDK_REQUIRED_RESULT std::vector<StringRef> splitRef(const String &separator, SplitBehavior behavior = SplitBehavior::KeepEmptyParts,
                                                       pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   PDK_REQUIRED_RESULT StringList split(Character separator, SplitBehavior behavior = SplitBehavior::KeepEmptyParts,
                                        pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   PDK_REQUIRED_RESULT std::vector<StringRef> splitRef(Character separator, SplitBehavior behavior = SplitBehavior::KeepEmptyParts,
                                                       pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   
   PDK_REQUIRED_RESULT String normalized(NormalizationForm mode, 
                                         Character::UnicodeVersion version = Character::UnicodeVersion::Unicode_Unassigned) const;
   PDK_REQUIRED_RESULT String repeated(int times) const;
   const char16_t *utf16() const;
   
   PDK_REQUIRED_RESULT ByteArray toLatin1() const &;
   PDK_REQUIRED_RESULT ByteArray toLatin1() const &&;
   
   PDK_REQUIRED_RESULT ByteArray toUtf8() const &;
   PDK_REQUIRED_RESULT ByteArray toUtf8() const &&;
   
   PDK_REQUIRED_RESULT ByteArray toLocal8Bit() const &;
   PDK_REQUIRED_RESULT ByteArray toLocal8Bit() const &&;
   
   PDK_REQUIRED_RESULT std::vector<char32_t> toUcs4() const;
   
   static inline String fromLatin1(const char *str, int size = -1)
   {
      StringDataPtr dataPtr = { fromLatin1Helper(str, (str && size == -1) ? static_cast<int>(std::strlen(str)) : size) };
      return String(dataPtr);
   }
   
   static inline String fromUtf8(const char *str, int size = -1)
   {
      return fromUtf8Helper(str, (str && size == -1) ? static_cast<int>(std::strlen(str)) : size);
   }
   
   static inline String fromLocal8Bit(const char *str, int size = -1)
   {
      
   }
   
   static inline String fromLatin1(const ByteArray &str)
   {
      if (str.isNull()) {
         return String();
      }
      return fromLatin1(str.getRawData(), pdk::strnlen(str.getRawData(), str.size()));
   }
   
   static inline String fromUtf8(const ByteArray &str)
   {
      
   }
   
   static inline String fromLocal8Bit(const ByteArray &str)
   {
      
   }
   
   static String fromUtf16(const char16_t *str, int size = -1);
   static String fromUcs4(const char32_t *str, int size = -1);
   static String fromRawData(const Character *str, int size);
   
   static String fromUtf16(const uchar *str, int size = -1);
   static String fromUcs4(const uint *str, int size = -1);
   
   inline int toWCharArray(wchar_t *array) const;
   PDK_REQUIRED_RESULT static inline String fromWCharArray(const wchar_t *string, int size = -1);
   
   String &setRawData(const Character *unicode, int size);
   String &setUnicode(const Character *unicode, int size);
   inline String &setUtf16(const char16_t *utf16, int size);
   
   int compare(const String &str, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept;
   int compare(Latin1String str, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept;
   inline int compare(const StringRef &str, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept;
   
   static inline int compare(const String &lhs, const String &rhs,
                             pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   static inline int compare(const String &lhs, Latin1String rhs,
                             pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   static inline int compare(Latin1String lhs, const String &rhs,
                             pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   static inline int compare(const String &lhs, const StringRef &rhs,
                             pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   
   int localeAwareCompare(const String &str) const;
   int localeAwareCompare(const StringRef &str) const;
   static int localeAwareCompare(const StringRef &lhs, const String &rhs);
   static int localeAwareCompare(const StringRef &lhs, const StringRef &rhs);
   
   short  toShort(bool *ok = nullptr, int base = 10) const;
   ushort toUShort(bool *ok = nullptr, int base = 10) const;
   int toInt(bool *ok = nullptr, int base = 10) const;
   uint toUInt(bool *ok = nullptr, int base = 10) const;
   long toLong(bool *ok =nullptr, int base = 10) const;
   ulong toULong(bool *ok = nullptr, int base = 10) const;
   pdk::plonglong toLongLong(bool *ok = nullptr, int base = 10) const;
   pdk::pulonglong toULongLong(bool *ok = nullptr, int base = 10) const;
   float toFloat(bool *ok = nullptr) const;
   double toDouble(bool *ok = nullptr) const;
   
   String &setNum(short, int base=10);
   String &setNum(ushort, int base=10);
   String &setNum(int, int base=10);
   String &setNum(uint, int base=10);
   String &setNum(long, int base=10);
   String &setNum(ulong, int base=10);
   String &setNum(pdk::plonglong, int base=10);
   String &setNum(pdk::pulonglong, int base=10);
   String &setNum(float, char f='g', int prec=6);
   String &setNum(double, char f='g', int prec=6);
   
   static String number(int, int base=10);
   static String number(uint, int base=10);
   static String number(long, int base=10);
   static String number(ulong, int base=10);
   static String number(pdk::plonglong, int base=10);
   static String number(pdk::pulonglong, int base=10);
   static String number(double, char f='g', int prec=6);
   
   friend PDK_CORE_EXPORT bool operator ==(const String &lhs, const String &rhs) noexcept;
   friend PDK_CORE_EXPORT bool operator <(const String &lhs, const String &rhs) noexcept;
   
   friend inline bool operator >(const String &lhs, const String &rhs) noexcept
   {
      return rhs < lhs;
   }
   
   friend inline bool operator !=(const String &lhs, const String &rhs) noexcept
   {
      return !(lhs == rhs);
   }
   
   friend inline bool operator <=(const String &lhs, const String &rhs) noexcept
   {
      return !(lhs > rhs);
   }
   
   friend inline bool operator >=(const String &lhs, const String &rhs) noexcept
   {
      return !(lhs < rhs);
   }
   
   bool operator ==(Latin1String other) const noexcept;
   bool operator <(Latin1String other) const noexcept;
   bool operator >(Latin1String other) const noexcept;
   
   inline bool operator !=(Latin1String other) const noexcept
   {
      return !operator ==(other);
   }
   
   inline bool operator <=(Latin1String other) const noexcept
   {
      return !operator >(other);
   }
   
   inline bool operator >=(Latin1String other) const noexcept
   {
      return !operator <(other);
   }
   
   inline Iterator begin();
   inline ConstIterator begin() const;
   inline ConstIterator cbegin() const;
   inline ConstIterator constBegin() const;
   
   inline Iterator end();
   inline ConstIterator end() const;
   inline ConstIterator cend() const;
   inline ConstIterator constEnd() const;
   
   ReverseIterator rbegin()
   {
      return ReverseIterator(end());
   }
   
   ReverseIterator rend()
   {
      return ReverseIterator(begin());
   }
   
   ConstReverseIterator rbegin() const
   {
      return ConstReverseIterator(end());
   }
   
   ConstReverseIterator rend() const
   {
      return ConstReverseIterator(begin());
   }
   
   ConstReverseIterator crbegin() const
   {
      return ConstReverseIterator(end());
   }
   
   ConstReverseIterator crend() const
   {
      return ConstReverseIterator(begin());
   }
   
   // STL compatibility interfaces
   inline void push_back(Character c)
   {
      
   }
   
   inline void push_back(const String &str)
   {}
   
   inline void push_front(Character c)
   {}
   
   inline void push_front(const String &str)
   {}
   
   static inline String fromStdString(const std::string &str);
   inline std::string toStdString() const;
   static inline String fromStdWString(const std::wstring &str);
   inline std::wstring toStdWString() const;
   
   static inline String fromStdU16String(const std::u16string &str);
   inline std::u16string toStdU16String() const;
   static inline String fromStdU32String(const std::u32string &str);
   inline std::u32string toStdU32String() const;
   
private:
   friend inline bool operator ==(Character lhs, const String &rhs) noexcept;
   friend inline bool operator <(Character lhs, const String &rhs) noexcept;
   friend inline bool operator >(Character lhs, const String &rhs) noexcept;
   friend inline bool operator ==(Character lhs, const StringRef &rhs) noexcept;
   friend inline bool operator <(Character lhs, const StringRef &rhs) noexcept;
   friend inline bool operator >(Character lhs, const StringRef &rhs) noexcept;
   friend inline bool operator ==(Character lhs, Latin1String rhs) noexcept;
   friend inline bool operator <(Character lhs, Latin1String rhs) noexcept;
   friend inline bool operator >(Character lhs, Latin1String rhs) noexcept;
   
   void reallocData(uint alloc, bool grow = false);
   String multiArg(int numArgs, const String **args) const;
   static int compareHelper(const Character *lhs, int lhsLength,
                            const Character *rhs, int rhsLength,
                            pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   static int compareHelper(const Character *lhs, int lhsLength,
                            const char *rhs, int rhsLength,
                            pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   static int compareHelper(const Character *lhs, int lhsLength, Latin1String rhs,
                            pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   static int localeAwareCompareHelper(const Character *lhs, int lhsLength,
                                       const Character *rhs, int rhsLength);
   
   static String toLowerHelper(const String &str);
   static String toLowerHelper(String &str);
   static String toUpperHelper(const String &str);
   static String toUpperHelper(String &str);
   static String toCaseFoldedHelper(const String &str);
   static String toCaseFoldedHelper(String &str);
   static String trimmedHelper(const String &str);
   static String trimmedHelper(String &str);
   static String simplifiedHelper(const String &str);
   static String simplifiedHelper(String &str);
   static Data *fromLatin1Helper(const char *str, int size = -1);
   static Data *fromAsciiHelper(const char *str, int size = -1);
   static String fromUtf8Helper(const char *str, int size = -1);
   static String fromLocal8BitHelper(const char *str, int size = -1);
   
   static ByteArray toLatin1Helper(const String &str);
   static ByteArray toLatin1Helper(const Character *str, int size);
   static ByteArray toLatin1HelperInplace(String &str);
   static ByteArray toUtf8Helper(const String &str);
   static ByteArray toLocal8BitHelper(const Character *str, int size);
   static int toUcs4Helper(const char16_t *str, int length, char32_t *out);
   static pdk::plonglong toIntegralHelper(const Character *data, int len, bool *ok, int base);
   static pdk::pulonglong toIntegralHelper(const Character *data, uint len, bool *ok, int base);
   void replaceHelper(uint *indices, int nIndices, int blength, const Character *after, int alength);
   
   friend class CharacterRef;
   friend class StringRef;
   friend class ByteArray;
   
public:
   static const Null sm_null;
   
private:
   Data *m_data;
};

PDK_DECLARE_OPERATORS_FOR_FLAGS(String::SectionFlags)

inline bool Latin1String::operator ==(const String &str) const noexcept
{
   return str == *this;
}

inline bool Latin1String::operator !=(const String &str) const noexcept
{
   return str != *this;
}

inline bool Latin1String::operator >(const String &str) const noexcept
{
   return str < *this;
}

inline bool Latin1String::operator <(const String &str) const noexcept
{
   return str > *this;
}

inline bool Latin1String::operator <=(const String &str) const noexcept
{
   return str >= *this;
}

inline bool Latin1String::operator >=(const String &str) const noexcept
{
   return str <= *this;
}

inline String::String() noexcept
   : m_data(Data::getSharedNull())
{}

inline String::String(Latin1String other)
   : m_data(fromLatin1Helper(other.latin1(), other.size()))
{}

inline String::String(const String &other) noexcept
   : m_data(other.m_data)
{
   PDK_ASSERT(&other != this);
   m_data->m_ref.ref();
}

inline String::~String()
{
   if (!m_data->m_ref.deref()) {
      Data::deallocate(m_data);
   }
}

inline int String::length() const
{
   return m_data->m_size;
}

inline const Character String::at(int i) const
{
   PDK_ASSERT(static_cast<uint>(i) < static_cast<uint>(size()));
   return m_data->getData()[i];
}

inline const Character String::operator [](int i) const
{
   PDK_ASSERT(static_cast<uint>(i) < static_cast<uint>(size()));
   return m_data->getData()[i];
}

inline const Character String::operator [](uint i) const
{
   PDK_ASSERT(i < static_cast<uint>(size()));
   return m_data->getData()[i];
}

inline bool String::isEmpty() const
{
   return m_data->m_size == 0;
}

inline const Character *String::unicode() const
{
   return reinterpret_cast<const Character *>(m_data->getData());
}

inline Character *String::getRawData()
{
   return reinterpret_cast<Character *>(m_data->getData());
}

inline const Character *String::getRawData() const
{
   return reinterpret_cast<const Character *>(m_data->getData());
}

inline const Character *String::getConstRawData() const
{
   return reinterpret_cast<const Character *>(m_data->getData());
}

inline void String::detach()
{
   if (m_data->m_ref.isShared() || (m_data->m_offset != sizeof(StringData))) {
      reallocData(static_cast<uint>(m_data->m_size) + 1u);
   }
}

inline bool String::isDetached() const
{
   return !m_data->m_ref.isShared();
}

inline void String::clear()
{
   if (!isNull()) {
      *this = String();
   }
}

inline int String::capacity() const
{
   return m_data->m_alloc ? m_data->m_alloc - 1 : 0;
}

inline void String::reserve(int size)
{
   if (m_data->m_ref.isShared() || static_cast<uint>(size) >= m_data->m_alloc) {
      reallocData(std::max(size, m_data->m_size) + 1u);
   }
   if (!m_data->m_capacityReserved) {
      m_data->m_capacityReserved = true;
   }
}

inline void String::squeeze()
{
   if (m_data->m_ref.isShared() || static_cast<uint>(m_data->m_size) + 1u < m_data->m_alloc)
   {
      reallocData(static_cast<uint>(m_data->m_size) + 1u);
   }
   if (m_data->m_capacityReserved) {
      m_data->m_capacityReserved = false;
   }
}

//inline int String::toWCharArray(wchar_t *array) const
//{
//   int length = size();
//   if (sizeof(wchar_t) == sizeof(Character)) {
//      std::memcpy(array, m_data->getData(), sizeof(Character) * length);
//      return length;
//   } else {
//      return toUcs4Helper(m_data->getData(), length, reinterpret_cast<char32_t *>(array));
//   }
//}

inline String String::fromWCharArray(const wchar_t *string, int size)
{
   return sizeof(wchar_t) == sizeof(Character)
         ? fromUtf16(reinterpret_cast<const char16_t *>(string), size)
         : fromUcs4(reinterpret_cast<const char32_t *>(string), size);
}

inline String &String::setUtf16(const char16_t *utf16, int size)
{
   return setUnicode(reinterpret_cast<const Character *>(utf16), size);
}

inline String::iterator String::begin()
{
   detach();
   return reinterpret_cast<Character *>(m_data->getData());
}

inline String::const_iterator String::begin() const
{
   return reinterpret_cast<Character *>(m_data->getData());
}

inline String::const_iterator String::cbegin() const
{
   return reinterpret_cast<Character *>(m_data->getData());
}

inline String::const_iterator String::constBegin() const
{
   return reinterpret_cast<Character *>(m_data->getData());
}

inline String::iterator String::end()
{
   detach();
   return reinterpret_cast<Character *>(m_data->getData());
}

inline String::const_iterator String::end() const
{
   return reinterpret_cast<Character *>(m_data->getData());
}

inline String::const_iterator String::cend() const
{
   return reinterpret_cast<Character *>(m_data->getData());
}

inline String::const_iterator String::constEnd() const
{
   return reinterpret_cast<Character *>(m_data->getData());
}

inline bool String::contains(const String &needle, CaseSensitivity cs) const
{
   return indexOf(needle, 0, cs) != -1;
}

inline bool String::contains(const StringRef &needle, CaseSensitivity cs) const
{
   return indexOf(needle, 0, cs) != -1;
}

inline bool String::contains(Latin1String needle, CaseSensitivity cs) const
{
   return indexOf(needle, 0, cs) != -1;
}

inline bool String::contains(Character needle, CaseSensitivity cs) const
{
   return indexOf(needle, 0, cs) != -1;
}

inline std::string String::toStdString() const
{
   return toUtf8().toStdString();
}

inline String String::fromStdString(const std::string &str)
{
   return fromUtf8(str.data(), static_cast<int>(str.size()));
}

inline std::wstring String::toStdWString() const
{
   std::wstring str;
   str.resize(length());
   
#if defined(_MSC_VER) && _MSC_VER >= 1400
   // VS2005 crashes if the string is empty
   if (!length()) {
      return str;
   }
#endif
   str.resize(toWCharArray(&(*str.begin())));
   return str;
}

inline String String::fromStdWString(const std::wstring &str)
{
   return fromWCharArray(str.data(), static_cast<int>(str.size()));
}

inline String String::fromStdU16String(const std::u16string &str)
{
   return fromUtf16(str.data(), str.length());
}

inline std::u16string String::toStdU16String() const
{
   return std::u16string(reinterpret_cast<const char16_t *>(utf16()));
}

inline String String::fromStdU32String(const std::u32string &str)
{
   return fromUcs4(str.data(), str.size());
}

//inline std::u32string String::toStdU32String() const
//{
//   std::u32string u32Str(length(), char32_t(0));
//   int len = toUcs4Helper(m_data->getData(), length(), reinterpret_cast<char32_t *>(&u32Str[0]));
//   u32Str.resize(len);
//   return u32Str;
//}

inline bool operator ==(String::Null, String::Null)
{
   return true;
}

inline bool operator ==(String::Null, const String &rhs)
{
   return rhs.isNull();
}

inline bool operator ==(const String &lhs, String::Null)
{
   return lhs.isNull();
}

inline bool operator !=(String::Null, String::Null)
{
   return false;
}

inline bool operator !=(String::Null, const String &rhs)
{
   return !rhs.isNull();
}

inline bool operator !=(const String &lhs, String::Null)
{
   return !lhs.isNull();
}

inline bool operator ==(Latin1String lhs, Latin1String rhs) noexcept
{
   return lhs.size() == rhs.size() && (!lhs.size() || !std::memcmp(lhs.latin1(), rhs.latin1(), lhs.size()));
}

inline bool operator !=(Latin1String lhs, Latin1String rhs) noexcept
{
   return !operator ==(lhs, rhs);
}

inline bool operator <(Latin1String lhs, Latin1String rhs) noexcept
{
   const int length = std::min(lhs.size(), rhs.size());
   const int result = length ? std::memcmp(lhs.latin1(), rhs.latin1(), length) : 0;
   return result < 0 || (result == 0 && lhs.size() < rhs.size());
}

inline bool operator >(Latin1String lhs, Latin1String rhs) noexcept
{
   return operator <(rhs, lhs);
}

inline bool operator <=(Latin1String lhs, Latin1String rhs) noexcept
{
   return !operator >(lhs, rhs);
}

inline bool operator >=(Latin1String lhs, Latin1String rhs) noexcept
{
   return !operator <(lhs, rhs);
}

inline const String operator +(const String &lhs, const String &rhs)
{
   String result(lhs);
   result += rhs;
   return result;
}

inline const String operator +(const String &lhs, Character rhs)
{
   String result(lhs);
   result += rhs;
   return result;
}

inline const String operator +(Character lhs, const String &rhs)
{
   String result(lhs);
   result += rhs;
   return result;
}

inline bool operator ==(Character lhs, const String &rhs) noexcept
{
   return String::compareHelper(&lhs, 1, rhs.getRawData(), rhs.length()) == 0;
}

inline bool operator <(Character lhs, const String &rhs) noexcept
{
   return String::compareHelper(&lhs, 1, rhs.getRawData(), rhs.length()) < 0;
}

inline bool operator >(Character lhs, const String &rhs) noexcept
{
   return String::compareHelper(&lhs, 1, rhs.getRawData(), rhs.length()) > 0;
}

inline bool operator !=(Character lhs, const String &rhs) noexcept
{
   return !(lhs == rhs);
}

inline bool operator >=(Character lhs, const String &rhs) noexcept
{
   return !(lhs < rhs);
}

inline bool operator <=(Character lhs, const String &rhs) noexcept
{
   return !(lhs > rhs);
}

inline bool operator ==(const String &lhs, Character rhs) noexcept
{
   return rhs == lhs;
}

inline bool operator <(const String &lhs, Character rhs) noexcept
{
   return rhs > lhs;
}

inline bool operator >(const String &lhs, Character rhs) noexcept
{
   return rhs < lhs;
}

inline bool operator !=(const String &lhs, Character rhs) noexcept
{
   return !(rhs == lhs);
}

inline bool operator >=(const String &lhs, Character rhs) noexcept
{
   return !(rhs > lhs);
}

inline bool operator <=(const String &lhs, Character rhs) noexcept
{
   return !(rhs < rhs);
}

class PDK_CORE_EXPORT CharacterRef
{
public:
   inline operator Character() const
   {
      return m_idx < m_str.m_data->m_size ? m_str.m_data->getData()[m_idx] : 0;
   }
   
   inline CharacterRef &operator =(Character c)
   {
      if (m_idx > m_str.m_data->m_size) {
         m_str.resize(m_idx + 1, Latin1Character(' '));
      } else {
         m_str.detach();
      }
      m_str.m_data->getData()[m_idx] = c.unicode();
      return *this;
   }
   
   inline CharacterRef &operator =(const CharacterRef &c)
   {
      return operator =(Character(c));
   }
   
   inline CharacterRef &operator =(ushort c)
   {
      return operator =(Character(c));
   }
   
   inline CharacterRef &operator =(short c)
   {
      return operator =(Character(c));
   }
   
   inline CharacterRef &operator =(uint c)
   {
      return operator =(Character(c));
   }
   
   inline CharacterRef &operator =(int c)
   {
      return operator =(Character(c));
   }
   
   inline bool isNull() const
   {
      return Character(*this).isNull();
   }
   
   inline bool isPrint() const
   {
      return Character(*this).isPrintable();
   }
   
   inline bool isPunct() const
   {
      return Character(*this).isPunct();
   }
   
   inline bool isSpace() const
   {
      return Character(*this).isSpace();
   }
   
   inline bool isMark() const
   {
      return Character(*this).isMark();
   }
   
   inline bool isLetter() const
   {
      return Character(*this).isLetter();
   }
   
   inline bool isNumber() const
   {
      return Character(*this).isNumber();
   }
   
   inline bool isLetterOrNumber() const
   {
      return Character(*this).isLetterOrNumber();
   }
   
   inline bool isDigit() const
   {
      return Character(*this).isDigit();
   }
   
   inline bool isLower() const
   {
      return Character(*this).isLower();
   }
   
   inline bool isUpper() const
   {
      return Character(*this).isUpper();
   }
   
   inline bool isTitleCase() const
   {
      return Character(*this).isTitleCase();  
   }
   
   inline int getDigitValue() const
   {
      return Character(*this).getDigitValue();
   }
   
   inline Character toLower() const
   {
      return Character(*this).toLower();
   }
   
   inline Character toUpper() const
   {
      return Character(*this).toUpper();
   }
   
   inline Character toTitleCase() const
   {
      return Character(*this).toTitleCase();
   }
   
   inline Character::Category getCategory() const
   {
      return Character(*this).getCategory();
   }
   
   inline Character::Direction getDirection() const
   {
      return Character(*this).getDirection();
   }
   
   inline Character::JoiningType getJoinType() const
   {
      return Character(*this).getJoiningType();
   }
   
   inline bool hasMirrored() const
   {
      return Character(*this).hasMirrored();
   }
   
   inline Character getMirroredCharacter() const
   {
      return Character(*this).getMirroredCharacter();
   }
   
   inline String getDecomposition() const
   {
      return Character(*this).getDecomposition();
   }
   
   inline Character::Decomposition getDecompositionTag() const
   {
      return Character(*this).getDecompositionTag();
   }
   
   inline uchar getCombiningClass()
   {
      return Character(*this).getCombiningClass();
   }
   
   inline Character::Script getScript() const
   {
      return Character(*this).getScript();
   }
   
   inline Character::UnicodeVersion getUnicodeVersion() const
   {
      return Character(*this).getUnicodeVersion();
   }
   
   inline uchar getCell() const
   {
      return Character(*this).getCell();
   }
   
   inline uchar getRow() const
   {
      return Character(*this).getRow();
   }
   
   inline void setCell(uchar cell);
   inline void setRow(uchar row);
   
   char toLatin1() const
   { 
      return Character(*this).toLatin1();
   }
   char16_t unicode() const
   {
      return Character(*this).unicode();
   }
   
   char16_t &unicode() 
   {
      return m_str.getRawData()[m_idx].unicode();
   }
   
private:
   inline CharacterRef(String &str, int idx)
      : m_str(str),
        m_idx(idx)
   {}
   
private:
   friend class String;
   String &m_str;
   int m_idx;
};

inline void CharacterRef::setRow(uchar row)
{
   Character(*this).setRow(row);
}

inline void CharacterRef::setCell(uchar cell)
{
   Character(*this).setCell(cell);
}

inline CharacterRef String::operator [](int i)
{
   PDK_ASSERT(i >= 0);
   return CharacterRef(*this, i);
}

inline CharacterRef String::operator [](uint i)
{
   return CharacterRef(*this, i);
}

class PDK_CORE_EXPORT StringRef
{
public:
   using size_type = String::size_type;
   using value_type = String::value_type;
   using const_iterator = const Character *;
   using const_reverse_iterator = std::reverse_iterator<const_iterator>;
   using const_pointer = String::const_pointer;
   using const_reference = String::const_reference;
   
   inline constexpr StringRef()
      : m_str(nullptr),
        m_position(0),
        m_size(0)
   {}
   
   inline StringRef(const String *str, int position, int size);
   inline StringRef(const String *str);
   
   inline const String *getStr() const
   {
      return m_str;
   }
   
   inline int getPosition() const
   {
      return m_position;
   }
   
   inline int size() const
   {
      return m_size;
   }
   
   inline int count() const
   {
      return m_size;
   }
   
   inline int length() const
   {
      return m_size;
   }
   
   int indexOf(const String &str, int from = 0, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int indexOf(Character c, int from = 0, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int indexOf(Latin1String str, int from = 0, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int indexOf(const StringRef &str, int from = 0, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int lastIndexOf(const String &str, int from = -1, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int lastIndexOf(Character c, int from = -1, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int lastIndexOf(Latin1String str, int from = -1, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int lastIndexOf(const StringRef &str, int from = -1, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   
   inline bool contains(const String &str, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   inline bool contains(Character c, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   inline bool contains(Latin1String str, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   inline bool contains(const StringRef &str, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   
   int count(const String &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int count(Character needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   int count(const StringRef &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   
   std::vector<StringRef> split(const String &separator, String::SplitBehavior behavior = String::SplitBehavior::KeepEmptyParts,
                                pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const PDK_REQUIRED_RESULT;
   std::vector<StringRef> split(Character separator, String::SplitBehavior behavior = String::SplitBehavior::KeepEmptyParts,
                                pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const PDK_REQUIRED_RESULT;
   
   StringRef left(int n) const PDK_REQUIRED_RESULT;
   StringRef right(int n) const PDK_REQUIRED_RESULT;
   StringRef substring(int pos, int n = -1) const PDK_REQUIRED_RESULT;
   
   void truncate(int pos) noexcept
   {
      m_size = pdk::bound(0, pos, m_size);
   }
   
   void chop(int n) noexcept
   {
      if (n >= m_size) {
         m_size = 0;
      } else if (n > 0) {
         m_size -= n;
      }
   }
   
   bool isRightToLeft() const;
   
   bool startsWith(const String &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool startsWith(Latin1String needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool startsWith(Character needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool startsWith(const StringRef &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   
   bool endsWith(const String &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool endsWith(Latin1String needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool endsWith(Character needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool endsWith(const StringRef &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   
   inline StringRef &operator =(const String *str);
   inline const Character *unicode() const 
   {
      if (!m_str) {
         return reinterpret_cast<const Character *>(String::Data::getSharedNull()->getData());
      }
      return m_str->unicode() + m_position;
   }
   
   inline const Character *getRawData() const
   {
      return unicode();
   }
   
   inline const Character *getConstRawData() const
   {
      return unicode();
   }
   
   inline const_iterator begin() const
   {
      return unicode();
   }
   
   inline const_iterator cbegin() const
   {
      return unicode();
   }
   
   inline const_iterator end() const
   {
      return unicode() + size();
   }
   
   inline const_iterator cend() const
   {
      return unicode() + size();
   }
   
   inline const_reverse_iterator rbegin() const
   {
      return const_reverse_iterator(end());
   }
   
   inline const_reverse_iterator rcbegin() const
   {
      return rbegin();
   }
   
   inline const_reverse_iterator rend() const
   {
      return const_reverse_iterator(begin());
   }
   
   inline const_reverse_iterator rcend() const
   {
      return rend();
   }
   
   PDK_REQUIRED_RESULT ByteArray toLatin1() const;
   PDK_REQUIRED_RESULT ByteArray toUtf8() const;
   PDK_REQUIRED_RESULT ByteArray toLocal8Bit() const;
   PDK_REQUIRED_RESULT std::vector<char32_t> toUcs4() const;
   
   inline void clear()
   {
      m_str = nullptr;
      m_position = 0;
      m_size = 0;
   }
   
   String toString() const;
   
   inline bool isEmpty() const
   {
      return m_size == 0;
   }
   
   inline bool isNull() const
   {
      return m_str == nullptr || m_str->isNull();
   }
   
   StringRef appendTo(String *str) const;
   
   inline const Character at(int i) const
   {
      PDK_ASSERT(static_cast<uint>(i) < static_cast<uint>(size()));
      return m_str->at(i + m_position);
   }
   
   Character operator [](int i) const
   {
      return at(i);
   }
   
   int compare(const String &rhs, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept;
   int compare(const StringRef &rhs, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept;
   int compare(Latin1String rhs, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept;
   
   static int compare(const StringRef &lhs, const String &rhs, 
                      pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   static int compare(const StringRef &lhs, const StringRef &rhs, 
                      pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   static int compare(const StringRef &lhs, Latin1String rhs, 
                      pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   
   int localeAwareCompare(const String &other) const;
   int localeAwareCompare(const StringRef &other) const;
   static int localeAwareCompare(const StringRef &lhs, const String &rhs);
   static int localeAwareCompare(const StringRef &lhs, const StringRef &rhs);
   
   PDK_REQUIRED_RESULT StringRef trimmed() const;
   short  toShort(bool *ok = nullptr, int base = 10) const;
   ushort toUShort(bool *ok = nullptr, int base = 10) const;
   int toInt(bool *ok = nullptr, int base = 10) const;
   uint toUInt(bool *ok = nullptr, int base = 10) const;
   long toLong(bool *ok = nullptr, int base = 10) const;
   ulong toULong(bool *ok = nullptr, int base = 10) const;
   pdk::plonglong toLongLong(bool *ok = nullptr, int base = 10) const;
   pdk::pulonglong toULongLong(bool *ok = nullptr, int base = 10) const;
   float toFloat(bool *ok = nullptr) const;
   double toDouble(bool *ok = nullptr) const;
private:
   const String *m_str; 
   int m_position;
   int m_size;
};

inline StringRef::StringRef(const String *str, int position, int size)
   : m_str(str),
     m_position(position),
     m_size(size)
{}

inline StringRef::StringRef(const String *str)
   : m_str(str),
     m_position(0),
     m_size(str ? str->size() : 0)
{}

inline StringRef &StringRef::operator =(const String *str)
{
   m_str = str;
   m_position = 0;
   m_size = str ? str->size() : 0;
   return *this;
}

PDK_CORE_EXPORT bool operator ==(const StringRef &lhs, const StringRef &rhs) noexcept;
inline bool operator !=(const StringRef &lhs, const StringRef &rhs) noexcept
{
   return !(lhs == rhs);
}

PDK_CORE_EXPORT bool operator <(const StringRef &lhs, const StringRef &rhs) noexcept;
inline bool operator >(const StringRef &lhs, const StringRef &rhs) noexcept
{
   return rhs < lhs;
}

inline bool operator <=(const StringRef &lhs, const StringRef &rhs) noexcept
{
   return !(lhs > rhs);
}

inline bool operator >=(const StringRef &lhs, const StringRef &rhs) noexcept
{
   return !(lhs < rhs);
}

PDK_CORE_EXPORT bool operator ==(const String &lhs, const StringRef &rhs) noexcept;
inline bool operator !=(const String &lhs, const StringRef &rhs) noexcept
{
   return lhs.compare(rhs) != 0;   
}

inline bool operator <(const String &lhs, const StringRef &rhs) noexcept
{
   return lhs.compare(rhs) < 0;
}

inline bool operator >(const String &lhs, const StringRef &rhs) noexcept
{
   return lhs.compare(rhs) > 0;
}

inline bool operator <=(const String &lhs, const StringRef &rhs) noexcept
{
   return lhs.compare(rhs) <= 0;
}

inline bool operator >=(const String &lhs, const StringRef &rhs) noexcept
{
   return lhs.compare(rhs) >= 0;
}

inline bool operator ==(const StringRef &lhs, const String &rhs) noexcept
{
   return rhs == lhs;   
}

inline bool operator !=(const StringRef &lhs, const String &rhs) noexcept
{
   return !(rhs == lhs);   
}

inline bool operator <(const StringRef &lhs, const String &rhs) noexcept
{
   return rhs > lhs;   
}

inline bool operator >(const StringRef &lhs, const String &rhs) noexcept
{
   return rhs < lhs;   
}

inline bool operator <=(const StringRef &lhs, const String &rhs) noexcept
{
   return rhs >= lhs;   
}

inline bool operator >=(const StringRef &lhs, const String &rhs) noexcept
{
   return rhs <= lhs;
}


inline int String::compare(const StringRef &str, CaseSensitivity cs) const noexcept
{
   return String::compareHelper(getConstRawData(), length(), str.getConstRawData(), str.length(), cs);
}

inline int String::compare(const String &lhs, const StringRef &rhs, CaseSensitivity cs) noexcept
{
   return String::compareHelper(lhs.getConstRawData(), lhs.length(), rhs.getConstRawData(), rhs.length(), cs);
}

inline int StringRef::compare(const String &rhs, CaseSensitivity cs) const noexcept
{
   return String::compareHelper(getConstRawData(), length(), rhs.getConstRawData(), rhs.length(), cs);
}

inline int StringRef::compare(const StringRef &rhs, CaseSensitivity cs) const noexcept
{
   return String::compareHelper(getConstRawData(), length(), rhs.getConstRawData(), rhs.length(), cs);
}

inline int StringRef::compare(Latin1String rhs, CaseSensitivity cs) const noexcept
{
   return String::compareHelper(getConstRawData(), length(), rhs, cs);
}

inline int StringRef::compare(const StringRef &lhs, const String &rhs, CaseSensitivity cs) noexcept
{
   return String::compareHelper(lhs.getConstRawData(), lhs.length(), rhs.getConstRawData(), rhs.length(), cs);
}

inline int StringRef::compare(const StringRef &lhs, const StringRef &rhs, CaseSensitivity cs) noexcept
{
   return String::compareHelper(lhs.getConstRawData(), lhs.length(), rhs.getConstRawData(), rhs.length(), cs);
}

inline int StringRef::compare(const StringRef &lhs, Latin1String rhs, CaseSensitivity cs) noexcept
{
   return String::compareHelper(lhs.getConstRawData(), lhs.length(), rhs, cs);
}

PDK_CORE_EXPORT bool operator ==(Latin1String lhs, const StringRef &rhs) noexcept;
inline bool operator !=(Latin1String lhs, const StringRef &rhs) noexcept
{
   return rhs.compare(lhs) != 0;
}

inline bool operator <(Latin1String lhs, const StringRef &rhs) noexcept
{
   return rhs.compare(lhs) > 0;
}

inline bool operator >(Latin1String lhs, const StringRef &rhs) noexcept
{
   return rhs.compare(lhs) < 0;   
}

inline bool operator <=(Latin1String lhs, const StringRef &rhs) noexcept
{
   return rhs.compare(lhs) >= 0;
}

inline bool operator >=(Latin1String lhs, const StringRef &rhs) noexcept
{
   return rhs.compare(lhs) <= 0;
}

inline bool operator ==(const StringRef &lhs, Latin1String rhs) noexcept
{
   return rhs == lhs;
}

inline bool operator !=(const StringRef &lhs, Latin1String rhs) noexcept
{
   return !(rhs == lhs);
}

inline bool operator <(const StringRef &lhs, Latin1String rhs) noexcept
{
   return rhs > lhs;
}

inline bool operator >(const StringRef &lhs, Latin1String rhs) noexcept
{
   return rhs < lhs;
}

inline bool operator <=(const StringRef &lhs, Latin1String rhs) noexcept
{
   return rhs >= lhs;
}

inline bool operator >=(const StringRef &lhs, Latin1String rhs) noexcept
{
   return rhs <= lhs;
}

inline bool operator ==(Character lhs, const StringRef &rhs) noexcept
{
   return String::compareHelper(&lhs, 1, rhs.getRawData(), rhs.size()) == 0;
}

inline bool operator !=(Character lhs, const StringRef &rhs) noexcept
{
   return !(lhs == rhs);
}

inline bool operator <(Character lhs, const StringRef &rhs) noexcept
{
   return String::compareHelper(&lhs, 1, rhs.getRawData(), rhs.size()) < 0;
}

inline bool operator >(Character lhs, const StringRef &rhs) noexcept
{
   return String::compareHelper(&lhs, 1, rhs.getRawData(), rhs.size()) > 0;
}

inline bool operator <=(Character lhs, const StringRef &rhs) noexcept
{
   return !(lhs > rhs);
}

inline bool operator >=(Character lhs, const StringRef &rhs) noexcept
{
   return !(lhs < rhs);
}

inline bool operator ==(const StringRef &lhs, Character rhs) noexcept
{
   return rhs == lhs;
}

inline bool operator !=(const StringRef &lhs, Character rhs) noexcept
{
   return !(rhs == lhs);
}

inline bool operator <(const StringRef &lhs, Character rhs) noexcept
{
   return rhs > lhs;
}

inline bool operator >(const StringRef &lhs, Character rhs) noexcept
{
   return rhs < lhs;
}

inline bool operator <=(const StringRef &lhs, Character rhs) noexcept
{
   return !(rhs < lhs);
}

inline bool operator >=(const StringRef &lhs, Character rhs) noexcept
{
   return !(rhs > lhs);
}

inline bool operator ==(Character lhs, Latin1String rhs) noexcept
{
   return String::compareHelper(&lhs, 1, rhs) == 0;
}

inline bool operator !=(Character lhs, Latin1String rhs) noexcept
{
   return !(lhs == rhs);
}

inline bool operator <(Character lhs, Latin1String rhs) noexcept
{
   return String::compareHelper(&lhs, 1, rhs) < 0;
}

inline bool operator >(Character lhs, Latin1String rhs) noexcept
{
   return String::compareHelper(&lhs, 1, rhs) > 0;
}

inline bool operator <=(Character lhs, Latin1String rhs) noexcept
{
   return !(lhs > rhs);
}

inline bool operator >=(Character lhs, Latin1String rhs) noexcept
{
   return !(lhs < rhs);
}

inline bool operator ==(Latin1String lhs, Character rhs) noexcept
{
   return rhs == lhs;
}

inline bool operator !=(Latin1String lhs, Character rhs) noexcept
{
   return !(rhs == lhs);
}

inline bool operator <(Latin1String lhs, Character rhs) noexcept
{
   return rhs > lhs;
}

inline bool operator >(Latin1String lhs, Character rhs) noexcept
{
   return rhs < lhs;
}

inline bool operator <=(Latin1String lhs, Character rhs) noexcept
{
   return !(rhs < lhs);
}

inline bool operator >=(Latin1String lhs, Character rhs) noexcept
{
   return !(rhs > lhs);
}

inline bool StringRef::contains(const String &str, CaseSensitivity cs) const
{
   return indexOf(str, 0, cs) != -1;
}

inline bool StringRef::contains(Latin1String str, CaseSensitivity cs) const
{
   return indexOf(str, 0, cs) != -1;
}

inline bool StringRef::contains(Character c, CaseSensitivity cs) const
{
   return indexOf(c, 0, cs) != -1;
}

inline bool StringRef::contains(const StringRef &str, CaseSensitivity cs) const
{
   return indexOf(str, 0, cs) != -1;
}

inline String &String::insert(int i, const StringRef &str)
{
   return insert(i, str.getConstRawData(), str.length());
}

inline String operator +(const String &lhs, const StringRef &rhs)
{
   String result;
   result.reserve(lhs.size() + rhs.size());
   result += lhs;
   result += rhs; 
   return result;
}

inline String operator +(const StringRef &lhs, const String &rhs)
{
   String result;
   result.reserve(lhs.size() + rhs.size());
   result += lhs;
   result += rhs; 
   return result;
}

inline String operator +(const StringRef &lhs, Latin1String rhs)
{
   String result;
   result.reserve(lhs.size() + rhs.size());
   result += lhs;
   result += rhs;
   return result;
}

inline String operator +(Latin1String lhs, const StringRef &rhs)
{
   String result;
   result.reserve(lhs.size() + rhs.size());
   result += lhs;
   result += rhs;
   return result;
}

inline String operator +(const StringRef &lhs, const StringRef &rhs)
{
   String result;
   result.reserve(lhs.size() + rhs.size());
   result += lhs;
   result += rhs;
   return result;
}

inline String operator +(const StringRef &lhs, Character rhs)
{
   String result;
   result.reserve(lhs.size() + 1);
   result += lhs;
   result += rhs;
   return result;
}

inline String operator +(Character lhs, const StringRef &rhs)
{ 
   String result;
   result.reserve(1 + rhs.size());
   result += lhs;
   result += rhs;
   return result; 
}

namespace internal {

void utf16_from_latin1(char16_t *dest, const char *str, size_t size) noexcept;

} // internal

} // lang
} // pdk

PDK_DECLARE_TYPEINFO(pdk::lang::Latin1String, PDK_MOVABLE_TYPE);

#endif // PDK_M_BASE_LANG_STRING_H
