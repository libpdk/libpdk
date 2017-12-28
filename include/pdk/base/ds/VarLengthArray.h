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
#include "pdk/base/ds/internal/ContainerFwd.h"
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
   
   using iterator = Iterator;
   using const_iterator = ConstIterator;
   using reverse_iterator = ReverseIterator;
   using const_reverse_iterator = ConstReverseIterator;
   
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
        m_ptr(reinterpret_cast<T *>(m_array))
   {
      if (args) {
         append(args.begin(), static_cast<int>(args.size()));
      }
   }
   
   inline VarLengthArray<T, PreAlloc> &operator =(std::initializer_list<T> list)
   {
      resize(list.size());
      std::copy(list.begin(), list.end(), this->begin());
      return *this;
   }
   
   inline VarLengthArray<T, PreAlloc> &operator =(const VarLengthArray<T, PreAlloc> &other)
   {
      if (this != &other) {
         clear();
         append(other.getConstRawData(), other.size());
      }
      return *this;
   }
   
   inline ~VarLengthArray()
   {
      if (pdk::TypeInfo<T>::isComplex) {
         T *iter = m_ptr + m_size;
         while (iter-- != m_ptr) {
            iter->~T();
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
   inline int indexOf(const T &value, int from = 0) const;
   inline int lastIndexOf(const T &value, int from = -1) const;
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
      const int idx = m_size++;
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
      PDK_ASSERT(m_size > 0);
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
   
   bool isValidIterator(const ConstIterator &iter) const
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
   : m_size(size)
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
      T *iter = m_ptr + m_size;
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
inline void VarLengthArray<T, PreAlloc>::squeeze()
{
   realloc(m_size, m_size);  
}

template <typename T, int PreAlloc>
inline int VarLengthArray<T, PreAlloc>::indexOf(const T &value, int from) const
{
   if (from < 0) {
      from = std::max(from + m_size, 0);
   }
   if (from < m_size) {
      T *iter = m_ptr + from - 1;
      T *end = m_ptr + m_size;
      while (++iter != end) {
         if (*iter == value) {
            return iter - m_ptr;
         }
      }
   }
   return -1;
}

template <typename T, int PreAlloc>
inline int VarLengthArray<T, PreAlloc>::lastIndexOf(const T &value, int from) const
{
   if (from < 0) {
      from += m_size;
   } else if (from >= m_size) {
      from = m_size - 1;
   }
   if (from >= 0) {
      T *begin = m_ptr;
      T *iter = m_ptr + from + 1;
      while (iter != begin) {
         if (*--iter == value) {
            return iter - begin;
         }
      }
   }
}

template <typename T, int PreAlloc>
bool VarLengthArray<T, PreAlloc>::contains(const T &value) const
{
   T *begin = m_ptr;
   T *iter = m_ptr + m_size;
   while (iter != begin) {
      if (*--iter == value) {
         return true;
      }
   }
   return false;
}

template <typename T, int PreAlloc>
void VarLengthArray<T, PreAlloc>::append(const T *buf, int increment)
{
   PDK_ASSERT(buf);
   if (increment <= 0) {
      return;
   }
   const int newSize = m_size + increment;
   if (newSize >= m_capacity) {
      realloc(m_size, std::max(m_size * 2, newSize));
   }
   if (pdk::TypeInfo<T>::isComplex) {
      // call constructor for new objects (which can throw)
      while (m_size < newSize) {
         new (m_ptr + (m_size++)) T(*buf++);
      }
   } else {
      std::memcpy(&m_ptr[m_size], buf, increment * sizeof(T));
      m_size = newSize;
   }
}

template <typename T, int PreAlloc>
inline void VarLengthArray<T, PreAlloc>::realloc(int size, int allocSize)
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
      }
   } else {
      m_size = size;
   }
}

template <typename T, int PreAlloc>
inline T VarLengthArray<T, PreAlloc>::value(int i) const
{
   if (static_cast<uint>(i) >= static_cast<uint>(size())) {
      return T();
   }
   return at(i);
}

template <typename T, int PreAlloc>
inline T VarLengthArray<T, PreAlloc>::value(int i, const T &defaultValue) const
{
   return (static_cast<uint>(i) >= static_cast<uint>(size())) ? defaultValue : at(i);
}

template <typename T, int PreAlloc>
inline void VarLengthArray<T, PreAlloc>::insert(int i, const T &value)
{
   PDK_ASSERT_X(i >= 0 && i <= m_size, "VarLengthArray::insert", "index out of range");
   insert(begin() + i, 1, value);
}

template <typename T, int PreAlloc>
inline void VarLengthArray<T, PreAlloc>::insert(int i, int n, const T &value)
{
   PDK_ASSERT_X(i >= 0 && i <= m_size, "VarLengthArray::insert", "index out of range");
   insert(begin() + i, n, value);
}

template <typename T, int PreAlloc>
inline void VarLengthArray<T, PreAlloc>::remove(int i, int n)
{
   PDK_ASSERT_X(i >= 0 && n >= 0 && i + n <= m_size, "VarLengthArray::remove", "index out of range");
   erase(begin() + i, begin() + i + n);
}

