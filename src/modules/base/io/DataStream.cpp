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

#include "pdk/base/io/DataStream.h"
#include "pdk/base/io/internal/DataStreamPrivate.h"
#include "pdk/base/io/Buffer.h"
#include "pdk/base/lang/String.h"
#include "pdk/global/Endian.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

namespace pdk {
namespace io {

#define PDK_VOID

#undef  CHECK_STREAM_PRECOND
#ifndef PDK_NO_DEBUG
#define CHECK_STREAM_PRECOND(retVal) \
   if (!m_device) { \
   return retVal; \
}
#else
#define CHECK_STREAM_PRECOND(retVal) \
   if (!m_device) { \
   return retVal; \
}
#endif

//#ifndef PDK_NO_DEBUG
//#define CHECK_STREAM_PRECOND(retVal) \
//   if (!m_device) { \
//   //warning_stream("DataStream: No device"); \
//return retVal; \
//}
//#else
//#define CHECK_STREAM_PRECOND(retVal) \
//   if (!m_device) { \
//   return retVal; \
//}
//#endif

#define CHECK_STREAM_WRITE_PRECOND(retVal) \
   CHECK_STREAM_PRECOND(retVal) \
   if (m_status != Status::Ok) \
   return retVal;

#define CHECK_STREAM_TRANSACTION_PRECOND(retVal) \
   if (!m_implPtr || m_implPtr->m_transactionDepth == 0) { \
   return retVal; \
}

//#define CHECK_STREAM_TRANSACTION_PRECOND(retVal) \
//   if (!implPtr || implPtr->m_transactionDepth == 0) { \
//   //warning_stream("DataStream: No transaction in progress"); \
//      return retVal; \
//}

DataStream::DataStream()
{
   m_device = 0;
   m_ownDevice = false;
   m_byteorder = ByteOrder::BigEndian;
   m_version = pdk::as_integer<Version>(Version::DefaultCompiledVersion);
   m_noswap = SysInfo::ByteOrder == SysInfo::BigEndian;
   m_status = Status::Ok;
}

DataStream::DataStream(IoDevice *d)
{
   m_device = d;                                // set device
   m_ownDevice = false;
   m_byteorder = ByteOrder::BigEndian;                        // default byte order
   m_version = pdk::as_integer<Version>(Version::DefaultCompiledVersion);
   m_noswap = SysInfo::ByteOrder == SysInfo::BigEndian;
   m_status = Status::Ok;
}

DataStream::DataStream(ByteArray *a, IoDevice::OpenMode flags)
{
   Buffer *buf = new Buffer(a);
   //#ifndef PDK_NO_OBJECT
   //   buf->blockSignals(true);
   //#endif
   buf->open(flags);
   m_device = buf;
   m_ownDevice = true;
   m_byteorder = ByteOrder::BigEndian;
   m_version = pdk::as_integer<Version>(Version::DefaultCompiledVersion);
   m_noswap = SysInfo::ByteOrder == SysInfo::BigEndian;
   m_status = Status::Ok;
}

DataStream::DataStream(const ByteArray &a)
{
   Buffer *buf = new Buffer;
   //#ifndef PDK_NO_OBJECT
   //   buf->blockSignals(true);
   //#endif
   buf->setData(a);
   buf->open(IoDevice::OpenMode::ReadOnly);
   m_device = buf;
   m_ownDevice = true;
   m_byteorder = ByteOrder::BigEndian;
   m_version = pdk::as_integer<Version>(Version::DefaultCompiledVersion);
   m_noswap = SysInfo::ByteOrder == SysInfo::BigEndian;
   m_status = Status::Ok;
}

DataStream::~DataStream()
{
   if (m_ownDevice) {
      delete m_device;
   }
}

void DataStream::setDevice(IoDevice *d)
{
   if (m_ownDevice) {
      delete m_device;
      m_ownDevice = false;
   }
   m_device = d;
}

void DataStream::unsetDevice()
{
   setDevice(0);
}

bool DataStream::atEnd() const
{
   return m_device ? m_device->atEnd() : true;
}

DataStream::FloatingPointPrecision DataStream::floatingPointPrecision() const
{
   return m_implPtr == nullptr 
         ? DataStream::FloatingPointPrecision::DoublePrecision 
         : m_implPtr->m_floatingPointPrecision;
}

void DataStream::setFloatingPointPrecision(DataStream::FloatingPointPrecision precision)
{
   if (m_implPtr == nullptr) {
      m_implPtr.reset(new DataStreamPrivate());
   }
   m_implPtr->m_floatingPointPrecision = precision;
}

DataStream::Status DataStream::status() const
{
   return m_status;
}

void DataStream::resetStatus()
{
   m_status = Status::Ok;
}

void DataStream::setStatus(Status status)
{
   if (m_status == Status::Ok) {
      m_status = status;
   }
}

void DataStream::setByteOrder(ByteOrder bo)
{
   m_byteorder = bo;
   if (SysInfo::ByteOrder == SysInfo::BigEndian) {
      m_noswap = (m_byteorder == ByteOrder::BigEndian);
   } else {
      m_noswap = (m_byteorder == ByteOrder::LittleEndian);
   }
}

void DataStream::startTransaction()
{
   CHECK_STREAM_PRECOND(PDK_VOID);
   if (m_implPtr == nullptr) {
      m_implPtr.reset(new DataStreamPrivate());
   }
   if (++m_implPtr->m_transactionDepth == 1) {
      m_device->startTransaction();
      resetStatus();
   }
}

bool DataStream::commitTransaction()
{
   CHECK_STREAM_TRANSACTION_PRECOND(false);
   if (--m_implPtr->m_transactionDepth == 0) {
      CHECK_STREAM_PRECOND(false);
      if (m_status == Status::ReadPastEnd) {
         m_device->rollbackTransaction();
         return false;
      }
      m_device->commitTransaction();
   }
   return m_status == Status::Ok;
}

void DataStream::rollbackTransaction()
{
   setStatus(Status::ReadPastEnd);
   CHECK_STREAM_TRANSACTION_PRECOND(PDK_VOID);
   if (--m_implPtr->m_transactionDepth != 0) {
      return;
   }
   CHECK_STREAM_PRECOND(PDK_VOID);
   if (m_status == Status::ReadPastEnd) {
      m_device->rollbackTransaction();
   } else {
      m_device->commitTransaction();
   }
}

void DataStream::abortTransaction()
{
   m_status = Status::ReadCorruptData;
   CHECK_STREAM_TRANSACTION_PRECOND(PDK_VOID);
   if (--m_implPtr->m_transactionDepth != 0) {
      return;
   }
   CHECK_STREAM_PRECOND(PDK_VOID);
   m_device->commitTransaction();
}

/*****************************************************************************
  DataStream read functions
 *****************************************************************************/

int DataStream::readBlock(char *data, int len)
{
   // Disable reads on failure in transacted stream
   if (m_status != Status::Ok && m_device->isTransactionStarted()){
      return -1;      
   }
   const int readResult = m_device->read(data, len);
   if (readResult != len) {
      setStatus(Status::ReadPastEnd);
   }
   return readResult;
}

DataStream &DataStream::operator>>(pdk::pint8 &i)
{
   i = 0;
   CHECK_STREAM_PRECOND(*this);
   char c;
   if (readBlock(&c, 1) == 1) {
      i = pdk::pint8(c);
   }
   return *this;
}

DataStream &DataStream::operator>>(pdk::pint16 &i)
{
   i = 0;
   CHECK_STREAM_PRECOND(*this);
   if (readBlock(reinterpret_cast<char *>(&i), 2) != 2) {
      i = 0;
   } else {
      if (!m_noswap) {
         i = pdk::bswap(i);
      }
   }
   return *this;
}

DataStream &DataStream::operator>>(pdk::pint32 &i)
{
   i = 0;
   CHECK_STREAM_PRECOND(*this);
   if (readBlock(reinterpret_cast<char *>(&i), 4) != 4) {
      i = 0;
   } else {
      if (!m_noswap) {
         i = pdk::bswap(i);
      }
   }
   return *this;
}

DataStream &DataStream::operator>>(pdk::pint64 &i)
{
   i = pdk::pint64(0);
   CHECK_STREAM_PRECOND(*this);
   if (version() < 6) {
      pdk::puint32 i1, i2;
      *this >> i2 >> i1;
      i = ((pdk::puint64)i1 << 32) + i2;
   } else {
      if (readBlock(reinterpret_cast<char *>(&i), 8) != 8) {
         i = pdk::pint64(0);
      } else {
         if (!m_noswap) {
            i = pdk::bswap(i);
         }
      }
   }
   return *this;
}

DataStream &DataStream::operator>>(bool &i)
{
   pdk::pint8 v;
   *this >> v;
   i = !!v;
   return *this;
}

DataStream &DataStream::operator>>(float &f)
{
   if (floatingPointPrecision() == DataStream::FloatingPointPrecision::DoublePrecision) {
      double d;
      *this >> d;
      f = d;
      return *this;
   }
   
   f = 0.0f;
   CHECK_STREAM_PRECOND(*this);
   if (readBlock(reinterpret_cast<char *>(&f), 4) != 4) {
      f = 0.0f;
   } else {
      if (!m_noswap) {
         union {
            float val1;
            pdk::puint32 val2;
         } x;
         x.val2 = pdk::bswap(*reinterpret_cast<pdk::puint32 *>(&f));
         f = x.val1;
      }
   }
   return *this;
}

DataStream &DataStream::operator>>(double &f)
{
   if (floatingPointPrecision() == DataStream::FloatingPointPrecision::SinglePrecision) {
      float d;
      *this >> d;
      f = d;
      return *this;
   }
   
   f = 0.0;
   CHECK_STREAM_PRECOND(*this);
   if (readBlock(reinterpret_cast<char *>(&f), 8) != 8) {
      f = 0.0;
   } else {
      if (!m_noswap) {
         union {
            double val1;
            pdk::puint64 val2;
         } x;
         x.val2 = pdk::bswap(*reinterpret_cast<pdk::puint64 *>(&f));
         f = x.val1;
      }
   }
   return *this;
}

DataStream &DataStream::operator>>(char *&s)
{
   uint len = 0;
   return readBytes(s, len);
}

DataStream &DataStream::readBytes(char *&s, uint &l)
{
   s = 0;
   l = 0;
   CHECK_STREAM_PRECOND(*this);
   pdk::puint32 len;
   *this >> len;
   if (len == 0) {
      return *this;
   }
   const pdk::puint32 Step = 1024 * 1024;
   pdk::puint32 allocated = 0;
   char *prevBuf = 0;
   char *curBuf = 0;
   do {
      int blockSize = std::min(Step, len - allocated);
      prevBuf = curBuf;
      curBuf = new char[allocated + blockSize + 1];
      if (prevBuf) {
         std::memcpy(curBuf, prevBuf, allocated);
         delete [] prevBuf;
      }
      if (readBlock(curBuf + allocated, blockSize) != blockSize) {
         delete [] curBuf;
         return *this;
      }
      allocated += blockSize;
   } while (allocated < len);
   
   s = curBuf;
   s[len] = '\0';
   l = (uint)len;
   return *this;
}

int DataStream::readRawData(char *s, int len)
{
   CHECK_STREAM_PRECOND(-1);
   return readBlock(s, len);
}

/*****************************************************************************
  DataStream write functions
 *****************************************************************************/

DataStream &DataStream::operator<<(pdk::pint8 i)
{
   CHECK_STREAM_WRITE_PRECOND(*this);
   if (!m_device->putChar(i)) {
      m_status = Status::WriteFailed;
   } 
   return *this;
}

DataStream &DataStream::operator<<(pdk::pint16 i)
{
   CHECK_STREAM_WRITE_PRECOND(*this);
   if (!m_noswap) {
      i = pdk::bswap(i);
   }
   if (m_device->write((char *)&i, sizeof(pdk::pint16)) != sizeof(pdk::pint16)) {
      m_status = Status::WriteFailed;
   }
   return *this;
}

DataStream &DataStream::operator<<(pdk::pint32 i)
{
   CHECK_STREAM_WRITE_PRECOND(*this);
   if (!m_noswap) {
      i = pdk::bswap(i);
   }
   if (m_device->write((char *)&i, sizeof(pdk::pint32)) != sizeof(pdk::pint32)) {
      m_status = Status::WriteFailed;
   }
   return *this;
}

DataStream &DataStream::operator<<(pdk::pint64 i)
{
   CHECK_STREAM_WRITE_PRECOND(*this);
   if (!m_noswap) {
      i = pdk::bswap(i);
   }
   if (m_device->write((char *)&i, sizeof(pdk::pint64)) != sizeof(pdk::pint64)) {
      m_status = Status::WriteFailed;
   }
   return *this;
}

DataStream &DataStream::operator<<(bool i)
{
   CHECK_STREAM_WRITE_PRECOND(*this);
   if (!m_device->putChar(pdk::pint8(i))) {
      m_status = Status::WriteFailed;
   }
   return *this;
}

DataStream &DataStream::operator<<(float f)
{
   if (floatingPointPrecision() == DataStream::FloatingPointPrecision::DoublePrecision) {
      *this << double(f);
      return *this;
   }
   
   CHECK_STREAM_WRITE_PRECOND(*this);
   float g = f;                                // fixes float-on-stack problem
   if (!m_noswap) {
      union {
         float val1;
         pdk::puint32 val2;
      } x;
      x.val1 = g;
      x.val2 = pdk::bswap(x.val2);
      
      if (m_device->write((char *)&x.val2, sizeof(float)) != sizeof(float)) {
         m_status = Status::WriteFailed;
      } 
      return *this;
   }
   if (m_device->write((char *)&g, sizeof(float)) != sizeof(float)) {
      m_status = Status::WriteFailed;
   }
   return *this;
}

DataStream &DataStream::operator<<(double f)
{
   if (floatingPointPrecision() == DataStream::FloatingPointPrecision::SinglePrecision) {
      *this << float(f);
      return *this;
   }
   CHECK_STREAM_WRITE_PRECOND(*this);
   if (m_noswap) {
      if (m_device->write((char *)&f, sizeof(double)) != sizeof(double)) {
         m_status = Status::WriteFailed;
      }
   } else {
      union {
         double val1;
         pdk::puint64 val2;
      } x;
      x.val1 = f;
      x.val2 = pdk::bswap(x.val2);
      if (m_device->write((char *)&x.val2, sizeof(double)) != sizeof(double)) {
         m_status = Status::WriteFailed;
      }
   }
   return *this;
}

DataStream &DataStream::operator<<(const char *s)
{
   if (!s) {
      *this << (pdk::puint32)0;
      return *this;
   }
   uint len = pdk::strlen(s) + 1;                        // also write null terminator
   *this << (pdk::puint32)len;                        // write length specifier
   writeRawData(s, len);
   return *this;
}

DataStream &DataStream::writeBytes(const char *s, uint len)
{
   CHECK_STREAM_WRITE_PRECOND(*this);
   *this << (pdk::puint32)len;                        // write length specifier
   if (len)
      writeRawData(s, len);
   return *this;
}

int DataStream::writeRawData(const char *s, int len)
{
   CHECK_STREAM_WRITE_PRECOND(-1);
   int ret = m_device->write(s, len);
   if (ret != len) {
      m_status = Status::WriteFailed;
   }
   return ret;
}

int DataStream::skipRawData(int len)
{
   CHECK_STREAM_PRECOND(-1);
   if (m_status != Status::Ok && m_device->isTransactionStarted()) {
      return -1;
   }
   const int skipResult = m_device->skip(len);
   if (skipResult != len) {
      setStatus(Status::ReadPastEnd);
   }
   return skipResult;
}

} // io
} // pdk


