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

#ifndef PDK_M_BASE_TEXT_CODECS_TEXT_CODEC_PRIVATE_H
#define PDK_M_BASE_TEXT_CODECS_TEXT_CODEC_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/text/codecs/TextCodec.h"
#include <cstring>

#if defined(PDK_OS_MAC)
#define PDK_LOCALE_IS_UTF8
#endif

namespace pdk {
namespace text {
namespace codecs {
namespace internal {

using TextCodecStateFreeFunc = void (*)(TextCodec::ConverterState *);

struct TextCodecUnalignedPointer
{
   static inline TextCodecStateFreeFunc decode(const uint *src)
   {
      pdk::uintptr data;
      std::memcpy(&data, src, sizeof(data));
      return reinterpret_cast<TextCodecStateFreeFunc>(data);
   }
   
   static inline void encode(uint *dest, TextCodecStateFreeFunc func)
   {
      pdk::uintptr data = reinterpret_cast<pdk::uintptr>(func);
      std::memcpy(dest, &data, sizeof(data));
   }
};

bool text_codec_name_match(const char *a, const char *b);

} // internal
} // codecs
} // text
} // pdk

#endif // PDK_M_BASE_TEXT_CODECS_TEXT_CODEC_PRIVATE_H
