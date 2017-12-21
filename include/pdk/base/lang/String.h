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
#include "pdk/base/ds/ByteArray.h"
#include "pdk/utils/RefCount.h"

#include <stdarg.h>
#include <string>
#include <iterator>
#include <vector>

#ifdef truncate
#error String.h must be included before any header file that defines truncate
#endif

#ifdef PDK_OS_DARWIN
PDK_FORWARD_DECLARE_CF_TYPE(CFString);
PDK_FORWARD_DECLARE_OBJC_CLASS(NSString);
#endif

namespace pdk {
namespace lang {

using pdk::ds::ByteArray;
using pdk::ds::internal::TypedArrayData;
using pdk::ds::internal::ArrayData;

class CharacterRef;
class String;
class StringRef;
class StringList;

class Latin1String
{
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
   
   constexpr Latin1Character at(int i) const
   {
      return Latin1Character(m_data[i]);
   }
   
   constexpr Latin1Character operator[](int i) const
   {
      return at(i);
   }
   
   constexpr Latin1String substring(int pos) const
   {
      return Latin1String(m_data + pos, m_size - pos);
   }
   
   constexpr Latin1String substring(int pos, int n) const
   {
      return Latin1String(m_data + pos, n);
   }
   
   constexpr Latin1String left(int n) const
   {
      return Latin1String(m_data, n);
   }
   
