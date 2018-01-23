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
// Created by softboy on 2018/01/19.

// Copyright Thorsten Ottosen, 2009.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef PDK_KERNEL_SIGNAL_INTERNAL_SIGNAL_AUTO_BUFFER_H
#define PDK_KERNEL_SIGNAL_INTERNAL_SIGNAL_AUTO_BUFFER_H

#include "pdk/global/Global.h"
#include "pdk/utils/ScopedObjectGuard.h"
#include "pdk/stdext/typetraits/HasTrivialAssign.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace pdk {
namespace kernel {
namespace signal {
namespace internal {

// Policies for creating the stack buffer.
template <unsigned N>
struct StoreNObjects
{
   static constexpr unsigned VALUE = N; 
};

template <unsigned N>
struct StoreNBytes
{
   static constexpr unsigned VALUE = N;
};

namespace autobufferdetail
{

template <typename Policy, typename T>
struct ComputeBufferSize
{
   static constexpr unsigned VALUE = Policy::VALUE * sizeof(T);
};

template <unsigned N, typename T>
struct ComputeBufferSize<StoreNBytes<N>, T>
{
   static constexpr unsigned VALUE = N;
};

template <typename Policy, typename T>
struct ComputeBufferObjects
{
   static constexpr unsigned VALUE = Policy::VALUE;
};

template <unsigned N, typename T>
struct ComputeBufferObjects<StoreNBytes<N>, T>
{
   static constexpr unsigned VALUE = N / sizeof(T);
};

} // autobufferdetail

struct DefaultGrowPolicy
{
   template<typename SizeType>
   static SizeType newCapacity(SizeType capacity)
   {
      //
      // @remark: we grow the capacity quite agressively.
      //          this is justified since we aim to minimize
      //          heap-allocations, and because we mostly use
      //          the buffer locally.
      return capacity * 4u;
   }
   
   template<typename SizeType>
   static bool shouldShrink(SizeType, SizeType)
   {
      //
      // @remark: when defining a new grow policy, one might
      //          choose that if the waated space is less
      //          than a certain percentage, then it is of
      //          little use to shrink.
      //
      return true;
   }
};

template <typename T,
          typename StackBufferPolicy = StoreNBytes<256>,
          typename GrowPolicy = DefaultGrowPolicy,
          typename Allocator = std::allocator<T>>
class AutoBuffer;

template <typename T,
          typename StackBufferPolicy,
          typename GrowPolicy,
          typename Allocator
          >
class AutoBuffer : Allocator
{
private:
   enum {
      N = autobufferdetail::ComputeBufferObjects<StackBufferPolicy, T>::VALUE
   };
   
   static constexpr bool IS_STACK_BUFFER_EMPTY = N == 0u;
   using LocalBuffer = AutoBuffer<T, StoreNObjects<0>, GrowPolicy, Allocator>;
   
public:
   using AllocatorType = Allocator;
   using ValueType = T;
   using SizeType = typename Allocator::size_type;
   using DifferenceType = typename Allocator::difference_type;
   using Pointer = T *;
   using ConstPointer = const T *;
   using AllocatorPointer = typename Allocator::pointer;
   using Reference = T &;
   using ConstReference = const T &;
   using Iterator = Pointer;
   using ConstIterator = ConstPointer;
   using ReverseIterator = std::reverse_iterator<Iterator>;
   using ConstReverseIterator = std::reverse_iterator<ConstIterator>;
   
   using allocator_type = AllocatorType;
   using value_type = ValueType;
   using size_type = SizeType;
   using difference_type = DifferenceType;
   using pointer = Pointer;
   using const_pointer = ConstPointer;
   using allocator_pointer = AllocatorPointer;
   using iterator = Iterator;
   using const_iterator = ConstIterator;
   using reverse_iterator = ReverseIterator;
   using const_reverse_iterator = ConstReverseIterator;
   
   using OptimizedConstReference = typename std::conditional<
   pdk::stdext::HasTrivialAssign<T>::value && sizeof(T) <= sizeof(long double),
   const ValueType,
   ConstReference>::type;
   

   
public:
   AutoBuffer()
      : m_members(N),
        m_buffer(static_cast<T *>(m_members.getAddress())),
        m_size(0u)
   {
      PDK_ASSERT(isValid());
   }
   
