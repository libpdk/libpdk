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

#ifndef PDK_M_BASE_DS_VAR_LENGTH_ARRAY_H
#define PDK_M_BASE_DS_VAR_LENGTH_ARRAY_H

#include "pdk/global/Global.h"
#include <new>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <initializer_list>
#include <iterator>

namespace pdk {
namespace ds {

template <typename T, int PreAlloc>
class PodList;

template<typename T, int PreAlloc>
class VarLengthArray
{
public:
   
   inline explicit VarLengthArray(int size = 0)
   {
      
   }
   
   inline VarLengthArray(const VarLengthArray<T, PreAlloc> &other)
   {
      
   }
   
   VarLengthArray(std::initializer_list<T> args)
      : m_array(PreAlloc),
        m_size(0),
        ptr(reinterpret_cast<T *>(m_array))
   {
      if (args) {
         
      }
   }
   
   inline int size() const
   {
      return m_size;
   }
   
   inline int count() const
   {
      return m_size;
   }
   
   inline int length() const
   {
      return m_size;
   }
   
   inline bool isEmpty() const
   {
      return m_size == 0;
   }
   
   inline void resize(int size);
   inline void clear()
   {
      resize(0);
   }
   
   inline void squeeze();
   inline int capacity() const
   {
      return m_capacity;
   }
   
   inline void reserve(int size);
   inline int indexOf(const T &value, int from = 0);
   inline int lastIndexOf(const T &value, int from = -1);
private:
   friend class PodList<T, PreAlloc>;
   void realloc(int size, int alloc);
   bool isValidIterator(const const_iterator &iter) const
   {
      
   }
   
private:
   int m_capacity;
   int m_size;
   T *m_ptr;
   union {
      char m_array[PreAlloc * sizeof(T)];
      pint64 m_alignment1;
      double m_alignment2;
   };
   
};

} // ds
} // pdk

#endif // PDK_M_BASE_DS_VAR_LENGTH_ARRAY_H