   constexpr Latin1String right(int n) const
   {
      return Latin1String(m_data + m_size - n, n);
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

using StringData = TypedArrayData<char16_t>;

#define PDK_UNICODE_LITERAL_II(str) u"" str
#define PDK_UNICODE_LITERAL(str) PDK_UNICODE_LITERAL_II(str)

#define StringLiteral(str) \
   ([]() -> pdk::lang::String { \
   enum { Size = sizeof(PDK_UNICODE_LITERAL(str))/2 - 1 }; \
   static const pdk::lang::StaticStringData<Size> stringLiteral = { \
   PDK_STATIC_STRING_DATA_HEADER_INITIALIZER(Size), \
   PDK_UNICODE_LITERAL(str) }; \
   pdk::lang::StringDataPtr holder = { stringLiteral.getDataPtr() }; \
   const pdk::lang::String qstringLiteralTemp(holder); \
   return qstringLiteralTemp; \
}()) \
   /**/

#define PDK_STATIC_STRING_DATA_HEADER_INITIALIZER_WITH_OFFSET(size, offset) \
{ PDK_REFCOUNT_INITIALIZE_STATIC, size, 0, 0, offset }\
   /**/

#define Q_STATIC_STRING_DATA_HEADER_INITIALIZER(size) \
   PDK_STATIC_STRING_DATA_HEADER_INITIALIZER_WITH_OFFSET(size, sizeof(StringData))
/**/

template <int N>
struct StaticStringData
{
   ArrayData m_header;
   char16_t m_data[N + 1];
   StringData *getDataPtr() const
   {
      PDK_ASSERT(m_header.m_ref.isStatic());
      return const_cast<StringData *>(static_cast<const StringData *>(&m_header));
   }
};

struct StringDataPtr
{
   StringData *m_ptr;
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
   {
      
   }
   inline String(const Null &);
   inline String(const String &other) noexcept;
   String(int size, pdk::Initialization);
   constexpr inline String(StringDataPtr dataPtr) : m_data(dataPtr.m_ptr) {}
   inline ~String();
   
   String &operator =(const Null &);
   String &operator =(Character c);
   String &operator =(const String &other) noexcept;
   inline String &operator =(Latin1String other)
   {
      
   }
   
   template <int N>
   String &operator =(const char (&str)[N]);
   
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
      m_data == Data::getSharedNull();
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
   
   String section(Character separator, int start, int end = -1, SectionFlags flag = SectionFlag::Default);
   String section(const String &separator, int start, int end = -1, SectionFlags flag = SectionFlag::Default);
   
   String left(int n) const PDK_REQUIRED_RESULT;
   String right(int n) const PDK_REQUIRED_RESULT;
   String substring(int pos, int n = -1) const PDK_REQUIRED_RESULT;
   StringRef leftRef(int n) const PDK_REQUIRED_RESULT;
   StringRef rightRef(int n) const PDK_REQUIRED_RESULT;
   StringRef substringRef(int pos, int n = -1) const PDK_REQUIRED_RESULT;
   
   bool startsWith(const String &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool startsWith(const StringRef &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool startsWith(Latin1String needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool startsWith(Character needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   
   bool endsWith(const String &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool endsWith(const StringRef &needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool endsWith(Latin1String needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   bool endsWith(Character needle, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const;
   
   String leftJustified(int width, Character fill = Latin1Character(' '), bool truncate = false) const PDK_REQUIRED_RESULT;
   String rightJustified(int width, Character fill = Latin1Character(' '), bool truncate = false) const PDK_REQUIRED_RESULT;
   
#if defined(PDK_CC_GNU)
   // required due to https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61941
#  pragma push_macro("PDK_REQUIRED_RESULT")
#  undef PDK_REQUIRED_RESULT
#  define PDK_REQUIRED_RESULT
#  define PDK_REQUIRED_RESULT_PUSHED
#endif
   
   PDK_ALWAYS_INLINE String toLower() const & PDK_REQUIRED_RESULT
   {}
   
   PDK_ALWAYS_INLINE String toLower() && PDK_REQUIRED_RESULT
   {}
   
   PDK_ALWAYS_INLINE String toUpper() const & PDK_REQUIRED_RESULT
   {}
   
   PDK_ALWAYS_INLINE String toUpper() && PDK_REQUIRED_RESULT
   {}
   
   PDK_ALWAYS_INLINE String toCaseFolded() const & PDK_REQUIRED_RESULT
   {}
   
   PDK_ALWAYS_INLINE String toCaseFolded() && PDK_REQUIRED_RESULT
   {}
   
   PDK_ALWAYS_INLINE String trimmed() const & PDK_REQUIRED_RESULT
   {}
   
   PDK_ALWAYS_INLINE String trimmed() && PDK_REQUIRED_RESULT
   {}
   
   PDK_ALWAYS_INLINE String simplified() const & PDK_REQUIRED_RESULT
   {}
   
   PDK_ALWAYS_INLINE String simplified() && PDK_REQUIRED_RESULT
   {}
   
#ifdef PDK_REQUIRED_RESULT_PUSHED
#  pragma pop_macro("PDK_REQUIRED_RESULT")
#endif
   
   String &insert(int i, Character c);
   String &insert(int i, const Character *str, int length);
   inline String &insert(int i, const String &str)
   {}
   inline String &insert(int i, const StringRef &str);
   String &insert(int i, Latin1String str);
   
   String &append(Character c);
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
   
   String &remove(int i, int length);
   String &remove(Character c, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &remove(const String &str, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   
   String &replace(int i, int length, Character after);
   String &replace(int i, int length, const Character *after, int alength);
   String &replace(int i, int length, const String &after);
   String &replace(Character before, Character after, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &replace(const Character *before, int blength, const Character *after, int alength, 
                   pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &replace(Latin1String before, Latin1String after, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &replace(Latin1String before, const String &after, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &replace(const String &before, Latin1String after, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &replace(const String &before, const String &after, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &replace(Character c, Latin1String after, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   String &replace(Character c, const String &after, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive);
   
   StringList split(const String &separator, SplitBehavior behavior = SplitBehavior::KeepEmptyParts,
                    pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const PDK_REQUIRED_RESULT;
   std::vector<String> splitRef(const String &separator, SplitBehavior behavior = SplitBehavior::KeepEmptyParts,
                                pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const PDK_REQUIRED_RESULT;
   StringList split(Character separator, SplitBehavior behavior = SplitBehavior::KeepEmptyParts,
                    pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const PDK_REQUIRED_RESULT;
   std::vector<String> splitRef(Character separator, SplitBehavior behavior = SplitBehavior::KeepEmptyParts,
                                pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const PDK_REQUIRED_RESULT;
   
   String normalized(NormalizationForm mode, Character::UnicodeVersion version = Character::UnicodeVersion::Unicode_Unassigned) const PDK_REQUIRED_RESULT;
   String repeated(int times) const PDK_REQUIRED_RESULT;
   const char16_t *utf16();
   
   ByteArray toLatin1() const & PDK_REQUIRED_RESULT;
   ByteArray toLatin1() const && PDK_REQUIRED_RESULT;
   
   ByteArray toUtf8() const & PDK_REQUIRED_RESULT;
   ByteArray toUtf8() const && PDK_REQUIRED_RESULT;
   
   ByteArray toLocal8Bit() const & PDK_REQUIRED_RESULT;
   ByteArray toLocal8Bit() const && PDK_REQUIRED_RESULT;
   
   std::vector<char32_t> toUcs4() const PDK_REQUIRED_RESULT;
   
   static inline String fromLatin1(const char *str, int size = -1)
   {
      
   }
   
   static inline String fromUtf8(const char *str, int size = -1)
   {
      
   }
   
   static inline String fromLocal8Bit(const char *str, int size = -1)
   {
      
   }
   
   static inline String fromLatin1(const ByteArray &str)
   {
      
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
   static inline String fromWCharArray(const wchar_t *string, int size = -1) PDK_REQUIRED_RESULT;
   
   String &setRawData(const Character *unicode, int size);
   String &setUnicode(const Character *unicode, int size);
   inline String &setUtf16(const char16_t *utf16, int size);
   
   int compare(const String &str, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept;
   int compare(Latin1String str, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept;
   int compare(const StringRef &str, pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) const noexcept;
   
   static inline int compare(const String &lhs, const String &rhs,
                             pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   static inline int compare(const String &lhs, Latin1String rhs,
                             pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   static inline int compare(Latin1String lhs, const String &rhs,
                             pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   static inline int compare(const String &lhs, const StringRef &rhs,
                             pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   
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
   inline std::u32string toStd32String() const;
   
private:
   String &operator +=(const char *str);
   String &operator +=(const ByteArray &str);
   String(const char *str);
   String(const ByteArray &str);
   String &operator =(const char *str);
   String &operator =(const ByteArray &str);
   
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
   
   static int compareHelper(const Character *lhs, int lhsLength,
                            const Character *rhs, int rhsLength,
                            pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   static int compareHelper(const Character *lhs, int lhsLength,
                            const char *rhs, int rhsLength,
                            pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   static int compareHelper(const Character *lhs, int lhsLength, Latin1String rhs,
                            pdk::CaseSensitivity cs = pdk::CaseSensitivity::Sensitive) noexcept;
   
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
   void replaceHelper(uint *indices, int nIndices, int blength, const Character *after, int alength);
   
   friend class CharacterRef;
   friend class StringRef;
   friend class ByteArray;
   
public:
   static const Null sm_null;
   
private:
   Data *m_data;
};

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

inline int String::toWCharArray(wchar_t *array) const
{
   int length = size();
   if (sizeof(wchar_t) == sizeof(Character)) {
      std::memcpy(array, m_data->getData(), sizeof(Character) * length);
      return length;
   } else {
      return toUcs4Helper(m_data->getData(), length, reinterpret_cast<char32_t *>(array));
   }
}

inline String String::fromWCharArray(const wchar_t *string, int size)
{
   return sizeof(wchar_t) == sizeof(Character)
         ? fromUtf16(reinterpret_cast<const char16_t *>(string), size)
         : fromUcs4(reinterpret_cast<const char32_t *>(string), size);
}

} // lang
} // pdk

#endif // PDK_M_BASE_LANG_STRING_H
