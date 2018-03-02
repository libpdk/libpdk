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
// Created by softboy on 2018/03/02.

#ifndef PDK_UTILS_TRANSLATOR_H
#define PDK_UTILS_TRANSLATOR_H

#include "pdk/kernel/Object.h"
#include "pdk/base/ds/ByteArray.h"

namespace pdk {
namespace utils {

using pdk::kernel::Object;
using pdk::lang::String;

#ifndef PDK_NO_TRANSLATION
class Locale;

// foraward declare class with namespace
namespace internal {
class TranslatorPrivate;
} // internal

using internal::TranslatorPrivate;

class PDK_CORE_EXPORT Translator : public Object
{
public:
   explicit Translator(Object *parent = nullptr);
   ~Translator();
   
   virtual String translate(const char *context, const char *sourceText,
                            const char *disambiguation = nullptr, int n = -1) const;
   
   virtual bool isEmpty() const;
   
   bool load(const String & filename,
             const String & directory = String(),
             const String & search_delimiters = String(),
             const String & suffix = String());
   bool load(const Locale & locale,
             const String & filename,
             const String & prefix = String(),
             const String & directory = String(),
             const String & suffix = String());
   bool load(const uchar *data, int len, const String &directory = String());
   
private:
   PDK_DISABLE_COPY(Translator);
   PDK_DECLARE_PRIVATE(Translator);
};

#endif // PDK_NO_TRANSLATION

} // utils
} // pdk

#endif // PDK_UTILS_TRANSLATOR_H
