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
// Created by softboy on 2018/02/24.

#ifndef PDK_UTILS_COLLATOR_H
#define PDK_UTILS_COLLATOR_H

#include "pdk/base/lang/String.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/utils/Locale.h"
#include "pdk/utils/SharedData.h"

namespace pdk {
namespace utils {

// forward declare class with namespace
namespace internal {
class CollatorPrivate;
class CollatorSortKeyPrivate;
} // internal

using internal::CollatorSortKeyPrivate;
using internal::CollatorPrivate;
using pdk::lang::Character;

class PDK_CORE_EXPORT CollatorSortKey
{
   friend class Collator;
public:
   CollatorSortKey(const CollatorSortKey &other);
   ~CollatorSortKey();
   CollatorSortKey &operator=(const CollatorSortKey &other);
   inline CollatorSortKey &operator=(CollatorSortKey &&other) noexcept
   {
      swap(other);
      return *this;
   }
   
   void swap(CollatorSortKey &other) noexcept
   {
      m_implPtr.swap(other.m_implPtr);
   }
   
   int compare(const CollatorSortKey &key) const;
   
protected:
   CollatorSortKey(CollatorSortKeyPrivate*);
   
   pdk::utils::ExplicitlySharedDataPointer<CollatorSortKeyPrivate> m_implPtr;
   
private:
   CollatorSortKey();
};

inline bool operator<(const CollatorSortKey &lhs, const CollatorSortKey &rhs)
{
   return lhs.compare(rhs) < 0;
}

class PDK_CORE_EXPORT Collator
{
public:
   explicit Collator(const Locale &locale = Locale());
   Collator(const Collator &);
   ~Collator();
   Collator &operator=(const Collator &);
   
   Collator(Collator &&other) noexcept
      : m_implPtr(other.m_implPtr)
   {
      other.m_implPtr = nullptr;
   }
   
   Collator &operator=(Collator &&other) noexcept
   {
      swap(other); return *this;
   }
   
   void swap(Collator &other) noexcept
   {
      std::swap(m_implPtr, other.m_implPtr);
   }
   
   void setLocale(const Locale &locale);
   Locale getLocale() const;
   
   pdk::CaseSensitivity getCaseSensitivity() const;
   void setCaseSensitivity(pdk::CaseSensitivity cs);
   
   void setNumericMode(bool on);
   bool getNumericMode() const;
   
   void setIgnorePunctuation(bool on);
   bool getIgnorePunctuation() const;
   
   int compare(const String &s1, const String &s2) const;
   int compare(const StringRef &s1, const StringRef &s2) const;
   int compare(const Character *s1, int len1, const Character *s2, int len2) const;
   
   bool operator()(const String &s1, const String &s2) const
   {
      return compare(s1, s2) < 0;
   }
   
   CollatorSortKey sortKey(const String &string) const;
   
private:
   CollatorPrivate *m_implPtr;
   void detach();
};

} // utils
} // pdk

PDK_DECLARE_SHARED(pdk::utils::CollatorSortKey)
PDK_DECLARE_SHARED(pdk::utils::Collator)

#endif // PDK_UTILS_COLLATOR_H
