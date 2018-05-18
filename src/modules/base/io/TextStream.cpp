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
#include "pdk/base/io/internal/TextStreamPrivate.h"
#include "pdk/base/io/Buffer.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/text/codecs/TextCodec.h"
#include "pdk/global/internal/NumericPrivate.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/kernel/StringUtils.h"
#include "pdk/utils/internal/LocalePrivate.h"
#include <cctype>
#include <locale.h>
#include <cstdlib>
#include <limits>
#include <new>

static const int PDK_TEXTSTREAM_BUFFERSIZE = 16384;
#define PDK_VOID
#define CHECK_VALID_STREAM(x) do { \
   if (!m_implPtr->m_string && !m_implPtr->m_device) { \
   warning_stream("TextStream: No device"); \
   return x; \
   } } while (0)

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
using pdk::lang::String;
using pdk::lang::StringRef;
using pdk::utils::internal::LocaleData;
using pdk::io::fs::File;

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
   m_stringOpenMode = IoDevice::OpenMode::NotOpen;
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
   File *file = 0;
   Q_UNUSED(file);
   if (device->isSequential()
       && (file = qobject_cast<File *>(device)) && file->handle() == 0
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
#if defined (PDK_PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStreamPrivate::fillReadBuffer(), using %s codec",
                codec ? codec->name().constData() : "no");
#endif
   
#if defined (PDK_PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStreamPrivate::fillReadBuffer(), device->read(\"%s\", %d) == %d",
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
   
#if defined (PDK_PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStreamPriv::fillReadBuffer() read %d bytes from device. readBuffer = [%s]", int(bytesRead),
                pdk_pretty_debug(readBuffer.toLatin1(), readBuffer.size(), readBuffer.size()).getRawData());
#endif
   return true;
}

void TextStreamPrivate::resetReadBuffer()
{
   m_readBuffer.clear();
   m_readBufferOffset = 0;
   m_readBufferStartDevicePos = (m_device ? m_device->getPosition() : 0);
}

void TextStreamPrivate::flushWriteBuffer()
{
   // no buffer next to the String itself; this function should only
   // be called internally, for devices.
   if (m_string || !m_device) {
      return;
   }
   // Stream went bye-bye already. Appending further data may succeed again,
   // but would create a corrupted stream anyway.
   if (m_status != TextStream::Status::Ok) {
      return;
   }
   if (m_writeBuffer.isEmpty()) {
      return;
   }
#if defined (PDK_OS_WIN)
   // handle text translation and bypass the Text flag in the device.
   bool textModeEnabled = m_device->isTextModeEnabled();
   if (textModeEnabled) {
      m_device->setTextModeEnabled(false);
      m_writeBuffer.replace(Latin1Character('\n'), Latin1String("\r\n"));
   }
#endif
   
#ifndef PDK_NO_TEXTCODEC
   if (!m_codec)
      m_codec = TextCodec::getCodecForLocale();
#if defined (PDK_PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStreamPrivate::flushWriteBuffer(), using %s codec (%s generating BOM)",
                m_codec ? m_codec->name().getConstRawData() : "no",
                !m_codec || (writeConverterState.m_flags & TextCodec::ConversionFlag::IgnoreHeader) ? "not" : "");
#endif
   
   // convert from unicode to raw data
   // codec might be null if we're already inside global destructors (TestCodec::codecForLocale returned null)
   ByteArray data = PDK_LIKELY(m_codec) 
         ? m_codec->fromUnicode(m_writeBuffer.getRawData(), m_writeBuffer.size(), &m_writeConverterState)
         : m_writeBuffer.toLatin1();
#else
   ByteArray data = m_writeBuffer.toLatin1();
#endif
   m_writeBuffer.clear();
   
   // write raw data to the device
   pdk::pint64 bytesWritten = m_device->write(data);
#if defined (PDK_PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStreamPrivate::flushWriteBuffer(), m_device->write(\"%s\") == %d",
                pdk_pretty_debug(data.getConstRawData(), std::min(data.size(),32), data.size()).getConstRawData(), int(bytesWritten));
#endif
   
#if defined (PDK_OS_WIN)
   // reset the text flag
   if (textModeEnabled) {
      m_device->setTextModeEnabled(true);
   }
   
#endif
   
   if (bytesWritten <= 0) {
      m_status = TextStream::Status::WriteFailed;
      return;
   }
   
   // flush the file
   //#ifndef PDK_NO_QOBJECT
   //   FileDevice *file = qobject_cast<FileDevice *>(device);
   //   bool flushed = !file || file->flush();
   //#else
   bool flushed = true;
   //#endif
   
#if defined (PDK_PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStreamPrivate::flushWriteBuffer() wrote %d bytes",
                int(bytesWritten));
#endif
   if (!flushed || bytesWritten != pdk::pint64(data.size())) {
      m_status = TextStream::Status::WriteFailed;
   }
}

String TextStreamPrivate::read(int maxlen)
{
   String ret;
   if (m_string) {
      m_lastTokenSize = std::min(maxlen, m_string->size() - m_stringOffset);
      ret = m_string->substring(m_stringOffset, m_lastTokenSize);
   } else {
      while (m_readBuffer.size() - m_readBufferOffset < maxlen && fillReadBuffer()) ;
      m_lastTokenSize = std::min(maxlen, m_readBuffer.size() - m_readBufferOffset);
      ret = m_readBuffer.substring(m_readBufferOffset, m_lastTokenSize);
   }
   consumeLastToken();
   
#if defined (PDK_PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStreamPrivate::read() maxlen = %d, token length = %d", maxlen, ret.length());
#endif
   return ret;
}

