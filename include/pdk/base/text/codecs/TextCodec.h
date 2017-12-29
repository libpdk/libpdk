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
namespace text {
namespace codecs {

using pdk::ds::ByteArray;
using pdk::lang::Character;
using pdk::lang::String;

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
   static TextCodec *codecForName(const char *name);
   static TextCodec *codecForMib(int mib);
   static std::list<ByteArray> getAvailableCodecs();
   static std::list<int> getAvailableMibs();
   
   static TextCodec *getCodecForLocale();
   static void setCodecForLocale(TextCodec *codec);
   
   static TextCodec *codecForUtfText(const ByteArray &ba);
   static TextCodec *codecForUtfText(const ByteArray &ba, TextCodec *defaultCodec);
   
   bool canEncode(Character c) const;
   bool canEncode(const String &str) const;
   
   String toUnicode(const ByteArray &str) const;
   String toUnicode(const char *str) const;
   ByteArray fromUnicode(const String &unicodeStr) const;
   
private:
   PDK_DISABLE_COPY(TextCodec);
};

} // codecs
} // text
} // pdk

#endif // PDK_M_BASE_TEXT_CODECS_TEXT_CODEC_H
