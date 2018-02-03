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

#ifndef PDK_M_BASE_LANG_INTERNAL_UNICODETOOLS_H
#define PDK_M_BASE_LANG_INTERNAL_UNICODETOOLS_H

#include "pdk/global/Global.h"
#include "pdk/base/lang/Character.h"

namespace pdk {
namespace lang {
namespace internal {
namespace unicodetools {

struct CharAttributes
{
   uchar m_graphemeBoundary : 1;
   uchar m_wordBreak        : 1;
   uchar m_sentenceBoundary : 1;
   uchar m_lineBreak        : 1;
   uchar m_whiteSpace       : 1;
   uchar m_wordStart        : 1;
   uchar m_wordEnd          : 1;
   uchar m_mandatoryBreak   : 1;
};


// ### temporary
struct ScriptItem
{
   int m_position;
   int m_script;
};

enum CharAttributeOption
{
   GraphemeBreaks = 0x01,
   WordBreaks = 0x02,
   SentenceBreaks = 0x04,
   LineBreaks = 0x08,
   WhiteSpaces = 0x10,
   DefaultOptionsCompat = GraphemeBreaks | LineBreaks | WhiteSpaces, // ### remove
   
   DontClearAttributes = 0x1000
};
PDK_DECLARE_FLAGS(CharAttributeOptions, CharAttributeOption);

// attributes buffer has to have a length of string length + 1
PDK_CORE_EXPORT void init_char_attributes(const ushort *string, int length,
                                          const ScriptItem *items, int numItems,
                                          CharAttributes *attributes, CharAttributeOptions options = DefaultOptionsCompat);


PDK_CORE_EXPORT void init_scripts(const ushort *string, int length, uchar *scripts);


} // unicodetools
} // internal
} // lang
} // pdk

PDK_DECLARE_TYPEINFO(pdk::lang::internal::unicodetools::CharAttributes, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(pdk::lang::internal::unicodetools::ScriptItem, PDK_PRIMITIVE_TYPE);


#endif // PDK_M_BASE_LANG_INTERNAL_UNICODETOOLS_H
