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
// Created by softboy on 2018/02/02.

#include "pdk/global/Global.h"
#include "pdk/base/lang/String.h"

namespace pdk {
namespace lang {

class StringIterator
{
   String::const_iterator m_start;
   String::const_iterator m_pos;
   String::const_iterator m_end;
   PDK_STATIC_ASSERT((std::is_same<String::const_iterator, const Character *>::value));
public:
   explicit StringIterator(StringView string, pdk::sizetype idx = 0)
      : m_start(string.begin()),
        m_pos(m_start + idx),
        m_end(string.end())
   {
   }
   
   inline explicit StringIterator(const Character *begin, const Character *end)
      : m_start(begin),
        m_pos(begin),
        m_end(end)
   {
   }
   
   inline explicit StringIterator(const Character *begin, int idx, const Character *end)
      : m_start(begin),
        m_pos(begin + idx),
        m_end(end)
   {
   }
   
   inline String::const_iterator position() const
   {
      return m_pos;
   }
   
   inline int index() const
   {
      return m_pos - m_start;
   }
   
   inline void setPosition(String::const_iterator position)
   {
      PDK_ASSERT_X(m_start <= position && position <= m_end, PDK_FUNC_INFO, "position out of bounds");
      m_pos = position;
   }
   
   // forward iteration
   
   inline bool hasNext() const
   {
      return m_pos < m_end;
   }
   
   inline void advance()
   {
      PDK_ASSERT_X(hasNext(), PDK_FUNC_INFO, "iterator hasn't a next item");
      if (PDK_UNLIKELY((m_pos++)->isHighSurrogate())) {
         if (PDK_LIKELY(m_pos != m_end && m_pos->isLowSurrogate())) {
            ++m_pos;
         }
      }
   }
   
   inline void advanceUnchecked()
   {
      PDK_ASSERT_X(hasNext(), PDK_FUNC_INFO, "iterator hasn't a next item");
      
      if (PDK_UNLIKELY((m_pos++)->isHighSurrogate())) {
         ++m_pos;
      }
   }
   
   inline uint peekNextUnchecked() const
   {
      PDK_ASSERT_X(hasNext(), PDK_FUNC_INFO, "iterator hasn't a next item");
      if (PDK_UNLIKELY(m_pos->isHighSurrogate())) {
         return Character::surrogateToUcs4(m_pos[0], m_pos[1]);
      }
      return m_pos->unicode();
   }
   
   inline uint peekNext(uint invalidAs = Character::ReplacementCharacter) const
   {
      PDK_ASSERT_X(hasNext(), PDK_FUNC_INFO, "iterator hasn't a next item");
      if (PDK_UNLIKELY(m_pos->isSurrogate())) {
         if (PDK_LIKELY(m_pos->isHighSurrogate())) {
            const Character *low = m_pos + 1;
            if (PDK_LIKELY(low != m_end && low->isLowSurrogate())) {
               return Character::surrogateToUcs4(*m_pos, *low);
            }
         }
         return invalidAs;
      }
      return m_pos->unicode();
   }
   
   inline uint nextUnchecked()
   {
      PDK_ASSERT_X(hasNext(), PDK_FUNC_INFO, "iterator hasn't a next item");
      const Character cur = *m_pos++;
      if (PDK_UNLIKELY(cur.isHighSurrogate())) {
         return Character::surrogateToUcs4(cur, *m_pos++);
      }
      return cur.unicode();
   }
   
   inline uint next(uint invalidAs = Character::ReplacementCharacter)
   {
      PDK_ASSERT_X(hasNext(), PDK_FUNC_INFO, "iterator hasn't a next item");
      const Character uc = *m_pos++;
      if (PDK_UNLIKELY(uc.isSurrogate())) {
         if (PDK_LIKELY(uc.isHighSurrogate() && m_pos < m_end && m_pos->isLowSurrogate())) {
            return Character::surrogateToUcs4(uc, *m_pos++);
         }
         return invalidAs;
      }
      return uc.unicode();
   }
   
   // backwards iteration
   
   inline bool hasPrevious() const
   {
      return m_pos > m_start;
   }
   
   inline void recede()
   {
      PDK_ASSERT_X(hasPrevious(), PDK_FUNC_INFO, "iterator hasn't a previous item");
      if (PDK_UNLIKELY((--m_pos)->isLowSurrogate())) {
         const Character *high = m_pos - 1;
         if (PDK_LIKELY(high != m_start - 1 && high->isHighSurrogate())) {
            --m_pos;
         }
      }
   }
   
   inline void recedeUnchecked()
   {
      PDK_ASSERT_X(hasPrevious(), PDK_FUNC_INFO, "iterator hasn't a previous item");
      if (PDK_UNLIKELY((--m_pos)->isLowSurrogate())) {
         --m_pos;
      }
   }
   
   inline uint peekPreviousUnchecked() const
   {
      PDK_ASSERT_X(hasPrevious(), PDK_FUNC_INFO, "iterator hasn't a previous item");
      if (PDK_UNLIKELY(m_pos[-1].isLowSurrogate())) {
         return Character::surrogateToUcs4(m_pos[-2], m_pos[-1]);
      }
      return m_pos[-1].unicode();
   }
   
   inline uint peekPrevious(uint invalidAs = Character::ReplacementCharacter) const
   {
      PDK_ASSERT_X(hasPrevious(), PDK_FUNC_INFO, "iterator hasn't a previous item");
      if (PDK_UNLIKELY(m_pos[-1].isSurrogate())) {
         if (PDK_LIKELY(m_pos[-1].isLowSurrogate())) {
            const Character *high = m_pos - 2;
            if (PDK_LIKELY(high != m_start - 1 && high->isHighSurrogate())) {
               return Character::surrogateToUcs4(*high, m_pos[-1]);
            }
         }
         return invalidAs;
      }
      return m_pos[-1].unicode();
   }
   
   inline uint previousUnchecked()
   {
      PDK_ASSERT_X(hasPrevious(), PDK_FUNC_INFO, "iterator hasn't a previous item");
      const Character cur = *--m_pos;
      if (PDK_UNLIKELY(cur.isLowSurrogate())) {
         return Character::surrogateToUcs4(*--m_pos, cur);
      }
      return cur.unicode();
   }
   
   inline uint previous(uint invalidAs = Character::ReplacementCharacter)
   {
      PDK_ASSERT_X(hasPrevious(), PDK_FUNC_INFO, "iterator hasn't a previous item");
      const Character uc = *--m_pos;
      if (PDK_UNLIKELY(uc.isSurrogate())) {
         if (PDK_LIKELY(uc.isLowSurrogate() && m_pos > m_start && m_pos[-1].isHighSurrogate())) {
            return Character::surrogateToUcs4(*--m_pos, uc);
         }
         return invalidAs;
      }
      return uc.unicode();
   }
};
} // lang
} // pdk
