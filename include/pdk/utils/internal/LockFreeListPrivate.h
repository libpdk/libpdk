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
// Created by softboy on 2018/01/05.

#ifndef PDK_UTILS_INTERNAL_LOCKFREELIST_PRIVATE_H
#define PDK_UTILS_INTERNAL_LOCKFREELIST_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/os/thread/Atomic.h"

namespace pdk {
namespace utils {
namespace internal {

using pdk::os::thread::AtomicInt;
using pdk::os::thread::AtomicPointer;

template <typename T>
struct LockFreeListElement
{
   using ConstReferenceType = const T &;
   using ReferenceType = T &;
   
   inline ConstReferenceType getType() const
   {
      return m_type;
   }
   
   inline ReferenceType getType()
   {
      return m_type;
   }
   
   T m_type;
   AtomicInt m_next;   
};

template <>
struct LockFreeListElement<void>
{
   using ConstReferenceType = void;
   using ReferenceType = void;
   
   inline void getType() const
   {}
   
   inline void getType()
   {}
   
   AtomicInt m_next;
};

struct PDK_UNITTEST_EXPORT LockFreeListDefaultConstants
{
   enum {
      InitialNextValue = 0,
      IndexMask = 0x00ffffff,
      SerialMask = ~IndexMask & ~0x80000000,
      SerialCounter = IndexMask + 1,
      MaxIndex = IndexMask,
      BlockCount = 4
   };
   static const int sm_sizes[BlockCount];
};

template <typename T, typename ConstantTypes = LockFreeListDefaultConstants>
class LockFreeList
{
   using ValueType = T;
   using ElementType = LockFreeListElement<T>;
   using ConstReferenceType = typename ElementType::ConstReferenceType;
   using ReferenceType = typename ElementType::ReferenceType;
   
public:
   constexpr inline LockFreeList();
   ~LockFreeList();
   
   inline ConstReferenceType at(int offset) const;
   inline ReferenceType operator[](int offset);
   
   inline int next();
   inline void release(int id);
   
private:
   static inline int blockfor(int &x)
   {
      for (int i = 0; i < ConstantTypes::BlockCount; ++i) {
         int size = ConstantTypes::sm_sizes[i];
         if (x < size) {
            return i;
         }
         x -= size;
      }
      PDK_ASSERT(false);
      return -1;
   }
   
   static inline ElementType *allocate(int offset, int size)
   {
      ElementType *array = new ElementType[size];
      for (int i = 0; i < size; ++i) {
         array[i].m_next.store(offset + i + 1);
      }
      return array;
   }
   
   static inline int incrementSerial(int o, int n)
   {
      return static_cast<int>((static_cast<uint>(n) & ConstantTypes::IndexMask) | 
                              ((static_cast<uint>(o) + ConstantTypes::SerialCounter) & ConstantTypes::SerialMask));
   }
   
   PDK_DISABLE_COPY(LockFreeList);
   
   AtomicPointer<ElementType> m_blocks[ConstantTypes::BlockCount];
   AtomicInt m_next;
   
};

template <typename T, typename ConstantTypes>
constexpr inline LockFreeList<T, ConstantTypes>::LockFreeList()
   : m_next(ConstantTypes::InitialNextValue)
{}

template <typename T, typename ConstantTypes>
inline LockFreeList<T, ConstantTypes>::~LockFreeList()
{
   for (int i = 0; i < ConstantTypes::BlockCount; ++i) {
      delete [] m_blocks[i].load();
   }
}

template <typename T, typename ConstantTypes>
inline typename LockFreeList<T, ConstantTypes>::ConstReferenceType
LockFreeList<T, ConstantTypes>::at(int offset) const
{
   const int bidx = blockfor(offset);
   return (m_blocks[bidx].load())[offset].getType();
}

template <typename T, typename ConstantTypes>
inline typename LockFreeList<T, ConstantTypes>::ReferenceType
LockFreeList<T, ConstantTypes>::operator[](int offset)
{
   const int bidx = blockfor(offset);
   return (m_blocks[bidx].load())[offset].getType();
}

template <typename T, typename ConstantTypes>
inline int LockFreeList<T, ConstantTypes>::next()
{
   int id;
   int newId;
   int offset;
   ElementType *valuePtr;
   do {
      id = m_next.load();
      offset = id & ConstantTypes::IndexMask;
      const int bidx = blockfor(offset);
      valuePtr = m_blocks[bidx].loadAcquire();
      
      if (!valuePtr) {
         valuePtr = allocate((id & ConstantTypes::IndexMask) - offset, ConstantTypes::sm_sizes[bidx]);
         if (!m_blocks[bidx].testAndSetRelease(0, valuePtr)) {
            delete [] valuePtr;
            valuePtr = m_blocks[bidx].loadAcquire();
            PDK_ASSERT(valuePtr != 0);
         }
      }
      
      newId = valuePtr[offset].m_next.load() | (id & ~ConstantTypes::IndexMask);
      
   } while (!m_next.testAndSetRelaxed(id, newId));
   return id & ConstantTypes::IndexMask;
}

template <typename T, typename ConstantTypes>
inline void LockFreeList<T, ConstantTypes>::release(int id)
{
   int offset = id & ConstantTypes::IndexMask;
   const int bidx = blockfor(offset);
   ElementType *array = m_blocks[bidx].load();
   int x;
   int newId;
   do {
      x = m_next.loadAcquire();
      array[offset].m_next.store(x & ConstantTypes::IndexMask);
      newId = incrementSerial(x, id);
   } while (!m_next.testAndSetRelease(x, newId));
}

} // internal
} // utils
} // pdk

#endif // PDK_UTILS_INTERNAL_LOCKFREELIST_PRIVATE_H