   AutoBuffer(const AutoBuffer &other)
      : m_members(std::max(other.m_size, static_cast<SizeType>(N))),
        m_buffer(allocate(m_members.m_capacity)),
        m_size(0)
   {
      copyImpl(other.begin(), other.end(), m_buffer);
      m_size = other.m_size;
      PDK_ASSERT(isValid());
   }
   
   explicit AutoBuffer(SizeType capacity)
      : m_members(std::max(capacity, SizeType(N))),
        m_buffer(allocate(m_members.m_capacity)),
        m_size(0)
   {
      PDK_ASSERT(isValid());
   }
   
   AutoBuffer(SizeType size, OptimizedConstReference initValue)
      : m_members(std::max(size, static_cast<SizeType>(N))),
        m_buffer(allocate((m_members.m_capacity))),
        m_size(0)
   {
      std::uninitialized_fill(m_buffer, m_buffer + size, initValue);
      m_size = size;
      PDK_ASSERT(isValid());
   }
   
   AutoBuffer(SizeType capacity, const AllocatorType *alloc)
      : AllocatorType(alloc),
        m_members(std::max(capacity, SizeType(N))),
        m_buffer(allocate(m_members.m_capacity)),
        m_size(0)
   {
      PDK_ASSERT(isValid());
   }
   
   AutoBuffer(SizeType size, OptimizedConstReference initValue,
              const AllocatorType &alloc)
      : AllocatorType(alloc),
        m_members(std::max(size, SizeType(N))),
        m_buffer(allocate(m_members.m_capacity)),
        m_size(0)
   {
      std::uninitialized_fill(m_buffer, m_buffer + size, initValue);
      m_size = size;
      PDK_ASSERT(isValid());
   }
   
   template <typename ForwardIterator>
   AutoBuffer(ForwardIterator begin, ForwardIterator end)
      : m_members(std::distance(begin, end)),
        m_buffer(allocate((m_members.m_capacity))),
        m_size(0)
   {
      copyImpl(begin, end, m_buffer);
      m_size = m_members.m_capacity;
      if (m_members.m_capacity < N) {
         m_members.m_capacity = N;
      }
      PDK_ASSERT(isValid());
   }
   
   template <typename ForwardIterator>
   AutoBuffer(ForwardIterator begin, ForwardIterator end,
              const AllocatorType &alloc)
      : AllocatorType(alloc),
        m_members(std::distance(begin, end)),
        m_buffer(allocate((m_members.m_capacity))),
        m_size(0)
   {
      copyImpl(begin, end, m_buffer);
      m_size = m_members.m_capacity;
      if (m_members.m_capacity < N) {
         m_members.m_capacity = N;
      }
      PDK_ASSERT(isValid());
   }
   
   AutoBuffer &operator =(const AutoBuffer &other) // basic
   {
      if (this == &other) {
         return *this;
      }
      DifferenceType diff = m_size - other.m_size;
      if (diff >= 0) {
         popBackN(static_cast<SizeType>(diff));
         assignImpl(other.begin(), other.end(), begin());
      } else {
         if (m_members.m_capacity >= other.size()) {
            
         } else {
            // @remark: we release memory as early as possible
            // since we only give the basic guarantee
            autoBufferDestroy();
            m_buffer = nullptr;
            Pointer newBuffer = allocate(other.size());
            pdk::utils::ScopedGuard guard =
                  pdk::utils::make_object_guard(*this,
                                                &AutoBuffer::deallocate,
                                                newBuffer,
                                                other.size());
            copyImpl(other.begin(), other.end(), newBuffer);
            guard.dismiss();
            m_buffer = newBuffer;
            m_members.m_capacity = other.size();
            m_size = m_members.m_capacity;
         }
      }
      PDK_ASSERT(size() == other.size());
      PDK_ASSERT(isValid());
      return *this;
   }
   
