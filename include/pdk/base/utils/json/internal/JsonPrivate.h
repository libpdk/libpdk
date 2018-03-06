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
// Created by softboy on 2018/03/05.

#ifndef PDK_M_BASE_JSON_INTERNAL_JSON_PRIVATE_H
#define PDK_M_BASE_JSON_INTERNAL_JSON_PRIVATE_H

#include "pdk/base/utils/json/JsonObject.h"
#include "pdk/base/utils/json/JsonValue.h"
#include "pdk/base/utils/json/JsonDocument.h"
#include "pdk/base/utils/json/JsonArray.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/base/lang/String.h"
#include "pdk/global/Endian.h"
#include "pdk/global/internal/EndianPrivate.h"
#include "pdk/global/Numeric.h"
#include "pdk/global/Logging.h"
#include "pdk/pal/kernel/Simd.h"
#include <limits.h>
#include <limits>

namespace pdk {
namespace utils {
namespace json {
namespace jsonprivate {

using pdk::LEInteger;
using pdk::internal::LEIntegerBitfield;
using pdk::lang::String;
using pdk::lang::Character;
using pdk::lang::Latin1String;
using pdk::os::thread::AtomicInt;

/*
  This defines a binary data structure for Json data. The data structure is optimised for fast reading
  and minimum allocations. The whole data structure can be mmap'ed and used directly.
  
  In most cases the binary structure is not as space efficient as a utf8 encoded text representation, but
  much faster to access.
  
  The size requirements are:
  
  String:
    Latin1 data: 2 bytes header + string.length()
    Full Unicode: 4 bytes header + 2*(string.length())
    
  Values: 4 bytes + size of data (size can be 0 for some data)
    bool: 0 bytes
    double: 8 bytes (0 if integer with less than 27bits)
    string: see above
    array: size of array
    object: size of object
  Array: 12 bytes + 4*length + size of Value data
  Object: 12 bytes + 8*length + size of Key Strings + size of Value data
  
  For an example such as
  
    {                                           // object: 12 + 5*8                   = 52
         "firstName": "John",                   // key 12, value 8                    = 20
         "lastName" : "Smith",                  // key 12, value 8                    = 20
         "age"      : 25,                       // key 8, value 0                     = 8
         "address"  :                           // key 12, object below               = 140
         {                                      // object: 12 + 4*8
             "streetAddress": "21 2nd Street",  // key 16, value 16
             "city"         : "New York",       // key 8, value 12
             "state"        : "NY",             // key 8, value 4
             "postalCode"   : "10021"           // key 12, value 8
         },                                     // object total: 128
         "phoneNumber":                         // key: 16, value array below         = 172
         [                                      // array: 12 + 2*4 + values below: 156
             {                                  // object 12 + 2*8
               "type"  : "home",                // key 8, value 8
               "number": "212 555-1234"         // key 8, value 16
             },                                 // object total: 68
             {                                  // object 12 + 2*8
               "type"  : "fax",                 // key 8, value 8
               "number": "646 555-4567"         // key 8, value 16
             }                                  // object total: 68
         ]                                      // array total: 156
    }                                           // great total:                         412 bytes
    
    The uncompressed text file used roughly 500 bytes, so in this case we end up using about
    the same space as the text representation.
    
    Other measurements have shown a slightly bigger binary size than a compact text
    representation where all possible whitespace was stripped out.
*/

class LocalArray;
class LocalObject;
class LocalValue;
class LocalEntry;

template<typename T>
using local_littleendian = LEInteger<T>;

using ple_short = local_littleendian<short>;
using ple_ushort = local_littleendian<unsigned short>;
using ple_int = local_littleendian<int>;
using ple_uint = local_littleendian<unsigned int>;

template<int pos, int width>
using ple_bitfield = LEIntegerBitfield<uint, pos, width>;

template<int pos, int width>
using ple_signedbitfield = LEIntegerBitfield<int, pos, width>;

using offset = ple_uint;

// round the size up to the next 4 byte boundary
inline int aligned_size(int size)
{
   return (size + 3) & ~3;
}

namespace {
inline bool use_compressed(const String &s)
{
   if (s.length() >= 0x8000) {
      return false;
   }
   const char16_t *uc = (const char16_t *)s.getConstRawData();
   const char16_t *e = uc + s.length();
   while (uc < e) {
      if (*uc > 0xff) {
         return false;
      }
      ++uc;
   }
   return true;
}

inline int string_size(const String &string, bool compress)
{
   int l = 2 + string.length();
   if (!compress) {
      l *= 2;
   }
   return aligned_size(l);
}

// returns INT_MAX if it can't compress it into 28 bits
inline int compressed_number(double d)
{
   // this relies on details of how ieee floats are represented
   const int exponentOff = 52;
   const pdk::puint64 fractionMask = 0x000fffffffffffffull;
   const pdk::puint64 exponentMask = 0x7ff0000000000000ull;
   pdk::puint64 val;
   memcpy (&val, &d, sizeof(double));
   int exp = (int)((val & exponentMask) >> exponentOff) - 1023;
   if (exp < 0 || exp > 25) {
      return INT_MAX;
   }
   pdk::puint64 nonInt = val & (fractionMask >> exp);
   if (nonInt) {
      return INT_MAX;
   }
   bool neg = (val >> 63) != 0;
   val &= fractionMask;
   val |= ((pdk::puint64)1 << 52);
   int res = (int)(val >> (52 - exp));
   return neg ? -res : res;
}

} // anonymous namespace

class LocalLatin1String;

class LocalString
{
public:
   explicit LocalString(const char *data) { d = (Data *)data; }
   
