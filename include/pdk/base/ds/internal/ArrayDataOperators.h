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
// Created by softboy on 2017/12/07.

#ifndef PDK_M_BASE_DS_INTERNAL_ARRAYDATA_OPERATOR_H
#define PDK_M_BASE_DS_INTERNAL_ARRAYDATA_OPERATOR_H

#include "ArrayData.h"
#include "pdk/global/TypeInfo.h"

#include <new>
#include <string.h>

namespace pdk {
namespace ds {
namespace internal {

template <typename T>
struct PodArrayOperator : TypedArrayData<T>
{
   void appendInitialize(size_t newSize)
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(!this->m_ref.isShared());
      PDK_ASSERT(newSize > static_cast<uint>(this->m_size));
      PDK_ASSERT(newSize <= static_cast<uint>(this->m_alloc));
      std::memset(this->end(), 0, (newSize - this->m_size) * sizeof(T));
      this->m_size = static_cast<int>(newSize);
   }
   
   void copyAppend(const T *begin, const T *end)
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(!this->m_ref.isShared());
      PDK_ASSERT(begin < end);
      PDK_ASSERT(static_cast<size_t>(end - begin) <= this->m_alloc - static_cast<uint>(this->m_size));
      std::memcpy(this->end(), begin, (end - begin) * sizeof(T));
      this->m_size += end - begin;
   }
   
   void copyAppend(size_t n, const T &value)
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(!this->m_ref.isShared());
      PDK_ASSERT(n <= this->m_alloc - static_cast<uint>(this->m_size));
      T *iterator = this->end();
      const T *const end = iterator + n;
      for (; iterator != end; ++iterator) {
         std::memcpy(iterator, &value, sizeof(T));
      }
      this->m_size += static_cast<int>(n);
   }
   
   void truncate(size_t newSize)
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(!this->m_ref.isShared());
      PDK_ASSERT(newSize < static_cast<size_t>(this->m_size));
      this->m_size = static_cast<int>(newSize);
   }
   
   void destroyAll()
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(this->m_ref.m_atomic.load() == 0);
      // As this is to be called only from destructor, it doesn't need to be
      // exception safe; size not updated.
   }
   
   void insert(T *where, const T *begin, const T *end)
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(!this->m_ref.isShared());
      PDK_ASSERT(where >= this->begin() && where < this->end());
      PDK_ASSERT(begin < end);
      PDK_ASSERT(where >= end || begin > this->end());
      PDK_ASSERT(static_cast<size_t>(end - begin) <= this->m_alloc - static_cast<uint>(this->m_size));
      std::memmove(where + (end - begin), where, (static_cast<const T*>(this->end()) - where) * sizeof(T));
      std::memcpy(where, begin, (end - begin) * sizeof(T));
      this->m_size += (end - begin);
   }
   
   void erase(T *begin, T *end)
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(begin < end);
      PDK_ASSERT(begin >= this->begin() && begin < this->end());
      PDK_ASSERT(end > this->begin() && end < this->end());
      std::memmove(begin, end, (static_cast<T *>(this->end()) - end) * sizeof(T));
      this->m_size -= (end - begin);
   }
};

template <typename T>
struct GenericArrayOperator : TypedArrayData<T>
{
   void appendInitialize(size_t newSize)
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(!this->m_ref.isShared());
      PDK_ASSERT(newSize > static_cast<uint>(this->m_size));
      PDK_ASSERT(newSize <= this->m_alloc);
      T *const begin = this->begin();
      do {
         new (begin + this->m_size) T();
      } while (static_cast<size_t>(++this->m_size) != newSize);
   }
   
   void copyAppend(const T *begin, const T *end)
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(!this->m_ref.isShared());
      PDK_ASSERT(begin < end);
      PDK_ASSERT(static_cast<size_t>(end - begin) <= this->m_alloc - static_cast<size_t>(this->m_size));
      T *iterator = this->end();
      for (; begin != end; ++iterator, ++begin) {
         new (iterator) T(*begin);
         ++this->m_size;
      }
   }
   
   void copyAppend(size_t n, const T &value)
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(!this->m_ref.isShared());
      PDK_ASSERT(n <= this->m_alloc - static_cast<uint>(this->m_size));
      T *iterator = this->end();
      const T *const end = iterator + n;
      for (; iterator != end; ++iterator) {
         new (iterator) T(value);
         ++this->m_size;
      }
   }
   
   void truncate(size_t newSize)
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(!this->m_ref.isShared());
      PDK_ASSERT(newSize < static_cast<size_t>(this->m_size));
      const T * const begin = this->begin();
      do {
         (begin + (--this->m_size))->~T();
      } while (static_cast<size_t>(this->m_size) != newSize);
   }
   
   void destroyAll()
   {
      PDK_ASSERT(this->isMutable());
      // As this is to be called only from destructor, it doesn't need to be
      // exception safe; size not updated.
      PDK_ASSERT(this->m_ref.m_atomic.load() == 0);
      const T *const begin = this->begin();
      const T *iterator = this->end();
      while (iterator != begin) {
         (--iterator)->~T();
      }
   }
   
   void insert(T *where, const T *begin, const T *end)
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(!this->m_ref.isShared());
      PDK_ASSERT(where >= this->begin() && where < this->end());
      PDK_ASSERT(static_cast<size_t>(end - begin));
      PDK_ASSERT(begin < end);
      PDK_ASSERT(end <= where || begin > this->end());
      // Array may be truncated at where in case of exceptions
      T *const selfEnd = this->end();
      const T *readIterator = selfEnd;
      T *writerIterator = selfEnd + (end - begin);
      const T *const step1End = where + std::max(end - begin, selfEnd  - where);
      // @TODO why use this?
      struct Destructor
      {
         Destructor(T *&iterator)
            : m_iterator(&iterator),
              m_end(iterator)
         {}
         
         void commit()
         {
            m_iterator = &m_end;
            //            std::cout << m_end << std::endl;
            //            std::cout << *m_iterator << std::endl;
            //            std::cout << "------" << std::endl;
         }
         
         ~Destructor()
         {
            //            std::cout << m_end << std::endl;
            //            std::cout << *m_iterator << std::endl;
            for (; *m_iterator != m_end; --*m_iterator) {
               (*m_iterator)->~T();
            }
         }
         
         T **m_iterator;
         T *m_end;
      } destroyer(writerIterator);
      
      // Construct new elements in array
      do {
         --readIterator;
         --writerIterator;
         new (writerIterator) T(*readIterator);
      } while (writerIterator != step1End);
      
      while (writerIterator != selfEnd) {
         --end;
         --writerIterator;
         new (writerIterator) T(*end);
      }
      
      destroyer.commit();
      this->m_size += destroyer.m_end - selfEnd;
      while (readIterator != where) {
         --readIterator;
         --writerIterator;
         *writerIterator = *readIterator;
      }
      
      while (writerIterator != where) {
         --end;
         --writerIterator;
         *writerIterator = *end;
      }
   }
   
   void erase(T *begin, T *end)
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(begin < end);
      PDK_ASSERT(begin >= this->begin() && begin < this->end());
      PDK_ASSERT(end > this->begin() && end < this->end());
      const T *const selfEnd = this->end();
      do {
         *begin = *end;
         ++begin;
         ++end;
      } while (end != selfEnd);
      
      do {
         (--end)->~T();
         --this->m_size;
      } while (end != begin);
   }
};