bool TextStreamPrivate::scan(const Character **ptr, int *length, int maxlen, TokenDelimiter delimiter)
{
   int totalSize = 0;
   int delimSize = 0;
   bool consumeDelimiter = false;
   bool foundToken = false;
   int startOffset = m_device ? m_readBufferOffset : m_stringOffset;
   Character lastChar;
   
   bool canStillReadFromDevice = true;
   do {
      int endOffset;
      const Character *chPtr;
      if (m_device) {
         chPtr = m_readBuffer.getConstRawData();
         endOffset = m_readBuffer.size();
      } else {
         chPtr = m_string->getConstRawData();
         endOffset = m_string->size();
      }
      chPtr += startOffset;
      for (; !foundToken && startOffset < endOffset && (!maxlen || totalSize < maxlen); ++startOffset) {
         const Character ch = *chPtr++;
         ++totalSize;
         switch (delimiter) {
         case TokenDelimiter::Space:
            if (ch.isSpace()) {
               foundToken = true;
               delimSize = 1;
            }
            break;
         case TokenDelimiter::NotSpace:
            if (!ch.isSpace()) {
               foundToken = true;
               delimSize = 1;
            }
            break;
         case TokenDelimiter::EndOfLine:
            if (ch == Latin1Character('\n')) {
               foundToken = true;
               delimSize = (lastChar == Latin1Character('\r')) ? 2 : 1;
               consumeDelimiter = true;
            }
            lastChar = ch;
            break;
         }
      }
   } while (!foundToken
            && (!maxlen || totalSize < maxlen)
            && (m_device && (canStillReadFromDevice = fillReadBuffer())));
   
   if (totalSize == 0) {
#if defined (PDK_PDK_TEXTSTREAM_DEBUG)
      debug_stream("TextStreamPrivate::scan() reached the end of input.");
#endif
      return false;
   }
   
   // if we find a '\r' at the end of the data when reading lines,
   // don't make it part of the line.
   if (delimiter == TokenDelimiter::EndOfLine && totalSize > 0 && !foundToken) {
      if (((m_string && m_stringOffset + totalSize == m_string->size()) || (m_device && m_device->atEnd()))
          && lastChar == Latin1Character('\r')) {
         consumeDelimiter = true;
         ++delimSize;
      }
   }
   
   // set the read offset and length of the token
   if (length) {
      *length = totalSize - delimSize;
   }
   
   if (ptr) {
      *ptr = readPtr();
   }
   
   
   // update last token size. the callee will call consumeLastToken() when
   // done.
   m_lastTokenSize = totalSize;
   if (!consumeDelimiter) {
      m_lastTokenSize -= delimSize;
   }  
   
#if defined (PDK_PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStreamPrivate::scan(%p, %p, %d, %x) token length = %d, delimiter = %d",
                ptr, length, maxlen, (int)delimiter, totalSize - delimSize, delimSize);
#endif
   return true;
}

inline const Character *TextStreamPrivate::readPtr() const
{
   PDK_ASSERT(m_readBufferOffset <= m_readBuffer.size());
   if (m_string) {
      return m_string->getConstRawData() + m_stringOffset;
   }
   return m_readBuffer.getConstRawData() + m_readBufferOffset;
}

inline void TextStreamPrivate::consumeLastToken()
{
   if (m_lastTokenSize) {
      consume(m_lastTokenSize);
   }
   m_lastTokenSize = 0;
}

inline void TextStreamPrivate::consume(int size)
{
#if defined (PDK_PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStreamPrivate::consume(%d)", size);
#endif
   if (m_string) {
      m_stringOffset += size;
      if (m_stringOffset > m_string->size()) {
         m_stringOffset = m_string->size();
      }
   } else {
      m_readBufferOffset += size;
      if (m_readBufferOffset >= m_readBuffer.size()) {
         m_readBufferOffset = 0;
         m_readBuffer.clear();
         saveConverterState(m_device->getPosition());
      } else if (m_readBufferOffset > PDK_TEXTSTREAM_BUFFERSIZE) {
         m_readBuffer = m_readBuffer.remove(0, m_readBufferOffset);
         m_readConverterSavedStateOffset += m_readBufferOffset;
         m_readBufferOffset = 0;
      }
   }
}

inline void TextStreamPrivate::saveConverterState(pdk::pint64 newPos)
{
#ifndef PDK_NO_TEXTCODEC
   if (m_readConverterState.m_data) {
      // converter cannot be copied, so don't save anything
      // don't update readBufferStartDevicePos either
      return;
   }
   
   if (!m_readConverterSavedState) {
      m_readConverterSavedState = new TextCodec::ConverterState;
   }
   copy_converter_state_helper(m_readConverterSavedState, &m_readConverterState);
#endif
   
   m_readBufferStartDevicePos = newPos;
   m_readConverterSavedStateOffset = 0;
}

inline void TextStreamPrivate::restoreToSavedConverterState()
{
#ifndef PDK_NO_TEXTCODEC
   if (m_readConverterSavedState) {
      // we have a saved state
      // that means the converter can be copied
      copy_converter_state_helper(&m_readConverterState, m_readConverterSavedState);
   } else {
      // the only state we could save was the initial
      // so reset to that
      reset_codec_converter_state_helper(&m_readConverterState);
   }
#endif
}

void TextStreamPrivate::write(const Character *data, int len)
{
   if (m_string) {
      // ### What about seek()??
      m_string->append(data, len);
   } else {
      m_writeBuffer.append(data, len);
      if (m_writeBuffer.size() > PDK_TEXTSTREAM_BUFFERSIZE) {
         flushWriteBuffer();
      }
   }
}

inline void TextStreamPrivate::write(Character ch)
{
   if (m_string) {
      // ### What about seek()??
      m_string->append(ch);
   } else {
      m_writeBuffer += ch;
      if (m_writeBuffer.size() > PDK_TEXTSTREAM_BUFFERSIZE) {
         flushWriteBuffer();
      }
   }
}

void TextStreamPrivate::write(Latin1String data)
{
   if (m_string) {
      // ### What about seek()??
      m_string->append(data);
   } else {
      m_writeBuffer += data;
      if (m_writeBuffer.size() > PDK_TEXTSTREAM_BUFFERSIZE) {
         flushWriteBuffer();
      }
   }
}

void TextStreamPrivate::writePadding(int len)
{
   if (m_string) {
      // ### What about seek()??
      m_string->resize(m_string->size() + len, m_params.m_padChar);
   } else {
      m_writeBuffer.resize(m_writeBuffer.size() + len, m_params.m_padChar);
      if (m_writeBuffer.size() > PDK_TEXTSTREAM_BUFFERSIZE) {
         flushWriteBuffer();
      } 
   }
}

inline bool TextStreamPrivate::getChar(Character *ch)
{
   if ((m_string && m_stringOffset == m_string->size())
       || (m_device && m_readBuffer.isEmpty() && !fillReadBuffer())) {
      if (ch) {
         *ch = 0;
      } 
      return false;
   }
   if (ch) {
      *ch = *readPtr();
   }
   consume(1);
   return true;
}

inline void TextStreamPrivate::ungetChar(Character ch)
{
   if (m_string) {
      if (m_stringOffset == 0) {
         m_string->prepend(ch);
      } else {
         (*m_string)[--m_stringOffset] = ch;
      }
      return;
   }
   if (m_readBufferOffset == 0) {
      m_readBuffer.prepend(ch);
      return;
   }
   m_readBuffer[--m_readBufferOffset] = ch;
}

inline void TextStreamPrivate::putChar(Character ch)
{
   if (m_params.m_fieldWidth > 0) {
      putString(&ch, 1);
   } else {
      write(ch);
   } 
}