   struct Data {
      ple_uint m_length;
      ple_ushort m_utf16[1];
   };
   
   Data *m_implPtr;
   
   int getByteSize() const
   {
      return sizeof(uint) + sizeof(ushort) * m_implPtr->m_length;
   }
   
   bool isValid(int maxSize) const
   {
      // Check byteSize() <= maxSize, avoiding integer overflow
      maxSize -= sizeof(uint);
      return maxSize >= 0 && uint(m_implPtr->m_length) <= maxSize / sizeof(ushort);
   }
   
   inline LocalString &operator=(const String &str)
   {
      m_implPtr->m_length = str.length();
#if PDK_BYTE_ORDER == PDK_BIG_ENDIAN
      const char16_t *uc = (const char16_t *)str.unicode();
      for (int i = 0; i < str.length(); ++i) {
         m_implPtr->m_utf16[i] = uc[i];
      } 
#else
      memcpy(m_implPtr->m_utf16, str.unicode(), str.length()*sizeof(char16_t));
#endif
      if (str.length() & 1) {
         m_implPtr->m_utf16[str.length()] = 0;
      }
      return *this;
   }
   
   inline bool operator ==(const String &str) const
   {
      int slen = str.length();
      int l = m_implPtr->m_length;
      if (slen != l) {
         return false;
      }         
      const char16_t *s = (const char16_t *)str.constData();
      const ple_ushort *a = d->utf16;
      const char16_t *b = s;
      while (l-- && *a == *b) {
         a++,b++;
      }
      return (l == -1);
   }
   
   inline bool operator !=(const String &str) const
   {
      return !operator ==(str);
   }
   
   inline bool operator >=(const String &str) const
   {
      return toString() >= str;
   }
   
   inline bool operator<(const LocalLatin1String &str) const;
   inline bool operator>=(const LocalLatin1String &str) const
   {
      return !operator <(str);
   }
   
   inline bool operator ==(const LocalLatin1String &str) const;
   
   inline bool operator ==(const LocalString &str) const
   {
      if (m_implPtr->m_length != str.m_implPtr->m_length) {
         return false;
      }
      
      return !memcmp(m_implPtr->m_utf16, str.m_implPtr->m_utf16, m_implPtr->m_length*sizeof(char16_t));
   }
   inline bool operator<(const LocalString &other) const;
   inline bool operator >=(const LocalString &other) const
   {
      return !(*this < other);
   }
   
