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
   using SizeType = int;
   using ValueType = T;
   using Pointer = SizeType *;
   using ConstPointer = const SizeType *;
   using Reference = ValueType &;
   using ConstReference = const ValueType &;
   using DifferenceType = pdk::ptrdiff;
   
   using size_type = SizeType;
   using value_type = ValueType;
   using pointer = Pointer;
   using const_pointer = ConstPointer;
   using reference =  Reference;
   using const_reference = ConstReference;
   using difference_type = DifferenceType;
   
   using Iterator = T *;
   using ConstIterator = const T *;
   using ReverseIterator = std::reverse_iterator<Iterator>;
   using ConstReverseIterator = std::reverse_iterator<ConstIterator>;
   
public:
   
   inline explicit VarLengthArray(int size = 0);
   inline VarLengthArray(const VarLengthArray<T, PreAlloc> &other)
      : m_capacity(PreAlloc),
        m_size(0),
        m_ptr(reinterpret_cast<T *>(m_array))
   {
      append(other.getConstRawData(), other.size());
   }
   
   VarLengthArray(std::initializer_list<T> args)
      : m_array(PreAlloc),
        m_size(0),
        ptr(reinterpret_cast<T *>(m_array))
   {
      if (args) {
         append(args.begin(), static_cast<int>(args.size()));
      }
   }
   
   inline ~VarLengthArray()
   {
      if (pdk::TypeInfo<T>::isComplex) {
         T *i = m_ptr + m_size;
         while (i-- != m_ptr) {
            i->~T();
         }
      }
      if (m_ptr != reinterpret_cast<T *>(m_array)) {
         free(m_ptr);
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
   inline bool contains(const T &value) const;
   inline T &operator[](int idx)
   {
      
   }
   inline const T &operator[](int idx) const
   {
      
   }
   
   inline const T &at(int idx) const
   {
      
   }
   
   T value(int i) const;
   T value(int i, const T &defaultValue) const;
   
   inline void append(const T &value)
   {
      
   }
   
   void append(const T *buf, int size);
   
   inline VarLengthArray<T, PreAlloc> &operator<<(const T &value);
   inline VarLengthArray<T, PreAlloc> &operator+=(const T &value);
   
   void prepend(const T &value);
   void insert(int i, const T &value);
   void insert(int i, int n, const T &value);
   void replace(int i, const T &value);
   void remove(int i);
   void remove(int i, int n);
   
   inline T *getRawData()
   {
      
   }
   
   inline const T *getRawData() const
   {
      
   }
   
   inline const T *getConstRawData() const
   {
      
   }
   
   inline Iterator begin()
   {}
   
   inline ConstIterator begin() const
   {
      
   }
   
   inline ConstIterator cbegin() const
   {
      
   }
   
   inline ConstIterator constBegin() const
   {
      
   }
   
   inline Iterator end()
   {}
   
   inline ConstIterator cend() const 
   {}
   
   inline ConstIterator constEnd() const
   {
      
   }
   
   ReverseIterator rbegin()
   {}
   
   ReverseIterator rend()
   {}
   
   ConstReverseIterator rbegin() const
   {
      
   }
   
   ConstReverseIterator rend() const
   {}
   
   ConstReverseIterator crbegin() const
   {}
   
   ConstReverseIterator crend() const
   {
      
   }
   
   inline Iterator insert(ConstIterator pos, int n, const T &value)
   {}
   
   Iterator erase(ConstIterator begin, ConstIterator end)
   {
      
   }
   
   inline Iterator erase(ConstIterator pos)
   {}
   
   // STL compatibility:
   inline bool empty() const
   {}
   
   inline void push_back(const T &value)
   {}
   
   inline void pop_back()
   {}
   
   inline T &front()
   {
      
   }
   
   inline const T &front() const
   {
      
   }
   
   inline T &back()
   {
      
   }
   
   inline const T &back() const
   {}
   
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
