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

#ifndef PDK_DLL_UUID_H
#define PDK_DLL_UUID_H

#include "pdk/base/lang/String.h"

#if defined(PDK_OS_WIN)
#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
   ulong   Data1;
   ushort  Data2;
   ushort  Data3;
   uchar   Data4[8];
} GUID, *REFGUID, *LPGUID;
#endif
#endif

#if defined(PDK_OS_DARWIN)
PDK_FORWARD_DECLARE_CF_TYPE(CFUUID);
PDK_FORWARD_DECLARE_OBJC_CLASS(NSUUID);
#endif

namespace pdk {

// forward declare class with namespace
namespace ds {
class ByteArray;
} // ds

// forward declare class with namespace
namespace lang {
class String;
class StringView;
class Latin1String;
} // lang

// forward declare class with namespace
namespace io {
class Debug;
class DataStream;
} // io

namespace dll {

using pdk::ds::ByteArray;
using pdk::lang::String;
using pdk::lang::StringView;
using pdk::lang::Latin1String;

using pdk::io::Debug;
using pdk::io::DataStream;

class PDK_CORE_EXPORT Uuid
{
   Uuid(pdk::Initialization) {}
public:
   enum Variant
   {
      VarUnknown        = -1,
      NCS                = 0, // 0 - -
      DCE                = 2, // 1 0 -
      Microsoft        = 6, // 1 1 0
      Reserved        = 7  // 1 1 1
   };
   
   enum Version
   {
      VerUnknown        =-1,
      Time                = 1, // 0 0 0 1
      EmbeddedPOSIX        = 2, // 0 0 1 0
      Md5                 = 3, // 0 0 1 1
      Name = Md5,
      Random                = 4,  // 0 1 0 0
      Sha1                 = 5 // 0 1 0 1
   };
   
   constexpr Uuid() noexcept 
      : m_data1(0),
        m_data2(0),
        m_data3(0),
        m_data4{0,0,0,0,0,0,0,0}
   {}
   
   constexpr Uuid(uint l, ushort w1, ushort w2, uchar b1, uchar b2, uchar b3,
                  uchar b4, uchar b5, uchar b6, uchar b7, uchar b8) noexcept
      : m_data1(l),
        m_data2(w1),
        m_data3(w2),
        m_data4{b1, b2, b3, b4, b5, b6, b7, b8}
   {}
   
   Uuid(const String &);
   static Uuid fromString(StringView string) noexcept;
   static Uuid fromString(Latin1String string) noexcept;
   Uuid(const char *);
   String toString() const;
   Uuid(const ByteArray &);
   ByteArray toByteArray() const;
   ByteArray toRfc4122() const;
   static Uuid fromRfc4122(const ByteArray &);
   bool isNull() const noexcept;
   
   PDK_DECL_RELAXED_CONSTEXPR bool operator==(const Uuid &orig) const noexcept
   {
      if (m_data1 != orig.m_data1 || m_data2 != orig.m_data2 ||
          m_data3 != orig.m_data3) {
         return false;
      }
      for (uint i = 0; i < 8; i++) {
         if (m_data4[i] != orig.m_data4[i]) {
            return false;
         }
      }
      return true;
   }
   
   PDK_DECL_RELAXED_CONSTEXPR bool operator!=(const Uuid &orig) const noexcept
   {
      return !(*this == orig);
   }
   
   bool operator<(const Uuid &other) const noexcept;
   bool operator>(const Uuid &other) const noexcept;
   
#if defined(PDK_OS_WIN)
   // On Windows we have a type GUID that is used by the platform API, so we
   // provide convenience operators to cast from and to this type.
   PDK_DECL_CONSTEXPR Uuid(const GUID &guid) noexcept
      : m_data1(guid.Data1), 
        m_data2(guid.Data2),
        m_data3(guid.Data3),
        m_data4{guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]}
   {}
   
   PDK_DECL_RELAXED_CONSTEXPR Uuid &operator=(const GUID &guid) noexcept
   {
      *this = Uuid(guid);
      return *this;
   }
   
   PDK_DECL_RELAXED_CONSTEXPR operator GUID() const noexcept
   {
      GUID guid = { data1, data2, data3, { data4[0], data4[1], data4[2], data4[3], data4[4], data4[5], data4[6], data4[7] } };
      return guid;
   }
   
   PDK_DECL_RELAXED_CONSTEXPR bool operator==(const GUID &guid) const noexcept
   {
      return *this == Uuid(guid);
   }
   
   PDK_DECL_RELAXED_CONSTEXPR bool operator!=(const GUID &guid) const noexcept
   {
      return !(*this == guid);
   }
#endif
   static Uuid createUuid();
   static Uuid createUuidV3(const Uuid &ns, const ByteArray &baseData);
   static Uuid createUuidV5(const Uuid &ns, const ByteArray &baseData);
   static inline Uuid createUuidV3(const Uuid &ns, const String &baseData)
   {
      return Uuid::createUuidV3(ns, baseData.toUtf8());
   }
   
   static inline Uuid createUuidV5(const Uuid &ns, const String &baseData)
   {
      return Uuid::createUuidV5(ns, baseData.toUtf8());
   }
   
   Uuid::Variant getVariant() const noexcept;
   Uuid::Version getVersion() const noexcept;
   
#if defined(PDK_OS_DARWIN)
   static Uuid fromCFUUID(CFUUIDRef uuid);
   CFUUIDRef toCFUUID() const PDK_DECL_CF_RETURNS_RETAINED;
   static Uuid fromNSUUID(const NSUUID *uuid);
   NSUUID *toNSUUID() const PDK_DECL_NS_RETURNS_AUTORELEASED;
#endif   
   uint    m_data1;
   ushort  m_data2;
   ushort  m_data3;
   uchar   m_data4[8];
};

#ifndef PDK_NO_DATASTREAM
PDK_CORE_EXPORT DataStream &operator<<(DataStream &, const Uuid &);
PDK_CORE_EXPORT DataStream &operator>>(DataStream &, Uuid &);
#endif

#ifndef PDK_NO_DEBUG_STREAM
PDK_CORE_EXPORT Debug operator<<(Debug, const Uuid &);
#endif

PDK_CORE_EXPORT uint pdk_hash(const Uuid &uuid, uint seed = 0) noexcept;

inline bool operator<=(const Uuid &lhs, const Uuid &rhs) noexcept
{
   return !(rhs < lhs);
}

inline bool operator>=(const Uuid &lhs, const Uuid &rhs) noexcept
{
   return !(lhs < rhs);
}

} // dll
} // pdk

PDK_DECLARE_TYPEINFO(pdk::dll::Uuid, PDK_PRIMITIVE_TYPE);

#endif // PDK_DLL_UUID_H
