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
// Created by softboy on 2018/02/05.

#ifndef PDK_M_BASE_IO_DATA_STREAM_H
#define PDK_M_BASE_IO_DATA_STREAM_H

#include "pdk/utils/ScopedPointer.h"
#include "pdk/base/io/IoDevice.h"
#include "pdk/global/SysInfo.h"
#include "pdk/global/Flags.h"

// @TODO refactor forward declare
#include <utility>
#include <vector>
#include <map>
#include <list>
#include <set>
#ifdef Status
#error pdk/pdl/base/io/DataStream.h must be included before any header file that defines Status
#endif

namespace pdk {
namespace io {

// forward declare class with namespace
namespace internal {
class DataStreamPrivate;
class StreamStateSaver;
} // internal

using internal::DataStreamPrivate;
using pdk::SysInfo;

class PDK_CORE_EXPORT DataStream
{
public:
   enum class Version
   {
      pdk_1_0,
      DefaultCompiledVersion = pdk_1_0
   };
   
   enum class ByteOrder
   {
      BigEndian = SysInfo::BigEndian,
      LittleEndian = SysInfo::LittleEndian
   };
   
   enum class Status
   {
      Ok,
      ReadPastEnd,
      ReadCorruptData,
      WriteFailed
   };
   
   enum class FloatingPointPrecision
   {
      SinglePrecision,
      DoublePrecision
   };
   
   DataStream();
   explicit DataStream(IoDevice *);
   DataStream(ByteArray *, IoDevice::OpenMode flags);
   DataStream(const ByteArray &);
   ~DataStream();
   
   IoDevice *getDevice() const;
   void setDevice(IoDevice *);
   void unsetDevice();
   
   bool atEnd() const;
   
   Status getStatus() const;
   void setStatus(Status status);
   void resetStatus();
   
   FloatingPointPrecision getFloatingPointPrecision() const;
   void setFloatingPointPrecision(FloatingPointPrecision precision);
   
   ByteOrder getByteOrder() const;
   void setByteOrder(ByteOrder);
   
   Version getVersion() const;
   void setVersion(Version);
   
   DataStream &operator>>(pdk::pint8 &i);
   DataStream &operator>>(pdk::puint8 &i);
   DataStream &operator>>(pdk::pint16 &i);
   DataStream &operator>>(pdk::puint16 &i);
   DataStream &operator>>(pdk::pint32 &i);
   DataStream &operator>>(pdk::puint32 &i);
   DataStream &operator>>(pdk::pint64 &i);
   DataStream &operator>>(pdk::puint64 &i);
   DataStream &operator>>(std::nullptr_t &ptr) { ptr = nullptr; return *this; }
   
   DataStream &operator>>(bool &i);
   DataStream &operator>>(float &f);
   DataStream &operator>>(double &f);
   DataStream &operator>>(char *&str);
   
   DataStream &operator<<(pdk::pint8 i);
   DataStream &operator<<(pdk::puint8 i);
   DataStream &operator<<(pdk::pint16 i);
   DataStream &operator<<(pdk::puint16 i);
   DataStream &operator<<(pdk::pint32 i);
   DataStream &operator<<(pdk::puint32 i);
   DataStream &operator<<(pdk::pint64 i);
   DataStream &operator<<(pdk::puint64 i);
   DataStream &operator<<(std::nullptr_t) { return *this; }
   DataStream &operator<<(bool i);
   DataStream &operator<<(float f);
   DataStream &operator<<(double f);
   DataStream &operator<<(const char *str);
   
   DataStream &readBytes(char *&, uint &len);
   int readRawData(char *, int len);
   
   DataStream &writeBytes(const char *, uint len);
   int writeRawData(const char *, int len);
   
   int skipRawData(int len);
   
   void startTransaction();
   bool commitTransaction();
   void rollbackTransaction();
   void abortTransaction();
   
private:
   PDK_DISABLE_COPY(DataStream);
   
   pdk::utils::ScopedPointer<DataStreamPrivate> m_implPtr;
   
   IoDevice *m_device;
   bool m_ownDevice;
   bool m_noswap;
   ByteOrder m_byteorder;
   Version m_version;
   Status m_status;
   