TextStreamPrivate::PaddingResult TextStreamPrivate::padding(int len) const
{
   PDK_ASSERT(m_params.m_fieldWidth > len); // calling padding() when no padding is needed is an error
   
   int left = 0, right = 0;
   
   const int padSize = m_params.m_fieldWidth - len;
   
   switch (m_params.m_fieldAlignment) {
   case TextStream::FieldAlignment::AlignLeft:
      right = padSize;
      break;
   case TextStream::FieldAlignment::AlignRight:
   case TextStream::FieldAlignment::AlignAccountingStyle:
      left  = padSize;
      break;
   case TextStream::FieldAlignment::AlignCenter:
      left  = padSize/2;
      right = padSize - padSize/2;
      break;
   }
   return { left, right };
}

void TextStreamPrivate::putString(const Character *data, int len, bool number)
{
   if (PDK_UNLIKELY(m_params.m_fieldWidth > len)) {
      // handle padding:
      const PaddingResult pad = padding(len);
      if (m_params.m_fieldAlignment == TextStream::FieldAlignment::AlignAccountingStyle && number) {
         const Character sign = len > 0 ? data[0] : Character();
         if (sign == m_locale.getNegativeSign() || sign == m_locale.getPositiveSign()) {
            // write the sign before the padding, then skip it later
            write(&sign, 1);
            ++data;
            --len;
         }
      }
      
      writePadding(pad.left);
      write(data, len);
      writePadding(pad.right);
   } else {
      write(data, len);
   }
}

void TextStreamPrivate::putString(Latin1String data, bool number)
{
   if (PDK_UNLIKELY(m_params.m_fieldWidth > data.size())) {
      // handle padding
      const PaddingResult pad = padding(data.size());
      if (m_params.m_fieldAlignment == TextStream::FieldAlignment::AlignAccountingStyle && number) {
         const Character sign = data.size() > 0 ? Latin1Character(*data.getRawData()) : Character();
         if (sign == m_locale.getNegativeSign() || sign == m_locale.getPositiveSign()) {
            // write the sign before the padding, then skip it later
            write(&sign, 1);
            data = Latin1String(data.getRawData() + 1, data.size() - 1);
         }
      }
      writePadding(pad.left);
      write(data);
      writePadding(pad.right);
   } else {
      write(data);
   }
}

TextStreamPrivate::NumberParsingStatus TextStreamPrivate::getNumber(pulonglong *ret)
{
   scan(0, 0, 0, TokenDelimiter::NotSpace);
   consumeLastToken();
   
   // detect int encoding
   int base = m_params.m_integerBase;
   if (base == 0) {
      Character ch;
      if (!getChar(&ch)) {
         return NumberParsingStatus::npsInvalidPrefix;
      }
      if (ch == Latin1Character('0')) {
         Character ch2;
         if (!getChar(&ch2)) {
            // Result is the number 0
            *ret = 0;
            return NumberParsingStatus::npsOk;
         }
         ch2 = ch2.toLower();
         if (ch2 == Latin1Character('x')) {
            base = 16;
         } else if (ch2 == Latin1Character('b')) {
            base = 2;
         } else if (ch2.isDigit() && ch2.getDigitValue() >= 0 && ch2.getDigitValue() <= 7) {
            base = 8;
         } else {
            base = 10;
         }
         ungetChar(ch2);
      } else if (ch == m_locale.getNegativeSign() || ch == m_locale.getPositiveSign() || ch.isDigit()) {
         base = 10;
      } else {
         ungetChar(ch);
         return NumberParsingStatus::npsInvalidPrefix;
      }
      ungetChar(ch);
      // State of the stream is now the same as on entry
      // (cursor is at prefix),
      // and local variable 'base' has been set appropriately.
   }
   
   pulonglong val=0;
   switch (base) {
   case 2: {
      Character pf1, pf2, dig;
      // Parse prefix '0b'
      if (!getChar(&pf1) || pf1 != Latin1Character('0'))
         return NumberParsingStatus::npsInvalidPrefix;
      if (!getChar(&pf2) || pf2.toLower() != Latin1Character('b'))
         return NumberParsingStatus::npsInvalidPrefix;
      // Parse digits
      int ndigits = 0;
      while (getChar(&dig)) {
         int n = dig.toLower().unicode();
         if (n == '0' || n == '1') {
            val <<= 1;
            val += n - '0';
         } else {
            ungetChar(dig);
            break;
         }
         ndigits++;
      }
      if (ndigits == 0) {
         // Unwind the prefix and abort
         ungetChar(pf2);
         ungetChar(pf1);
         return NumberParsingStatus::npsMissingDigit;
      }
      break;
   }
   case 8: {
      Character pf, dig;
      // Parse prefix '0'
      if (!getChar(&pf) || pf != Latin1Character('0'))
         return NumberParsingStatus::npsInvalidPrefix;
      // Parse digits
      int ndigits = 0;
      while (getChar(&dig)) {
         int n = dig.toLower().unicode();
         if (n >= '0' && n <= '7') {
            val *= 8;
            val += n - '0';
         } else {
            ungetChar(dig);
            break;
         }
         ndigits++;
      }
      if (ndigits == 0) {
         // Unwind the prefix and abort
         ungetChar(pf);
         return NumberParsingStatus::npsMissingDigit;
      }
      break;
   }
   case 10: {
      // Parse sign (or first digit)
      Character sign;
      int ndigits = 0;
      if (!getChar(&sign))
         return NumberParsingStatus::npsMissingDigit;
      if (sign != m_locale.getNegativeSign() && sign != m_locale.getPositiveSign()) {
         if (!sign.isDigit()) {
            ungetChar(sign);
            return NumberParsingStatus::npsMissingDigit;
         }
         val += sign.getDigitValue();
         ndigits++;
      }
      // Parse digits
      Character ch;
      while (getChar(&ch)) {
         if (ch.isDigit()) {
            val *= 10;
            val += ch.getDigitValue();
         } else if (m_locale != Locale::c() && ch == m_locale.getGroupSeparator()) {
            continue;
         } else {
            ungetChar(ch);
            break;
         }
         ndigits++;
      }
      if (ndigits == 0)
         return NumberParsingStatus::npsMissingDigit;
      if (sign == m_locale.getNegativeSign()) {
         pdk::plonglong ival = pdk::plonglong(val);
         if (ival > 0)
            ival = -ival;
         val = pulonglong(ival);
      }
      break;
   }
   case 16: {
      Character pf1, pf2, dig;
      // Parse prefix ' 0x'
      if (!getChar(&pf1) || pf1 != Latin1Character('0'))
         return NumberParsingStatus::npsInvalidPrefix;
      if (!getChar(&pf2) || pf2.toLower() != Latin1Character('x'))
         return NumberParsingStatus::npsInvalidPrefix;
      // Parse digits
      int ndigits = 0;
      while (getChar(&dig)) {
         int n = dig.toLower().unicode();
         if (n >= '0' && n <= '9') {
            val <<= 4;
            val += n - '0';
         } else if (n >= 'a' && n <= 'f') {
            val <<= 4;
            val += 10 + (n - 'a');
         } else {
            ungetChar(dig);
            break;
         }
         ndigits++;
      }
      if (ndigits == 0) {
         return NumberParsingStatus::npsMissingDigit;
      }
      break;
   }
   default:
      // Unsupported integerBase
      return NumberParsingStatus::npsInvalidPrefix;
   }
   if (ret) {
      *ret = val;
   } 
   return NumberParsingStatus::npsOk;
}