   ~AutoBuffer()
   {
      autoBufferDestroy();
   }
   
public:
   bool isValid() const // invariant
   {
      // @remark: allowed for N==0 and when
      // using a locally instance
      // in insert()/oneSidedSwap()
      if (m_buffer == nullptr) {
         return true;
      }
      if (m_members.m_capacity < N) {
         return false;
      }
      
      if (!isOnStack() && m_members.m_capacity <= N) {
         return false;
      }
      if (m_buffer == m_members.getAddress()) {
         if (m_members.m_capacity > N) {
            return false;
         }
      }
      if (m_size > m_members.m_capacity) {
         return false;
      }
      return true;
   }
   
   bool empty() const
   {
      return m_size == 0;
   }
   
   bool full() const
   {
      return m_size == m_members.m_capacity;
   }
   
   bool isOnStack() const
   {
      return m_members.m_capacity <= N;
   }
   
   SizeType size() const
   {
      return m_size;
   }
   
   SizeType capacity() const
   {
      return m_members.m_capacity;
   }
   
   Pointer getData()
   {
      return m_buffer;
   }
   
   ConstPointer getData() const
   {
      return m_buffer;
   }
   
   AllocatorType &getAllocator()
   {
      return static_cast<AllocatorType &>(*this);
   }
   
   const AllocatorType &getAllocator() const
   {
      return static_cast<const AllocatorType &>(*this);   
   }
   
public:
   Iterator begin()
   {
      return m_buffer;
   }
   
   ConstIterator begin() const
   {
      return m_buffer;
   }
   
   Iterator end()
   {
      return m_buffer + m_size;
   }
   
   ConstIterator end() const
   {
      return m_buffer + m_size;
   }
   
   ReverseIterator rbegin()
   {
      return ReverseIterator(end());
   }
   
   ConstReverseIterator rbegin() const
   {
      return ConstReverseIterator(end());
   }
   
   ReverseIterator rend()
   {
      return ReverseIterator(begin());
   }
   
   ConstReverseIterator rend() const
   {
      return ConstReverseIterator(begin());
   }
   
   ConstIterator cbegin() const
   {
      return const_cast<const AutoBuffer *>(this)->begin();
   }
   
   ConstIterator cend() const
   {
      return const_cast<const AutoBuffer *>(this)->end();
   }
   
   ConstReverseIterator crbegin() const
   {
      return const_cast<const AutoBuffer *>(this)->rbegin();
   }
   
   ConstReverseIterator crend() const
   {
      return const_cast<const AutoBuffer *>(this)->rend();
   }
   
public:
   Reference front()
   {
      return m_buffer[0];
   }
   
   OptimizedConstReference front() const
   {
      return m_buffer[0];
   }
   
   Reference back()
   {
      return m_buffer[m_size - 1];
   }
   
   OptimizedConstReference back() const
   {
      return m_buffer[m_size - 1];   
   }
   
   Reference operator [](SizeType n)
   {
      PDK_ASSERT(n < m_size);
      return m_buffer[n];
   }
   
   OptimizedConstReference operator [](SizeType n) const
   {
      PDK_ASSERT(n < m_size);
      return m_buffer[n];
   }
   
   void uncheckedPushBack()
   {
      PDK_ASSERT(!full());
      new (m_buffer + m_size) T;
      ++m_size;
   }
   
   void uncheckedPushBack(OptimizedConstReference value)
   {
      PDK_ASSERT(!full());
      new (m_buffer + m_size) T(value);
      ++m_size;
   }
   
   template <typename ForwardIterator>
   void uncheckedPushBack(ForwardIterator begin, ForwardIterator end) // non-growing
   {
      PDK_ASSERT(m_size + std::distance(begin, end) <= m_members.m_capacity);
      copyImpl(begin, end, m_buffer + m_size);
      m_size += std::distance(begin, end);
   }
   
   void uncheckedPushBackN(SizeType n)
   {
      PDK_ASSERT(m_size + n <= m_members.m_capacity);
      uncheckedPushBackN(n, pdk::stdext::HasTrivialAssign<T>());
   }
   
