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
// Created by softboy on 2017/12/15.

#ifndef PDK_M_BASE_DS_BYTE_ARRAY_H
#define PDK_M_BASE_DS_BYTE_ARRAY_H

#include <cstdlib>
#include <cstring>
#include <cstdarg>

#include <string>
#include <iterator>
#include <list>

#include "pdk/utils/RefCount.h"
#include "pdk/base/ds/internal/ArrayData.h"
#include "pdk/global/EnumDefs.h"
#include "pdk/kernel/StringUtils.h"

#ifdef truncate
#error ByteArray.h must be included before any header file that defines truncate
#endif

#if defined(PDK_OS_DARWIN)
PDK_FORWARD_DECLARE_CF_TYPE(CFData);
PDK_FORWARD_DECLARE_OBJC_CLASS(NSData);
#endif

namespace pdk {

// forward declare class with namespace
namespace lang {
class String;
} // ds

namespace ds {

using ByteArrayData = internal::ArrayData;
using internal::TypedArrayData;

template <int N>
struct StaticByteArrayData
{
   ByteArrayData m_header;
   char m_data[N + 1];
   
   ByteArrayData *getDataPtr() const
   {
      PDK_ASSERT(m_header.m_ref.isStatic());
      return const_cast<ByteArrayData *>(&m_header);
   }
};

struct ByteArrayDataPtr
{
   ByteArrayData *m_ptr; 
};

#define PDK_STATIC_BYTE_DATA_HEADER_INITIALIZER_WITH_OFFSET(size, offset) \
   PDK_STATIC_ARRAY_HEADER_INITIALIZER_WITH_OFFSET(size, offset)

#define PDK_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER(size) \
   PDK_STATIC_BYTE_DATA_HEADER_INITIALIZER_WITH_OFFSET(size, sizeof(pdk::ds::ByteArrayData))

#define ByteArrayLiteral(str) \
   ([]()-> pdk::ds::ByteArray {\
   enum { Size = sizeof(str) - 1 };\
   static const pdk::ds::StaticByteArrayData<Size> byteArrayLiteral = {\
   PDK_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER(Size),\
   str\
};\
   pdk::ds::ByteArrayDataPtr holder = { byteArrayLiteral.getDataPtr() };\
   const pdk::ds::ByteArray byteArray(holder);\
   return byteArray;\
}())

using pdk::lang::String;

class ByteRef;

class PDK_CORE_EXPORT ByteArray
{
private:
   using Data = TypedArrayData<char>;
   
public:
   using DataPtr = Data *;
   
public:
   enum Base64Option
   {
      Base64Encoding = 0,
      Base64UrlEncoding = 1,
      KeepTrailingEquals = 0,
      OmitTrailingEquals = 2
   };
   
   PDK_DECLARE_FLAGS(Base64Options, Base64Option);
   
   inline ByteArray() noexcept;
   ByteArray(const char *data, int size = -1);
   ByteArray(int size, char c);
   ByteArray(int size, pdk::Initialization);
   inline ByteArray(const ByteArray &data) noexcept;
   inline ByteArray(ByteArrayDataPtr dataPtr)
      : m_data(static_cast<Data *>(dataPtr.m_ptr))
   {}
   
   inline ~ByteArray();
   
   ByteArray &operator=(const ByteArray &other) noexcept;
   ByteArray &operator=(const char *str);
   
   inline ByteArray(ByteArray &&other) noexcept
      : m_data(other.m_data)
   {
      other.m_data = Data::getSharedNull();
   }
   
   inline ByteArray &operator =(ByteArray &&other) noexcept
   {
      std::swap(m_data, other.m_data);
      return *this;
   }
   
   inline void swap(ByteArray &other) noexcept
   {
      std::swap(m_data, other.m_data);
   }
   
   inline int size() const;
   inline bool isEmpty() const;
   void resize(int size);
   
   ByteArray &fill(char c, int size = -1);
   inline int capacity() const;
   inline void reserve(int size);
   inline void squeeze();
   
#ifndef PDK_NO_CAST_FROM_BYTEARRAY
   inline operator const char *() const;
   inline operator const void *() const;
#endif
   