bool TextStreamPrivate::getReal(double *f)
{
   // We use a table-driven FSM to parse floating point numbers
   // strtod() cannot be used directly since we may be reading from a
   // IoDevice.
   enum ParserState {
      Init = 0,
      Sign = 1,
      Mantissa = 2,
      Dot = 3,
      Abscissa = 4,
      ExpMark = 5,
      ExpSign = 6,
      Exponent = 7,
      Nan1 = 8,
      Nan2 = 9,
      Inf1 = 10,
      Inf2 = 11,
      NanInf = 12,
      Done = 13
   };
   enum InputToken {
      None = 0,
      InputSign = 1,
      InputDigit = 2,
      InputDot = 3,
      InputExp = 4,
      InputI = 5,
      InputN = 6,
      InputF = 7,
      InputA = 8,
      InputT = 9
   };
   
   static const uchar table[13][10] = {
      // None InputSign InputDigit InputDot InputExp InputI    InputN    InputF    InputA    InputT
      { 0,    Sign,     Mantissa,  Dot,     0,       Inf1,     Nan1,     0,        0,        0      }, // 0  Init
      { 0,    0,        Mantissa,  Dot,     0,       Inf1,     Nan1,     0,        0,        0      }, // 1  Sign
      { Done, Done,     Mantissa,  Dot,     ExpMark, 0,        0,        0,        0,        0      }, // 2  Mantissa
      { 0,    0,        Abscissa,  0,       0,       0,        0,        0,        0,        0      }, // 3  Dot
      { Done, Done,     Abscissa,  Done,    ExpMark, 0,        0,        0,        0,        0      }, // 4  Abscissa
      { 0,    ExpSign,  Exponent,  0,       0,       0,        0,        0,        0,        0      }, // 5  ExpMark
      { 0,    0,        Exponent,  0,       0,       0,        0,        0,        0,        0      }, // 6  ExpSign
      { Done, Done,     Exponent,  Done,    Done,    0,        0,        0,        0,        0      }, // 7  Exponent
      { 0,    0,        0,         0,       0,       0,        0,        0,        Nan2,     0      }, // 8  Nan1
      { 0,    0,        0,         0,       0,       0,        NanInf,   0,        0,        0      }, // 9  Nan2
      { 0,    0,        0,         0,       0,       0,        Inf2,     0,        0,        0      }, // 10 Inf1
      { 0,    0,        0,         0,       0,       0,        0,        NanInf,   0,        0      }, // 11 Inf2
      { Done, 0,        0,         0,       0,       0,        0,        0,        0,        0      }, // 11 NanInf
   };
   
   ParserState state = Init;
   InputToken input = None;
   
   scan(0, 0, 0, TokenDelimiter::NotSpace);
   consumeLastToken();
   
   const int BufferSize = 128;
   char buf[BufferSize];
   int i = 0;
   
   Character c;
   while (getChar(&c)) {
      switch (c.unicode()) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
         input = InputDigit;
         break;
      case 'i': case 'I':
         input = InputI;
         break;
      case 'n': case 'N':
         input = InputN;
         break;
      case 'f': case 'F':
         input = InputF;
         break;
      case 'a': case 'A':
         input = InputA;
         break;
      case 't': case 'T':
         input = InputT;
         break;
      default: {
         Character lc = c.toLower();
         if (lc == m_locale.getDecimalPoint().toLower()) {
            input = InputDot;
         } else if (lc == m_locale.getExponential().toLower()) {
            input = InputExp;
         } else if (lc == m_locale.getNegativeSign().toLower() 
                    || lc == m_locale.getPositiveSign().toLower()) {
            input = InputSign;
         } else if (m_locale != Locale::c() // backward-compatibility
                    && lc == m_locale.getGroupSeparator().toLower()) {
            input = InputDigit; // well, it isn't a digit, but no one cares.
         } else {
            input = None;
         }
      }
         break;
      }
      
      state = ParserState(table[state][input]);
      
      if  (state == Init || state == Done || i > (BufferSize - 5)) {
         ungetChar(c);
         if (i > (BufferSize - 5)) { // ignore rest of digits
            while (getChar(&c)) {
               if (!c.isDigit()) {
                  ungetChar(c);
                  break;
               }
            }
         }
         break;
      }
      
      buf[i++] = c.toLatin1();
   }
   
   if (i == 0)
      return false;
   if (!f)
      return true;
   buf[i] = '\0';
   
   // backward-compatibility. Old implementation supported +nan/-nan
   // for some reason. QLocale only checks for lower-case
   // nan/+inf/-inf, so here we also check for uppercase and mixed
   // case versions.
   if (!pdk::stricmp(buf, "nan") || !pdk::stricmp(buf, "+nan") || !pdk::stricmp(buf, "-nan")) {
      *f = pdk::pdk_snan();
      return true;
   } else if (!pdk::stricmp(buf, "+inf") || !pdk::stricmp(buf, "inf")) {
      *f = pdk::pdk_inf();
      return true;
   } else if (!pdk::stricmp(buf, "-inf")) {
      *f = -pdk::pdk_inf();
      return true;
   }
   bool ok;
   *f = m_locale.toDouble(String::fromLatin1(buf), &ok);
   return ok;
}

void TextStreamPrivate::putNumber(pdk::pulonglong number, bool negative)
{
   String result;
   
   LocaleData::Flags flags = 0;
   const TextStream::NumberFlags numberFlags = m_params.m_numberFlags;
   if (numberFlags & TextStream::NumberFlag::ShowBase) {
      flags |= LocaleData::Flag::ShowBase;
   }
   if (numberFlags & TextStream::NumberFlag::ForceSign) {
      flags |= LocaleData::Flag::AlwaysShowSign;
   }
   if (numberFlags & TextStream::NumberFlag::UppercaseBase) {
      flags |= LocaleData::Flag::UppercaseBase;
   }
   if (numberFlags & TextStream::NumberFlag::UppercaseDigits) {
      flags |= LocaleData::Flag::CapitalEorX;
   }
   // add thousands group separators. For backward compatibility we
   // don't add a group separator for C locale.
   if (m_locale != Locale::c() && !m_locale.numberOptions().testFlag(Locale::NumberOption::OmitGroupSeparator)) {
      flags |= LocaleData::Flag::ThousandsGroup;
   }
   
   const LocaleData *dd = m_locale.m_implPtr->m_data;
   int base = m_params.m_integerBase ? m_params.m_integerBase : 10;
   if (negative && base == 10) {
      result = dd->longLongToString(-static_cast<pdk::plonglong>(number), -1,
                                    base, -1, flags);
   } else if (negative) {
      // Workaround for backward compatibility for writing negative
      // numbers in octal and hex:
      // TextStream(result) << showbase << hex << -1 << oct << -1
      // should output: -0x1 -0b1
      result = dd->unsLongLongToString(number, -1, base, -1, flags);
      result.prepend(m_locale.getNegativeSign());
   } else {
      result = dd->unsLongLongToString(number, -1, base, -1, flags);
      // workaround for backward compatibility - in octal form with
      // ShowBase flag set zero should be written as '00'
      if (number == 0 && base == 8 && m_params.m_numberFlags & TextStream::NumberFlag::ShowBase
          && result == Latin1String("0")) {
         result.prepend(Latin1Character('0'));
      }
   }
   putString(result, true);
}

} // internal

