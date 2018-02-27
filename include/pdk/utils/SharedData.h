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
// Created by softboy on 2018/01/30.

#ifndef PDK_UTILS_SHARED_DATA_H
#define PDK_UTILS_SHARED_DATA_H

#include "pdk/global/Global.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/kernel/HashFuncs.h"

namespace pdk {
namespace utils {

using pdk::os::thread::AtomicInt;

template <typename T>
class SharedDataPointer;

class PDK_CORE_EXPORT SharedData
{
public:
   mutable AtomicInt m_ref;
   inline SharedData()
      : m_ref(0)
   {}
   
   inline SharedData(const SharedData &)
      : m_ref(0) 
   {}
   
private:
   // using the assignment operator would lead to corruption in the ref-counting
   SharedData &operator=(const SharedData &);
};

template <typename T>
class SharedDataPointer
{
public:
   using Type = T;
   using pointer = T *;
   using Pointer = pointer;
   
   inline void detach()
   {
      if (m_implPtr && m_implPtr->m_ref.load() != 1) {
         detachHelper();
      }
   }
   
   inline T &operator*()
   {
      detach();
      return *m_implPtr;
   }
   
   inline const T &operator*() const
   {
      return *m_implPtr;
   }
   
   inline T *operator->()
   { 
      detach();
      return m_implPtr;
   }
   
   inline const T *operator->() const
   {
      return m_implPtr;
   }
   
   inline operator T *() 
   {
      detach();
      return m_implPtr;
   }
   
   inline operator const T *() const
   {
      return m_implPtr;
   }
   
   inline T *data()
   {
      detach();
      return m_implPtr;
   }
   
   inline const T *data() const
   {
      return m_implPtr;
   }
   
   inline const T *constData() const
   {
      return m_implPtr;
   }
   
   inline bool operator==(const SharedDataPointer<T> &other) const
   {
      return m_implPtr == other.m_implPtr;
   }
   
   inline bool operator!=(const SharedDataPointer<T> &other) const
   {
      return m_implPtr != other.m_implPtr;
   }
   
   inline SharedDataPointer()
   { 
      m_implPtr = nullptr;
   }
   
   inline ~SharedDataPointer()
   {
      if (m_implPtr && !m_implPtr->m_ref.deref()) {
         delete m_implPtr;
      }
   }
   
   explicit SharedDataPointer(T *data) noexcept;
   
   inline SharedDataPointer(const SharedDataPointer<T> &other) 
      : m_implPtr(other.m_implPtr)
   {
      if (m_implPtr) {
         m_implPtr->m_ref.ref();
      }
   }
   
   inline SharedDataPointer<T> & operator=(const SharedDataPointer<T> &other)
   {
      if (other.m_implPtr != m_implPtr) {
         if (other.m_implPtr) {
            other.m_implPtr->m_ref.ref();
         }
         T *old = m_implPtr;
         m_implPtr = other.m_implPtr;
         if (old && !old->m_ref.deref()) {
            delete old;
         }
      }
      return *this;
   }
   
   inline SharedDataPointer &operator=(T *other) {
      if (other != m_implPtr) {
         if (other) {
            other->m_ref.ref();
         }
         
         T *old = m_implPtr;
         m_implPtr = other;
         if (old && !old->m_ref.deref()){
            delete old;
         }
      }
      return *this;
   }
   
   SharedDataPointer(SharedDataPointer &&other) noexcept 
      : m_implPtr(other.m_implPtr)
   {
      other.m_implPtr = nullptr;
   }
   
   inline SharedDataPointer<T> &operator=(SharedDataPointer<T> &&other) noexcept
   {
      std::swap(m_implPtr, other.m_implPtr);
      return *this;
   }
   
   inline bool operator!() const { return !m_implPtr; }
   
   inline void swap(SharedDataPointer &other) noexcept
   {
      std::swap(m_implPtr, other.m_implPtr);
   }
   
protected:
   T *clone();
   
private:
   void detachHelper();
   T *m_implPtr;
};

template <typename T> 
class ExplicitlySharedDataPointer
{
public:
   using Type = T;
   using pointer = T *;
   using Pointer = pointer;
   
   inline T &operator*() const
   {
      return *m_implPtr;
   }
   
   inline T *operator->()
   {
      return m_implPtr;
   }
   
   inline T *operator->() const
   {
      return m_implPtr;
   }
   
   inline T *data() const
   {
      return m_implPtr;
   }
   
   inline const T *constData() const
   {
      return m_implPtr;
   }
   
   inline void detach()
   {
      if (m_implPtr && m_implPtr->m_ref.load() != 1) {
         detachHelper(); 
      }
   }
   
   inline void reset()
   {
      if(m_implPtr && !m_implPtr->m_ref.deref()) {
         delete m_implPtr;
      }
      m_implPtr = nullptr;
   }
   
   inline operator bool () const 
   { 
      return m_implPtr != nullptr;
   }
   
   inline bool operator==(const ExplicitlySharedDataPointer<T> &other) const
   {
      return m_implPtr == other.m_implPtr;
   }
   
   inline bool operator!=(const ExplicitlySharedDataPointer<T> &other) const
   {
      return m_implPtr != other.m_implPtr;
   }
   
   inline bool operator==(const T *ptr) const
   {
      return m_implPtr == ptr;
   }
   
