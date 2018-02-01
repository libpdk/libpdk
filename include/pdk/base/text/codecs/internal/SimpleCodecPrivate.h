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

#ifndef PDK_M_BASE_TEXT_CODECS_INTERNAL_SIMPLE_CODEC_PRIVATE_H
#define PDK_M_BASE_TEXT_CODECS_INTERNAL_SIMPLE_CODEC_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/text/codecs/TextCodec.h"

namespace pdk {
namespace text {
namespace codecs {
namespace internal {

template <typename T>
class AtomicPointer;

class SimpleTextCodec: public TextCodec
{
public:
   static constexpr const int NUM_SIMPLE_CODEC = 30;
   explicit SimpleTextCodec(int);
   ~SimpleTextCodec();
   
   String convertToUnicode(const char *, int, ConverterState *) const override;
   ByteArray convertFromUnicode(const Character *, int, ConverterState *) const override;
   
   ByteArray name() const override;
   std::list<ByteArray> aliases() const override;
   int mibEnum() const override;
   
private:
   int m_forwardIndex;
   mutable AtomicPointer<ByteArray> m_reverseMap;
};


} // internal
} // codecs
} // text
} // pdk

#endif // PDK_M_BASE_TEXT_CODECS_INTERNAL_SIMPLE_CODEC_PRIVATE_H