   int readBlock(char *data, int len);
   friend class internal::StreamStateSaver;
};

namespace internal {

class StreamStateSaver
{
public:
   inline StreamStateSaver(DataStream *s) 
      : m_stream(s),
        m_oldStatus(s->getStatus())
   {
      if (!m_stream->m_device || !m_stream->m_device->isTransactionStarted())
         m_stream->resetStatus();
   }
   inline ~StreamStateSaver()
   {
      if (m_oldStatus != DataStream::Status::Ok) {
         m_stream->resetStatus();
         m_stream->setStatus(m_oldStatus);
      }
   }
   
private:
   DataStream *m_stream;
   DataStream::Status m_oldStatus;
};

template <typename Container>
DataStream &read_array_based_container(DataStream &s, Container &c)
{
   StreamStateSaver stateSaver(&s);
   
   c.clear();
   pdk::puint32 n;
   s >> n;
   c.reserve(n);
   for (pdk::puint32 i = 0; i < n; ++i) {
      typename Container::value_type t;
      s >> t;
      if (s.getStatus() != DataStream::Status::Ok) {
         c.clear();
         break;
      }
      c.push_back(t);
   }
   
   return s;
}

template <typename Container>
DataStream &read_list_based_container(DataStream &s, Container &c)
{
   StreamStateSaver stateSaver(&s);
   
   c.clear();
   pdk::puint32 n;
   s >> n;
   for (pdk::puint32 i = 0; i < n; ++i) {
      typename Container::value_type t;
      s >> t;
      if (s.getStatus() != DataStream::Status::Ok) {
         c.clear();
         break;
      }
      c.push_back(t);
   }
   
   return s;
}

template <typename Container>
DataStream &read_associative_container(DataStream &s, Container &c)
{
   StreamStateSaver stateSaver(&s);
   
   c.clear();
   pdk::puint32 n;
   s >> n;
   for (pdk::puint32 i = 0; i < n; ++i) {
      typename Container::key_type k;
      typename Container::value_type t;
      s >> k >> t;
      if (s.getStatus() != DataStream::Status::Ok) {
         c.clear();
         break;
      }
      c.insert(k, t);
   }
   
   return s;
}

template <typename Container>
DataStream &write_sequential_container(DataStream &s, const Container &c)
{
   s << pdk::puint32(c.size());
   for (const typename Container::value_type &t : c) {
      s << t;
   }
   return s;
}

template <typename Container>
DataStream &write_associative_container(DataStream &s, const Container &c)
{
   s << pdk::puint32(c.size());
   // Deserialization should occur in the reverse order.
   // Otherwise, value() will return the least recently inserted
   // value instead of the most recently inserted one.
   auto it = c.cend();
   auto begin = c.cbegin();
   while (it != begin) {
      --it;
      s << it->first << it->second;
   }
   
   return s;
}

} // internal namespace

inline IoDevice *DataStream::getDevice() const
{
   return m_device;
}

inline DataStream::ByteOrder DataStream::getByteOrder() const
{
   return m_byteorder;
}

inline DataStream::Version DataStream::getVersion() const
{
   return m_version;
}

inline void DataStream::setVersion(DataStream::Version v)
{
   m_version = v;
}

inline DataStream &DataStream::operator>>(pdk::puint8 &i)
{
   return *this >> reinterpret_cast<pdk::pint8 &>(i);
}

inline DataStream &DataStream::operator>>(pdk::puint16 &i)
{
   return *this >> reinterpret_cast<pdk::pint16 &>(i);
}

inline DataStream &DataStream::operator>>(pdk::puint32 &i)
{
   return *this >> reinterpret_cast<pdk::pint32 &>(i);
}

inline DataStream &DataStream::operator>>(pdk::puint64 &i)
{
   return *this >> reinterpret_cast<pdk::pint64 &>(i);
}

inline DataStream &DataStream::operator<<(pdk::puint8 i)
{
   return *this << pdk::pint8(i);
}

inline DataStream &DataStream::operator<<(pdk::puint16 i)
{
   return *this << pdk::pint16(i);
}

inline DataStream &DataStream::operator<<(pdk::puint32 i)
{
   return *this << pdk::pint32(i);
}

inline DataStream &DataStream::operator<<(pdk::puint64 i)
{
   return *this << pdk::pint64(i);
}

template <typename Enum>
inline DataStream &operator<<(DataStream &s, Flags<Enum> e)
{
   return s << e.m_data;
}

template <typename Enum>
inline DataStream &operator>>(DataStream &s, Flags<Enum> &e)
{
   return s >> e.m_data;
}

template <typename T>
inline DataStream &operator>>(DataStream &s, std::list<T> &l)
{
    return internal::read_list_based_container(s, l);
}

template <typename T>
inline DataStream &operator<<(DataStream &s, const std::list<T> &l)
{
    return internal::write_sequential_container(s, l);
}

template<typename T>
inline DataStream &operator>>(DataStream &s, std::vector<T> &v)
{
    return internal::read_array_based_container(s, v);
}

template<typename T>
inline DataStream &operator<<(DataStream &s, const std::vector<T> &v)
{
    return internal::write_sequential_container(s, v);
}

template <typename T>
inline DataStream &operator>>(DataStream &s, std::set<T> &set)
{
    return internal::read_list_based_container(s, set);
}

template <typename T>
inline DataStream &operator<<(DataStream &s, const std::set<T> &set)
{
    return internal::write_sequential_container(s, set);
}

template <class Key, class T>
inline DataStream &operator>>(DataStream &s, std::map<Key, T> &map)
{
    return internal::read_associative_container(s, map);
}

template <class Key, class T>
inline DataStream &operator<<(DataStream &s, const std::map<Key, T> &map)
{
    return internal::write_associative_container(s, map);
}

template <class T1, class T2>
inline DataStream& operator>>(DataStream& s, std::pair<T1, T2>& p)
{
    s >> p.first >> p.second;
    return s;
}

template <class T1, class T2>
inline DataStream& operator<<(DataStream& s, const std::pair<T1, T2>& p)
{
    s << p.first << p.second;
    return s;
}

} // io
} // pdk

#endif //  PDK_M_BASE_IO_DATA_STREAM_H