   inline bool operator!=(const T *ptr) const
   {
      return m_implPtr != ptr;
   }
   
   inline ExplicitlySharedDataPointer()
   {
      m_implPtr = nullptr;
   }
   
   inline ~ExplicitlySharedDataPointer()
   {
      if (m_implPtr && !m_implPtr->m_ref.deref()) {
         delete m_implPtr;
      }
   }
   
   explicit ExplicitlySharedDataPointer(T *data) noexcept;
   inline ExplicitlySharedDataPointer(const ExplicitlySharedDataPointer<T> &other)
      : m_implPtr(other.m_implPtr) 
   { 
      if (other) {     
         other->m_ref.ref();
      }
   }
   
   template<class X>
   inline ExplicitlySharedDataPointer(const ExplicitlySharedDataPointer<X> &other)
      : m_implPtr(static_cast<T *>(other.data()))
   {
      if(m_implPtr) {
         m_implPtr->m_ref.ref();
      }
   }
   
   inline ExplicitlySharedDataPointer<T> & operator=(const ExplicitlySharedDataPointer<T> &other)
   {
      if (other.m_implPtr != m_implPtr) {
         if (other.m_implPtr) {
            other.m_implPtr->m_ref.ref();
         }
         T *old = m_implPtr;
         m_implPtr = other.m_implPtr;
         if (old && !old->m_ref.deref()) {
            delete old;
         } 
      }
      return *this;
   }
   
   inline ExplicitlySharedDataPointer &operator=(T *other) {
      if (other != m_implPtr) {
         if (other) {
            other->m_ref.ref();
         }
         T *old = m_implPtr;
         m_implPtr = other;
         if (old && !old->m_ref.deref()) {
            delete old;
         }
      }
      return *this;
   }
   
   inline ExplicitlySharedDataPointer(ExplicitlySharedDataPointer &&other) noexcept
      : m_implPtr(other.m_implPtr)
   { 
      other.m_implPtr = nullptr;
   }
   
   inline ExplicitlySharedDataPointer<T> &operator=(ExplicitlySharedDataPointer<T> &&other) noexcept
   {
      std::swap(m_implPtr, other.m_implPtr);
      return *this;
   }
   
   inline bool operator!() const
   {
      return !m_implPtr;
   }
   
   inline void swap(ExplicitlySharedDataPointer &other) noexcept
   {
      std::swap(m_implPtr, other.m_implPtr);
   }
   
protected:
   T *clone();
   
private:
   void detachHelper();
   
   T *m_implPtr;
};

template <typename T>
inline SharedDataPointer<T>::SharedDataPointer(T *data) noexcept
   : m_implPtr(data)
{
   if (m_implPtr) {
      m_implPtr->m_ref.ref();
   }
}


template <typename T>
inline T *SharedDataPointer<T>::clone()
{
   return new T(*m_implPtr);
}

template <typename T>
inline void SharedDataPointer<T>::detachHelper()
{
   T *x = clone();
   x->m_ref.ref();
   if (!m_implPtr->m_ref.deref()) {
      delete m_implPtr;
   }
   m_implPtr = x;
}

template <typename T>
inline T *ExplicitlySharedDataPointer<T>::clone()
{
   return new T(*m_implPtr);
}

template <typename T>
inline void ExplicitlySharedDataPointer<T>::detachHelper()
{
   T *x = clone();
   x->ref.ref();
   if (!m_implPtr->m_ref.deref()) {
      delete m_implPtr;
   }
   m_implPtr = x;
}

template <typename T>
inline ExplicitlySharedDataPointer<T>::ExplicitlySharedDataPointer(T *data) noexcept
   : m_implPtr(data)
{ 
   if (m_implPtr){
      m_implPtr->m_ref.ref();
   }
}

template <typename T>
inline void swap(SharedDataPointer<T> &lhs, SharedDataPointer<T> &rhs)
{
   lhs.swap(rhs);
}

template <typename T>
inline void swap(ExplicitlySharedDataPointer<T> &lhs, ExplicitlySharedDataPointer<T> &rhs)
{
   lhs.swap(rhs);
}

template <typename T>
inline uint hash(const SharedDataPointer<T> &ptr, uint seed = 0) noexcept
{
   return hash(ptr.data(), seed);
}
template <typename T>
inline uint hash(const ExplicitlySharedDataPointer<T> &ptr, uint seed = 0) noexcept
{
   return hash(ptr.data(), seed);
}

} // utils
} // pdk

namespace std {

template <class T>
inline void swap(pdk::utils::SharedDataPointer<T> &lhs, pdk::utils::SharedDataPointer<T> &rhs)
{
   lhs.swap(rhs);
}

template <class T>
inline void swap(pdk::utils::ExplicitlySharedDataPointer<T> &lhs, pdk::utils::ExplicitlySharedDataPointer<T> &rhs)
{
   lhs.swap(rhs);
}

} // std

template<typename T> PDK_DECLARE_TYPEINFO_BODY(pdk::utils::SharedDataPointer<T>, PDK_MOVABLE_TYPE);
template<typename T> PDK_DECLARE_TYPEINFO_BODY(pdk::utils::ExplicitlySharedDataPointer<T>, PDK_MOVABLE_TYPE);

#endif // PDK_UTILS_SHARED_DATA_H

