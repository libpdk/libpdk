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

#include "pdk/dll/Uuid.h"
#include "pdk/utils/CryptographicHash.h"
#include "pdk/base/io/DataStream.h"
#include "pdk/base/io/Debug.h"
#include "pdk/global/Endian.h"
#include "pdk/global/Random.h"
#include "pdk/kernel/StringUtils.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/lang/Character.h"

namespace pdk {
namespace dll {

using pdk::utils::CryptographicHash;
using pdk::io::DebugStateSaver;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::lang::StringView;
using pdk::lang::String;
using pdk::lang::Character;

// 16 bytes (a uint, two shorts and a uchar[8]), each represented by two hex
// digits; plus four dashes and a pair of enclosing brace: 16*2 + 4 + 2 = 38.
enum { MaxStringUuidLength = 38 };

template <class Integral>
void local_to_hex(char *&dst, Integral value)
{
   value = pdk::to_big_endian(value);
   
   const char* p = reinterpret_cast<const char*>(&value);
   
   for (uint i = 0; i < sizeof(Integral); ++i, dst += 2) {
      dst[0] = pdk::to_hex_lower((p[i] >> 4) & 0xf);
      dst[1] = pdk::to_hex_lower(p[i] & 0xf);
   }
}

template <class Integral>
bool local_from_hex(const char *&src, Integral &value)
{
   value = 0;   
   for (uint i = 0; i < sizeof(Integral) * 2; ++i) {
      uint ch = *src++;
      int tmp = pdk::from_hex(ch);
      if (tmp == -1) {
         return false;
      }
      value = value * 16 + tmp;
   }
   return true;
}

namespace {

char *local_uuid_to_hex(const Uuid &uuid, char *dst)
{
   *dst++ = '{';
   local_to_hex(dst, uuid.m_data1);
   *dst++ = '-';
   local_to_hex(dst, uuid.m_data2);
   *dst++ = '-';
   local_to_hex(dst, uuid.m_data3);
   *dst++ = '-';
   for (int i = 0; i < 2; i++) {
      local_to_hex(dst, uuid.m_data4[i]);
   }
   *dst++ = '-';
   for (int i = 2; i < 8; i++) {
      local_to_hex(dst, uuid.m_data4[i]);
   }
   *dst++ = '}';
   return dst;
}

PDK_NEVER_INLINE
static Uuid local_uuid_from_hex(const char *src)
{
   uint d1;
   ushort d2, d3;
   uchar d4[8];
   if (src) {
      if (*src == '{') {
         src++;
      }
      if (PDK_LIKELY(local_from_hex(src, d1)
                     && *src++ == '-'
                     && local_from_hex(src, d2)
                     && *src++ == '-'
                     && local_from_hex(src, d3)
                     && *src++ == '-'
                     && local_from_hex(src, d4[0])
                     && local_from_hex(src, d4[1])
                     && *src++ == '-'
                     && local_from_hex(src, d4[2])
                     && local_from_hex(src, d4[3])
                     && local_from_hex(src, d4[4])
                     && local_from_hex(src, d4[5])
                     && local_from_hex(src, d4[6])
                     && local_from_hex(src, d4[7]))) {
         return Uuid(d1, d2, d3, d4[0], d4[1], d4[2], d4[3], d4[4], d4[5], d4[6], d4[7]);
      }
   }
   
   return Uuid();
}

Uuid create_from_name(const Uuid &ns, const ByteArray &baseData, CryptographicHash::Algorithm algorithm, int version)
{
   ByteArray hashResult;
   // create a scope so later resize won't reallocate
   {
      CryptographicHash hash(algorithm);
      hash.addData(ns.toRfc4122());
      hash.addData(baseData);
      hashResult = hash.result();
   }
   hashResult.resize(16); // Sha1 will be too long
   Uuid result = Uuid::fromRfc4122(hashResult);
   result.m_data3 &= 0x0FFF;
   result.m_data3 |= (version << 12);
   result.m_data4[0] &= 0x3F;
   result.m_data4[0] |= 0x80;
   return result;
}

} // anonymous namespace

Uuid::Uuid(const String &text)
   : Uuid(fromString(text))
{}

Uuid Uuid::fromString(StringView text) PDK_DECL_NOTHROW
{
   if (text.size() > MaxStringUuidLength) {
      text = text.left(MaxStringUuidLength); // text.truncate(MaxStringUuidLength);
   }
   char latin1[MaxStringUuidLength + 1];
   char *dst = latin1;
   for (Character ch : text)
      *dst++ = ch.toLatin1();
   
   *dst++ = '\0'; // don't read garbage as potentially valid data
   
   return local_uuid_from_hex(latin1);
}

Uuid Uuid::fromString(Latin1String text) PDK_DECL_NOTHROW
{
   if (PDK_UNLIKELY(text.size() < MaxStringUuidLength - 2
                    || (text.front() == Latin1Character('{') && text.size() < MaxStringUuidLength - 1))) {
      // Too short. Don't call local_uuid_from_hex(); QL1Ss need not be NUL-terminated,
      // and we don't want to read trailing garbage as potentially valid data.
      text = Latin1String();
   }
   return local_uuid_from_hex(text.getRawData());
}

Uuid::Uuid(const char *text)
   : Uuid(local_uuid_from_hex(text))
{}

Uuid::Uuid(const ByteArray &text)
   : Uuid(fromString(Latin1String(text.getRawData(), text.size())))
{}

Uuid Uuid::createUuidV3(const Uuid &ns, const ByteArray &baseData)
{
   return create_from_name(ns, baseData, CryptographicHash::Md5, 3);
}

Uuid Uuid::createUuidV5(const Uuid &ns, const ByteArray &baseData)
{
   return create_from_name(ns, baseData, CryptographicHash::Sha1, 5);
}

Uuid Uuid::fromRfc4122(const ByteArray &bytes)
{
   if (bytes.isEmpty() || bytes.length() != 16) {
      return Uuid();
   }
   uint d1;
   ushort d2, d3;
   uchar d4[8];
   const uchar *data = reinterpret_cast<const uchar *>(bytes.getConstRawData());
   d1 = pdk::from_big_endian<pdk::puint32>(data);
   data += sizeof(pdk::puint32);
   d2 = pdk::from_big_endian<pdk::puint16>(data);
   data += sizeof(pdk::puint16);
   d3 = pdk::from_big_endian<pdk::puint16>(data);
   data += sizeof(pdk::puint16);
   for (int i = 0; i < 8; ++i) {
      d4[i] = *(data);
      data++;
   }
   return Uuid(d1, d2, d3, d4[0], d4[1], d4[2], d4[3], d4[4], d4[5], d4[6], d4[7]);
}

String Uuid::toString() const
{
   char latin1[MaxStringUuidLength];
   const auto end = local_uuid_to_hex(*this, latin1);
   PDK_ASSERT(end - latin1 == MaxStringUuidLength);
   PDK_UNUSED(end);
   return String::fromLatin1(latin1, MaxStringUuidLength);
}

ByteArray Uuid::toByteArray() const
{
   ByteArray result(MaxStringUuidLength, pdk::Uninitialized);
   const auto end = local_uuid_to_hex(*this, const_cast<char*>(result.getConstRawData()));
   PDK_ASSERT(end - result.getConstRawData() == MaxStringUuidLength);
   PDK_UNUSED(end);
   return result;
}

ByteArray Uuid::toRfc4122() const
{
   // we know how many bytes a UUID has, I hope :)
   ByteArray bytes(16, pdk::Uninitialized);
   uchar *data = reinterpret_cast<uchar*>(bytes.getRawData());
   pdk::to_big_endian(m_data1, data);
   data += sizeof(pdk::puint32);
   pdk::to_big_endian(m_data2, data);
   data += sizeof(pdk::puint16);
   pdk::to_big_endian(m_data3, data);
   data += sizeof(pdk::puint16);
   for (int i = 0; i < 8; ++i) {
      *(data) = m_data4[i];
      data++;
   }
   return bytes;
}

#ifndef PDK_NO_DATASTREAM
DataStream &operator<<(DataStream &s, const Uuid &id)
{
   ByteArray bytes;
   if (s.getByteOrder() == DataStream::ByteOrder::BigEndian) {
      bytes = id.toRfc4122();
   } else {
      // we know how many bytes a UUID has, I hope :)
      bytes = ByteArray(16, pdk::Uninitialized);
      uchar *data = reinterpret_cast<uchar*>(bytes.getRawData());
      pdk::to_little_endian(id.m_data1, data);
      data += sizeof(pdk::puint32);
      pdk::to_little_endian(id.m_data2, data);
      data += sizeof(pdk::puint16);
      pdk::to_little_endian(id.m_data3, data);
      data += sizeof(pdk::puint16);
      for (int i = 0; i < 8; ++i) {
         *(data) = id.m_data4[i];
         data++;
      }
   }
   if (s.writeRawData(bytes.getRawData(), 16) != 16) {
      s.setStatus(DataStream::Status::WriteFailed);
   }
   return s;
}

DataStream &operator>>(DataStream &s, Uuid &id)
{
   ByteArray bytes(16, pdk::Uninitialized);
   if (s.readRawData(bytes.getRawData(), 16) != 16) {
      s.setStatus(DataStream::Status::ReadPastEnd);
      return s;
   }
   
   if (s.getByteOrder() == DataStream::ByteOrder::BigEndian) {
      id = Uuid::fromRfc4122(bytes);
   } else {
      const uchar *data = reinterpret_cast<const uchar *>(bytes.getConstRawData());
      id.m_data1 = pdk::from_little_endian<pdk::puint32>(data);
      data += sizeof(pdk::puint32);
      id.m_data2 = pdk::from_little_endian<pdk::puint16>(data);
      data += sizeof(pdk::puint16);
      id.m_data3 = pdk::from_little_endian<pdk::puint16>(data);
      data += sizeof(pdk::puint16);
      for (int i = 0; i < 8; ++i) {
         id.m_data4[i] = *(data);
         data++;
      }
   }
   return s;
}
#endif // PDK_NO_DATASTREAM

bool Uuid::isNull() const noexcept
{
   return m_data4[0] == 0 && m_data4[1] == 0 && m_data4[2] == 0 && m_data4[3] == 0 &&
         m_data4[4] == 0 && m_data4[5] == 0 && m_data4[6] == 0 && m_data4[7] == 0 &&
         m_data1 == 0 && m_data2 == 0 && m_data3 == 0;
}

Uuid::Variant Uuid::getVariant() const noexcept
{
   if (isNull())
      return VarUnknown;
   // Check the 3 MSB of data4[0]
   if ((m_data4[0] & 0x80) == 0x00) return NCS;
   else if ((m_data4[0] & 0xC0) == 0x80) return DCE;
   else if ((m_data4[0] & 0xE0) == 0xC0) return Microsoft;
   else if ((m_data4[0] & 0xE0) == 0xE0) return Reserved;
   return VarUnknown;
}

Uuid::Version Uuid::getVersion() const noexcept
{
   // Check the 4 MSB of data3
   Version ver = (Version)(m_data3>>12);
   if (isNull()
       || (getVariant() != DCE)
       || ver < Time
       || ver > Sha1)
      return VerUnknown;
   return ver;
}

bool Uuid::operator<(const Uuid &other) const noexcept
{
   if (getVariant() != other.getVariant())
      return getVariant() < other.getVariant();
   
#define ISLESS(f1, f2) if (f1!=f2) return (f1<f2);
   ISLESS(m_data1, other.m_data1);
   ISLESS(m_data2, other.m_data2);
   ISLESS(m_data3, other.m_data3);
   for (int n = 0; n < 8; n++) {
      ISLESS(m_data4[n], other.m_data4[n]);
   }
#undef ISLESS
   return false;
}

bool Uuid::operator>(const Uuid &other) const noexcept
{
   return other < *this;
}

#if defined(PDK_OS_WIN)

} // dll
} // pdk