TextStream::TextStream()
   : m_implPtr(new TextStreamPrivate(this))
{
#if defined (PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStream::TextStream()");
#endif
   PDK_D(TextStream);
   implPtr->m_status = Status::Ok;
}

TextStream::TextStream(IoDevice *device)
   : m_implPtr(new TextStreamPrivate(this))
{
#if defined (PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStream::TextStream(IoDevice *device == *%p)",
                device);
#endif
   PDK_D(TextStream);
   implPtr->m_device = device;
   implPtr->m_deviceClosedNotifier.setupDevice(this, implPtr->m_device);
   implPtr->m_status = Status::Ok;
}

TextStream::TextStream(String *string, IoDevice::OpenMode openMode)
   : m_implPtr(new TextStreamPrivate(this))
{
#if defined (PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStream::TextStream(String *string == *%p, openMode = %d)",
                string, int(openMode));
#endif
   PDK_D(TextStream);
   implPtr->m_string = string;
   implPtr->m_stringOpenMode = openMode;
   implPtr->m_status = Status::Ok;
}

TextStream::TextStream(ByteArray *array, IoDevice::OpenMode openMode)
   : m_implPtr(new TextStreamPrivate(this))
{
#if defined (PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStream::TextStream(ByteArray *array == *%p, openMode = %d)",
                array, int(openMode));
#endif
   PDK_D(TextStream);
   implPtr->m_device = new Buffer(array);
   implPtr->m_device->open(openMode);
   implPtr->m_deleteDevice = true;
   implPtr->m_deviceClosedNotifier.setupDevice(this, implPtr->m_device);
   implPtr->m_status = Status::Ok;
}

TextStream::TextStream(const ByteArray &array, IoDevice::OpenMode openMode)
   : m_implPtr(new TextStreamPrivate(this))
{
#if defined (PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStream::TextStream(const ByteArray &array == *(%p), openMode = %d)",
                &array, int(openMode));
#endif
   Buffer *buffer = new Buffer;
   buffer->setData(array);
   buffer->open(openMode);
   
   PDK_D(TextStream);
   implPtr->m_device = buffer;
   implPtr->m_deleteDevice = true;
   implPtr->m_deviceClosedNotifier.setupDevice(this, implPtr->m_device);
   implPtr->m_status = Status::Ok;
}

TextStream::TextStream(FILE *fileHandle, IoDevice::OpenMode openMode)
   : m_implPtr(new TextStreamPrivate(this))
{
#if defined (PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStream::TextStream(FILE *fileHandle = %p, openMode = %d)",
                fileHandle, int(openMode));
#endif
   File *file = new File;
   file->open(fileHandle, openMode);
   PDK_D(TextStream);
   implPtr->m_device = file;
   implPtr->m_deleteDevice = true;
   implPtr->m_deviceClosedNotifier.setupDevice(this, implPtr->m_device);
   implPtr->m_status = Status::Ok;
}

TextStream::~TextStream()
{
   PDK_D(TextStream);
#if defined (PDK_TEXTSTREAM_DEBUG)
   debug_stream("TextStream::~TextStream()");
#endif
   if (!implPtr->m_writeBuffer.isEmpty()) {
      implPtr->flushWriteBuffer();
   }
}

void TextStream::reset()
{
   PDK_D(TextStream);
   implPtr->m_params.reset();
}

void TextStream::flush()
{
   PDK_D(TextStream);
   implPtr->flushWriteBuffer();
}

bool TextStream::seek(pdk::pint64 pos)
{
   PDK_D(TextStream);
   implPtr->m_lastTokenSize = 0;
   if (implPtr->m_device) {
      // Empty the write buffer
      implPtr->flushWriteBuffer();
      if (!implPtr->m_device->seek(pos)) {
         return false;
      }
      implPtr->resetReadBuffer();
      
#ifndef PDK_NO_TEXTCODEC
      // Reset the codec converter states.
      reset_codec_converter_state_helper(&implPtr->m_readConverterState);
      reset_codec_converter_state_helper(&implPtr->m_writeConverterState);
      delete implPtr->m_readConverterSavedState;
      implPtr->m_readConverterSavedState = 0;
      implPtr->m_writeConverterState.m_flags |= TextCodec::ConversionFlag::IgnoreHeader;
#endif
      return true;
   }
   
   // string
   if (implPtr->m_string && pos <= implPtr->m_string->size()) {
      implPtr->m_stringOffset = int(pos);
      return true;
   }
   return false;
}

pdk::pint64 TextStream::getPosition() const
{
   PDK_D(const TextStream);
   if (implPtr->m_device) {
      // Cutoff
      if (implPtr->m_readBuffer.isEmpty()) {
         return implPtr->m_device->getPosition();
      }
      if (implPtr->m_device->isSequential()) {
         return 0;
      }
      // Seek the device
      if (!implPtr->m_device->seek(implPtr->m_readBufferStartDevicePos)) {
         return pdk::pint64(-1);
      }
      
      // Reset the read buffer
      TextStreamPrivate *thatd = const_cast<TextStreamPrivate *>(implPtr);
      thatd->m_readBuffer.clear();
      
#ifndef PDK_NO_TEXTCODEC
      thatd->restoreToSavedConverterState();
      if (implPtr->m_readBufferStartDevicePos == 0) {
         thatd->m_autoDetectUnicode = true;
      }
#endif
      // Rewind the device to get to the current position Ensure that
      // readBufferOffset is unaffected by fillReadBuffer()
      int oldReadBufferOffset = implPtr->m_readBufferOffset + implPtr->m_readConverterSavedStateOffset;
      while (implPtr->m_readBuffer.size() < oldReadBufferOffset) {
         if (!thatd->fillReadBuffer(1)) {
            return pdk::pint64(-1);
         }
      }
      thatd->m_readBufferOffset = oldReadBufferOffset;
      thatd->m_readConverterSavedStateOffset = 0;
      
      // Return the device position.
      return implPtr->m_device->getPosition();
   }
   if (implPtr->m_string) {
      return implPtr->m_stringOffset;
   }
   warning_stream("TextStream::pos: no device");
   return pdk::pint64(-1);
}

