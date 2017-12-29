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
// Created by softboy on 2017/12/29.

#ifndef PDK_UTILS_SCOPED_POINTER_H
#define PDK_UTILS_SCOPED_POINTER_H

#include "pdk/global/Global.h"
#include <cstdlib>

namespace pdk {

// forward class with namespace
namespace kernel {

class Object;

} // kernel

namespace utils {

template <typename T>
struct ScopedPointerDeleter
{
   static inline void cleanup(T *pointer)
   {
      // Enforce a complete type.
      // If you get a compile error here, read the section on forward declared
      // classes in the ScopedPointer documentation.
      using IsIncompleteType = char[sizeof(T) ? 1: -1];
      (void) sizeof(IsIncompleteType);
      delete pointer;
   }
};

template <typename T>
struct ScopedPointerArrayDeleter
{
   static inline void cleanup(T *pointer)
   {
      // Enforce a complete type.
      // If you get a compile error here, read the section on forward declared
      // classes in the QScopedPointer documentation.
      using IsIncompleteType = char[sizeof(T) ? 1 : -1];
      (void) sizeof(IsIncompleteType);
      delete []pointer;
   }
};

struct ScopedPointerPodDeleter
{
   static inline void cleanup(void *pointer)
   {
      if (pointer) {
         std::free(pointer);
      }
   }
};

template <typename T>
struct ScopedPointerObjectDeleteLater
{
   static inline void cleanup(T *pointer)
   {
      if (pointer) {
         pointer->deleteLater();
      }
   }
};

using pdk::kernel::Object;

using ScopedPointerDeleteLater = ScopedPointerObjectDeleteLater<Object>;

template <typename T, typename Deleter = ScopedPointerDeleter<T>>
class ScopedPointer
{
   using RestrictedBool = T *ScopedPointer:: *;
public:
   explicit ScopedPointer(T *pointer = nullptr)
      : m_data(pointer)
   {}
   
   inline ~ScopedPointer()
   {
      T *oldPointer = this->m_data;
      Deleter::cleanup(oldPointer);
   }
   
   inline T &operator *() const
   {
      PDK_ASSERT(m_data);
      return *m_data;
   }
   
   T *operator->() const noexcept
   {
      return m_data;
   }
   
   bool operator !() const noexcept
   {
      return !m_data;
   }
   
   inline operator bool() const
   {
      return isNull() ? false : static_cast<bool>(&ScopedPointer::m_data);
   }
   
   T *getData() const noexcept
   {
      return m_data;
   }
   
   bool isNull() const noexcept
   {
      return !m_data;
   }
   
   void reset(T *other = nullptr) noexcept(noexcept(Deleter::cleanup(std::declval<T *>())))
   {
      if (m_data == other) {
         return;
      }
      T *oldData = m_data;
      m_data = other;
      Deleter::cleanup(oldData);
   }
   
   T *take() noexcept
   {
      T *oldData = m_data;
      m_data = nullptr;
      return oldData;
   }
   
   void swap(ScopedPointer<T, Deleter> &other) noexcept
   {
      std::swap(m_data, other.m_data);
   }
   
protected:
   T *m_data;
   
private:
   PDK_DISABLE_COPY(ScopedPointer);
};

template <typename T, typename Deleter>
inline bool operator ==(const ScopedPointer<T, Deleter> &lhs, const ScopedPointer<T, Deleter> &rhs) noexcept
{
   return lhs.getData() == rhs.getData();
}

template <typename T, typename Deleter>
inline bool operator !=(const ScopedPointer<T, Deleter> &lhs, const ScopedPointer<T, Deleter> &rhs) noexcept
{
   return lhs.getData() != rhs.getData();
}

template <typename T, typename Deleter>
inline bool operator ==(const ScopedPointer<T, Deleter> &lhs, std::nullptr_t) noexcept
{
   return lhs.isNull();
}

template <typename T, typename Deleter>
inline bool operator ==(std::nullptr_t, const ScopedPointer<T, Deleter> &rhs) noexcept
{
   return rhs.isNull();
}

template <typename T, typename Deleter>
inline bool operator !=(const ScopedPointer<T, Deleter> &lhs, std::nullptr_t) noexcept
{
   return !lhs.isNull();
}

template <typename T, typename Deleter>
inline bool operator !=(std::nullptr_t, const ScopedPointer<T, Deleter> &rhs) noexcept
{
   return !rhs.isNull();
}

template <typename T, typename Deleter>
inline void swap(const ScopedPointer<T, Deleter> &lhs, const ScopedPointer<T, Deleter> &rhs)
{
   lhs.swap(rhs);
}

namespace internal {

template <typename X, typename Y>
struct ScopedArrayEnsureSameType;

template <typename X>
struct ScopedArrayEnsureSameType<X, X>
{
   using Type = X *;
};

template <typename X>
struct ScopedArrayEnsureSameType<const X, X>
{
   using Type = X *;
};

} // internal

template <typename T, typename Deleter = ScopedPointerArrayDeleter<T>>
class ScopedArrayPointer : public ScopedPointer<T, Deleter>
{
public:
   inline ScopedArrayPointer()
      : ScopedPointer<T, Deleter>(nullptr)
   {}
   
   template<typename D>
   explicit inline ScopedArrayPointer(D *p, typename internal::ScopedArrayEnsureSameType<T,D>::Type = nullptr)
      : ScopedPointer<T, Deleter>(p)
   {}
   
   inline T &operator [](int i)
   {
      return this->m_data[i];
   }
   
   inline const T &operator [](int i) const
   {
      return this->m_data[i];
   }
   
   void swap(ScopedArrayPointer &other) noexcept
   {
      ScopedPointer<T, Deleter>::swap(other);
   }
private:
   explicit inline ScopedArrayPointer(void *)
   {
      // Enforce the same type.
      
      // If you get a compile error here, make sure you declare
      // ScopedArrayPointer with the same template type as you pass to the
      // constructor. See also the ScopedPointer documentation.
      
      // Storing a scalar array as a pointer to a different type is not
      // allowed and results in undefined behavior.
   }
   
   PDK_DISABLE_COPY(ScopedArrayPointer);
};

template <typename T, typename Deleter>
inline void swap(ScopedArrayPointer<T, Deleter> &lhs, ScopedArrayPointer<T, Deleter> &rhs)
{
   lhs.swap(rhs);
}

} // utils
} // pdk

#endif // PDK_UTILS_SCOPED_POINTER_H