template <typename T, int PreAlloc>
inline void VarLengthArray<T, PreAlloc>::remove(int i)
{
   PDK_ASSERT_X(i >= 0 && i < m_size, "VarLengthArray::remove", "index out of range");
   erase(begin() + i, begin() + i + 1);
}

template <typename T, int PreAlloc>
inline void VarLengthArray<T, PreAlloc>::prepend(const T &value)
{
   insert(begin(), 1, value);
}

template <typename T, int PreAlloc>
inline void VarLengthArray<T, PreAlloc>::replace(int i, const T &value)
{
   PDK_ASSERT_X(i >= 0 && i < m_size, "VarLengthArray::replace", "index out of range");
   const T copy(value);
   getRawData()[i] = copy;
}

template <typename T, int PreAlloc>
inline typename VarLengthArray<T, PreAlloc>::Iterator
VarLengthArray<T, PreAlloc>::insert(ConstIterator before, SizeType n, const T &value)
{
   PDK_ASSERT_X(isValidIterator(before), "VarLengthArray::insert", 
                "The specified const_iterator argument 'before' is invalid");
   int offset = static_cast<int>(before - m_ptr);
   if (n != 0) {
      resize(m_size + n);
      const T copy(value);
      if (pdk::TypeInfo<T>::isStatic) {
         T *begin = m_ptr + offset;
         T *end = m_ptr + m_size;
         T *iter = end - n;
         while (iter != begin) {
            *--end = *--iter;
         }
         iter = begin + n;
         while (iter != begin) {
            *--iter = copy;
         }
      } else {
         T *begin = m_ptr + offset;
         T *iter = begin + n;
         std::memmove(iter, begin, (m_size - offset - n) * sizeof(T));
         while (iter != begin) {
            new (--iter) T(copy);
         }
      }
   }
   return m_ptr + offset;
}

template <typename T, int PreAlloc>
inline typename VarLengthArray<T, PreAlloc>::Iterator
VarLengthArray<T, PreAlloc>::erase(ConstIterator begin, ConstIterator end)
{
   PDK_ASSERT_X(isValidIterator(begin), "VarLengthArray::erase", "The specified const_iterator argument 'begin' is invalid");
   PDK_ASSERT_X(isValidIterator(end), "VarLengthArray::erase", "The specified const_iterator argument 'end' is invalid");
   int f = static_cast<int>(begin - m_ptr);
   int l = static_cast<int>(end - m_ptr);
   int n = l - f;
   if (pdk::TypeInfo<T>::isComplex) {
      std::copy(m_ptr + l, m_ptr + m_size, m_ptr + f);
      T *iter = m_ptr + m_size;
      T *stop = m_ptr + m_size - n;
      while (iter != stop) {
         (--iter)->~T();
      }
   } else {
      std::memmove(m_ptr + f, m_ptr + l, (m_size - l) * sizeof(T));
   }
   m_size -= n;
   return m_ptr + f;
}

template <typename T, int LhsPreAlloc, int RhsPreAlloc>
bool operator ==(const VarLengthArray<T, LhsPreAlloc> &lhs, const VarLengthArray<T, RhsPreAlloc> &rhs)
{
   if (lhs.size() != rhs.size()) {
      return false;
   }
   
   const T *lhsBegin = lhs.begin();
   const T *lhsEnd = lhs.end();
   const T *rhsBegin = rhs.begin();
   return std::equal(lhsBegin, lhsEnd, rhsBegin);
}

template <typename T, int LhsPreAlloc, int RhsPreAlloc>
bool operator !=(const VarLengthArray<T, LhsPreAlloc> &lhs, const VarLengthArray<T, RhsPreAlloc> &rhs)
{
   return !(lhs == rhs);
}

template <typename T, int LhsPreAlloc, int RhsPreAlloc>
bool operator <(const VarLengthArray<T, LhsPreAlloc> &lhs, const VarLengthArray<T, RhsPreAlloc> &rhs)
noexcept(noexcept(std::lexicographical_compare(lhs.begin(), lhs.end(),
                                               rhs.begin(), rhs.end)))
{
   return std::lexicographical_compare(lhs.begin(), lhs.end(),
                                       rhs.begin(), rhs.end);
}

template <typename T, int LhsPreAlloc, int RhsPreAlloc>
bool operator >(const VarLengthArray<T, LhsPreAlloc> &lhs, const VarLengthArray<T, RhsPreAlloc> &rhs)
noexcept(noexcept(rhs < lhs))
{
   return rhs < lhs;
}

template <typename T, int LhsPreAlloc, int RhsPreAlloc>
bool operator <=(const VarLengthArray<T, LhsPreAlloc> &lhs, const VarLengthArray<T, RhsPreAlloc> &rhs)
noexcept(noexcept(!(lhs > rhs)))
{
   return !(lhs > rhs);
}

template <typename T, int LhsPreAlloc, int RhsPreAlloc>
bool operator >=(const VarLengthArray<T, LhsPreAlloc> &lhs, const VarLengthArray<T, RhsPreAlloc> &rhs)
noexcept(noexcept(!(lhs > rhs)))
{
   return !(lhs < rhs);
}

} // ds
} // pdk

#endif // PDK_M_BASE_DS_VAR_LENGTH_ARRAY_H