void TextStream::skipWhiteSpace()
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(PDK_VOID);
   implPtr->scan(0, 0, 0, TextStreamPrivate::TokenDelimiter::NotSpace);
   implPtr->consumeLastToken();
}

void TextStream::setDevice(IoDevice *device)
{
   PDK_D(TextStream);
   flush();
   if (implPtr->m_deleteDevice) {
      // @TODO review
      // implPtr->m_deviceClosedNotifier.disconnect();
      delete implPtr->m_device;
      implPtr->m_deleteDevice = false;
   }
   
   implPtr->reset();
   implPtr->m_status = Status::Ok;
   implPtr->m_device = device;
   implPtr->resetReadBuffer();
   // @TODO review
   implPtr->m_deviceClosedNotifier.setupDevice(this, implPtr->m_device);
}

IoDevice *TextStream::getDevice() const
{
   PDK_D(const TextStream);
   return implPtr->m_device;
}

void TextStream::setString(String *string, IoDevice::OpenMode openMode)
{
   PDK_D(TextStream);
   flush();
   if (implPtr->m_deleteDevice) {
      // @TODO review
      //       implPtr->m_deviceClosedNotifier.disconnect();
      //       implPtr->m_device->blockSignals(true);
      delete implPtr->m_device;
      implPtr->m_deleteDevice = false;
   }
   
   implPtr->reset();
   implPtr->m_status = Status::Ok;
   implPtr->m_string = string;
   implPtr->m_stringOpenMode = openMode;
}

String *TextStream::getString() const
{
   PDK_D(const TextStream);
   return implPtr->m_string;
}

void TextStream::setFieldAlignment(FieldAlignment mode)
{
   PDK_D(TextStream);
   implPtr->m_params.m_fieldAlignment = mode;
}

TextStream::FieldAlignment TextStream::getFieldAlignment() const
{
   PDK_D(const TextStream);
   return implPtr->m_params.m_fieldAlignment;
}

void TextStream::setPadChar(Character ch)
{
   PDK_D(TextStream);
   implPtr->m_params.m_padChar = ch;
}

Character TextStream::getPadChar() const
{
   PDK_D(const TextStream);
   return implPtr->m_params.m_padChar;
}

void TextStream::setFieldWidth(int width)
{
   PDK_D(TextStream);
   implPtr->m_params.m_fieldWidth = width;
}

int TextStream::getFieldWidth() const
{
   PDK_D(const TextStream);
   return implPtr->m_params.m_fieldWidth;
}

void TextStream::setNumberFlags(NumberFlags flags)
{
   PDK_D(TextStream);
   implPtr->m_params.m_numberFlags = flags;
}

TextStream::NumberFlags TextStream::getNumberFlags() const
{
   PDK_D(const TextStream);
   return implPtr->m_params.m_numberFlags;
}

void TextStream::setIntegerBase(int base)
{
   PDK_D(TextStream);
   implPtr->m_params.m_integerBase = base;
}

int TextStream::getIntegerBase() const
{
   PDK_D(const TextStream);
   return implPtr->m_params.m_integerBase;
}

void TextStream::setRealNumberNotation(RealNumberNotation notation)
{
   PDK_D(TextStream);
   implPtr->m_params.m_realNumberNotation = notation;
}

TextStream::RealNumberNotation TextStream::getRealNumberNotation() const
{
   PDK_D(const TextStream);
   return implPtr->m_params.m_realNumberNotation;
}

void TextStream::setRealNumberPrecision(int precision)
{
   PDK_D(TextStream);
   if (precision < 0) {
      warning_stream("TextStream::setRealNumberPrecision: Invalid precision (%d)", precision);
      implPtr->m_params.m_realNumberPrecision = 6;
      return;
   }
   implPtr->m_params.m_realNumberPrecision = precision;
}

int TextStream::getRealNumberPrecision() const
{
   PDK_D(const TextStream);
   return implPtr->m_params.m_realNumberPrecision;
}

TextStream::Status TextStream::getStatus() const
{
   PDK_D(const TextStream);
   return implPtr->m_status;
}

void TextStream::resetStatus()
{
   PDK_D(TextStream);
   implPtr->m_status = Status::Ok;
}

void TextStream::setStatus(Status status)
{
   PDK_D(TextStream);
   if (implPtr->m_status == Status::Ok) {
      implPtr->m_status = status;
   }
}

bool TextStream::atEnd() const
{
   PDK_D(const TextStream);
   CHECK_VALID_STREAM(true);
   if (implPtr->m_string) {
      return implPtr->m_string->size() == implPtr->m_stringOffset;
   }
   return implPtr->m_readBuffer.isEmpty() && implPtr->m_device->atEnd();
}

String TextStream::readAll()
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(String());
   return implPtr->read(INT_MAX);
}

String TextStream::readLine(pdk::pint64 maxlen)
{
   String line;
   readLineInto(&line, maxlen);
   return line;
}

bool TextStream::readLineInto(String *line, pdk::pint64 maxlen)
{
   PDK_D(TextStream);
   // keep in sync with CHECK_VALID_STREAM
   if (!implPtr->m_string && !implPtr->m_device) {
      warning_stream("TextStream: No device");
      if (line && !line->isNull()) {
         line->resize(0);
      }
      return false;
   }
   
   const Character *readPtr;
   int length;
   if (!implPtr->scan(&readPtr, &length, int(maxlen), TextStreamPrivate::TokenDelimiter::EndOfLine)) {
      if (line && !line->isNull()) {
         line->resize(0);
      }
      return false;
   }
   
   if (PDK_LIKELY(line)) {
      line->setUnicode(readPtr, length);
   }
   implPtr->consumeLastToken();
   return true;
}

String TextStream::read(pdk::pint64 maxlen)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(String());
   
   if (maxlen <= 0)
      return String::fromLatin1("");     // empty, not null
   
   return implPtr->read(int(maxlen));
}

TextStream &TextStream::operator>>(Character &c)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->scan(0, 0, 0, TextStreamPrivate::TokenDelimiter::NotSpace);
   if (!implPtr->getChar(&c)) {
      setStatus(Status::ReadPastEnd);
   }
   
   return *this;
}

TextStream &TextStream::operator>>(char &c)
{
   Character ch;
   *this >> ch;
   c = ch.toLatin1();
   return *this;
}

TextStream &TextStream::operator>>(signed short &i)
{
   IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(signed short);
}

TextStream &TextStream::operator>>(unsigned short &i)
{
   IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(unsigned short);
}