   void reservePrecisely(SizeType n)
   {
      PDK_ASSERT(m_members.m_capacity >= N);
      if (n <= m_members.m_capacity) {
         return;
      }
      reserveImpl(n);
      PDK_ASSERT(m_members.m_capacity == n);
   }
   
   void reserve(SizeType n) // strong
   {
      PDK_ASSERT(m_members.m_capacity >= N);
      if (n <= m_members.m_capacity) {
         return;
      }
      reserveImpl(newCapacityImpl(n));
      PDK_ASSERT(m_members.m_capacity >= n);
   }
   
   void pushBack()
   {
      if (m_size != m_members.m_capacity) {
         uncheckedPushBack();
      } else {
         reserve(m_size + 1u);
         uncheckedPushBack();
      }
   }
   
   void push_back()
   {
      pushBack();
   }
   
   void pushBack(OptimizedConstReference value)
   {
      if (m_size != m_members.m_capacity) {
         uncheckedPushBack(value);
      } else {
         reserve(m_size + 1u);
         uncheckedPushBack(value);
      }
   }
   
   void push_back(OptimizedConstReference value)
   {
      pushBack(value);
   }
   
   template<typename ForwardIterator>
   void pushBack(ForwardIterator begin, ForwardIterator end)
   {
      DifferenceType diff = std::distance(begin, end);
      if (m_size + diff > m_members.m_capacity) {
         reserve(m_size + diff);
      }
      uncheckedPushBack(begin, end);
   }
   
   template<typename ForwardIterator>
   void push_back(ForwardIterator begin, ForwardIterator end)
   {
      pushBack(begin, end);
   }
   
   Iterator insert(ConstIterator before, OptimizedConstReference value) // basic
   {
      // @todo: consider if we want to support value in 'this'
      if (m_size < m_members.m_capacity) {
         bool isBackInsertion = before == cend();
         Iterator where = const_cast<T *>(before);
         if (!isBackInsertion) {
            growBackOne();
            std::copy(before, cend() - 1u, where + 1u);
            *where = value;
            PDK_ASSERT(isValid());
         } else {
            uncheckedPushBack(value);
         }
         return where;
      }
      AutoBuffer temp(newCapacityImpl(m_size + 1u));
      temp.uncheckedPushBack(cbegin(), before);
      Iterator result = temp.end();
      temp.uncheckedPushBack(value);
      temp.uncheckedPushBack(before, cend());
      oneSidedSwap(temp);
      PDK_ASSERT(isValid());
      return result;
   }
   
   void insert(ConstIterator before, SizeType n, OptimizedConstReference value)
   {
      // @todo: see problems above
      if (m_size + n <= m_members.m_capacity) {
         growBack(n);
         Iterator where = const_cast<T *>(before);
         std::copy(before, cend() - n, where + n);
         std::fill(where, where + n, value);
         PDK_ASSERT(isValid());
         return;
      }
      AutoBuffer temp(newCapacityImpl(m_size + n));
      temp.uncheckedPushBack(cbegin(), before);
      std::uninitialized_fill_n(temp.end(), n, value);
      temp.m_size += n;
      temp.uncheckedPushBack(before, cend());
      oneSidedSwap(temp);
      PDK_ASSERT(isValid());
   }
   
   template<typename ForwardIterator>
   void insert(ConstIterator before, ForwardIterator begin, ForwardIterator end) // basic
   {
      using category = typename std::iterator_traits<ForwardIterator>::iterator_category;
      insertImpl(before, begin, end, category());
   }
   
   void popBack()
   {
      PDK_ASSERT(!empty());
      autoBufferDestroy(m_buffer + m_size - 1, std::is_trivially_destructible<T>());
      --m_size;
   }
   
   void pop_back()
   {
      popBack();
   }
   
   void popBackN(SizeType n)
   {
      PDK_ASSERT(n <= m_size);
      if (n) {
         destroyBackN(n);
         m_size -= n;
      }
   }
   
   void clear()
   {
      popBackN(m_size);
   }
   
