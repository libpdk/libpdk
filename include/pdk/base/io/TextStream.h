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

#ifndef PDK_M_BASE_IO_TEXTSTREAM_H
#define PDK_M_BASE_IO_TEXTSTREAM_H

#include "pdk/base/io/IoDevice.h"
#include "pdk/base/lang/Character.h"
#include "pdk/utils/Locale.h"
#include "pdk/utils/ScopedPointer.h"
#include <string>
#include <cstdio>

#ifdef Status
#  error pdk/base/io/Textstream.h must be included before any header file that defines Status
#endif

namespace pdk {

// forward declare class with namespace
namespace ds {
class ByteArray;
} // ds

// forward declare class with namespace
namespace text {
namespace codecs {
class TextCodec;
} // codecs
} // text

// forward declare class with namespace
namespace lang {
class String;
class Latin1String;
class StringRef;
} // lang

namespace io {

namespace internal {
class TextStreamPrivate;
class DebugStateSaverPrivate;
} // internal

using pdk::ds::ByteArray;
using pdk::lang::Character;
using pdk::lang::Latin1String;
using pdk::lang::String;
using pdk::lang::StringRef;
using pdk::utils::Locale;
using pdk::text::codecs::TextCodec;

using internal::TextStreamPrivate;
using internal::DebugStateSaverPrivate;
class PDK_CORE_EXPORT TextStream
{
   PDK_DECLARE_PRIVATE(TextStream);
public:
   enum class RealNumberNotation
   {
      SmartNotation,
      FixedNotation,
      ScientificNotation
   };
   
   enum class FieldAlignment
   {
      AlignLeft,
      AlignRight,
      AlignCenter,
      AlignAccountingStyle
   };
   
   enum class Status
   {
      Ok,
      ReadPastEnd,
      ReadCorruptData,
      WriteFailed
   };
   
   enum class NumberFlag
   {
      ShowBase = 0x1,
      ForcePoint = 0x2,
      ForceSign = 0x4,
      UppercaseBase = 0x8,
      UppercaseDigits = 0x10
   };
   PDK_DECLARE_FLAGS(NumberFlags, NumberFlag);
public:
   TextStream();
   explicit TextStream(IoDevice *device);
   explicit TextStream(FILE *fileHandle, IoDevice::OpenModes openMode = IoDevice::OpenMode::ReadWrite);
   explicit TextStream(String *string, IoDevice::OpenModes openMode = IoDevice::OpenMode::ReadWrite);
   explicit TextStream(ByteArray *array, IoDevice::OpenModes openMode = IoDevice::OpenMode::ReadWrite);
   explicit TextStream(const ByteArray &array, IoDevice::OpenModes openMode = IoDevice::OpenMode::ReadOnly);
   virtual ~TextStream();
   
#ifndef PDK_NO_TEXTCODEC
   void setCodec(TextCodec *codec);
   void setCodec(const char *codecName);
   TextCodec *getCodec() const;
   void setAutoDetectUnicode(bool enabled);
   bool getAutoDetectUnicode() const;
   void setGenerateByteOrderMark(bool generate);
   bool getGenerateByteOrderMark() const;
#endif
   
   void setLocale(const Locale &locale);
   Locale getLocale() const;
   
   void setDevice(IoDevice *device);
   IoDevice *getDevice() const;
   
   void setString(String *string, IoDevice::OpenMode openMode = IoDevice::OpenMode::ReadOnly);
   String *getString() const;
   
   Status getStatus() const;
   void setStatus(Status status);
   void resetStatus();
   
   bool atEnd() const;
   void reset();
   void flush();
   bool seek(pdk::pint64 pos);
   pdk::pint64 getPosition() const;
   
   void skipWhiteSpace();
   String readLine(pdk::pint64 maxLength = 0);
   bool readLineInto(String *line, pdk::pint64 maxLength = 0);
   String readAll();
   String read(pdk::pint64 maxLength);
   
   void setFieldAlignment(FieldAlignment alignment);
   FieldAlignment getFieldAlignment() const;
   
   void setPadChar(Character ch);
   Character getPadChar() const;
   
   void setFieldWidth(int width);
   int getFieldWidth() const;
   
   void setNumberFlags(NumberFlags flags);
   NumberFlags getNumberFlags() const;
   
   void setIntegerBase(int base);
   int getIntegerBase() const;
   
   void setRealNumberNotation(RealNumberNotation notation);
   RealNumberNotation getRealNumberNotation() const;
   
   void setRealNumberPrecision(int precision);
   int getRealNumberPrecision() const;
   
   TextStream &operator>>(Character &ch);
   TextStream &operator>>(char &ch);
   TextStream &operator>>(signed short &i);
   TextStream &operator>>(unsigned short &i);
   TextStream &operator>>(signed int &i);
   TextStream &operator>>(unsigned int &i);
   TextStream &operator>>(signed long &i);
   TextStream &operator>>(unsigned long &i);
   TextStream &operator>>(plonglong &i);
   TextStream &operator>>(pulonglong &i);
   TextStream &operator>>(float &f);
   TextStream &operator>>(double &f);
   TextStream &operator>>(String &s);
   TextStream &operator>>(ByteArray &array);
   TextStream &operator>>(char *c);
   
