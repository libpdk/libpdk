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
// Created by softboy on 2018/02/03.

#include "pdk/base/io/TextStream.h"
#include "pdk/base/text/codecs/TextCodec.h"
#include "pdk/base/io/internal/TextStreamPrivate.h"
#include "pdk/base/io/Buffer.h"
#include "pdk/global/Numeric.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/kernel/StringUtils.h"
#include <cctype>
#include <locale.h>
#include <cstdlib>
#include <limits>
#include <new>

static const int PDK_TEXTSTREAM_BUFFERSIZE = 16384;

// Base implementations of operator>> for ints and reals
#define IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(type) do { \
   PDK_D(TextStream); \
   CHECK_VALID_STREAM(*this); \
   pdk::pulonglong tmp; \
   switch (implPtr->getNumber(&tmp)) { \
   case TextStreamPrivate::NumberParsingStatus::npsOk: \
   i = (type)tmp; \
   break; \
   case TextStreamPrivate::NumberParsingStatus::npsMissingDigit: \
   case TextStreamPrivate::NumberParsingStatus::npsInvalidPrefix: \
   i = (type)0; \
   setStatus(atEnd() ? TextStream::Status::ReadPastEnd : TextStream::Status::ReadCorruptData); \
   break; \
   } \
   return *this; } while (false)

#define IMPLEMENT_STREAM_RIGHT_REAL_OPERATOR(type) do { \
   PDK_D(TextStream); \
   CHECK_VALID_STREAM(*this); \
   double tmp; \
   if (implPtr->getReal(&tmp)) { \
   f = (type)tmp; \
   } else { \
   f = (type)0; \
   setStatus(atEnd() ? TextStream::Status::ReadPastEnd : TextStream::Status::ReadCorruptData); \
   } \
   return *this; } while (false)