   inline String toString() const {
#if PDK_BYTE_ORDER == PDK_LITTLE_ENDIAN
      return String((Character *)m_implPtr->m_utf16, m_implPtr->m_length);
#else
      int l = m_implPtr->m_length;
      String str(l, pdk::Uninitialized);
      Character *ch = str.data();
      for (int i = 0; i < l; ++i) {
         ch[i] = Character(m_implPtr->m_utf16[i]);
      }  
      return str;
#endif
   }
};

class LocalLatin1String
{
public:
   explicit LocalLatin1String(const char *data) { d = (Data *)data; }
   
   struct Data
   {
      ple_ushort m_length;
      char m_latin1[1];
   };
   Data *m_implPtr;
   
   int getByteSize() const 
   {
      return sizeof(char16_t) + sizeof(char)*(m_implPtr->m_length);
   }
   
   bool isValid(int maxSize) const
   {
      return getByteSize() <= maxSize;
   }
   
   inline LocalLatin1String &operator=(const String &str)
   {
      int len = m_implPtr->m_length = str.length();
      uchar *l = (uchar *)d->latin1;
      const char16_t *uc = (const char16_t *)str.unicode();
      int i = 0;
#ifdef __SSE2__
      for ( ; i + 16 <= len; i += 16) {
         __m128i chunk1 = _mm_loadu_si128((__m128i*)&uc[i]); // load
         __m128i chunk2 = _mm_loadu_si128((__m128i*)&uc[i + 8]); // load
         // pack the two vector to 16 x 8bits elements
         const __m128i result = _mm_packus_epi16(chunk1, chunk2);
         _mm_storeu_si128((__m128i*)&l[i], result); // store
      }
#  ifdef PDK_PROCESSOR_X86_64
      // we can do one more round, of 8 characters
      if (i + 8 <= len) {
         __m128i chunk = _mm_loadu_si128((__m128i*)&uc[i]); // load
         // pack with itself, we'll discard the high part anyway
         chunk = _mm_packus_epi16(chunk, chunk);
         // unaligned 64-bit store
         pdk::to_unaligned(_mm_cvtsi128_si64(chunk), l + i);
         i += 8;
      }
#  endif
#endif
      for ( ; i < len; ++i) {
         l[i] = uc[i];
      }
      for ( ; (pdk::uintptr)(l+i) & 0x3; ++i) {
         l[i] = 0;
      }
      return *this;
   }
   
   Latin1String toLatin1String() const noexcept
   {
      return Latin1String(m_implPtr->m_latin1, m_implPtr->m_length);
   }
   
   inline bool operator<(const LocalString &str) const
   {
      const ple_ushort *uc = (ple_ushort *) str.m_implPtr->m_utf16;
      if (!uc || *uc == 0) {
         return false;
      }
      const uchar *c = (uchar *)m_implPtr->m_latin1;
      const uchar *e = c + std::min((int)m_implPtr->m_length, (int)str.m_implPtr->m_length);
      
      while (c < e) {
         if (*c != *uc) {
            break;
         }
         ++c;
         ++uc;
      }
      return (c == e ? (int)m_implPtr->m_length < (int)str.m_implPtr->m_length : *c < *uc);
      
   }
   
   inline bool operator ==(const LocalString &str) const
   {
      return (str == *this);
   }
   
   inline bool operator >=(const LocalString &str) const
   {
      return !(*this < str);
   }
   
