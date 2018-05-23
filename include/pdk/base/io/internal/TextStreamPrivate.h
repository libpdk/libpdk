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
// Created by softboy on 2018/01/30.

#ifndef PDK_M_BASE_IO_INTERNAL_TEXTSTREAM_PRIVATE_H
#define PDK_M_BASE_IO_INTERNAL_TEXTSTREAM_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/io/TextStream.h"
#include "pdk/kernel/Object.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/text/codecs/TextCodec.h"

namespace pdk {
namespace io {
namespace internal {

using pdk::kernel::Object;
using pdk::text::codecs::TextCodec;
using pdk::lang::String;

class DeviceClosedNotifier : public Object
{
public:
   inline DeviceClosedNotifier()
   {}
   
   inline void setupDevice(TextStream *stream, IoDevice *device)
   {
      // @TODO need disconnect ?
      if (device) {
         device->connectAboutToCloseSignal(this, &DeviceClosedNotifier::flushStream);
      }
      this->m_stream = stream;
   }
   
   // SLOTS
public:
   inline void flushStream()
   {
      m_stream->flush();
   }
   
private:
   TextStream *m_stream;
};

class TextStreamPrivate
{
   PDK_DECLARE_PUBLIC(TextStream);
public:
   class Params
   {
   public:
      void reset();
      
      int m_realNumberPrecision;
      int m_integerBase;
      int m_fieldWidth;
      Character m_padChar;
      TextStream::FieldAlignment m_fieldAlignment;
      TextStream::RealNumberNotation m_realNumberNotation;
      TextStream::NumberFlags m_numberFlags;
   };   
   
   TextStreamPrivate(TextStream *apiPtr);
   ~TextStreamPrivate();
   void reset();
   
   IoDevice *m_device;
   DeviceClosedNotifier m_deviceClosedNotifier;
   // string
   String *m_string;
   int m_stringOffset;
   IoDevice::OpenModes m_stringOpenMode;
   TextCodec *m_codec;
   TextCodec::ConverterState m_readConverterState;
   TextCodec::ConverterState m_writeConverterState;
   TextCodec::ConverterState *m_readConverterSavedState;
   
   String m_writeBuffer;
   String m_readBuffer;
   int m_readBufferOffset;
   int m_readConverterSavedStateOffset; //the offset between readBufferStartDevicePos and that start of the buffer
   pdk::pint64 m_readBufferStartDevicePos;
   
   Params m_params;
   
   // status
   TextStream::Status m_status;
   Locale m_locale;
   TextStream *m_apiPtr;
   
   int m_lastTokenSize;
   bool m_deleteDevice;
   bool m_autoDetectUnicode;
   
   // i/o
   enum class TokenDelimiter
   {
      Space,
      NotSpace,
      EndOfLine
   };
   
   String read(int maxLength);
   bool scan(const Character **ptr, int *tokenLength,
             int maxLength, TokenDelimiter delimiter);
   inline const Character *readPtr() const;
   inline void consumeLastToken();
   inline void consume(int nchars);
   void saveConverterState(pdk::pint64 newPos);
   void restoreToSavedConverterState();
   
   // Return value type for getNumber()
   enum class NumberParsingStatus 
   {
      npsOk,
      npsMissingDigit,
      npsInvalidPrefix
   };
   
   inline bool getChar(Character *ch);
   inline void ungetChar(Character ch);
   NumberParsingStatus getNumber(pulonglong *l);
   bool getReal(double *f);
   
   inline void write(const String &data) 
   {
      write(data.begin(), data.length());
   }
   
   inline void write(Character ch);
   void write(const Character *data, int len);
   void write(Latin1String data);
   void writePadding(int len);
   inline void putString(const String &ch, bool number = false)
   {
      putString(ch.getRawData(), ch.length(), number);
   }
   void putString(const Character *data, int len, bool number = false);
   void putString(Latin1String data, bool number = false);
   inline void putChar(Character ch);
   void putNumber(pulonglong number, bool negative);
   
   struct PaddingResult {
      int left;
      int right;
   };
   PaddingResult padding(int len) const;
   
   // buffers
   bool fillReadBuffer(pdk::pint64 maxBytes = -1);
   void resetReadBuffer();
   void flushWriteBuffer();
};

} // internal
} // io
} // pdk

#endif // PDK_M_BASE_IO_INTERNAL_TEXTSTREAM_PRIVATE_H