   Iterator erase(ConstIterator where)
   {
      PDK_ASSERT(!empty());
      PDK_ASSERT(cbegin() <= where);
      PDK_ASSERT(cend() > where);
      unsigned elements = cend() - where - 1u;
      if (elements > 0u) {
         ConstIterator start = where + 1u;
         std::copy(start, start + elements, const_cast<T *>(where));
      }
      popBack();
      PDK_ASSERT(!full());
      Iterator result = const_cast<T *>(where);
      PDK_ASSERT(result <= end());
      return result;
   }
   
   Iterator erase(ConstIterator from, ConstIterator to)
   {
      PDK_ASSERT(!(std::distance(from, to) > 0) ||
                 !empty());
      PDK_ASSERT(cbegin() <= from);
      PDK_ASSERT(cend() >= to);
      unsigned elements = std::distance(to, cend());
      if (elements > 0u) {
         PDK_ASSERT(elements > 0u);
         std::copy(to, to + elements, const_cast<T *>(from));
      }
      popBackN(std::distance(from, to));
      PDK_ASSERT(!full());
      Iterator result = const_cast<T *>(from);
      PDK_ASSERT(result <= end());
      return result;
   }
   
   void shrinkToFit()
   {
      if (isOnStack() || !GrowPolicy::shouldShrink(m_size, m_members.m_capacity)) {
         return;
      }
      reserveImpl(m_size);
      m_members.m_capacity = std::max(static_cast<SizeType>(N), m_members.m_capacity);
      PDK_ASSERT(isOnStack() || m_size == m_members.m_capacity);
      PDK_ASSERT(!isOnStack() || m_size <= m_members.m_capacity);
   }
   
   Pointer uninitializeGrow(SizeType n) // strong
   {
      if (m_size + n > m_members.m_capacity) {
         reserve(m_size + n);
      }
      Pointer result = end();
      m_size += n;
      return result;
   }
   
   void uninitializeShrink(SizeType n) // nothrow
   {
      // @remark: test for wrap-around
      PDK_ASSERT(m_size - n <= m_members.m_capacity);
      m_size -= n;
   }
   
   void uninitializeResize(SizeType n)
   {
      if (n > size()) {
         uninitializeGrow(n - size());
      } else if (n < size()){
         uninitializeShrink(size() - n);
      }
      PDK_ASSERT(size() == n);
   }
   
   // nothrow  - if both buffer are on the heap, or
   //          - if one buffer is on the heap and one has
   //            'hasAllocatedBuffer() == false', or
   //          - if copy-construction cannot throw
   // basic    - otherwise (better guarantee impossible)
   // requirement: the allocator must be no-throw-swappable
   void swap(AutoBuffer &other)
   {
      bool onStack = isOnStack();
      bool otherOnStack = other.isOnStack();
      bool bothOnHeap = !onStack && !other.otherOnStack;
      if (bothOnHeap) {
         std::swap(getAllocator(), other.getAllocator());
         std::swap(m_members.m_capacity, other.m_members.m_capacity);
         std::swap(m_buffer, other.m_buffer);
         std::swap(m_size, other.m_size);
         PDK_ASSERT(isValid());
         PDK_ASSERT(other.isValid());
         return;
      }
      PDK_ASSERT(onStack || otherOnStack);
      bool exactlyOneOnStack = (onStack && !otherOnStack) ||
            (!onStack && otherOnStack);
      //
      // Remark: we now know that we can copy into
      //         the unused stack buffer.
      //
      if (exactlyOneOnStack) {
         AutoBuffer *oneOnStack = onStack ? this : &other;
         AutoBuffer *other = onStack ? &other : this;
         Pointer newBuffer = static_cast<T *>(other->m_members.getAddress());
         copyImpl(oneOnStack->begin(), oneOnStack->end(), newBuffer); // strong
         oneOnStack->autoBufferDestroy();
         std::swap(getAllocator(), other->getAllocator());
         std::swap(m_members.m_capacity, other->m_members.m_capacity);
         std::swap(m_size, other->m_size);
         oneOnStack->m_buffer = other->m_buffer;
         other->m_buffer = newBuffer;
         PDK_ASSERT(other->isOnStack());
         PDK_ASSERT(!oneOnStack->isOnStack());
         PDK_ASSERT(isValid());
         PDK_ASSERT(other->isValid());
         return;
      }
      PDK_ASSERT(onStack && otherOnStack);
      swapHelper(*this, other, pdk::stdext::HasTrivialAssign<T>());
      PDK_ASSERT(isValid());
      PDK_ASSERT(other.isValid());
   }
   
private:
   Pointer allocate(SizeType capacity)
   {
      if (capacity > N) {
         return &*getAllocator().allocate(capacity);
      } else {
         return static_cast<T *>(m_members.getAddress());
      }
   }
   