   inline String toString() const
   {
      return String::fromLatin1(m_implPtr->m_latin1, m_implPtr->m_length);
   }
};

#define DEF_OP(op) \
   inline bool operator op(LocalLatin1String lhs, LocalLatin1String rhs) noexcept \
{ \
   return lhs.toLatin1String() op rhs.toLatin1String(); \
} \
   inline bool operator op(Latin1String lhs, LocalLatin1String rhs) noexcept \
{ \
   return lhs op rhs.toLatin1String(); \
} \
   inline bool operator op(LocalLatin1String lhs, Latin1String rhs) noexcept \
{ \
   return lhs.toLatin1String() op rhs; \
} \
   inline bool operator op(const String &lhs, LocalLatin1String rhs) noexcept \
{ \
   return lhs op rhs.toLatin1String(); \
} \
   inline bool operator op(LocalLatin1String lhs, const String &rhs) noexcept \
{ \
   return lhs.toLatin1String() op rhs; \
} \
   /*end*/
DEF_OP(==)
DEF_OP(!=)
DEF_OP(< )
DEF_OP(> )
DEF_OP(<=)
DEF_OP(>=)
#undef DEF_OP

inline bool LocalString::operator ==(const Latin1String &str) const
{
   if ((int)d->length != (int)str.d->length)
      return false;
   const qle_ushort *uc = d->utf16;
   const qle_ushort *e = uc + d->length;
   const uchar *c = (uchar *)str.d->latin1;
   
   while (uc < e) {
      if (*uc != *c)
         return false;
      ++uc;
      ++c;
   }
   return true;
}

inline bool LocalString::operator <(const LocalString &other) const
{
   int alen = m_implPtr->m_length;
   int blen = other.m_implPtr->m_length;
   int l = std::min(alen, blen);
   ple_ushort *a = m_implPtr->m_utf16;
   ple_ushort *b = other.m_implPtr->m_utf16;
   while (l-- && *a == *b) {
      a++,b++;
   } 
   if (l==-1) {
      return (alen < blen);
   }
   return (char16_t)*a < (char16_t)*b;
}

inline bool LocalString::operator<(const LocalLatin1String &str) const
{
   const uchar *c = (uchar *) str.m_implPtr->m_latin1;
   if (!c || *c == 0) {
      return false;
   }
   const ple_ushort *uc = m_implPtr->m_utf16;
   const ple_ushort *e = uc + std::min((int)m_implPtr->m_length, (int)str.m_implPtr->m_length);
   
   while (uc < e) {
      if (*uc != *c) {
         break;
      }      
      ++uc;
      ++c;
   }
   return (uc == e ? (int)m_implPtr->m_length < (int)str.m_implPtr->m_length : (char16_t)*uc < *c);   
}

namespace {

inline void copy_string(char *dest, const String &str, bool compress)
{
   if (compress) {
      LocalLatin1String string(dest);
      string = str;
   } else {
      LocalString string(dest);
      string = str;
   }
}

} // anonymous namespace

/*
 Base is the base class for both Object and Array. Both classe work more or less the same way.
 The class starts with a header (defined by the struct below), then followed by data (the data for
 values in the Array case and Entry's (see below) for objects.
 
 After the data a table follows (tableOffset points to it) containing Value objects for Arrays, and
 offsets from the beginning of the object to Entry's in the case of Object.
 
 Entry's in the Object's table are lexicographically sorted by key in the table(). This allows the usage
 of a binary search over the keys in an Object.
 */

class Base
{
public:
   ple_uint m_size;
   union {
      uint m_dummy;
      ple_bitfield<0, 1> m_isObject;
      ple_bitfield<1, 31> m_length;
   };
   offset m_tableOffset;
   // content follows here
   
   inline bool isObject() const
   {
      return !!m_isObject;
   }
   
   inline bool isArray() const
   {
      return !isObject();}
   
   
   inline offset *getTable() const
   {
      return (offset *) (((char *) this) + m_tableOffset);
   }
   
   int reserveSpace(uint dataSize, int posInTable, uint numItems, bool replace);
   void removeItems(int pos, int numItems);
};

class LocalObject : public Base
{
public:
   LocalEntry *entryAt(int i) const
   {
      return reinterpret_cast<LocalEntry *>(((char *)this) + getTable()[i]);
   }
   int indexOf(const String &key, bool *exists) const;
   int indexOf(Latin1String key, bool *exists) const;
   bool isValid(int maxSize) const;
};


class LocalArray : public Base
{
public:
   inline Value at(int i) const;
   inline Value &operator [](int i);
   
   bool isValid(int maxSize) const;
};

class LocalValue
{
public:
   enum {
      MaxSize = (1<<27) - 1
   };
   union {
      uint m_dummy;
      ple_bitfield<0, 3> m_type;
      ple_bitfield<3, 1> m_latinOrIntValue;
      ple_bitfield<4, 1> m_latinKey;
      ple_bitfield<5, 27> m_value;
      ple_signedbitfield<5, 27> m_intValue;
   };
   