   char *getRawData();
   const char *getRawData() const;
   inline const char *getConstRawData() const;
   inline void detach();
   inline bool isDetached() const;
   
   inline bool isSharedWith(const ByteArray &other) const;
   void clear();
   inline char at(int i) const;
   inline char operator [](int i) const;
   inline char operator [](uint i) const;
   inline ByteRef operator [](int i);
   inline ByteRef operator [](uint i);
   
   PDK_REQUIRED_RESULT char front() const
   {
      return at(0);
   }
   
   PDK_REQUIRED_RESULT inline ByteRef front();
   PDK_REQUIRED_RESULT char back() const {
      return at(size() - 1);
   }
   
   PDK_REQUIRED_RESULT inline ByteRef back();
   
   int indexOf(char needle, int from = 0) const;
   int indexOf(const char *needle, int from = 0) const;
   int indexOf(const ByteArray &needle, int from = 0) const;
   int lastIndexOf(char needle, int from = -1) const;
   int lastIndexOf(const char *needle, int from = -1) const;
   int lastIndexOf(const ByteArray &needle, int from = -1) const;
   
   inline bool contains(char c) const;
   inline bool contains(const char *array) const;
   inline bool contains(const ByteArray &array) const;
   
   int count(char c) const;
   int count(const char *array) const;
   int count(const ByteArray &array) const;
   
   PDK_REQUIRED_RESULT ByteArray left(int length) const;
   PDK_REQUIRED_RESULT ByteArray right(int length) const;
   PDK_REQUIRED_RESULT ByteArray mid(int index, int length = -1) const;
   PDK_REQUIRED_RESULT ByteArray chopped(int len) const
   {
      PDK_ASSERT(len >= 0);
      PDK_ASSERT(len <= size());
      return left(size() - len);
   }
   
   bool startsWith(const ByteArray &array) const;
   bool startsWith(char c) const;
   bool startsWith(const char *str) const;
   
   bool endsWith(const ByteArray &array) const;
   bool endsWith(const char c) const;
   bool endsWith(const char *str) const;
   
   void truncate(int pos);
   void chop(int n);
   
#  if defined(PDK_CC_GNU) && !defined(PDK_CC_CLANG) && !defined(PDK_CC_INTEL) && !PDK_HAS_CPP_ATTRIBUTE(nodiscard)
   // required due to https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61941
#    pragma push_macro("PDK_REQUIRED_RESULT")
#    undef PDK_REQUIRED_RESULT
#    define PDK_REQUIRED_RESULT
#    define PDK_REQUIRED_RESULT_pushed
#  endif
   
   PDK_REQUIRED_RESULT ByteArray toLower() const &
   {
      return toLowerHelper(*this);
   }
   
   PDK_REQUIRED_RESULT ByteArray toLower() const &&
   {
      return toLowerHelper(*this);
   }
   
   PDK_REQUIRED_RESULT ByteArray toUpper() const &
   {
      return toUpperHelper(*this);
   }
   
   PDK_REQUIRED_RESULT ByteArray toUpper() const &&
   {
      return toUpperHelper(*this);
   }
   
   PDK_REQUIRED_RESULT ByteArray trimmed() const &
   {
      return trimmedHelper(*this);
   }
   
   PDK_REQUIRED_RESULT ByteArray trimmed() const &&
   {
      return trimmedHelper(*this);
   }
   
   PDK_REQUIRED_RESULT ByteArray simplified() const &
   {
      return simplifiedHelper(*this);
   }
   
   PDK_REQUIRED_RESULT ByteArray simplified() const &&
   {
      return simplifiedHelper(*this);
   }
   
#  ifdef PDK_REQUIRED_RESULT_pushed
#    pragma pop_macro("PDK_REQUIRED_RESULT")
#  endif
   
   PDK_REQUIRED_RESULT ByteArray leftJustified(int width, char fill = ' ', bool truncate = false) const;
   PDK_REQUIRED_RESULT ByteArray rightJustified(int width, char fill = ' ', bool truncate = false) const;
   
