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

#ifndef PDK_M_BASE_TEXT_CODECS_INTERNAL_JPUNICODE_PRIVATE_H
#define PDK_M_BASE_TEXT_CODECS_INTERNAL_JPUNICODE_PRIVATE_H

#include "pdk/global/Global.h"

namespace pdk {
namespace text {
namespace codecs {
namespace internal {

class JpUnicodeConv 
{
public:
   virtual ~JpUnicodeConv()
   {}
   
   enum Rules {
      // "ASCII" is ANSI X.3.4-1986, a.k.a. US-ASCII here.
      Default                        = 0x0000,
      
      Unicode                        = 0x0001,
      Unicode_JISX0201                = 0x0001,
      Unicode_ASCII                 = 0x0002,
      JISX0221_JISX0201         = 0x0003,
      JISX0221_ASCII                = 0x0004,
      Sun_JDK117                     = 0x0005,
      Microsoft_CP932                = 0x0006,
      
      NEC_VDC                = 0x0100,                // NEC Vender Defined Char
      UDC                        = 0x0200,                // User Defined Char
      IBM_VDC                = 0x0400                // IBM Vender Defined Char
   };
   static JpUnicodeConv *newConverter(int rule);
   
   virtual uint asciiToUnicode(uint h, uint l) const;
   /*virtual*/ uint jisx0201ToUnicode(uint h, uint l) const;
   virtual uint jisx0201LatinToUnicode(uint h, uint l) const;
   /*virtual*/ uint jisx0201KanaToUnicode(uint h, uint l) const;
   virtual uint jisx0208ToUnicode(uint h, uint l) const;
   virtual uint jisx0212ToUnicode(uint h, uint l) const;
   
   uint asciiToUnicode(uint ascii) const
   {
      return asciiToUnicode((ascii & 0xff00) >> 8, (ascii & 0x00ff));
   }
   
   uint jisx0201ToUnicode(uint jis) const
   {
      return jisx0201ToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
   }
   
   uint jisx0201LatinToUnicode(uint jis) const
   {
      return jisx0201LatinToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
   }
   
   uint jisx0201KanaToUnicode(uint jis) const
   {
      return jisx0201KanaToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
   }
   
   uint jisx0208ToUnicode(uint jis) const
   {
      return jisx0208ToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
   }
   
   uint jisx0212ToUnicode(uint jis) const
   {
      return jisx0212ToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
   }
   
   virtual uint unicodeToAscii(uint h, uint l) const;
   /*virtual*/ uint unicodeToJisx0201(uint h, uint l) const;
   virtual uint unicodeToJisx0201Latin(uint h, uint l) const;
   /*virtual*/ uint unicodeToJisx0201Kana(uint h, uint l) const;
   virtual uint unicodeToJisx0208(uint h, uint l) const;
   virtual uint unicodeToJisx0212(uint h, uint l) const;
   
   uint unicodeToAscii(uint unicode) const
   {
      return unicodeToAscii((unicode & 0xff00) >> 8, (unicode & 0x00ff));
   }
   
   uint unicodeToJisx0201(uint unicode) const
   {
      return unicodeToJisx0201((unicode & 0xff00) >> 8, (unicode & 0x00ff));
   }
   
   uint unicodeToJisx0201Latin(uint unicode) const
   {
      return unicodeToJisx0201Latin((unicode & 0xff00) >> 8, (unicode & 0x00ff));
   }
   
   uint unicodeToJisx0201Kana(uint unicode) const
   {
      return unicodeToJisx0201Kana((unicode & 0xff00) >> 8, (unicode & 0x00ff));
   }
   
   uint unicodeToJisx0208(uint unicode) const
   {
      return unicodeToJisx0208((unicode & 0xff00) >> 8, (unicode & 0x00ff));
   }
   
   uint unicodeToJisx0212(uint unicode) const
   {
      return unicodeToJisx0212((unicode & 0xff00) >> 8, (unicode & 0x00ff));
   }
   
   uint sjisToUnicode(uint h, uint l) const;
   uint unicodeToSjis(uint h, uint l) const;
   uint sjisibmvdcToUnicode(uint h, uint l) const;
   uint unicodeToSjisibmvdc(uint h, uint l) const;
   uint cp932ToUnicode(uint h, uint l) const;
   uint unicodeToCp932(uint h, uint l) const;
   
   uint sjisToUnicode(uint sjis) const
   {
      return sjisToUnicode((sjis & 0xff00) >> 8, (sjis & 0x00ff));
   }
   
   uint unicodeToSjis(uint unicode) const
   {
      return unicodeToSjis((unicode & 0xff00) >> 8, (unicode & 0x00ff));
   }
   
protected:
   explicit JpUnicodeConv(int r)
      : m_rule(r)
   {}
   
private:
   int m_rule;
};

} // internal
} // codecs
} // text
} // pdk

#endif // PDK_M_BASE_TEXT_CODECS_INTERNAL_JPUNICODE_PRIVATE_H
