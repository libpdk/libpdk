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
// Created by softboy on 2018/02/27.

#ifndef PDK_UTILS_INTERNAL_COLLATOR_PRIVATE_H
#define PDK_UTILS_INTERNAL_COLLATOR_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/utils/Collator.h"

#if PDK_CONFIG(icu)
#include <unicode/ucol.h>
#elif defined(PDK_OS_OSX)
#include <CoreServices/CoreServices.h>
#elif defined(PDK_OS_WIN)
#include "pdk/global/Windows.h"
#endif
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/utils/Locale.h"
#include "pdk/utils/SharedData.h"

#include <vector>

namespace pdk {
namespace utils {
namespace internal {

using pdk::ds::ByteArray;
using pdk::lang::String;
using pdk::os::thread::AtomicInt;
using pdk::utils::Locale;
using pdk::utils::SharedData;
using pdk::utils::Collator;

#if PDK_CONFIG(icu)
using CollatorType = UCollator *;
using CollatorKeyType = ByteArray;

#elif defined(PDK_OS_OSX)
using CollatorType = CollatorRef;
using CollatorKeyType = std::vector<UCCollationValue>;
#elif defined(PDK_OS_WIN)
using CollatorKeyType = String;
using CollatorType = int;
#else //posix
using CollatorKeyType = std::vector<wchar_t>;
using CollatorType = int;
#endif

class CollatorPrivate
{
public:
   AtomicInt m_ref;
   Locale m_locale;
#if defined(PDK_OS_WIN) && !PDK_CONFIG(icu)
#ifdef USE_COMPARESTRINGEX
   String m_localeName;
#else
   LCID m_localeID;
#endif
#endif
   pdk::CaseSensitivity m_caseSensitivity;
   bool m_numericMode;
   bool m_ignorePunctuation;
   bool m_dirty;
   
   CollatorType m_collator;
   
   void clear() {
      cleanup();
      m_collator = 0;
   }
   
   void init();
   void cleanup();
   
   CollatorPrivate()
      : m_ref(1),
        m_caseSensitivity(pdk::CaseSensitivity::Sensitive),
        m_numericMode(false),
        m_ignorePunctuation(false),
        m_dirty(true),
        m_collator(0)
   {
      cleanup();
   }
   
   ~CollatorPrivate()
   {
      cleanup();
   }
   
private:
   PDK_DISABLE_COPY(CollatorPrivate);
};


class CollatorSortKeyPrivate : public SharedData
{
   friend class Collator;
public:
   template <typename...T>
   explicit CollatorSortKeyPrivate(T &&...args)
      : SharedData()
      , m_key(std::forward<T>(args)...)
   {
   }
   
   CollatorKeyType m_key;
   
private:
   PDK_DISABLE_COPY(CollatorSortKeyPrivate);
};

} // internal
} // utils
} // pdk

#endif // PDK_UTILS_INTERNAL_COLLATOR_PRIVATE_H
