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

namespace pdk {
namespace utils {

using internal::CollatorPrivate;

Collator::Collator(const Locale &locale)
   : m_implPtr(new CollatorPrivate)
{
   m_implPtr->m_locale = locale;
   m_implPtr->init();
}

Collator::Collator(const Collator &other)
   : m_implPtr(other.m_implPtr)
{
   m_implPtr->m_ref.ref();
}

Collator::~Collator()
{
   if (m_implPtr && !m_implPtr->m_ref.deref()) {
      delete m_implPtr;
   }
}

Collator &Collator::operator=(const Collator &other)
{
   if (this != &other) {
      if (m_implPtr && !m_implPtr->m_ref.deref()) {
         delete m_implPtr;
      }
      m_implPtr = other.m_implPtr;
      if (m_implPtr) {
         m_implPtr->m_ref.ref();
      }
   }
   return *this;
}

void Collator::detach()
{
   if (m_implPtr->m_ref.load() != 1) {
      CollatorPrivate *x = new CollatorPrivate;
      x->m_ref.store(1);
      x->m_locale = m_implPtr->m_locale;
      x->m_collator = 0;
      if (!m_implPtr->m_ref.deref()) {
         delete m_implPtr;
      }
      m_implPtr = x;
      m_implPtr->init();
   }
}

void Collator::setLocale(const Locale &locale)
{
   if (locale == m_implPtr->m_locale) {
      return;
   }
   detach();
   m_implPtr->m_locale = locale;
   m_implPtr->m_dirty = true;
}

Locale Collator::getLocale() const
{
   return m_implPtr->m_locale;
}

void Collator::setCaseSensitivity(pdk::CaseSensitivity cs)
{
   if (m_implPtr->m_caseSensitivity == cs) {
      return;
   }
   detach();
   m_implPtr->m_caseSensitivity = cs;
   m_implPtr->m_dirty = true;
}

pdk::CaseSensitivity Collator::getCaseSensitivity() const
{
   return m_implPtr->m_caseSensitivity;
}

void Collator::setNumericMode(bool on)
{
   if (m_implPtr->m_numericMode == on) {
      return;
   }
   detach();
   m_implPtr->m_numericMode = on;
   m_implPtr->m_dirty = true;
}

bool Collator::getNumericMode() const
{
   return m_implPtr->m_numericMode;
}

void Collator::setIgnorePunctuation(bool on)
{
   if (m_implPtr->m_ignorePunctuation == on) {
      return;
   }
   detach();
   m_implPtr->m_ignorePunctuation = on;
   m_implPtr->m_dirty = true;
}

bool Collator::getIgnorePunctuation() const
{
   return m_implPtr->m_ignorePunctuation;
}

CollatorSortKey::CollatorSortKey(CollatorSortKeyPrivate *d)
   : m_implPtr(d)
{}

CollatorSortKey::CollatorSortKey(const CollatorSortKey &other)
   : m_implPtr(other.m_implPtr)
{}

CollatorSortKey::~CollatorSortKey()
{}

CollatorSortKey& CollatorSortKey::operator=(const CollatorSortKey &other)
{
   if (this != &other) {
      m_implPtr = other.m_implPtr;
   }
   return *this;
}

} // utils
} // pdk
