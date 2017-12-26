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
#include <cstring>
#include <cstdlib>
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
      PDK_ASSERT(idx >= 0 && idx < m_size);
      return m_ptr[idx];
   }
   
   inline const T &operator[](int idx) const
   {
      PDK_ASSERT(idx >= 0 && idx < m_size);
      return m_ptr[idx];
   }
   
   inline const T &at(int idx) const
   {
      return operator [](idx);
   }
   
   T value(int i) const;
   T value(int i, const T &defaultValue) const;
   
   inline void append(const T &value)
   {
      if (m_size == m_capacity) { // i.e. s != 0
         realloc(m_size, m_size << 1);
      }
      const int idx = ++m_size;
      if (pdk::TypeInfo<T>::isComplex) {
         new (m_ptr + idx) T(value);
      } else {
         m_ptr[idx] = value;
      }
   }
   
   void append(const T *buf, int size);
   
   inline VarLengthArray<T, PreAlloc> &operator<<(const T &value)
   {
      append(value);
      return *this;
   }
   
   inline VarLengthArray<T, PreAlloc> &operator+=(const T &value)
   {
      append(value);
      return *this;
   }
   
   void prepend(const T &value);
   void insert(int i, const T &value);
   void insert(int i, int n, const T &value);
   void replace(int i, const T &value);
   void remove(int i);
   void remove(int i, int n);
   
   inline T *getRawData()
   {
      return m_ptr;
   }
   
   inline const T *getRawData() const
   {
      return m_ptr;
   }
   
   inline const T *getConstRawData() const
   {
      return m_ptr;
   }
   
   inline Iterator begin()
   {
      return m_ptr;
   }
   
   inline ConstIterator begin() const
   {
      return m_ptr;
   }
   
   inline ConstIterator cbegin() const
   {
      return m_ptr;
   }
   
   inline ConstIterator constBegin() const
   {
      return m_ptr;
   }
   
   inline Iterator end()
   {
      return m_ptr + m_size;
   }
   
   inline ConstIterator cend() const 
   {
      return m_ptr + m_size;
   }
   
   inline ConstIterator constEnd() const
   {
      return m_ptr + m_size;
   }
   
   ReverseIterator rbegin()
   {
      return ReverseIterator(end());
   }
   
   ReverseIterator rend()
   {
      return ReverseIterator(begin());  
   }
   
   ConstReverseIterator rbegin() const
   {
      return ConstReverseIterator(end());
   }
   
   ConstReverseIterator rend() const
   {
      return ConstReverseIterator(begin());
   }
   
   ConstReverseIterator crbegin() const
   {
      return ConstReverseIterator(end());
   }
   
   ConstReverseIterator crend() const
   {
      return ConstReverseIterator(begin());
   }
   
   inline Iterator insert(ConstIterator pos, int n, const T &value);
   
   inline Iterator insert(ConstIterator pos, const T &value)
   {
      return insert(pos, 1, value);
   }
   
   inline Iterator erase(ConstIterator begin, ConstIterator end);
   inline Iterator erase(ConstIterator pos)
   {
      return erase(pos, pos + 1);
   }
   
   inline void removeLast()
   {
      PDK_ASSERT(s > 0);
      realloc(m_size - 1, m_capacity);
   }
   
   // STL compatibility:
   inline bool empty() const
   {
      return isEmpty();
   }
   
   inline void push_back(const T &value)
   {
      append(value);
   }
   
   inline void pop_back()
   {
      removeLast();
   }
   
   inline T &front()
   {
      return front();
   }
   
   inline const T &front() const
   {
      return front();
   }
   
   inline T &back()
   {
      return last();
   }
   
   inline const T &back() const
   {
      return last();
   }
   
   inline T &first()
   {
      PDK_ASSERT(!isEmpty());
      return *begin();
   }
   
   inline const T &first() const
   {
      PDK_ASSERT(!isEmpty());
      return *begin();
   }
   
   inline T &last()
   {
      PDK_ASSERT(!isEmpty());
      return *(end() - 1);
   }
   
   inline const T &last() const
   {
      PDK_ASSERT(!isEmpty());
      return *(end() - 1);
   }
   