   void deallocate(Pointer where, SizeType capacity)
   {
      if (capacity <= N) {
         return;
      }
      getAllocator().deallocate(AllocatorPointer(where), capacity);
   }
   
   template <typename IterType>
   static void copyImpl(IterType begin, IterType end, Pointer where, std::random_access_iterator_tag)
   {
      copyRai(begin, end, where, std::integral_constant<bool, pdk::stdext::HasTrivialAssign<T>::value>());
   }
   
   static void copyRai(const T* begin, const T* end, Pointer where, const std::true_type&)
   {
      std::memcpy(where, begin, sizeof(T) * std::distance(begin,end));
   }
   
   template <typename IterType, bool b>
   static void copyRai(IterType begin, IterType end, Pointer where, 
                       const std::integral_constant<bool, b> &)
   {
      std::uninitialized_copy(begin, end, where);
   }
   
   template <typename IterType>
   static void copyImpl(IterType begin, IterType end, Pointer where, std::bidirectional_iterator_tag)
   {
      std::uninitialized_copy(begin, end, where);
   }
   
   template <typename IterType>
   static void copyImpl(IterType begin, IterType end, Pointer where)
   {
      copyImpl(begin, end, where, typename std::iterator_traits<IterType>::iterator_category());
   }
   
   template <typename IterType, typename IterType2>
   static void assignImpl(IterType begin, IterType end, IterType2 where)
   {
      assignImpl(begin, end, where, pdk::stdext::HasTrivialAssign<T>());
   }
   
   template <typename IterType, typename IterType2>
   static void assignImpl(IterType begin, IterType end, IterType2 where, const std::true_type &)
   {
      std::memcpy(where, begin, sizeof(T) * std::distance(begin, end));
   }
   
   template <typename IterType, typename IterType2>
   static void assignImpl(IterType begin, IterType end, IterType2 where, const std::false_type &)
   {
      for (; begin != end; ++begin, ++where) {
         *where = *begin;
      }
   }
   
   void uncheckedPushBackN(SizeType n, const std::true_type &)
   {
      std::uninitialized_fill(end(), end() + n, T());
      m_size += n;
   }
   
   void uncheckedPushBackN(SizeType n, const std::false_type &)
   {
      for (SizeType i = 0u; i < n; ++i) {
         uncheckedPushBack();
      }
   }
   
   void autoBufferDestroy(Pointer where, const std::false_type &)
   {
      (*where).~T();
   }
   
   void autoBufferDestroy(Pointer where, const std::true_type &)
   {}
   
   void autoBufferDestroy(Pointer where)
   {
      autoBufferDestroy(where, std::is_trivially_destructible<T>());
   }
   
   void autoBufferDestroy()
   {
      PDK_ASSERT(isValid());
      if (m_buffer) {
         // do we need this check? Yes, but only
         // for N = 0u + local instances in oneSidedSwap()
         autoBufferDestroy(std::is_trivially_destructible<T>());
      }
   }
   
   void autoBufferDestroy(const std::false_type &selector)
   {
      if (m_size) {
         destroyBackN(m_size, selector);
      }
      deallocate(m_buffer, m_members.m_capacity);
   }
   
   void autoBufferDestroy(const std::true_type &)
   {
      deallocate(m_buffer, m_members.m_capacity);
   }
   
   void destroyBackN(SizeType n, const std::false_type &)
   {
      PDK_ASSERT(n > 0);
      Pointer buffer = m_buffer + m_size - 1u;
      Pointer newEnd = buffer - n;
      for (; buffer > newEnd; --buffer) {
         autoBufferDestroy(buffer);
      }
   }
   
