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
// Created by softboy on 2017/12/25.

#ifndef PDK_M_BASE_TEXT_CODECS_TEXT_CODEC_H
#define PDK_M_BASE_TEXT_CODECS_TEXT_CODEC_H

#include "pdk/base/lang/String.h"
#include <list>

namespace pdk {

// forward declare class with namespace
namespace io {
class IoDevice;
} // io

namespace kernel {
namespace internal {

struct CoreGlobalData;

} // internal
} // kernel

namespace text {
namespace codecs {

using pdk::ds::ByteArray;
using pdk::lang::Character;
using pdk::lang::String;
using pdk::lang::StringView;
using pdk::io::IoDevice;
using pdk::kernel::internal::CoreGlobalData;

class TextDecoder;
class TextEncoder;

class PDK_CORE_EXPORT TextCodec
{
public:
   enum class ConversionFlag : unsigned int
   {
      DefaultConversion,
      ConvertInvalidToNull = 0x80000000,
      IgnoreHeader = 0x1,
      FreeFunction = 0x2
   };
   PDK_DECLARE_FLAGS(ConversionFlags, ConversionFlag);
   
   struct PDK_CORE_EXPORT ConverterState
   {
      ConverterState(ConversionFlags flag = ConversionFlag::DefaultConversion)
         : m_flags(flag),
           m_remainingChars(0),
           m_invalidChars(0),
           m_data(nullptr)
      {
         m_stateData[0] = m_stateData[1] = m_stateData[2] = 0;
      }
      
      ~ConverterState();
      
   public:
      ConversionFlags m_flags;
      int m_remainingChars;
      int m_invalidChars;
      uint m_stateData[3];
      void *m_data;
      
   private:
      PDK_DISABLE_COPY(ConverterState);
   };
   
public:
   static TextCodec *codecForName(const ByteArray &name);
   
   inline static TextCodec *codecForName(const char *name)
   {
      return codecForName(ByteArray(name));
   }
   
   static TextCodec *codecForMib(int mib);
   static std::list<ByteArray> getAvailableCodecs();
   static std::list<int> getAvailableMibs();
   
   static TextCodec *getCodecForLocale();
   static void setCodecForLocale(TextCodec *codec);
   
   static TextCodec *codecForHtml(const ByteArray &ba);
   static TextCodec *codecForHtml(const ByteArray &ba, TextCodec *defaultCodec);
   
   static TextCodec *codecForUtfText(const ByteArray &ba);
   static TextCodec *codecForUtfText(const ByteArray &ba, TextCodec *defaultCodec);
   
   bool canEncode(Character c) const;
#if PDK_STRINGVIEW_LEVEL < 2
   bool canEncode(const String &str) const;
#endif
   bool canEncode(const StringView &str) const;
   
   String toUnicode(const ByteArray &str) const;
   String toUnicode(const char *str) const;
#if PDK_STRINGVIEW_LEVEL < 2
   ByteArray fromUnicode(const String &unicodeStr) const;
#endif
   ByteArray fromUnicode(const StringView &unicodeStr) const;
   
   String toUnicode(const char *in, int length, ConverterState *state = nullptr) const
   {
      return convertToUnicode(in, length, state);
   }
   
   ByteArray fromUnicode(const Character *in, int length, ConverterState *state = nullptr) const
   {
      return convertFromUnicode(in, length, state);
   }
   
   TextDecoder *makeDecoder(ConversionFlags flags = ConversionFlag::DefaultConversion) const;
   TextEncoder *makeEncoder(ConversionFlags flags = ConversionFlag::DefaultConversion) const;
   
   virtual ByteArray name() const = 0;
   virtual std::list<ByteArray> aliases() const;
   virtual int mibEnum() const = 0;
   
protected:
   virtual String convertToUnicode(const char *in, int length, ConverterState *state) const = 0;
   virtual ByteArray convertFromUnicode(const Character *in, int length, ConverterState *state) const = 0;
   
   TextCodec();
   virtual ~TextCodec();
private:
   PDK_DISABLE_COPY(TextCodec);
   friend struct CoreGlobalData;
};

PDK_DECLARE_OPERATORS_FOR_FLAGS(TextCodec::ConversionFlags)

class PDK_CORE_EXPORT TextEncoder {
public:
   explicit TextEncoder(const TextCodec *codec)
      : m_codec(codec),
        m_state()
   {}
   
   explicit TextEncoder(const TextCodec *codec, TextCodec::ConversionFlags flags);
   ~TextEncoder();
#if PDK_STRINGVIEW_LEVEL < 2
   ByteArray fromUnicode(const String& str);
#endif
   ByteArray fromUnicode(StringView str);
   ByteArray fromUnicode(const Character *uc, int len);
   bool hasFailure() const;
private:
   PDK_DISABLE_COPY(TextEncoder);
   const TextCodec *m_codec;
   TextCodec::ConverterState m_state;
};


class PDK_CORE_EXPORT TextDecoder
{
public:
   explicit TextDecoder(const TextCodec *codec)
      : m_codec(codec), m_state()
   {}
   explicit TextDecoder(const TextCodec *codec, TextCodec::ConversionFlags flags);
   ~TextDecoder();
   String toUnicode(const char* chars, int len);
   String toUnicode(const ByteArray &ba);
   void toUnicode(String *target, const char *chars, int len);
   bool hasFailure() const;
private:
   PDK_DISABLE_COPY(TextDecoder);
   const TextCodec *m_codec;
   TextCodec::ConverterState m_state;
};

} // codecs
} // text
} // pdk



#endif // PDK_M_BASE_TEXT_CODECS_TEXT_CODEC_H