namespace pdk {
namespace io {

using pdk::ds::ByteArray;
using pdk::text::codecs::TextCodec;
using pdk::lang::Latin1Character;
using pdk::io::IoDevice;

namespace {

// Returns a human readable representation of the first \a len
// characters in \a data.
ByteArray pdk_pretty_debug(const char *data, int len, int maxSize)
{
   if (!data) {
      return "(null)";
   }
   ByteArray out;
   for (int i = 0; i < len; ++i) {
      char c = data[i];
      if (std::isprint(int(uchar(c)))) {
         out += c;
      } else {
         switch (c) {
         case '\n': out += "\\n"; break;
         case '\r': out += "\\r"; break;
         case '\t': out += "\\t"; break;
         default: {
            const char buf[] = {
               '\\',
               'x',
               pdk::to_hex_lower(uchar(c) / 16),
               pdk::to_hex_lower(uchar(c) % 16),
               0
            };
            out += buf;
         }
         }
      }
   }
   
   if (len < maxSize) {
      out += "...";
   }
   return out;
}

void reset_codec_converter_state_helper(TextCodec::ConverterState *state)
{
   state->~ConverterState();
   new (state) TextCodec::ConverterState;
}

void copy_converter_state_helper(TextCodec::ConverterState *dest,
                                 const TextCodec::ConverterState *src)
{
   // ### TextCodec::ConverterState's copy constructors and assignments are
   // private. This function copies the structure manually.
   PDK_ASSERT(!src->m_data);
   dest->m_flags = src->m_flags;
   dest->m_invalidChars = src->m_invalidChars;
   dest->m_stateData[0] = src->m_stateData[0];
   dest->m_stateData[1] = src->m_stateData[1];
   dest->m_stateData[2] = src->m_stateData[2];
}

} // anonymous namespace

namespace internal {

TextStreamPrivate::TextStreamPrivate(TextStream *apiPtr)
   : m_readConverterSavedState(nullptr),
     m_readConverterSavedStateOffset(0),
     m_locale(Locale::c())
{
   this->m_apiPtr = apiPtr;
   reset();
}

TextStreamPrivate::~TextStreamPrivate()
{
   if (m_deleteDevice) {
      //      m_device->blockSignals(true);
      delete m_device;
   }
   delete m_readConverterSavedState;
}

void TextStreamPrivate::Params::reset()
{
   m_realNumberPrecision = 6;
   m_integerBase = 0;
   m_fieldWidth = 0;
   m_padChar = Latin1Character(' ');
   m_fieldAlignment = TextStream::FieldAlignment::AlignRight;
   m_realNumberNotation = TextStream::RealNumberNotation::SmartNotation;
   m_numberFlags = 0;
}

void TextStreamPrivate::reset()
{
   m_params.reset();
   m_device = nullptr;
   m_deleteDevice = false;
   m_string = nullptr;
   m_stringOffset = 0;
   m_stringOpenMode = IoDevice::NotOpen;
   m_readBufferOffset = 0;
   m_readBufferStartDevicePos = 0;
   m_lastTokenSize = 0;
   
   m_codec = TextCodec::getCodecForLocale();
   reset_codec_converter_state_helper(&m_readConverterState);
   reset_codec_converter_state_helper(&m_writeConverterState);
   delete m_readConverterSavedState;
   m_readConverterSavedState = 0;
   m_writeConverterState.m_flags |= TextCodec::ConversionFlag::IgnoreHeader;
   m_autoDetectUnicode = true;
   
}

bool TextStreamPrivate::fillReadBuffer(pdk::pint64 maxBytes)
{
   // no buffer next to the String itself; this function should only
   // be called internally, for devices.
   PDK_ASSERT(!m_string);
   PDK_ASSERT(m_device);
   
   // handle text translation and bypass the Text flag in the device.
   bool textModeEnabled = m_device->isTextModeEnabled();
   if (textModeEnabled) {
      m_device->setTextModeEnabled(false);
   }
   // read raw data into a temporary buffer
   char buf[PDK_TEXTSTREAM_BUFFERSIZE];
   pdk::pint64 bytesRead = 0;
#if defined(PDK_OS_WIN)
   // On Windows, there is no non-blocking stdin - so we fall back to reading
   // lines instead. If there is no OBJECT, we read lines for all sequential
   // devices; otherwise, we read lines only for stdin.
   QFile *file = 0;
   Q_UNUSED(file);
   if (device->isSequential()
       && (file = qobject_cast<QFile *>(device)) && file->handle() == 0
       ) {
      if (maxBytes != -1)
         bytesRead = device->readLine(buf, std:min<pdk::pint64>(sizeof(buf), maxBytes));
      else
         bytesRead = device->readLine(buf, sizeof(buf));
   } else
#endif
   {
      if (maxBytes != -1) {
         bytesRead = m_device->read(buf, std::min<pdk::pint64>(sizeof(buf), maxBytes));
      } else {
         bytesRead = m_device->read(buf, sizeof(buf));
      }
   }
   // reset the Text flag.
   if (textModeEnabled) {
      m_device->setTextModeEnabled(true);
   }
   if (bytesRead <= 0) {
      return false;
   }
   // codec auto detection, explicitly defaults to locale encoding if the
   // codec has been set to 0.
   if (!m_codec || m_autoDetectUnicode) {
      m_autoDetectUnicode = false;
      m_codec = TextCodec::codecForUtfText(ByteArray::fromRawData(buf, bytesRead), m_codec);
      if (!m_codec) {
         m_codec = TextCodec::getCodecForLocale();
         m_writeConverterState.m_flags |= TextCodec::ConversionFlag::IgnoreHeader;
      }
   }
#if defined (PDK_TEXTSTREAM_DEBUG)
   qDebug("TextStreamPrivate::fillReadBuffer(), using %s codec",
          codec ? codec->name().constData() : "no");
#endif
   
#if defined (PDK_TEXTSTREAM_DEBUG)
   qDebug("TextStreamPrivate::fillReadBuffer(), device->read(\"%s\", %d) == %d",
          pdk_pretty_debug(buf, std::min(32,int(bytesRead)) , int(bytesRead)).getConstRawData(), int(sizeof(buf)), int(bytesRead));
#endif
   
   int oldReadBufferSize = m_readBuffer.size();
   // convert to unicode
   m_readBuffer += PDK_LIKELY(m_codec) ? m_codec->toUnicode(buf, bytesRead, &m_readConverterState)
                                       : String::fromLatin1(buf, bytesRead);
   
   // remove all '\r\n' in the string.
   if (m_readBuffer.size() > oldReadBufferSize && textModeEnabled) {
      Character CR = Latin1Character('\r');
      Character *writePtr = m_readBuffer.getRawData() + oldReadBufferSize;
      Character *readPtr = m_readBuffer.getRawData() + oldReadBufferSize;
      Character *endPtr = m_readBuffer.getRawData() + m_readBuffer.size();
      
      int n = oldReadBufferSize;
      if (readPtr < endPtr) {
         // Cut-off to avoid unnecessary self-copying.
         while (*readPtr++ != CR) {
            ++n;
            if (++writePtr == endPtr) {
               break;
            }
         }
      }
      while (readPtr < endPtr) {
         Character ch = *readPtr++;
         if (ch != CR) {
            *writePtr++ = ch;
         } else {
            if (n < m_readBufferOffset) {
               --m_readBufferOffset;
            }
            --bytesRead;
         }
         ++n;
      }
      m_readBuffer.resize(writePtr - m_readBuffer.getRawData());
   }
   
#if defined (PDK_TEXTSTREAM_DEBUG)
   qDebug("TextStreamPriv::fillReadBuffer() read %d bytes from device. readBuffer = [%s]", int(bytesRead),
          pdk_pretty_debug(readBuffer.toLatin1(), readBuffer.size(), readBuffer.size()).getRawData());
#endif
   return true;
}


} // internal

} // io
} // pdk