template <typename T>
struct MovableArrayOperator : GenericArrayOperator<T>
{
   // using GenericArrayOperator<T>::appendInitialize;
   // using GenericArrayOperator<T>::copyAppend;
   // using GenericArrayOperator<T>::truncate;
   // using GenericArrayOperator<T>::destroyAll;
   
   void insert(T *where, const T *begin, const T *end)
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(!this->m_ref.isShared());
      PDK_ASSERT(where >= this->begin() && where < this->end());
      PDK_ASSERT(begin < end);
      PDK_ASSERT(end <= where || begin > this->end());
      PDK_ASSERT(static_cast<size_t>(end - begin) <= this->m_alloc - static_cast<uint>(this->m_size));
      // Provides strong exception safety guarantee,
      // provided T::~T() nothrow
      struct ReversableDisplace
      {
         ReversableDisplace(T *start, T *finish, size_t diff)
            : m_begin(start),
              m_end(finish),
              m_displace(diff)
         {
            std::memmove(static_cast<void *>(m_begin), static_cast<void *>(m_begin + m_displace)
                         (m_end - m_begin) * sizeof(T));
         }
         
         void commit()
         {
            m_displace = 0; 
         }
         
         ~ReversableDisplace()
         {
            if (m_displace) {
               std::memmove(static_cast<void *>(begin), static_cast<void *>(begin + m_displace),
                            (end - begin) * sizeof(T));
            }
         }
         
         T *const m_begin;
         T *const m_end;
         size_t m_displace;
      } displace(where, this->end(), static_cast<size_t>(end - begin));
      
      struct CopyConstructor
      {
         CopyConstructor(T *where) 
            : m_where(where) 
         {}
         
         void copy(const T *src, const T *const srcEnd)
         {
            m_size = 0;
            for (; src != srcEnd; ++src) {
               new (where + m_size) T(*src);
               ++m_size;
            }
            m_size = 0;
         }
         
         ~CopyConstructor()
         {
            while (m_size) {
               where[--m_size].~T();
            }
         }
         
         T *const m_where;
         size_t m_size;
      } copier(where);
      
      copier.copy(begin, end);
      displace.commit();
      this->size += (end - begin);
   }
   
   void erase(T *begin, T *end)
   {
      PDK_ASSERT(this->isMutable());
      PDK_ASSERT(begin < end);
      PDK_ASSERT(begin >= this->begin() && begin < this->end());
      PDK_ASSERT(end > this->begin() && end < this->end());
      struct Mover
      {
         Mover(T *&start, const T *finish, int &size)
            : m_destination(start),
              m_source(start),
              m_count(finish - start),
              m_size(size)
         {
            
         }
         
         ~Mover()
         {
            std::memmove(static_cast<void *>(m_destination), static_cast<const void *>(m_source),
                         m_count * sizeof(T));
            m_size -= (m_source - m_destination);
         }
         
         T *&m_destination;
         const T *const m_source;
         size_t m_count;
         int &m_size;
      } mover(end, this->end(), this->m_size);
      
      do {
         // Exceptions or not, dtor called once per instance
         static_cast<T *>(--end)->~T();
      } while (end != begin);
   }
};

template <typename T, typename = void>
struct ArrayOperatorSelector
{
   using Type = GenericArrayOperator<T>;
};

template <typename T>
struct ArrayOperatorSelector<T, 
      typename std::enable_if<!pdk::TypeInfo<T>::isComplex && !pdk::TypeInfo<T>::isStatic>::type>
{
   using Type = PodArrayOperator<T>;
};

template <typename T>
struct ArrayOperatorSelector<T, 
      typename std::enable_if<pdk::TypeInfo<T>::isComplex && !pdk::TypeInfo<T>::isStatic>::type>
{
   using Type = MovableArrayOperator<T>;
};

template <typename T>
struct ArrayDataOperator : ArrayOperatorSelector<T>::Type
{};

} // internal
} // ds
} // pdk

#endif // PDK_M_BASE_DS_INTERNAL_ARRAYDATA_OPERATOR_H
