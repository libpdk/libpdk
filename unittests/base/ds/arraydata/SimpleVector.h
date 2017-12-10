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
// Created by zzu_softboy on 2017/12/08.

#ifndef PDK_BASE_DS_ARRAY_DATA_TEST_SIMPLE_VECTOR_H
#define PDK_BASE_DS_ARRAY_DATA_TEST_SIMPLE_VECTOR_H

#include "pdk/base/ds/internal/ArrayData.h"
#include "pdk/base/ds/internal/ArrayDataPointer.h"

using pdk::ds::internal::TypedArrayData;
using pdk::ds::internal::ArrayDataPointer;
using pdk::ds::internal::ArrayDataPointerRef;

template <typename T>
class SimpleVector
{
private:
   using Data = TypedArrayData<T>;
   using Iterator = typename Data::Iterator;
   using ConstIterator = typename Data::ConstIterator;
   
public:
   SimpleVector()
   {}
   
   explicit SimpleVector(size_t size)
      : m_data(Data::allocate(size))
   {
      if (size) {
         m_data->appendInitialize(size);
      }
   }
   
   SimpleVector(size_t size, const T &data)
      : m_data(Data::allocate(size))
   {
      if (size) {
         m_data->copyAppend(size, data);
      }
   }
   
   SimpleVector(const T *begin, const T *end)
      : m_data(Data::allocate(end - begin))
   {
      if (end - begin) {
         m_data->copyAppend(begin, end);
      }
   }
   
   SimpleVector(ArrayDataPointerRef<T> ptr)
      : m_data(ptr)
   {}
   
   SimpleVector(Data *ptr)
      : m_data(ptr)
   {}
   
   bool empty() const
   {
      return m_data->m_size == 0;
   }
   
   bool isNull() const
   {
      return m_data.isNull();
   }
   
   bool isEmpty()
   {
      return this->empty();
   }
   
   bool isStatic() const 
   {
      return m_data->m_ref.isStatic();
   }
   
   bool isShared() const
   {
      return m_data->m_ref.isShared();
   }
   
   bool isSharedWith(const SimpleVector &other) const
   {
      return m_data == other.m_data;
   }
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
   bool isSharable() const
   {
      return m_data->m_ref.isSharable();
   }
   
   void setSharable(bool sharable)
   {
      m_data.setSharable(sharable);
   }
#endif
   size_t size() const
   {
      return m_data->m_size;
   }
   
   size_t capacity() const
   {
      return m_data->m_alloc;
   }
   
   Iterator begin() 
   {
      detach();
      return m_data->begin();
   }
   
   Iterator end()
   {
      detach();
      return m_data->end();
   }
   
   ConstIterator constBegin() const
   {
      return begin();
   }
   
   ConstIterator constEnd() const
   {
      return end();
   }
   
   ConstIterator begin() const
   {
      return m_data->constBegin();
   }
   
   ConstIterator end() const
   {
      return m_data->constEnd();
   }
   
   T &operator[](size_t i) 
   {
      PDK_ASSERT(i < static_cast<size_t>(m_data->m_size));
      detach();
      return begin()[i];
   }
   
   T &at(size_t i) 
   { 
      PDK_ASSERT(i < static_cast<size_t>(m_data->m_size)); 
      detach(); 
      return begin()[i]; 
   }
   
   const T &operator[](size_t i) const
   {
      PDK_ASSERT(i < static_cast<size_t>(m_data->m_size));
      detach();
      return begin()[i];
   }
   
   const T &at(size_t i) const
   { 
      PDK_ASSERT(i < static_cast<size_t>(m_data->m_size)); 
      detach(); 
      return begin()[i]; 
   }
   
   void clear()
   {
      m_data.clear();
   }
   
   void detach()
   {
      m_data.detach();
   }
   
private:
   ArrayDataPointer<T> m_data;
};

#endif