   void destroyBackN(SizeType, const std::true_type &)
   {}
   
   void destroyBackN(SizeType n)
   {
      destroyBackN(n, std::is_trivially_destructible<T>());
   }
   
   Pointer moveToNewBuffer(SizeType newCapacity, const std::false_type &)
   {
      Pointer newBuffer = allocate(newCapacity); // strong
      pdk::utils::ScopedGuard guard =
            pdk::utils::make_object_guard(*this, &AutoBuffer::deallocate, newBuffer, newCapacity);
      copyImpl(begin(), end(), newBuffer);// strong
      guard.dismiss();// nothrow
      return newBuffer;
   }
   
   Pointer moveToNewBuffer(SizeType newCapacity, const std::true_type &)
   {
      Pointer newBuffer = allocate(newCapacity); // strong
      copyImpl(begin(), end(), newBuffer); // nothrow
      return newBuffer;
   }
   
   void reserveImpl(SizeType newCapacity)
   {
      Pointer newBuffer = moveToNewBuffer(newCapacity, std::is_nothrow_copy_constructible<T>());
      autoBufferDestroy();
      m_buffer = newBuffer;
      m_members.m_capacity = newCapacity;
      PDK_ASSERT(m_size <= m_members.m_capacity);
   }
   
   SizeType newCapacityImpl(SizeType n)
   {
      PDK_ASSERT(n > m_members.m_capacity);
      SizeType newCapacity = GrowPolicy::newCapacity(m_members.m_capacity);
      // @todo: consider to check for allocator.max_size()
      return std::max(newCapacity, n);
   }
   
   static void swapHelper(AutoBuffer &lhs, AutoBuffer &rhs, const std::true_type &)
   {
      PDK_ASSERT(lhs.isOnStack() && rhs.isOnStack());
      AutoBuffer temp(lhs.begin(), lhs.end());
      assignImpl(rhs.begin(), rhs.end(), lhs.begin());
      assignImpl(temp.begin(), temp.end(), rhs.begin());
      std::swap(lhs.m_size, rhs.m_size);
      std::swap(lhs.m_members.m_capacity, rhs.m_members.m_capacity);
   }
   
   static void swapHelper(AutoBuffer &lhs, AutoBuffer &rhs, const std::false_type &)
   {
      PDK_ASSERT(lhs.isOnStack() && rhs.isOnStack());
      SizeType minSize = std::min(lhs.m_size, rhs.m_size);
      SizeType maxSize = std::max(lhs.m_size, rhs.m_size);
      SizeType diff = maxSize - minSize;
      AutoBuffer *smallest = lhs.m_size == minSize ? &lhs : &rhs;
      AutoBuffer *largest = smallest == &lhs ? &rhs : &lhs;
      
      // @remark: the implementation below is not as fast
      //          as it could be if we assumed T had a default
      //          constructor.
      SizeType i = 0u;
      for (; i < minSize; ++i) {
         std::swap((*smallest)[i], (*largest)[i]);
      }
      for (; i < maxSize; ++i) {
         smallest->uncheckedPushBack((*largest)[i]);
      }
      largest->popBackN(diff);
      std::swap(lhs.m_members.m_capacity, rhs.m_members.m_capacity);
   }
   
   void oneSidedSwap(AutoBuffer &temp) // nothrow
   {
      PDK_ASSERT(!temp.isOnStack());
      autoBufferDestroy();
      // @remark: must be nothrow
      getAllocator() = temp.getAllocator();
      m_members.m_capacity = temp.m_members.m_capacity;
      m_buffer = temp.m_buffer;
      PDK_ASSERT(temp.m_size >= m_size + 1u);
      m_size = temp.m_size;
      temp.m_buffer = 0;
      PDK_ASSERT(temp.isValid());
   }
   
   template <typename Iter>
   void insertImpl(ConstIterator before, Iter begin, Iter end, std::input_iterator_tag)
   {
      for (; begin != end; ++begin) {
         before = insert(before, *begin);
         ++before;
      }
   }
   
