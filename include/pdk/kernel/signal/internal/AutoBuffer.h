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
   static const unsigned VALUE = N; 
};

template <unsigned N>
struct StoreNBytes
{
   static const unsigned VALUE = N;
};

namespace autobufferdetail
{

template <typename Policy, typename T>
struct ComputeBufferSize
{
   static const unsigned VALUE = Policy::VALUE * sizeof(T);
};

template <unsigned N, typename T>
struct ComputeBufferSize<StoreNBytes<N>, T>
{
   static const unsigned VALUE = N;
};

template <typename Policy, typename T>
struct ComputeBufferObjects
{
   static const unsigned VALUE = Policy::VALUE;
};

template <typename Policy, typename T>
struct ComputeBufferObjects<StoreNBytes<N>, T>
{
   static const unsigned VALUE = N / sizeof(T);
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
      N = autobufferdetail::ComputeBufferObjects<StackBufferPolicy, T>::sm_value
   };
   
   static const bool IS_STACK_BUFFER_EMPTY = N == 0u;
   using LocalBuffer = AutoBuffer<T, StoreNObjects<0>, GrowPolicy, Allocator>;
   
public:
   using AllocatorType = Allocator;
   using ValueType = T;
   using SizeType = typename Allocator::size_type;
   using DifferenceType = typename Allocator::difference_type;
   using Pointer = T *;
   using ConstPointer = const T *;
   using AllocatorPointer = typename Allocator::pointer;
   using ConstPointer = const T *;
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
   std::is_trivially_assignable<T>::value && sizeof(T) <= sizeof(long double),
   const ValueType,
   ConstReference>::type;
   
private:
   Pointer allocate(SizeType capacityArg)
   {
      if (capacityArg > N) {
         return &*getAllocator().allocate(capacityArg);
      } else {
         return static_cast<T *>(members.getAddress());
      }
   }
   
   void deallocate(Pointer where, SizeType capacityArg)
   {
      if (capacityArg <= N) {
         return;
      }
      getAllocator().deallocate(allocatorPointer(where), capacityArg);
   }
   
   template <typename IterType>
   static void copyImpl(IterType begin, IterType end, Pointer where, std::random_access_iterator_tag)
   {
      copyRai(begin, end, where, std::is_trivially_assignable<T>::value);
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
      copyImpl(begin, end, typename std::iterator_traits<IterType>::iterator_category());
   }
   
   template <typename IterType, typename IterType2>
   static void assignImpl(IterType begin, IterType end, IterType2 where)
   {
      assignImpl(begin, end, where, std::is_trivially_assignable<T>());
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
   
   void uncheckPushBackN(SizeType n, const std::true_type &)
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
       Pointer new_buffer = allocate(newCapacity); // strong
       
   }
};

} // internal
} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_INTERNAL_SIGNAL_AUTO_BUFFER_H