TextStream &TextStream::operator>>(signed int &i)
{
   IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(signed int);
}

TextStream &TextStream::operator>>(unsigned int &i)
{
   IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(unsigned int);
}

TextStream &TextStream::operator>>(signed long &i)
{
   IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(signed long);
}

TextStream &TextStream::operator>>(unsigned long &i)
{
   IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(unsigned long);
}

TextStream &TextStream::operator>>(pdk::plonglong &i)
{
   IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(pdk::plonglong);
}

TextStream &TextStream::operator>>(pdk::pulonglong &i)
{
   IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(pdk::pulonglong);
}

TextStream &TextStream::operator>>(float &f)
{
   IMPLEMENT_STREAM_RIGHT_REAL_OPERATOR(float);
}

TextStream &TextStream::operator>>(double &f)
{
   IMPLEMENT_STREAM_RIGHT_REAL_OPERATOR(double);
}

TextStream &TextStream::operator>>(String &str)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   
   str.clear();
   implPtr->scan(0, 0, 0, TextStreamPrivate::TokenDelimiter::NotSpace);
   implPtr->consumeLastToken();
   
   const Character *ptr;
   int length;
   if (!implPtr->scan(&ptr, &length, 0, TextStreamPrivate::TokenDelimiter::Space)) {
      setStatus(Status::ReadPastEnd);
      return *this;
   }
   
   str = String(ptr, length);
   implPtr->consumeLastToken();
   return *this;
}

TextStream &TextStream::operator>>(ByteArray &array)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   
   array.clear();
   implPtr->scan(0, 0, 0, TextStreamPrivate::TokenDelimiter::NotSpace);
   implPtr->consumeLastToken();
   
   const Character *ptr;
   int length;
   if (!implPtr->scan(&ptr, &length, 0, TextStreamPrivate::TokenDelimiter::Space)) {
      setStatus(Status::ReadPastEnd);
      return *this;
   }
   
   for (int i = 0; i < length; ++i) {
      array += ptr[i].toLatin1();
   }
   
   implPtr->consumeLastToken();
   return *this;
}

TextStream &TextStream::operator>>(char *c)
{
   PDK_D(TextStream);
   *c = 0;
   CHECK_VALID_STREAM(*this);
   implPtr->scan(0, 0, 0, TextStreamPrivate::TokenDelimiter::NotSpace);
   implPtr->consumeLastToken();
   
   const Character *ptr;
   int length;
   if (!implPtr->scan(&ptr, &length, 0, TextStreamPrivate::TokenDelimiter::Space)) {
      setStatus(Status::ReadPastEnd);
      return *this;
   }
   
   for (int i = 0; i < length; ++i) {
      *c++ = ptr[i].toLatin1();
   }
   *c = '\0';
   implPtr->consumeLastToken();
   return *this;
}

TextStream &TextStream::operator<<(Character c)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->putChar(c);
   return *this;
}

TextStream &TextStream::operator<<(char c)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->putChar(Character::fromLatin1(c));
   return *this;
}

TextStream &TextStream::operator<<(signed short i)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->putNumber((pdk::pulonglong)std::abs(pdk::plonglong(i)), i < 0);
   return *this;
}

TextStream &TextStream::operator<<(unsigned short i)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->putNumber((pdk::pulonglong)i, false);
   return *this;
}

TextStream &TextStream::operator<<(signed int i)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->putNumber((pdk::pulonglong)std::abs(pdk::plonglong(i)), i < 0);
   return *this;
}

TextStream &TextStream::operator<<(unsigned int i)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->putNumber((pdk::pulonglong)i, false);
   return *this;
}

TextStream &TextStream::operator<<(signed long i)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->putNumber((pdk::pulonglong)std::abs(pdk::plonglong(i)), i < 0);
   return *this;
}

TextStream &TextStream::operator<<(unsigned long i)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->putNumber((pdk::pulonglong)i, false);
   return *this;
}

TextStream &TextStream::operator<<(pdk::plonglong i)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->putNumber((pdk::pulonglong)std::abs(i), i < 0);
   return *this;
}

TextStream &TextStream::operator<<(pdk::pulonglong i)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->putNumber(i, false);
   return *this;
}

TextStream &TextStream::operator<<(float f)
{
   return *this << double(f);
}

TextStream &TextStream::operator<<(double f)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   
   LocaleData::DoubleForm form = LocaleData::DoubleForm::DFDecimal;
   switch (getRealNumberNotation()) {
   case RealNumberNotation::FixedNotation:
      form = LocaleData::DoubleForm::DFDecimal;
      break;
   case RealNumberNotation::ScientificNotation:
      form = LocaleData::DoubleForm::DFExponent;
      break;
   case RealNumberNotation::SmartNotation:
      form = LocaleData::DoubleForm::DFSignificantDigits;
      break;
   }
   
   LocaleData::Flags flags = LocaleData::Flag::NoFlags;
   const Locale::NumberOptions numberOptions = getLocale().numberOptions();
   if (getNumberFlags() & NumberFlag::ShowBase) {
      flags |= LocaleData::Flag::ShowBase;
   }
   if (getNumberFlags() & NumberFlag::ForceSign) {
      flags |= LocaleData::Flag::AlwaysShowSign;
   }
   if (getNumberFlags() & NumberFlag::UppercaseBase) {
      flags |= LocaleData::Flag::UppercaseBase;
   }
   if (getNumberFlags() & NumberFlag::UppercaseDigits) {
      flags |= LocaleData::Flag::CapitalEorX;
   }
   if (getNumberFlags() & NumberFlag::ForcePoint) {
      flags |= LocaleData::Flag::ForcePoint;
      // Only for backwards compatibility
      flags |= LocaleData::Flag::AddTrailingZeroes;
      flags |= LocaleData::Flag::ShowBase;
   }
   if (getLocale() != Locale::c() && !(numberOptions & Locale::NumberOption::OmitGroupSeparator)) {
      flags |= LocaleData::Flag::ThousandsGroup;
   }
   if (!(numberOptions & Locale::NumberOption::OmitLeadingZeroInExponent)) {
      flags |= LocaleData::Flag::ZeroPadExponent;
   }
   if (numberOptions & Locale::NumberOption::IncludeTrailingZeroesAfterDot) {
      flags |= LocaleData::Flag::AddTrailingZeroes;
   }
   const LocaleData *dd = implPtr->m_locale.m_implPtr->m_data;
   String num = dd->doubleToString(f, implPtr->m_params.m_realNumberPrecision, form, -1, flags);
   implPtr->putString(num, true);
   return *this;
}

TextStream &TextStream::operator<<(const String &string)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->putString(string);
   return *this;
}

TextStream &TextStream::operator<<(Latin1String string)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->putString(string);
   return *this;
}

TextStream &TextStream::operator<<(const StringRef &string)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->putString(string.getRawData(), string.size());
   return *this;
}