   void growBack(SizeType n, const std::true_type &)
   {
      PDK_ASSERT(m_size + n <= m_members.m_capacity);
      m_size += n;
   }
   
   void growBack(SizeType n, const std::false_type &)
   {
      uncheckedPushBackN(n);
   }
   
   void growBack(SizeType n)
   {
      growBack(n, std::is_trivially_constructible<T>());
   }
   
   void growBackOne(const std::true_type &)
   {
      PDK_ASSERT(m_size + 1 <= m_members.m_capacity);
      m_size += 1;
   }
   
   void growBackOne(const std::false_type &)
   {
      uncheckedPushBack();
   }
   
   void growBackOne()
   {
      growBackOne(std::is_trivially_constructible<T>());
   }
   
   template <typename Iter>
   void insertImpl(ConstIterator before, Iter begin, Iter end, std::forward_iterator_tag)
   {
      DifferenceType n = std::distance(begin, end);
      if (m_size + n <= m_members.m_capacity) {
         bool isBackInsertion = before == cend();
         if (!isBackInsertion) {
            growBack(n);
            Iterator where = const_cast<T *>(before);
            std::copy(before, cend() - n, where + n);
            assignImpl(begin, end, where);
         } else {
            uncheckedPushBack(begin, end);
         }
         PDK_ASSERT(isValid());
         return;
      }
      AutoBuffer temp(newCapacityImpl(m_size + n));
      temp.uncheckedPushBack(cbegin(), before);
      temp.uncheckedPushBack(begin, end);
      temp.uncheckedPushBack(before, cend());
      oneSidedSwap(temp);
      PDK_ASSERT(isValid());
   }
   
private:
   using StorageType = typename std::aligned_storage<N * sizeof(T), alignof(T)>::type;
   struct MembersType : StorageType
   {
      SizeType m_capacity;
      MembersType(SizeType capacity)
         : m_capacity(capacity)
      {}
      
      void *getAddress() const
      {
         return &const_cast<StorageType &>(static_cast<const StorageType &>(*this));
      }
   };
   
   MembersType m_members;
   Pointer m_buffer;
   SizeType m_size;
};

template <typename T, typename SBP, typename GP, typename A>
inline void swap(AutoBuffer<T, SBP, GP, A> &lhs, AutoBuffer<T, SBP, GP, A> &rhs)
{
   lhs.swap(rhs);
}

template <typename T, typename SBP, typename GP, typename A>
inline bool operator==(const AutoBuffer<T, SBP, GP, A> &lhs,
                       const AutoBuffer<T, SBP, GP, A> &rhs)
{
   if (lhs.size() != rhs.size()) {
      return false;
   }
   return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T, typename SBP, typename GP, typename A>
inline bool operator!=(const AutoBuffer<T, SBP, GP, A> &lhs,
                       const AutoBuffer<T, SBP, GP, A> &rhs)
{
   return !(lhs == rhs);
}

template <typename T, typename SBP, typename GP, typename A>
inline bool operator<(const AutoBuffer<T, SBP, GP, A> &lhs,
                      const AutoBuffer<T, SBP, GP, A> &rhs)
{
   return std::lexicographical_compare(lhs.begin(), lhs.end(),
                                       rhs.begin(), rhs.end());
}

template <typename T, typename SBP, typename GP, typename A>
inline bool operator>(const AutoBuffer<T, SBP, GP, A> &lhs,
                      const AutoBuffer<T, SBP, GP, A> &rhs)
{
   return rhs < lhs;
}

template <typename T, typename SBP, typename GP, typename A>
inline bool operator<=(const AutoBuffer<T, SBP, GP, A> &lhs,
                       const AutoBuffer<T, SBP, GP, A> &rhs)
{
   return !(lhs > rhs);
}

template <typename T, typename SBP, typename GP, typename A>
inline bool operator>=(const AutoBuffer<T, SBP, GP, A> &lhs,
                       const AutoBuffer<T, SBP, GP, A> &rhs)
{
   return !(lhs < rhs);
}

} // internal
} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_INTERNAL_SIGNAL_AUTO_BUFFER_H