   ByteArray &prepend(char c);
   inline ByteArray &prepend(int count, char c);
   ByteArray &prepend(const char *str);
   ByteArray &prepend(const char *str, int length);
   ByteArray &prepend(const ByteArray &array);
   
   ByteArray &append(char c);
   inline ByteArray &append(int count, char c);
   ByteArray &append(const char *str);
   ByteArray &append(const char *str, int length);
   ByteArray &append(const ByteArray &array);
   
   ByteArray &insert(int pos, char c);
   ByteArray &insert(int pos, int count, char c);
   ByteArray &insert(int pos, const char *str);
   ByteArray &insert(int pos, const char *str, int length);
   ByteArray &insert(int pos, const ByteArray &array);
   
   ByteArray &remove(int index, int length);
   
   ByteArray &replace(int index, int length, const char *after);
   ByteArray &replace(int index, int length, const char *after, int alength);
   ByteArray &replace(int index, int length, const ByteArray &after);
   inline ByteArray &replace(char before, const char *after);
   ByteArray &replace(char before, const ByteArray &after);
   inline ByteArray &replace(const char *before, const char *after);
   ByteArray &replace(const char *before, int blength, const char *after, int alength);
   ByteArray &replace(const ByteArray &before, const ByteArray &after);
   inline ByteArray &replace(const ByteArray &before, const char *after);
   ByteArray &replace(const char *before, const ByteArray &after);
   ByteArray &replace(char before, char after);
   
   inline ByteArray &operator+=(char c);
   inline ByteArray &operator+=(const char *str);
   inline ByteArray &operator+=(const ByteArray &array);
   
   std::list<ByteArray> split(char sep) const;
   PDK_REQUIRED_RESULT ByteArray repeated(int times) const;
   
   short toShort(bool *ok = nullptr, int base = 10) const;
   ushort toUShort(bool *ok = nullptr, int base = 10) const;
   int toInt(bool *ok = nullptr, int base = 10) const;
   uint toUInt(bool *ok = nullptr, int base = 10) const;
   long toLong(bool *ok = nullptr, int base = 10) const;
   ulong toULong(bool *ok = nullptr, int base = 10) const;
   pdk::plonglong toLongLong(bool *ok = nullptr, int base = 10) const;
   pdk::pulonglong toULongLong(bool *ok = nullptr, int base = 10) const;
   float toFloat(bool *ok = nullptr) const;
   double toDouble(bool *ok = nullptr) const;
   ByteArray toBase64(Base64Options options = Base64Encoding) const;
   ByteArray toHex(char separator = '\0') const;
   ByteArray toPercentEncoding(const ByteArray &exclude = ByteArray(),
                               const ByteArray &include = ByteArray(),
                               char percent = '%') const;
   
   inline ByteArray &setNum(short, int base = 10);
   inline ByteArray &setNum(ushort, int base = 10);
   inline ByteArray &setNum(int, int base = 10);
   inline ByteArray &setNum(uint, int base = 10);
   ByteArray &setNum(pdk::plonglong, int base = 10);
   ByteArray &setNum(pdk::pulonglong, int base = 10);
   inline ByteArray &setNum(float, char f = 'g', int prec = 6);
   ByteArray &setNum(double, char f = 'g', int prec = 6);
   ByteArray &setRawData(const char *a, int n);
   
   PDK_REQUIRED_RESULT static ByteArray number(int, int base = 10);
   PDK_REQUIRED_RESULT static ByteArray number(uint, int base = 10);
   PDK_REQUIRED_RESULT static ByteArray number(pdk::plonglong, int base = 10);
   PDK_REQUIRED_RESULT static ByteArray number(pdk::pulonglong, int base = 10);
   PDK_REQUIRED_RESULT static ByteArray number(double, char f = 'g', int prec = 6);
   PDK_REQUIRED_RESULT static ByteArray fromRawData(const char *, int size);
   PDK_REQUIRED_RESULT static ByteArray fromBase64(const ByteArray &base64, Base64Options options = Base64Encoding);
   PDK_REQUIRED_RESULT static ByteArray fromHex(const ByteArray &hexEncoded);
   PDK_REQUIRED_RESULT static ByteArray fromPercentEncoding(const ByteArray &pctEncoded, char percent = '%');
   
#if defined(PDK_OS_DARWIN)
   static ByteArray fromCFData(CFDataRef data);
   static ByteArray fromRawCFData(CFDataRef data);
   CFDataRef toCFData() const PDK_DECL_CF_RETURNS_RETAINED;
   CFDataRef toRawCFData() const PDK_DECL_CF_RETURNS_RETAINED;
   static ByteArray fromNSData(const NSData *data);
   static ByteArray fromRawNSData(const NSData *data);
   NSData *toNSData() const PDK_DECL_NS_RETURNS_AUTORELEASED;
   NSData *toRawNSData() const PDK_DECL_NS_RETURNS_AUTORELEASED;
#endif
   