#include <objbase.h> // For CoCreateGuid

namespace pdk {
namespace dll {

Uuid Uuid::createUuid()
{
   GUID guid;
   CoCreateGuid(&guid);
   Uuid result = guid;
   return result;
}

#else // PDK_OS_WIN

Uuid Uuid::createUuid()
{
   Uuid result(pdk::Uninitialized);
   uint *data = &(result.m_data1);
   enum { AmountToRead = 4 };
   RandomGenerator::system()->fillRange(data, AmountToRead);
   result.m_data4[0] = (result.m_data4[0] & 0x3F) | 0x80;        // UV_DCE
   result.m_data3 = (result.m_data3 & 0x0FFF) | 0x4000;        // UV_Random
   return result;
}
#endif // !PDK_OS_WIN

#ifndef PDK_NO_DEBUG_STREAM
Debug operator<<(Debug dbg, const Uuid &id)
{
   DebugStateSaver saver(dbg);
   dbg.nospace() << "Uuid(" << id.toString() << ')';
   return dbg;
}
#endif

uint pdk_hash(const Uuid &uuid, uint seed) PDK_DECL_NOTHROW
{
   return uuid.m_data1 ^ uuid.m_data2 ^ (uuid.m_data3 << 16)
         ^ ((uuid.m_data4[0] << 24) | (uuid.m_data4[1] << 16) | (uuid.m_data4[2] << 8) | uuid.m_data4[3])
         ^ ((uuid.m_data4[4] << 24) | (uuid.m_data4[5] << 16) | (uuid.m_data4[6] << 8) | uuid.m_data4[7])
         ^ seed;
}

} // dll
} // pdk