TextStream &TextStream::operator<<(const ByteArray &array)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   implPtr->putString(String::fromUtf8(array.getConstRawData(), array.length()));
   return *this;
}

TextStream &TextStream::operator<<(const char *string)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   // ### Qt6: consider changing to UTF-8
   implPtr->putString(Latin1String(string));
   return *this;
}

TextStream &TextStream::operator<<(const void *ptr)
{
   PDK_D(TextStream);
   CHECK_VALID_STREAM(*this);
   const int oldBase = implPtr->m_params.m_integerBase;
   const NumberFlags oldFlags = implPtr->m_params.m_numberFlags;
   implPtr->m_params.m_integerBase = 16;
   implPtr->m_params.m_numberFlags |= NumberFlag::ShowBase;
   implPtr->putNumber(reinterpret_cast<pdk::uintptr>(ptr), false);
   implPtr->m_params.m_integerBase = oldBase;
   implPtr->m_params.m_numberFlags = oldFlags;
   return *this;
}

TextStream &bin(TextStream &stream)
{
   stream.setIntegerBase(2);
   return stream;
}

TextStream &oct(TextStream &stream)
{
   stream.setIntegerBase(8);
   return stream;
}

TextStream &dec(TextStream &stream)
{
   stream.setIntegerBase(10);
   return stream;
}

TextStream &hex(TextStream &stream)
{
   stream.setIntegerBase(16);
   return stream;
}

TextStream &showbase(TextStream &stream)
{
   stream.setNumberFlags(stream.getNumberFlags() | TextStream::NumberFlag::ShowBase);
   return stream;
}

TextStream &forcesign(TextStream &stream)
{
   stream.setNumberFlags(stream.getNumberFlags() | TextStream::NumberFlag::ForceSign);
   return stream;
}

TextStream &forcepoint(TextStream &stream)
{
   stream.setNumberFlags(stream.getNumberFlags() | TextStream::NumberFlag::ForcePoint);
   return stream;
}

TextStream &noshowbase(TextStream &stream)
{
   stream.setNumberFlags(stream.getNumberFlags() &= ~pdk::as_integer<TextStream::NumberFlag>(TextStream::NumberFlag::ShowBase));
   return stream;
}

TextStream &noforcesign(TextStream &stream)
{
   stream.setNumberFlags(stream.getNumberFlags() &= ~pdk::as_integer<TextStream::NumberFlag>(TextStream::NumberFlag::ForceSign));
   return stream;
}

TextStream &noforcepoint(TextStream &stream)
{
   stream.setNumberFlags(stream.getNumberFlags() &= ~pdk::as_integer<TextStream::NumberFlag>(TextStream::NumberFlag::ForcePoint));
   return stream;
}

TextStream &uppercasebase(TextStream &stream)
{
   stream.setNumberFlags(stream.getNumberFlags() | TextStream::NumberFlag::UppercaseBase);
   return stream;
}

TextStream &uppercasedigits(TextStream &stream)
{
   stream.setNumberFlags(stream.getNumberFlags() | TextStream::NumberFlag::UppercaseDigits);
   return stream;
}

TextStream &lowercasebase(TextStream &stream)
{
   stream.setNumberFlags(stream.getNumberFlags() & ~pdk::as_integer<TextStream::NumberFlag>(TextStream::NumberFlag::UppercaseBase));
   return stream;
}

TextStream &lowercasedigits(TextStream &stream)
{
   stream.setNumberFlags(stream.getNumberFlags() & ~pdk::as_integer<TextStream::NumberFlag>(TextStream::NumberFlag::UppercaseDigits));
   return stream;
}

TextStream &fixed(TextStream &stream)
{
   stream.setRealNumberNotation(TextStream::RealNumberNotation::FixedNotation);
   return stream;
}

TextStream &scientific(TextStream &stream)
{
   stream.setRealNumberNotation(TextStream::RealNumberNotation::ScientificNotation);
   return stream;
}

TextStream &left(TextStream &stream)
{
   stream.setFieldAlignment(TextStream::FieldAlignment::AlignLeft);
   return stream;
}

TextStream &right(TextStream &stream)
{
   stream.setFieldAlignment(TextStream::FieldAlignment::AlignRight);
   return stream;
}

TextStream &center(TextStream &stream)
{
   stream.setFieldAlignment(TextStream::FieldAlignment::AlignCenter);
   return stream;
}

TextStream &endl(TextStream &stream)
{
   return stream << Latin1Character('\n') << flush;
}

TextStream &flush(TextStream &stream)
{
   stream.flush();
   return stream;
}

TextStream &reset(TextStream &stream)
{
   stream.reset();
   return stream;
}

TextStream &ws(TextStream &stream)
{
   stream.skipWhiteSpace();
   return stream;
}

#ifndef PDK_NO_TEXTCODEC

TextStream &bom(TextStream &stream)
{
   stream.setGenerateByteOrderMark(true);
   return stream;
}

void TextStream::setCodec(TextCodec *codec)
{
   PDK_D(TextStream);
   pdk::pint64 seekPos = -1;
   if (!implPtr->m_readBuffer.isEmpty()) {
      if (!implPtr->m_device->isSequential()) {
         seekPos = getPosition();
      }
   }
   implPtr->m_codec = codec;
   if (seekPos >=0 && !implPtr->m_readBuffer.isEmpty()) {
      seek(seekPos);
   }
}

void TextStream::setCodec(const char *codecName)
{
   TextCodec *codec = TextCodec::codecForName(codecName);
   if (codec) {
      setCodec(codec);
   }
   
}

TextCodec *TextStream::getCodec() const
{
   PDK_D(const TextStream);
   return implPtr->m_codec;
}

void TextStream::setAutoDetectUnicode(bool enabled)
{
   PDK_D(TextStream);
   implPtr->m_autoDetectUnicode = enabled;
}

bool TextStream::getAutoDetectUnicode() const
{
   PDK_D(const TextStream);
   return implPtr->m_autoDetectUnicode;
}

void TextStream::setGenerateByteOrderMark(bool generate)
{
   PDK_D(TextStream);
   if (implPtr->m_writeBuffer.isEmpty()) {
      implPtr->m_writeConverterState.m_flags.setFlag(TextCodec::ConversionFlag::IgnoreHeader, !generate);
   }
}

bool TextStream::getGenerateByteOrderMark() const
{
   PDK_D(const TextStream);
   return (implPtr->m_writeConverterState.m_flags & TextCodec::ConversionFlag::IgnoreHeader) == 0;
}

#endif

void TextStream::setLocale(const Locale &locale)
{
   PDK_D(TextStream);
   implPtr->m_locale = locale;
}

Locale TextStream::getLocale() const
{
   PDK_D(const TextStream);
   return implPtr->m_locale;
}

} // io
} // pdk