   using Iterator = char *;
   using iterator = Iterator;
   using ConstIterator = const char *;
   using const_iterator = ConstIterator;
   using ReverseIterator = std::reverse_iterator<Iterator>;
   using reverse_iterator = ReverseIterator;
   using ConstReverseIterator = std::reverse_iterator<ConstIterator>;
   using const_reverse_iterator = ConstReverseIterator;
   
   inline iterator begin();
   inline const_iterator begin() const;
   inline const_iterator cbegin() const;
   inline const_iterator constBegin() const;
   inline iterator end();
   inline const_iterator end() const;
   inline const_iterator cend() const;
   inline const_iterator constEnd() const;
   
   reverse_iterator rbegin()
   {
      return reverse_iterator(end()); 
   }
   
   reverse_iterator rend()
   {
      return reverse_iterator(begin());
   }
   
   const_reverse_iterator rbegin() const
   {
      return const_reverse_iterator(end());
   }
   
   const_reverse_iterator rend() const
   {
      return const_reverse_iterator(begin());
   }
   
   const_reverse_iterator crbegin() const
   {
      return const_reverse_iterator(end());
   }
   
   const_reverse_iterator crend() const
   {
      return const_reverse_iterator(begin());
   }
   
   using size_type = int;
   using difference_type = pdk::ptrdiff;
   using const_reference = const char &;
   using reference = char &;
   using pointer = char *;
   using const_pointer = const char *;
   using value_type = char;
   
   inline void push_back(char c);
   inline void push_back(const char *c);
   inline void push_back(const ByteArray &array);
   inline void push_front(char c);
   inline void push_front(const char *c);
   inline void push_front(const ByteArray &array);
   void shrink_to_fit()
   {
      squeeze();
   }
   
   static inline ByteArray fromStdString(const std::string &str);
   inline std::string toStdString() const;
   
   inline int count() const
   {
      return m_data->m_size;
   }
   
   int length() const
   {
      return m_data->m_size;
   }
   
   bool isNull() const;
   
   inline DataPtr &getDataPtr()
   {
      return m_data;
   }
   
private:
   operator pdk::NoImplicitBoolCast() const;
   void reallocData(uint alloc, Data::AllocationOptions options);
   void expand(int i);
   ByteArray getNullTerminated() const;
   
   static ByteArray toLowerHelper(const ByteArray &a);
   static ByteArray toLowerHelper(ByteArray &a);
   static ByteArray toUpperHelper(const ByteArray &a);
   static ByteArray toUpperHelper(ByteArray &a);
   static ByteArray trimmedHelper(const ByteArray &a);
   static ByteArray trimmedHelper(ByteArray &a);
   static ByteArray simplifiedHelper(const ByteArray &a);
   static ByteArray simplifiedHelper(ByteArray &a);
   
   friend class ByteRef;
   friend class String;
private:
   Data *m_data;
};

PDK_DECLARE_OPERATORS_FOR_FLAGS(ByteArray::Base64Options)

class PDK_CORE_EXPORT ByteRef
{
   public:
   inline operator char() const
   {
      return m_index < m_array.m_data->m_size
            ? m_array.m_data->getData()[m_index] : static_cast<char>(0);
   }
   
   inline ByteRef &operator =(char c)
   {
      if (m_index >= m_array.m_data->m_size) {
         m_array.expand(m_index);
      } else {
         m_array.detach();
      }
      m_array.m_data->getData()[m_index] = c;
      return *this;
   }
   