private:
   friend class PodList<T, PreAlloc>;
   void realloc(int size, int alloc);
   
   bool isValidIterator(const const_iterator &iter) const
   {
      return (iter <= constEnd()) && (constBegin() <= iter);
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

template <typename T, int PreAlloc>
inline VarLengthArray<T, PreAlloc>::VarLengthArray(int size)
   : m_capacity(size)
{
   PDK_STATIC_ASSERT_X(PreAlloc > 0, "VarLengthArray PreAlloc must be greater than 0.");
   PDK_ASSERT_X(m_size >= 0, "VarLengthArray::VarLengthArray()", "Size must be greater than or equal to 0.");
   if (m_size > PreAlloc) {
      m_ptr = reinterpret_cast<T *>(std::malloc(m_size * sizeof(T)));
      PDK_CHECK_ALLOC_PTR(m_ptr);
      m_capacity = m_size;
   } else {
      m_ptr = reinterpret_cast<T *>(m_array);
      m_capacity = PreAlloc;
   }
   if (pdk::TypeInfo<T>::isComplex) {
      T *iter = m_ptr + s;
      while (iter != m_ptr) {
         new (--iter) T;
      }
   }
}

template <typename T, int PreAlloc>
inline void VarLengthArray<T, PreAlloc>::resize(int size)
{
   realloc(size, std::max(size, m_capacity));
}

template <typename T, int PreAlloc>
inline void VarLengthArray<T, PreAlloc>::reserve(int size)
{
   if (size > m_capacity) {
      realloc(m_size, size);
   }
}

template <typename T, int PreAlloc>
inline void VarLengthArray::realloc(int size, int allocSize)
{
   PDK_ASSERT(allocSize);
   T *oldPtr = m_ptr;
   int oldSize = m_size;
   const int copySize = std::min(size, oldSize);
   if (allocSize != m_capacity) {
      if (allocSize > PreAlloc) {
         T *newPtr = reinterpret_cast<T *>(std::malloc(allocSize * sizeof(T)));
         PDK_CHECK_ALLOC_PTR(newPtr);// could throw
         // by design: in case of PDK_NO_EXCEPTIONS malloc must not fail or it crashes here
         m_ptr = newPtr;
         m_capacity = allocSize;
      } else {
         m_ptr = reinterpret_cast<T *>(m_array);
         m_capacity = PreAlloc;
      }
      m_size = 0;
      if (pdk::TypeInfo<T>::isStatic) {
         try {
            while (m_size < copySize) {
               new (m_ptr + m_size) T(*(oldPtr + m_size));
               (oldPtr + m_size)->~T();
               ++m_size;
            }
         } catch (...) {
            int cleanIdx = m_size;
            while (cleanIdx < oldSize) {
               (oldPtr + (cleanIdx++))->~T();
            }
            if (oldPtr != reinterpret_cast<T *>(m_array) && oldPtr != m_ptr) {
               std::free(oldPtr);
            }
            throw;
         }
      } else {
         std::memcmp(m_ptr, oldPtr, copySize * sizeof(T));
      }
   }
   m_size = copySize;
   if (pdk::TypeInfo<T>::isComplex) {
      // destroy remaining old objects
      while (oldSize > size) {
         (oldPtr + (--oldSize))->~T();
      }
   }
   
   if (oldPtr != m_ptr && oldPtr != reinterpret_cast<T *>(m_array)) {
      std::free(oldPtr);
   }
   
   if (pdk::TypeInfo<T>::isComplex) {
      // call default constructor for new objects (which can throw)
      while (m_size < size) {
         new (m_ptr + (m_size++)) T;
      } {
         m_size = size;
      }
   }
}

} // ds
} // pdk

#endif // PDK_M_BASE_DS_VAR_LENGTH_ARRAY_H