   inline char *getData(const Base *b) const
   {
      return ((char *)b) + value;
   }
   
   int getUsedStorage(const Base *b) const;
   
   bool toBoolean() const;
   double toDouble(const Base *b) const;
   String toString(const Base *b) const;
   LocalString asString(const Base *b) const;
   LocalLatin1String asLatin1String(const Base *b) const;
   Base *base(const Base *b) const;
   bool isValid(const Base *b) const;
   static int requiredStorage(JsonValue &v, bool *compressed);
   static uint valueToStore(const JsonValue &v, uint offset);
   static void copyData(const JsonValue &v, char *dest, bool compressed);
};

inline LocalValue LocalArray::at(int i) const
{
   return *(LocalValue *) (getTable() + i);
}

inline LocalValue &LocalArray::operator [](int i)
{
   return *(LocalValue *) (getTable() + i);
}

class LocalEntry {
public:
   LocalValue m_value;
   // key
   // value data follows key
   
   uint getSize() const
   {
      int s = sizeof(LocalEntry);
      if (m_value.m_latinKey) {
         s += getShallowLatin1Key().getByteSize();
      } else {
         s += getShallowKey().getByteSize();
      }
      return aligned_size(s);
   }
   
   int getUsedStorage(Base *b) const
   {
      return getSize() + m_value.getUsedStorage(b);
   }
   
   LocalString getShallowKey() const
   {
      PDK_ASSERT(!m_value.m_latinKey);
      return String((const char *)this + sizeof(LocalEntry));
   }
   
   LocalLatin1String getShallowLatin1Key() const
   {
      PDK_ASSERT(m_value.m_latinKey);
      return Latin1String((const char *)this + sizeof(LocalEntry));
   }
   
   String getKey() const
   {
      if (m_value.m_latinKey) {
         return getShallowLatin1Key().toString();
      }
      return getShallowKey().toString();
   }
   
   bool isValid(int maxSize) const
   {
      if (maxSize < (int)sizeof(LocalEntry)) {
         return false;
      }
      maxSize -= sizeof(LocalEntry);
      if (m_value.m_latinKey) {
         return getShallowLatin1Key().isValid(maxSize);
      }
      return getShallowKey().isValid(maxSize);
   }
   
   bool operator ==(const String &key) const;
   inline bool operator !=(const String &key) const
   {
      return !operator ==(key);
   }
   
   inline bool operator >=(const String &key) const;
   
   bool operator==(Latin1String key) const;
   inline bool operator!=(Latin1String key) const
   {
      return !operator ==(key);
   }
   
   inline bool operator>=(Latin1String key) const;
   
   bool operator ==(const LocalEntry &other) const;
   bool operator >=(const LocalEntry &other) const;
};

inline bool LocalEntry::operator >=(const String &key) const
{
   if (m_value.m_latinKey) {
      return (getShallowLatin1Key() >= key);
   } else {
      return (getShallowKey() >= key);
   }
}

inline bool LocalEntry::operator >=(Latin1String key) const
{
   if (m_value.m_latinKey) {
      return getShallowLatin1Key() >= key;
   } else {
      return getShallowKey() >= key;
   }
}

inline bool operator <(const String &key, const LocalEntry &entry)
{
   return entry >= key;
}

inline bool operator<(Latin1String key, const LocalEntry &entry)
{
   return entry >= key;
}

class Header
{
public:
   ple_uint m_tag;
   ple_uint m_version; // 1
   Base *getRoot()
   {
      return (Base *)(this + 1);
   }
};

inline bool LocalValue::toBoolean() const
{
   PDK_ASSERT(m_type == JsonValue::Type::Bool);
   return m_value != 0;
}

inline double LocalValue::toDouble(const Base *b) const
{
   PDK_ASSERT(m_type == JsonValue::Type::Double);
   if (m_latinOrIntValue) {
      return m_intValue;
   }
   pdk::puint64 i = pdk::from_little_endian<pdk::puint64>((const uchar *)b + value);
   double d;
   memcpy(&d, &i, sizeof(double));
   return d;
}

inline LocalString LocalValue::asString(const Base *b) const
{
   PDK_ASSERT(m_type == JsonValue::Type::String && !m_latinOrIntValue);
   return LocalString(getData(b));
}

inline LocalLatin1String LocalValue::asLatin1String(const Base *b) const
{
   PDK_ASSERT(type == JsonValue::Type::String && m_latinOrIntValue);
   return LocalLatin1String(getData(b));
}

inline String LocalValue::toString(const Base *b) const
{
   if (m_latinOrIntValue) {
      return asLatin1String(b).toString();
   } else {
      return asString(b).toString();
   } 
}

inline Base *LocalValue::base(const Base *b) const
{
   PDK_ASSERT(m_type == JsonValue::Type::Array || m_type == JsonValue::Type::Object);
   return reinterpret_cast<Base *>(getData(b));
}

class Data {
public:
   enum Validation
   {
      Unchecked,
      Validated,
      Invalid
   };
   