   inline ByteRef &operator =(const ByteRef &c)
   {
      if (m_index >= m_array.m_data->m_size) {
         m_array.expand(m_index);
      } else {
         m_array.detach();
      }
      m_array.m_data->getData()[m_index] = c.m_array.m_data->getData()[m_index];
      return *this;
   }
   
   inline bool operator ==(char c) const
   {
      return m_array.m_data->getData()[m_index] == c;
   }
   
   inline bool operator !=(char c) const
   {
      return m_array.m_data->getData()[m_index] != c;
   }
   
   inline bool operator >(char c) const
   {
      return m_array.m_data->getData()[m_index] != c;
   }
   
   inline bool operator >=(char c) const
   {
      return m_array.m_data->getData()[m_index] >= c;
   }
   
   inline bool operator <=(char c) const
   {
      return m_array.m_data->getData()[m_index] <= c;
   }
   
   inline bool operator <(char c) const
   {
      return m_array.m_data->getData()[m_index] < c;
   }
   
   private:
   inline ByteRef(ByteArray &array, int index)
      : m_array(array),
        m_index(index)
   {}
   
   private:
   friend class ByteArray;
   
   private:
   ByteArray &m_array;
   int m_index;
};

inline ByteArray::ByteArray() noexcept
   : m_data(Data::getSharedNull())
{}

inline ByteArray::ByteArray(const ByteArray &data) noexcept
   : m_data(data.m_data)
{
   m_data->m_ref.ref();
}

inline ByteArray::~ByteArray()
{
   if (!m_data->m_ref.deref()) {
      Data::deallocate(m_data);
   }
}

inline int ByteArray::size() const
{
   return m_data->m_size;
}

inline char ByteArray::at(int i) const
{
   PDK_ASSERT(static_cast<uint>(i)< static_cast<uint>(size()));
   return m_data->getData()[i];
}

inline char ByteArray::operator [](int i) const
{
   PDK_ASSERT(static_cast<uint>(i)< static_cast<uint>(size()));
   return m_data->getData()[i];
}

inline char ByteArray::operator [](uint i) const
{
   PDK_ASSERT(i < static_cast<uint>(size()));
   return m_data->getData()[i];
}

inline bool ByteArray::isEmpty() const
{
   return m_data->m_size == 0;
}

inline int ByteArray::capacity() const
{
   return m_data->m_alloc ? m_data->m_alloc - 1 : 0;
}

#ifndef PDK_NO_CAST_FROM_BYTEARRAY
inline ByteArray::operator const char *() const
{
   return m_data->getData();
}

inline ByteArray::operator const void *() const
{
   return m_data->getData();
}
#endif

inline char *ByteArray::getRawData()
{
   detach();
   return m_data->getData();
}

inline const char *ByteArray::getRawData() const
{
   return m_data->getData();
}

inline const char *ByteArray::getConstRawData() const
{
   return m_data->getData();
}

inline void ByteArray::detach()
{
   if (m_data->m_ref.isShared() || (m_data->m_offset != sizeof(ByteArrayData))) {
      reallocData(static_cast<uint>(m_data->m_size) + 1u, m_data->detachFlags());
   }
}

inline bool ByteArray::isDetached() const
{
   return !m_data->m_ref.isShared();
}

inline bool ByteArray::isSharedWith(const ByteArray &other) const
{
   return m_data == other.m_data;
}

inline void ByteArray::reserve(int asize)
{
   if (m_data->m_ref.isShared() || static_cast<uint>(asize) + 1u > m_data->m_alloc) {
      reallocData(std::max(static_cast<uint>(size()), static_cast<uint>(asize)) + 1u, 
                  m_data->detachFlags() | Data::CapacityReserved);
   } else {
      // @TODO maybe bug
      // cannot set unconditionally, since d could be the shared_null or
      // otherwise static
      m_data->m_capacityReserved = true;
   }
}

inline void ByteArray::squeeze()
{
   if (m_data->m_ref.isShared() || static_cast<uint>(m_data->m_size) + 1u < m_data->m_alloc) {
      reallocData(static_cast<uint>(size()) + 1u, 
                  m_data->detachFlags() & ~Data::CapacityReserved);
   } else {
      // @TODO maybe bug
      // cannot set unconditionally, since d could be the shared_null or
      // otherwise static
      m_data->m_capacityReserved = false;
   }
}

inline ByteRef ByteArray::operator [](int index)
{
   PDK_ASSERT(index >= 0);
   return ByteRef(*this, index);
}

inline ByteRef ByteArray::operator [](uint index)
{
   return ByteRef(*this, index);
}

inline ByteRef ByteArray::front()
{
   return operator[](0);
}

inline ByteRef ByteArray::back()
{
   return operator[](size() - 1);
}

inline ByteArray::iterator ByteArray::begin()
{
   detach();
   return m_data->getData();
}

inline ByteArray::const_iterator ByteArray::begin() const
{
   return m_data->getData();
}

inline ByteArray::const_iterator ByteArray::cbegin() const
{
   return m_data->getData();
}

inline ByteArray::const_iterator ByteArray::constBegin() const
{
   return m_data->getData();
}

inline ByteArray::iterator ByteArray::end()
{
   detach();
   return m_data->getData() + m_data->m_size;
}

inline ByteArray::const_iterator ByteArray::end() const
{
   return m_data->getData() + m_data->m_size;
}


inline ByteArray::const_iterator ByteArray::cend() const
{
   return m_data->getData() + m_data->m_size;
}

inline ByteArray::const_iterator ByteArray::constEnd() const
{
   return m_data->getData() + m_data->m_size;
}

inline ByteArray &ByteArray::append(int count, char ch)
{
   return insert(m_data->m_size, count, ch);
}

inline ByteArray &ByteArray::prepend(int count, char ch)
{
   return insert(0, count, ch);
}

inline ByteArray &ByteArray::operator +=(char c)
{
   return append(c);
}

inline ByteArray &ByteArray::operator +=(const char *str)
{
   return append(str);
}

inline ByteArray &ByteArray::operator +=(const ByteArray &array)
{
   return append(array);
}

inline void ByteArray::push_back(char c)
{
   append(c);
}

inline void ByteArray::push_back(const char *str)
{
   append(str);
}

inline void ByteArray::push_back(const ByteArray &array)
{
   append(array);
}

inline void ByteArray::push_front(char c)
{
   prepend(c);
}

inline void ByteArray::push_front(const char *str)
{
   prepend(str);
}

inline void ByteArray::push_front(const ByteArray &array)
{
   prepend(array);
}

inline bool ByteArray::contains(const ByteArray &array) const
{
   return -1 != indexOf(array);
}

inline bool ByteArray::contains(const char *array) const
{
   return -1 != indexOf(array);
}

inline bool ByteArray::contains(char c) const
{
   return -1 != indexOf(c);
}

inline bool operator ==(const ByteArray &lhs, const ByteArray &rhs) noexcept
{
   return (lhs.size() == rhs.size())
         && (0 == std::memcmp(lhs.getConstRawData(), rhs.getConstRawData(), lhs.size()));
}

inline bool operator ==(const ByteArray &lhs, const char *rhs) noexcept
{
   return rhs ? pdk::strcmp(lhs, rhs) == 0 : lhs.isEmpty();
}

inline bool operator ==(const char *lhs, const ByteArray &rhs) noexcept
{
   return lhs ? pdk::strcmp(lhs, rhs) == 0 : rhs.isEmpty();
}

inline bool operator !=(const ByteArray &lhs, const ByteArray &rhs) noexcept
{
   return !(lhs == rhs);
}

inline bool operator !=(const ByteArray &lhs, const char *rhs) noexcept
{
   return rhs ? pdk::strcmp(lhs, rhs) != 0 : !lhs.isEmpty();
}

inline bool operator !=(const char *lhs, const ByteArray &rhs) noexcept
{
   return lhs ? pdk::strcmp(lhs, rhs) != 0 : !rhs.isEmpty();
}

inline bool operator <(const ByteArray &lhs, const ByteArray &rhs) noexcept
{
   return pdk::strcmp(lhs, rhs) < 0;
}

inline bool operator <(const ByteArray &lhs, const char *rhs) noexcept
{
   return pdk::strcmp(lhs, rhs) < 0;
}

inline bool operator <(const char *lhs, const ByteArray &rhs) noexcept
{
   return pdk::strcmp(lhs, rhs) < 0;
}

inline bool operator <=(const ByteArray &lhs, const ByteArray &rhs) noexcept
{
   return pdk::strcmp(lhs, rhs) <= 0;
}

inline bool operator <=(const ByteArray &lhs, const char *rhs) noexcept
{
   return pdk::strcmp(lhs, rhs) <= 0;
}

inline bool operator <=(const char *lhs, const ByteArray &rhs) noexcept
{
   return pdk::strcmp(lhs, rhs) <= 0;
}

inline bool operator >(const ByteArray &lhs, const ByteArray &rhs) noexcept
{
   return pdk::strcmp(lhs, rhs) > 0;
}

inline bool operator >(const ByteArray &lhs, const char *rhs) noexcept
{
   return pdk::strcmp(lhs, rhs) > 0;
}

inline bool operator >(const char *lhs, const ByteArray &rhs) noexcept
{
   return pdk::strcmp(lhs, rhs) > 0;
}

inline bool operator >=(const ByteArray &lhs, const ByteArray &rhs) noexcept
{
   return pdk::strcmp(lhs, rhs) >= 0;
}

inline bool operator >=(const ByteArray &lhs, const char *rhs) noexcept
{
   return pdk::strcmp(lhs, rhs) >= 0;
}

inline bool operator >=(const char *lhs, const ByteArray &rhs) noexcept
{
   return pdk::strcmp(lhs, rhs) >= 0;
}

inline const ByteArray operator+(const ByteArray &lhs, const ByteArray &rhs)
{
   return ByteArray(lhs) += rhs;
}

inline const ByteArray operator+(const ByteArray &lhs, const char *rhs)
{
   return ByteArray(lhs) += rhs;
}

inline const ByteArray operator+(const ByteArray &lhs, char rhs)
{
   return ByteArray(lhs) += rhs;
}

inline const ByteArray operator+(const char *lhs, const ByteArray &rhs)
{
   return ByteArray(lhs) += rhs;
}

inline const ByteArray operator+(char lhs, const ByteArray &rhs)
{
   return ByteArray(&lhs, 1) += rhs;
}

inline ByteArray &ByteArray::replace(char before, const char *after)
{
   return replace(&before, 1, after, pdk::strlen(after));
}

inline ByteArray &ByteArray::replace(const ByteArray &before, const char *after)
{
   return replace(before.getConstRawData(), before.size(), after, pdk::strlen(after));
}

inline ByteArray &ByteArray::replace(const char *before, const char *after)
{
   return replace(before, pdk::strlen(before), after, pdk::strlen(after));
}

inline ByteArray &ByteArray::setNum(short n, int base)
{
   return base == 10 ? setNum(static_cast<pdk::plonglong>(n), base) 
                     : setNum(static_cast<pdk::pulonglong>(ushort(n)), base);
}

inline ByteArray &ByteArray::setNum(ushort n, int base)
{
   return setNum(static_cast<pdk::pulonglong>(n), base);
}

inline ByteArray &ByteArray::setNum(int n, int base)
{
   return base == 10 ? setNum(static_cast<pdk::plonglong>(n), base) 
                     : setNum(static_cast<pdk::pulonglong>(static_cast<uint>(n)), base);
}

inline ByteArray &ByteArray::setNum(uint n, int base)
{
   return setNum(static_cast<pdk::pulonglong>(n), base);
}

inline ByteArray &ByteArray::setNum(float n, char f, int prec)
{
   return setNum(static_cast<double>(n), f, prec);
}

inline std::string ByteArray::toStdString() const
{
   return std::string(getConstRawData(), length());
}

inline ByteArray ByteArray::fromStdString(const std::string &str)
{
   return ByteArray(str.data(), static_cast<int>(str.size()));
}

} // ds

PDK_DECLARE_SHARED(ds::ByteArray)

} // pdk

#endif // PDK_M_BASE_DS_BYTE_ARRAY_H
