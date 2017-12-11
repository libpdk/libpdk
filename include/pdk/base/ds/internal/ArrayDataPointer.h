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

#ifndef PDK_M_BASE_DS_INTERNAL_ARRAYDATA_POINTER_H
#define PDK_M_BASE_DS_INTERNAL_ARRAYDATA_POINTER_H

#include "ArrayDataOperators.h"

namespace pdk {
namespace ds {
namespace internal {

template <typename T>
class ArrayDataPointer
{
private:
   using Data = TypedArrayData<T>;
   using DataOperator = ArrayDataOperator<T>;
   
public:
   ArrayDataPointer() noexcept
      : m_data(Data::getSharedNull())
   {}
   
   ArrayDataPointer(const ArrayDataPointer &other)
      : m_data(other.m_data->m_ref.ref()
               ? other.m_data
               : other.clone(other.m_data->cloneFlags()))
   {}
   
   explicit ArrayDataPointer(TypedArrayData<T> *ptr)
      : m_data(ptr)
   {
      if (!ptr) {
         throw new std::bad_alloc();
      }
   }
   
   ArrayDataPointer(ArrayDataPointerRef<T> ref)
      : m_data(ref.m_ptr)
   {}
   
   ArrayDataPointer &operator=(const ArrayDataPointer &other)
   {
      ArrayDataPointer temp(other);
      this->swap(temp);
      return *this;
   }
   
   ArrayDataPointer(ArrayDataPointer &&other) noexcept
      : m_data(other.m_data)
   {
      other.m_data = Data::getSharedNull();
   }
   
   ArrayDataPointer &operator=(const ArrayDataPointer &&other) noexcept
   {
      ArrayDataPointer moved(std::move(other));
      this->swap(moved);
      return *this;
   }
   
   DataOperator &operator*() const
   {
      PDK_ASSERT(m_data);
      return *static_cast<DataOperator *>(m_data);
   }
   
   DataOperator *operator->() const
   {
      PDK_ASSERT(m_data);
      return static_cast<DataOperator *>(m_data);
   }
   
   bool isNull() const
   {
      return m_data == Data::getSharedNull();
   }
   
   Data *getData() const
   {
      return m_data;
   }
   
   bool needsDetach() const
   {
      return (!m_data->isMutable() || m_data->m_ref.isShared());
   }
   
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
   void setSharable(bool sharable)
   {
      if (needsDetach()) {
         Data *detached = clone(sharable
                                ? m_data->detachFlags() & ~ArrayData::Unsharable
                                : m_data->detachFlags() | ArrayData::Unsharable);
         ArrayDataPointer old(m_data);
         PDK_UNUSED(old);
         m_data = detached;
      } else {
         m_data->m_ref.setSharable(sharable);
      }
   }
#endif
   
   void swap(ArrayDataPointer &other) noexcept
   {
      std::swap(m_data, other.m_data);
   }
   
   void clear()
   {
      ArrayDataPointer temp(m_data);
      m_data = Data::getSharedNull();
      PDK_UNUSED(temp);
   }
   
   bool detach()
   {
      if (needsDetach()) {
         Data *copy = clone(m_data->detachFlags());
         ArrayDataPointer old(m_data);
         PDK_UNUSED(old);
         m_data = copy;
         return true;
      }
      return false;
   }
   
   ~ArrayDataPointer()
   {
      if (!m_data->m_ref.deref()) {
         if (m_data->isMutable()) {
            (*this)->destroyAll();
         }
         Data::deallocate(m_data);
      }
   }
private:
   Data *clone(ArrayData::AllocationOptions options) const PDK_REQUIRED_RESULT
   {
      Data *x = Data::allocate(m_data->detachCapacity(m_data->m_size), options);
      if (!x) {
         throw new std::bad_alloc();
      }
      ArrayDataPointer copy(x);
      if (m_data->m_size) {
         copy->copyAppend(m_data->begin(), m_data->end());
      }
      Data *result = copy.m_data;
      copy.m_data = Data::getSharedNull();
      return result;
   }
   
private:
   Data *m_data;
};

template <typename T>
inline bool operator ==(const ArrayDataPointer<T> &lhs, const ArrayDataPointer<T> &rhs)
{
   return lhs.getData() == rhs.getData();
}

template <typename T>
inline bool operator !=(const ArrayDataPointer<T> &lhs, const ArrayDataPointer<T> &rhs)
{
   return lhs.getData() != rhs.getData();
}

template <typename T>
inline void swap(ArrayDataPointer<T> &left, ArrayDataPointer<T> &right)
{
   left.swap(right);
}

} // internal
} // ds
} // pdk

#endif // PDK_M_BASE_DS_INTERNAL_ARRAYDATA_POINTER_H
