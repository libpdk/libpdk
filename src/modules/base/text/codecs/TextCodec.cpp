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

#include "pdk/global/PlatformDefs.h"
#include "pdk/base/text/codecs/TextCodec.h"
#include "pdk/base/text/codecs/internal/TextCodecPrivate.h"
#include "pdk/base/lang/StringMatcher.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/kernel/internal/CoreGlobalDataPrivate.h"

namespace pdk {
namespace text {
namespace codecs {

TextCodec::ConverterState::~ConverterState()
{}

TextCodec::TextCodec()
{}

TextCodec::~TextCodec()
{}

TextCodec *TextCodec::codecForName(const ByteArray &name)
{
}

TextCodec* TextCodec::codecForMib(int mib)
{
   
}

std::list<ByteArray> TextCodec::getAvailableCodecs()
{
   
}

std::list<int> TextCodec::getAvailableMibs()
{
}

void TextCodec::setCodecForLocale(TextCodec *c)
{
    
}

TextCodec* TextCodec::getCodecForLocale()
{
   
}

std::list<ByteArray> TextCodec::aliases() const
{
}

TextDecoder* TextCodec::makeDecoder(TextCodec::ConversionFlags flags) const
{
   
}

TextEncoder* TextCodec::makeEncoder(TextCodec::ConversionFlags flags) const
{
   
}

#if PDK_STRINGVIEW_LEVEL < 2
ByteArray TextCodec::fromUnicode(const String& str) const
{
}
#endif

ByteArray TextCodec::fromUnicode(StringView str) const
{
}

String TextCodec::toUnicode(const ByteArray& a) const
{
}

bool TextCodec::canEncode(Character ch) const
{
}

#if PDK_STRINGVIEW_LEVEL < 2

bool TextCodec::canEncode(const String& s) const
{
}
#endif

bool TextCodec::canEncode(StringView s) const
{
}

String TextCodec::toUnicode(const char *chars) const
{
}

TextEncoder::TextEncoder(const TextCodec *codec, TextCodec::ConversionFlags flags)
    : m_codec(codec),
      m_state()
{
}

TextEncoder::~TextEncoder()
{
}

bool TextEncoder::hasFailure() const
{
}

#if PDK_STRINGVIEW_LEVEL < 2
ByteArray TextEncoder::fromUnicode(const String& str)
{
}
#endif

ByteArray TextEncoder::fromUnicode(StringView str)
{

}

ByteArray TextEncoder::fromUnicode(const Character *uc, int len)
{
}

TextDecoder::TextDecoder(const TextCodec *codec, TextCodec::ConversionFlags flags)
   : m_codec(codec),
     m_state()
{
}

TextDecoder::~TextDecoder()
{
}

String TextDecoder::toUnicode(const char *chars, int len)
{
}

void TextDecoder::toUnicode(String *target, const char *chars, int len)
{
}

String TextDecoder::toUnicode(const ByteArray &ba)
{
}


bool TextDecoder::hasFailure() const
{
   
}

TextCodec *TextCodec::codecForHtml(const ByteArray &ba, TextCodec *defaultCodec)
{
}

TextCodec *TextCodec::codecForHtml(const ByteArray &ba)
{
   
}

TextCodec *TextCodec::codecForUtfText(const ByteArray &ba, TextCodec *defaultCodec)
{
}

TextCodec *TextCodec::codecForUtfText(const ByteArray &ba)
{
   
}

} // codecs
} // text
} // pdk
