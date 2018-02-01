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
// Created by softboy on 2018/02/01.

#include "pdk/base/text/codecs/internal/LatinCodecPrivate.h"
#include <list>

namespace pdk {
namespace text {
namespace codecs {
namespace internal {

Latin1Codec::~Latin1Codec()
{
}

String Latin1Codec::convertToUnicode(const char *chars, int len, ConverterState *) const
{
   if (chars == 0)
      return String();
   
   return String::fromLatin1(chars, len);
}

ByteArray Latin1Codec::convertFromUnicode(const Character *ch, int len, ConverterState *state) const
{
   const char replacement = (state && state->m_flags & ConversionFlag::ConvertInvalidToNull) ? 0 : '?';
   ByteArray r(len, pdk::Uninitialized);
   char *d = r.getRawData();
   int invalid = 0;
   for (int i = 0; i < len; ++i) {
      if (ch[i] > 0xff) {
         d[i] = replacement;
         ++invalid;
      } else {
         d[i] = (char)ch[i].getCell();
      }
   }
   if (state) {
      state->m_invalidChars += invalid;
   }
   return r;
}

ByteArray Latin1Codec::name() const
{
   return "ISO-8859-1";
}

std::list<ByteArray> Latin1Codec::aliases() const
{
   std::list<ByteArray> list;
   list.push_back("latin1");
   list.push_back("CP819");
   list.push_back("IBM819");
   list.push_back("iso-ir-100");
   list.push_back("csISOLatin1");
   return list;
}

int Latin1Codec::mibEnum() const
{
   return 4;
}

Latin15Codec::~Latin15Codec()
{
}

String Latin15Codec::convertToUnicode(const char* chars, int len, ConverterState *) const
{
   if (chars == 0)
      return String();
   
   String str = String::fromLatin1(chars, len);
   Character *uc = str.getRawData();
   while(len--) {
      switch(uc->unicode()) {
      case 0xa4:
         *uc = 0x20ac;
         break;
      case 0xa6:
         *uc = 0x0160;
         break;
      case 0xa8:
         *uc = 0x0161;
         break;
      case 0xb4:
         *uc = 0x017d;
         break;
      case 0xb8:
         *uc = 0x017e;
         break;
      case 0xbc:
         *uc = 0x0152;
         break;
      case 0xbd:
         *uc = 0x0153;
         break;
      case 0xbe:
         *uc = 0x0178;
         break;
      default:
         break;
      }
      uc++;
   }
   return str;
}

ByteArray Latin15Codec::convertFromUnicode(const Character *in, int length, ConverterState *state) const
{
   const char replacement = (state && state->m_flags & ConversionFlag::ConvertInvalidToNull) ? 0 : '?';
   ByteArray r(length, pdk::Uninitialized);
   char *d = r.getRawData();
   int invalid = 0;
   for (int i = 0; i < length; ++i) {
      uchar c;
      ushort uc = in[i].unicode();
      if (uc < 0x0100) {
         if (uc > 0xa3) {
            switch(uc) {
            case 0xa4:
            case 0xa6:
            case 0xa8:
            case 0xb4:
            case 0xb8:
            case 0xbc:
            case 0xbd:
            case 0xbe:
               c = replacement;
               ++invalid;
               break;
            default:
               c = (unsigned char) uc;
               break;
            }
         } else {
            c = (unsigned char) uc;
         }
      } else {
         if (uc == 0x20ac)
            c = 0xa4;
         else if ((uc & 0xff00) == 0x0100) {
            switch(uc) {
            case 0x0160:
               c = 0xa6;
               break;
            case 0x0161:
               c = 0xa8;
               break;
            case 0x017d:
               c = 0xb4;
               break;
            case 0x017e:
               c = 0xb8;
               break;
            case 0x0152:
               c = 0xbc;
               break;
            case 0x0153:
               c = 0xbd;
               break;
            case 0x0178:
               c = 0xbe;
               break;
            default:
               c = replacement;
               ++invalid;
            }
         } else {
            c = replacement;
            ++invalid;
         }
      }
      d[i] = (char)c;
   }
   if (state) {
      state->m_remainingChars = 0;
      state->m_invalidChars += invalid;
   }
   return r;
}


ByteArray Latin15Codec::name() const
{
   return "ISO-8859-15";
}

std::list<ByteArray> Latin15Codec::aliases() const
{
   std::list<ByteArray> list;
   list.push_back("latin9");
   return list;
}

int Latin15Codec::mibEnum() const
{
   return 111;
}

} // internal
} // codecs
} // text
} // pdk
