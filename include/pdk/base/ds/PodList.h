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
// Created by softboy on 2018/01/25.

#ifndef PDK_M_BASE_DS_POD_LIST_PRIVATE_H
#define PDK_M_BASE_DS_POD_LIST_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/ds/VarLengthArray.h"

namespace pdk {
namespace ds {

using pdk::ds::VarLengthArray;

template <typename T, int Prealloc>
class PodList : public VarLengthArray<T, Prealloc>
{
   using VarLengthArray<T, Prealloc>::m_size;
   using VarLengthArray<T, Prealloc>::m_capacity;
   using VarLengthArray<T, Prealloc>::m_ptr;
   using VarLengthArray<T, Prealloc>::realloc;
   
public:
   inline explicit PodList(int size = 0)
      : VarLengthArray<T, Prealloc>(size)
   {}
   
   inline void insert(int idx, const T &value)
   {
      const int size = m_size++;
      if (m_size == m_capacity) {
         realloc(m_size, m_size << 1);
      }
      
      ::memmove(m_ptr + idx + 1, m_ptr + idx, (size - idx) * sizeof(T));
      m_ptr[idx] = value;
   }
   
   inline void removeAll(const T &value)
   {
      int i = 0;
      for (int j = 0; j < m_size; ++j) {
         if (m_ptr[j] != value) {
            m_ptr[i++] = m_ptr[j];
         }
      }
      m_size = i;
   }
   
   inline void removeAt(int idx)
   {
      PDK_ASSERT(idx >= 0 && idx < m_size);
      ::memmove(m_ptr + idx, m_ptr + idx + 1, (m_size - idx - 1) * sizeof(T));
      --m_size;
   }
   
   inline T takeFirst()
   {
      PDK_ASSERT(m_size > 0);
      T tmp = m_ptr[0];
      removeAt(0);
      return tmp;
   }
};

} // ds
} // pdk

#endif // PDK_M_BASE_DS_POD_LIST_PRIVATE_H