   AtomicInt m_ref;
   int m_alloc;
   union {
      char *m_rawData;
      Header *m_header;
   };
   uint m_compactionCounter : 31;
   uint m_ownsData : 1;
   
   inline Data(char *raw, int a)
      : m_alloc(a), 
        m_rawData(raw), 
        m_compactionCounter(0), 
        m_ownsData(true)
   {}
   
   inline Data(int reserved, JsonValue::Type valueType)
      : m_rawData(0),
        m_compactionCounter(0), 
        m_ownsData(true)
   {
      PDK_ASSERT(valueType == JsonValue::Type::Array || valueType == JsonValue::Type::Object);
      
      m_alloc = sizeof(Header) + sizeof(Base) + reserved + sizeof(offset);
      m_header = (Header *)malloc(m_alloc);
      PDK_CHECK_ALLOC_PTR(m_header);
      m_header->m_tag = JsonDocument::BinaryFormatTag;
      m_header->m_version = 1;
      Base *b = m_header->getRoot();
      b->m_size = sizeof(Base);
      b->m_isObject = (valueType == JsonValue::Type::Object);
      b->m_tableOffset = sizeof(Base);
      b->m_length = 0;
   }
   
   inline ~Data()
   {
      if (m_ownsData) {
         free(m_ownsData);
      }
   }
   
   uint offsetOf(const void *ptr) const 
   {
      return (uint)(((char *)ptr - m_rawData));
   }
   
   JsonObject toObject(LocalObject *object) const
   {
      return JsonObject(const_cast<Data *>(this), object);
   }
   
   JsonArray toArray(LocalArray *array) const
   {
      return JsonArray(const_cast<Data *>(this), array);
   }
   
   Data *clone(Base *base, int reserve = 0)
   {
      int size = sizeof(Header) + base->m_size;
      if (base == m_header->getRoot() && m_ref.load() == 1 && m_alloc >= size + reserve) {
         return this;
      }
      if (reserve) {
         if (reserve < 128) {
            reserve = 128;
         }
         size = std::max(size + reserve, std::min(size *2, (int)LocalValue::MaxSize));
         if (size > LocalValue::MaxSize) {
            warning_stream("Json: Document too large to store in data structure");
            return 0;
         }
      }
      char *raw = (char *)malloc(size);
      PDK_CHECK_ALLOC_PTR(raw);
      memcpy(raw + sizeof(Header), base, base->m_size);
      Header *header = (Header *)raw;
      header->m_tag = JsonDocument::BinaryFormatTag;
      header->m_version = 1;
      Data *data = new Data(raw, size);
      data->m_compactionCounter = (base == header->getRoot()) ? m_compactionCounter : 0;
      return data;
   }
   
   void compact();
   bool valid() const;
   
private:
   PDK_DISABLE_COPY(Data);
};

} // jsonprivate
} // json
} // utils
} // pdk

PDK_DECLARE_TYPEINFO(pdk::utils::json::jsonprivate::LocalValue, PDK_PRIMITIVE_TYPE);


#endif // PDK_M_BASE_JSON_INTERNAL_JSON_PRIVATE_H
