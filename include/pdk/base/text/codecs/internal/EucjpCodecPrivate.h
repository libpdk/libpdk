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

#ifndef PDK_M_BASE_TEXT_CODECS_INTERNAL_EUCJP_CODEC_PRIVATE_H
#define PDK_M_BASE_TEXT_CODECS_INTERNAL_EUCJP_CODEC_PRIVATE_H

#include "pdk/base/text/codecs/internal/JpUnicodePrivate.h"
#include "pdk/base/text/codecs/TextCodec.h"
#include <list>

namespace pdk {
namespace text {
namespace codecs {
namespace internal {

class EucJpCodec : public TextCodec
{
public:
   static ByteArray nameImpl();
   static std::list<ByteArray> aliasesImpl() 
   {
      return std::list<ByteArray>();
   }
   static int mibEnumImpl();
   
   ByteArray name() const
   {
      return nameImpl();
   }
   
   std::list<ByteArray> aliases() const
   {
      return aliasesImpl();
   }
   
   int mibEnum() const 
   {
      return mibEnumImpl();
   }
   
   String convertToUnicode(const char *, int, ConverterState *) const;
   ByteArray convertFromUnicode(const Character *, int, ConverterState *) const;
   
   EucJpCodec();
   ~EucJpCodec();
   
protected:
   const JpUnicodeConv *conv;
};

} // internal
} // codecs
} // text
} // pdk

#endif // PDK_M_BASE_TEXT_CODECS_INTERNAL_EUCJP_CODEC_PRIVATE_H
