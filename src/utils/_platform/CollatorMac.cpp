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

#include "pdk/utils/internal/CollatorPrivate.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/io/Debug.h"
#include "pdk/utils/Locale.h"
#include "pdk/utils/internal/LocalePrivate.h"

#include "pdk/kernel/internal/CoreMacPrivate.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFLocale.h>
#include <cstring>

namespace pdk {
namespace utils {

namespace internal {

void CollatorPrivate::init()
{
   cleanup();
   LocaleRef localeRef;
   int rc = LocaleRefFromLocaleString(LocalePrivate::get(m_locale)->bcp47Name().getConstRawData(), &localeRef);
   if (rc != 0) {
      warning_stream("couldn't initialize the locale");
   }
   UInt32 options = 0;
   if (m_caseSensitivity == pdk::CaseSensitivity::Insensitive) {
      options |= kUCCollateCaseInsensitiveMask;
   }  
   if (m_numericMode) {
      options |= kUCCollateDigitsAsNumberMask | kUCCollateDigitsOverrideMask;
   }
   if (!m_ignorePunctuation) {
      options |= kUCCollatePunctuationSignificantMask;
   }
   OSStatus status = UCCreateCollator(
            localeRef,
            0,
            options,
            &m_collator
            );
   if (status != 0) {
      warning_stream("Couldn't initialize the collator");
   }
   m_dirty = false;
}

void CollatorPrivate::cleanup()
{
   if (m_collator) {
      UCDisposeCollator(&m_collator);
   }
   m_collator = 0;
}

} // internal

int Collator::compare(const Character *s1, int len1, const Character *s2, int len2) const
{
   if (m_implPtr->m_dirty) {
      m_implPtr->init();
   }
   SInt32 result;
   Boolean equivalent;
   UCCompareText(m_implPtr->m_collator,
                 reinterpret_cast<const UniChar *>(s1), len1,
                 reinterpret_cast<const UniChar *>(s2), len2,
                 &equivalent,
                 &result);
   if (equivalent) {
      return 0;
   }
   return result < 0 ? -1 : 1;
}
int Collator::compare(const String &str1, const String &str2) const
{
   return compare(str1.getConstRawData(), str1.size(), str2.getConstRawData(), str2.size());
}

int Collator::compare(const StringRef &s1, const StringRef &s2) const
{
   return compare(s1.getConstRawData(), s1.size(), s2.getConstRawData(), s2.size());
}

CollatorSortKey Collator::sortKey(const String &string) const
{
   if (m_implPtr->m_dirty) {
      m_implPtr->init();
   }
   //Documentation recommends having it 5 times as big as the input
   std::vector<UCCollationValue> ret(string.size() * 5);
   ItemCount actualSize;
   int status = UCGetCollationKey(m_implPtr->m_collator, reinterpret_cast<const UniChar *>(string.getConstRawData()), string.size(),
                                  ret.size(), &actualSize, ret.data());
   ret.resize(actualSize+1);
   if (status == kUCOutputBufferTooSmall) {
      UCGetCollationKey(m_implPtr->m_collator, reinterpret_cast<const UniChar *>(string.getConstRawData()), string.size(),
                        ret.size(), &actualSize, ret.data());
   }
   ret[actualSize] = 0;
   return CollatorSortKey(new CollatorSortKeyPrivate(std::move(ret)));
}

int CollatorSortKey::compare(const CollatorSortKey &key) const
{
   SInt32 order;
   UCCompareCollationKeys(m_implPtr->m_key.data(), m_implPtr->m_key.size(),
                          key.m_implPtr->m_key.data(), key.m_implPtr->m_key.size(),
                          0, &order);
   return order;
}

} // utils
} // pdk