   TextStream &operator<<(Character ch);
   TextStream &operator<<(char ch);
   TextStream &operator<<(signed short i);
   TextStream &operator<<(unsigned short i);
   TextStream &operator<<(signed int i);
   TextStream &operator<<(unsigned int i);
   TextStream &operator<<(signed long i);
   TextStream &operator<<(unsigned long i);
   TextStream &operator<<(plonglong i);
   TextStream &operator<<(pulonglong i);
   TextStream &operator<<(float f);
   TextStream &operator<<(double f);
   TextStream &operator<<(const String &s);
   TextStream &operator<<(Latin1String s);
   TextStream &operator<<(const StringRef &s);
   TextStream &operator<<(const ByteArray &array);
   TextStream &operator<<(const char *c);
   TextStream &operator<<(const void *ptr);
   
private:
   PDK_DISABLE_COPY(TextStream);
   pdk::utils::ScopedPointer<TextStreamPrivate> m_implPtr;
   friend class DebugStateSaverPrivate;
   friend class Debug;
};

PDK_DECLARE_OPERATORS_FOR_FLAGS(TextStream::NumberFlags)

using TextStreamFunc = TextStream & (*)(TextStream &);// manipulator function
using PDKSMFI = void (TextStream::*)(int); // manipulator w/int argument
using PDKSMFC = void (TextStream::*)(Character); // manipulator w/QChar argument

class PDK_CORE_EXPORT TextStreamManipulator
{
public:
   constexpr TextStreamManipulator(PDKSMFI m, int a) noexcept
      : m_mf(m),
        m_mc(nullptr),
        m_arg(a),
        m_ch()
   {}
   
   constexpr TextStreamManipulator(PDKSMFC m, Character c) noexcept 
      : m_mf(nullptr), 
        m_mc(m),
        m_arg(-1),
        m_ch(c)
   {}
   
   void exec(TextStream &s)
   {
      if (m_mf) {
         (s.*m_mf)(m_arg);
      } else {
         (s.*m_mc)(m_ch);
      }
   }
   
private:
   PDKSMFI m_mf;                                        // TextStream member function
   PDKSMFC m_mc;                                        // TextStream member function
   int m_arg;                                         // member function argument
   Character m_ch;
};

inline TextStream &operator>>(TextStream &s, TextStreamFunc func)
{
   return (*func)(s);
}

inline TextStream &operator<<(TextStream &s, TextStreamFunc func)
{
   return (*func)(s);
}

inline TextStream &operator<<(TextStream &s, TextStreamManipulator m)
{
   m.exec(s);
   return s;
}

PDK_CORE_EXPORT TextStream &bin(TextStream &s);
PDK_CORE_EXPORT TextStream &oct(TextStream &s);
PDK_CORE_EXPORT TextStream &dec(TextStream &s);
PDK_CORE_EXPORT TextStream &hex(TextStream &s);

PDK_CORE_EXPORT TextStream &showbase(TextStream &s);
PDK_CORE_EXPORT TextStream &forcesign(TextStream &s);
PDK_CORE_EXPORT TextStream &forcepoint(TextStream &s);
PDK_CORE_EXPORT TextStream &noshowbase(TextStream &s);
PDK_CORE_EXPORT TextStream &noforcesign(TextStream &s);
PDK_CORE_EXPORT TextStream &noforcepoint(TextStream &s);

PDK_CORE_EXPORT TextStream &uppercasebase(TextStream &s);
PDK_CORE_EXPORT TextStream &uppercasedigits(TextStream &s);
PDK_CORE_EXPORT TextStream &lowercasebase(TextStream &s);
PDK_CORE_EXPORT TextStream &lowercasedigits(TextStream &s);

PDK_CORE_EXPORT TextStream &fixed(TextStream &s);
PDK_CORE_EXPORT TextStream &scientific(TextStream &s);

PDK_CORE_EXPORT TextStream &left(TextStream &s);
PDK_CORE_EXPORT TextStream &right(TextStream &s);
PDK_CORE_EXPORT TextStream &center(TextStream &s);

PDK_CORE_EXPORT TextStream &endl(TextStream &s);
PDK_CORE_EXPORT TextStream &flush(TextStream &s);
PDK_CORE_EXPORT TextStream &reset(TextStream &s);

PDK_CORE_EXPORT TextStream &bom(TextStream &s);

PDK_CORE_EXPORT TextStream &ws(TextStream &s);

inline TextStreamManipulator set_field_width(int width)
{
   PDKSMFI func = &TextStream::setFieldWidth;
   return TextStreamManipulator(func,width);
}

inline TextStreamManipulator set_pad_char(Character ch)
{
   PDKSMFC func = &TextStream::setPadChar;
   return TextStreamManipulator(func, ch);
}

inline TextStreamManipulator set_real_number_precision(int precision)
{
   PDKSMFI func = &TextStream::setRealNumberPrecision;
   return TextStreamManipulator(func, precision);
}

} // io
} // pdk

#endif // PDK_M_BASE_IO_TEXTSTREAM_H
