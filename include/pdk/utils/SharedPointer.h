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
// Created by softboy on 2017/12/31.

#ifndef PDK_UTILS_SHARED_POINTER_H
#define PDK_UTILS_SHARED_POINTER_H

#include "pdk/global/Global.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/kernel/Object.h"
#include "pdk/kernel/HashFuncs.h"

namespace pdk {
namespace utils {

#ifdef PDK_NO_DEBUG
#  define PDK_SHARED_POINTER_VERIFY_AUTO_CAST(T, X) pdk::pdk_noop()
#else
template <typename T>
inline void pkd_shared_pointer_cast_check(T *)
{}
#define PDK_SHARED_POINTER_VERIFY_AUTO_CAST(T, X) \
   pkd_shared_pointer_cast_check<T>(static_cast<X *>(nullptr));
#endif

// forward class declarations
template <typename T> class WeakPointer;
template <typename T> class SharedPointer;
template <typename T> class EnableSharedFromThis;
using pdk::kernel::Object;

template <class X, class T>
SharedPointer<X> shared_pointer_cast(const SharedPointer<T> &ptr);
template <class X, class T>
SharedPointer<X> shared_pointer_dynamiccast(const SharedPointer<T> &ptr);
template <class X, class T>
SharedPointer<X> shared_pointer_constcast(const SharedPointer<T> &ptr);

namespace internal {

using pdk::os::thread::BasicAtomicInt;

template <typename T> class ExternalRefCount;
template <typename TargetType, typename SourceType>
SharedPointer<TargetType> copy_and_set_pointer(TargetType *ptr, const SharedPointer<SourceType> &src);

PDK_CORE_EXPORT void internal_safety_check_add(const void *, const volatile void *);
PDK_CORE_EXPORT void internal_safety_check_remove(const void *);

template <typename T, typename KClass, typename RetValue>
inline void execute_deleter(T *t, RetValue (KClass::*memberDeleter)())
{
   (t->*memberDeleter)();
}

template <typename T, typename Deleter>
inline void execute_deleter(T *t, Deleter d)
{
   d(t);
}

struct NormalDeleter
{};

// this uses partial template specialization
template <typename T> struct RemovePointer;
template <typename T> struct RemovePointer<T *>
{
   using Type = T;
};

template <typename T> struct RemovePointer<SharedPointer<T>>
{
   using Type = T;
};

template <typename T> struct RemovePointer<WeakPointer<T>>
{
   using Type = T;
};

// This class is the d-pointer of QSharedPointer and QWeakPointer.
//
// It is a reference-counted reference counter. "strongref" is the inner
// reference counter, and it tracks the lifetime of the pointer itself.
// "weakref" is the outer reference counter and it tracks the lifetime of
// the ExternalRefCountData object.
//
// The deleter is stored in the destroyer member and is always a pointer to
// a static function in ExternalRefCountWithCustomDeleter or in
// ExternalRefCountWithContiguousData
struct ExternalRefCountData
{
   using DestroyerFn = void (*)(ExternalRefCountData *);
   
   inline ExternalRefCountData(DestroyerFn destroyer)
      : m_destroyer(destroyer)
   {
      m_strongRef.store(1);
      m_weakRef.store(1);
   }
   
   inline ExternalRefCountData(pdk::Initialization)
   {}
   
   ~ExternalRefCountData()
   {
      PDK_ASSERT(!m_weakRef.load());
      PDK_ASSERT(m_strongRef <= 0);
   }
   
   void destroy()
   {
      m_destroyer(this);
   }
   
   PDK_CORE_EXPORT static ExternalRefCountData *getAndRef(const Object *);
   PDK_CORE_EXPORT void setObjectShared(const Object *, bool enable);
   PDK_CORE_EXPORT void checkObjectShared(const Object *);
   
   inline void checkObjectShared(...)
   {}
   
   inline void setObjectShared(...)
   {}
   
   inline void operator delete(void *ptr)
   {
      ::operator delete(ptr);
   }
   
   inline void operator delete(void *, void *)
   {}
   
   BasicAtomicInt m_weakRef;
   BasicAtomicInt m_strongRef;
   DestroyerFn m_destroyer;
};
// sizeof(ExternalRefCountData) = 12 (32-bit) / 16 (64-bit)

template <typename T, typename Deleter>
struct CustomDeleter
{
   Deleter m_deleter;
   T *m_ptr;
   
   CustomDeleter(T *ptr, Deleter deleter)
      : m_deleter(deleter),
        m_ptr(ptr)
   {}
   
   void execute()
   {
      execute_deleter(m_ptr, m_deleter);
   }
};
// sizeof(CustomDeleter) = sizeof(Deleter) + sizeof(void*) + padding
// for Deleter = stateless functor: 8 (32-bit) / 16 (64-bit) due to padding
// for Deleter = function pointer:  8 (32-bit) / 16 (64-bit)
// for Deleter = PMF: 12 (32-bit) / 24 (64-bit)  (GCC)

// This specialization of CustomDeleter for a deleter of type NormalDeleter
// is an optimization: instead of storing a pointer to a function that does
// the deleting, we simply delete the pointer ourselves.
template <typename T>
struct CustomDeleter<T, NormalDeleter>
{
   T *m_ptr;
   
   CustomDeleter(T *ptr, NormalDeleter)
      : m_ptr(ptr)
   {}
   
   void execute()
   {
      delete m_ptr;
   }
};
// sizeof(CustomDeleter specialization) = sizeof(void*)

// This class extends ExternalRefCountData and implements
// the static function that deletes the object. The pointer and the
// custom deleter are kept in the "extra" member so we can construct
// and destruct it independently of the full structure.
template <typename T, typename Deleter>
struct ExternalRefCountWithCustomDeleter : public ExternalRefCountData
{
   using Self = ExternalRefCountWithCustomDeleter;
   using BaseClass = ExternalRefCountData;
   
   CustomDeleter<T, Deleter> m_extra;
   
   static inline void deleter(ExternalRefCountData *self)
   {
      Self *realSelf = static_cast<Self *>(self);
      realSelf->m_extra.execute();
      // delete the deleter too
      realSelf->m_extra.~CustomDeleter<T, Deleter>();
   }
   
   static void safetyCheckDeleter(ExternalRefCountData *self)
   {
      internal_safety_check_remove(self);
      deleter(self);
   }
   
   static inline Self *create(T *ptr, Deleter userDeleter, DestroyerFn actualDeleter)
   {
      Self *data = static_cast<Self *>(::operator new(sizeof(Self)));
      // initialize the two sub-objects
      new (&data->m_extra) CustomDeleter<T, Deleter>(ptr, userDeleter);
      new (data) BaseClass(actualDeleter);
      return data;
   }
   
private:
   ExternalRefCountWithCustomDeleter() = delete;
   ~ExternalRefCountWithCustomDeleter() = delete;
   PDK_DISABLE_COPY(ExternalRefCountWithCustomDeleter);
};

template <typename T>
struct ExternalRefCountWithContiguousData : public ExternalRefCountData
{
   using ParentDataType = ExternalRefCountData;
   T m_data;
   
   static void deleter(ExternalRefCountData *self)
   {
      ExternalRefCountWithContiguousData *that =
            static_cast<ExternalRefCountWithContiguousData *>(self);
      that->m_data.~T();
      PDK_UNUSED(that);
   }
   
   static void safetyCheckDeleter(ExternalRefCountData *self)
   {
      internal_safety_check_remove(self);
      deleter(self);
   }
   
   static inline ExternalRefCountData *create(T **ptr, DestroyerFn destroy)
   {
      ExternalRefCountWithContiguousData *d
            = static_cast<ExternalRefCountWithContiguousData *>(::operator new(sizeof(ExternalRefCountWithContiguousData)));
      new (d) ParentDataType(destroy);
      *ptr = &d->m_data;
      return d;
   }
private:
   ExternalRefCountWithContiguousData() = delete;
   ~ExternalRefCountWithContiguousData() = delete;
   PDK_DISABLE_COPY(ExternalRefCountWithContiguousData);
};

} // internal

template <typename T>
class SharedPointer
{
   using RestrictedBool = T *SharedPointer:: *;
   using RefCountData = internal::ExternalRefCountData;
public:
   using Type = T;
   using ElementType = T;
   using ValueType = T;
   using Pointer = ValueType *;
   using ConstPointer = const ValueType *;
   using Reference = ValueType &;
   using ConstReference = const ValueType &;
   using DifferenceType = pdk::ptrdiff;
   
   using element_type = ElementType;
   using value_type = ValueType;
   using pointer = Pointer;
   using const_pointer = ConstPointer;
   using reference = Reference;
   using const_reference = ConstReference;
   using difference_type = DifferenceType;
   
   constexpr SharedPointer() noexcept
      : m_value(nullptr),
        m_refCountData(nullptr)
   {}
   
   constexpr SharedPointer(std::nullptr_t)
      : m_value(nullptr),
        m_refCountData(nullptr)
   {}
   
   template <typename X>
   inline explicit SharedPointer(X *ptr)
      : m_value(ptr)
   {
      internalConstruct(ptr, internal::NormalDeleter());
   }
   
   template <typename X, typename Deleter>
   inline SharedPointer(X *ptr, Deleter deleter)
      : m_value(ptr)
   {
      internalConstruct(ptr, deleter);
   }
   
   template <typename X, typename Deleter>
   inline SharedPointer(std::nullptr_t, Deleter)
      : m_value(nullptr),
        m_refCountData(nullptr)
   {}
   
   SharedPointer(const SharedPointer &other) noexcept
      : m_value(other.m_value),
        m_refCountData(other.m_refCountData)
   {
      if (m_refCountData) {
         ref();
      }
   }
   
   SharedPointer &operator =(const SharedPointer &other) noexcept
   {
      SharedPointer copy(other);
      swap(copy);
      return *this;
   }
   
   SharedPointer(SharedPointer &&other) noexcept
      : m_value(other.m_value),
        m_refCountData(other.m_refCountData)
   {
      other.m_value = nullptr;
      other.m_refCountData = nullptr;
   }
   
   SharedPointer &operator =(SharedPointer &&other) noexcept
   {
      SharedPointer moved(std::move(other));
      swap(moved);
      return *this;
   }
   
   template <class X>
   SharedPointer(SharedPointer<X> &&other) noexcept
      : m_value(other.m_value),
        m_refCountData(other.m_refCountData)
   {
      other.m_value = nullptr;
      other.m_refCountData = nullptr;
   }
   
   template <class X>
   SharedPointer &operator =(SharedPointer<X> &&other) noexcept
   {
      SharedPointer moved(std::move(other));
      swap(moved);
      return *this;
   }
   
   template <typename X>
   SharedPointer(const SharedPointer<X> &other) noexcept
      : m_value(other.m_value),
        m_refCountData(other.m_refCountData)
   {
      if (m_refCountData) {
         ref();
      }
   }
   
   template <typename X>
   inline SharedPointer(const WeakPointer<X> &other)
      : m_value(nullptr),
        m_refCountData(nullptr)
   {
      *this = other;
   }
   
   template <typename X>
   inline SharedPointer<T> &operator =(const WeakPointer<X> &other)
   {
      internalSet(other.m_refcountData, other.m_value);
      return *this;
   }
   
   ~SharedPointer()
   {
      deref();
   }
   
   T *getData() const 
   {
      return m_value;
   }
   
   bool isNull() const noexcept
   {
      return !getData();
   }
   
   operator RestrictedBool() const noexcept
   {
      return isNull() ? nullptr : &SharedPointer::m_value;
   }
   
   bool operator !() const noexcept
   {
      return isNull();
   }
   
   T &operator*() const
   {
      return *getData();
   }
   
   T *operator->() const noexcept
   {
      return getData();
   }
   
   inline void swap(SharedPointer &other)
   {
      this->internalSwap(other);
   }
   
   inline void reset()
   {
      clear();
   }
   
   inline void reset(T *target)
   {
      SharedPointer copy(target);
      swap(copy);
   }
   
   template <typename Deleter>
   inline void reset(T *target, Deleter deleter)
   {
      SharedPointer copy(target, deleter);
      swap(copy);
   }
   
   template <typename X>
   SharedPointer<X> staticCast() const
   {
      return shared_pointer_constcast<X, T>(*this);
   }
   
   template <typename X>
   SharedPointer<X> dynamicCast() const
   {
      return shared_pointer_dynamiccast<X, T>(*this);
   }
   
   template <typename X>
   SharedPointer<X> constCast() const
   {
      return shared_pointer_cast<X, T>(*this);
   }
   
   inline void clear()
   {
      SharedPointer copy;
      swap(copy);
   }
   
   WeakPointer<T> toWeakRef() const;
   
   template <typename... Args>
   static SharedPointer create(Args && ...arguments)
   {
      using Private = internal::ExternalRefCountWithContiguousData<T>;
#ifdef PDK_SHAREDPOINTER_TRACK_POINTERS
      typename Private::DestroyerFn destroy = &Private::safetyCheckDeleter;
#else
      typename Private::DestroyerFn destroy = &Private::deleter;
#endif
      SharedPointer result(pdk::Initialization::Uninitialized);
      result.m_refCountData = Private::create(&result.m_value, destroy);
      new (result.getData()) T(std::forward<Args>(arguments)...);
      result.m_refCountData->setObjectShared(result.m_value, true);
#ifdef PDK_SHAREDPOINTER_TRACK_POINTERS
      internal::internal_safety_check_add(result.m_refCountData, result.m_value);
#endif
      result.enableSharedFromThis(result.getData());
      return result;
   }
private:
   explicit SharedPointer(pdk::Initialization)
   {}
   
   template <typename X, typename Deleter>
   inline void internalConstruct(X *ptr, Deleter deleter)
   {
      if (!ptr) {
         m_refCountData = nullptr;
         return;
      }
      using Private = internal::ExternalRefCountWithCustomDeleter<X, Deleter>;
#ifdef PDK_SHAREDPOINTER_TRACK_POINTERS
      typename Private::DestroyerFn actualDeleter = &Private::safetyCheckDeleter;
#else
      typename Private::DestroyerFn actualDeleter = &Private::deleter;
#endif
      m_refCountData = Private::create(ptr, deleter, actualDeleter);
#ifdef PDK_SHAREDPOINTER_TRACK_POINTERS
      internal::internal_safety_check_add(m_refCountData, ptr);
#endif
      m_refCountData->setObjectShared(ptr, true);
      enableSharedFromThis(ptr);
   }
   
   void internalSwap(SharedPointer &other) noexcept
   {
      std::swap(m_refCountData, other.m_refCountData);
      std::swap(this->m_value, other.m_value);
   }
   
   void ref() const noexcept
   {
      m_refCountData->m_weakRef.ref();
      m_refCountData->m_strongRef.ref();
   }
   
   void deref() noexcept
   {
      deref(m_refCountData);
   }
   
   static void deref(RefCountData *refcountData) noexcept
   {
      if (!refcountData) {
         return;
      }
      if (!refcountData->m_strongRef.deref()) {
         refcountData->destroy();
      }
      if (!refcountData->m_weakRef.deref()) {
         delete refcountData;
      }
   }
   
   template <typename X>
   inline void enableSharedFromThis(const EnableSharedFromThis<X> *ptr)
   {
      ptr->initializeFromSharedPointer(constCast<typename std::remove_cv<T>::type>());
   }
   
   inline void enableSharedFromThis(...)
   {}
   
#if defined(PDK_NO_TEMPLATE_FRIENDS)
public:
#else
   template <typename X>
   friend class SharedPointer;
   template <typename X>
   friend class WeakPointer;
   template <typename TargetType, typename SourceType>
   friend SharedPointer<TargetType> internal::copy_and_set_pointer(TargetType *ptr, const SharedPointer<SourceType> &src);
#endif
   
   inline void internalSet(RefCountData *refcountData, T *actual)
   {
      if (refcountData) {
         int temp = refcountData->m_strongRef.load();
         while (temp > 0) {
            if (refcountData->m_strongRef.testAndSetRelaxed(temp, temp + 1)) {
               break;
            }
            temp = refcountData->m_strongRef.load();
         }
         if (temp > 0) {
            refcountData->m_weakRef.ref();
         } else {
            refcountData->checkObjectShared(actual);
            refcountData = nullptr;
         }
      }
      std::swap(m_refCountData, refcountData);
      std::swap(this->m_value, actual);
      if (!m_refCountData || m_refCountData->m_strongRef.load() == 0) {
         this->m_value = nullptr;
      }
      deref(refcountData);
   }
   
   Type *m_value;
   RefCountData *m_refCountData;
};

template <typename T>
class WeakPointer
{
   using RestrictedBool = T *WeakPointer:: *;
   using RefCountData = internal::ExternalRefCountData;
   
public:
   using ElementType = T;
   using ValueType = T;
   using PointerType = ValueType *;
   using ConstPointer = const ValueType *;
   using Reference = ValueType &;
   using ConstReference = const ValueType &;
   using DifferenceType = pdk::ptrdiff;
   
   using element_type = ElementType;
   using value_type = ValueType;
   using pointer = PointerType;
   using const_pointer = ConstPointer;
   using reference = Reference;
   using const_reference = ConstReference;
   using difference_type = DifferenceType;
   
public:
   inline WeakPointer() noexcept
      : m_refcountData(nullptr),
        m_value(nullptr)
   {}
   
   WeakPointer(const WeakPointer &other) noexcept
      : m_refcountData(other.m_refcountData),
        m_value(other.m_value)
   {
      if (m_refcountData) {
         m_refcountData->m_weakRef.ref();
      }  
   }
   
   inline WeakPointer(const SharedPointer<T> &other)
      : m_refcountData(other.m_refCountData),
        m_value(other.getData())
   {
      if (m_refcountData) {
         m_refcountData->m_weakRef.ref();
      }
   }
   
   inline WeakPointer &operator =(const SharedPointer<T> &other)
   {
      internalSet(other.m_refCountData, other.getData());
      return *this;
   }
   
   template <typename X>
   inline WeakPointer(const WeakPointer<X> &other)
      :  m_refcountData(nullptr),
        m_value(nullptr)
   {
      *this = other;
   }
   
   template <typename X>
   inline WeakPointer &operator =(const WeakPointer<X> &other)
   {
      // conversion between X and T could require access to the virtual table
      // so force the operation to go through SharedPointer
      *this = other.toStrongRef();
      return *this;
   }
   
   template <typename X>
   inline WeakPointer(const SharedPointer<X> &other)
      : m_refcountData(nullptr),
        m_value(nullptr)
   {
      *this = other;
   }
   
   template <typename X>
   inline WeakPointer &operator =(const SharedPointer<X> &other)
   {
      PDK_SHARED_POINTER_VERIFY_AUTO_CAST(T, X);
      internalSet(other.m_refCountData, other.getData());
      return *this;
   }
   
   template <class X>
   bool operator==(const SharedPointer<X> &other) const noexcept
   {
      return m_refcountData == other.m_refCountData;
   }
   
   template <class X>
   bool operator!=(const SharedPointer<X> &other) const noexcept
   {
      return !(*this == other);
   }
   
   WeakPointer(WeakPointer &&other) noexcept
      : m_refcountData(other.m_refcountData),
        m_value(other.m_value)
   {
      other.m_refcountData = nullptr;
      other.m_value = nullptr;
   }
   
   WeakPointer &operator =(WeakPointer &&other) noexcept
   {
      WeakPointer moved(std::move(other));
      swap(moved);
      return *this;
   }
   
   WeakPointer &operator =(const WeakPointer &other) noexcept
   {
      WeakPointer copy(other);
      swap(copy);
      return *this;
   }
   
   void swap(WeakPointer &other) noexcept
   {
      std::swap(this->m_refcountData, other.m_refcountData);
      std::swap(this->m_value, other.m_value);
   }
   
   inline ~WeakPointer()
   {
      if (m_refcountData && !m_refcountData->m_weakRef.deref()) {
         delete m_refcountData;
      }
   }
   
   bool isNull() const noexcept
   {
      return m_refcountData == nullptr || m_refcountData->m_strongRef.load() == 0
            || m_value == nullptr;
   }
   
   operator RestrictedBool() const noexcept
   {
      return isNull() ? nullptr : &WeakPointer::m_value;
   }
   
   bool operator !() const noexcept
   {
      return isNull();
   }
   
   T *getData() const noexcept
   {
      return m_refcountData == nullptr || 
            m_refcountData->m_strongRef.load() == 0 ? nullptr : m_value;
   }
   
   inline void clear()
   {
      *this = WeakPointer();
   }
   
   inline SharedPointer<T> toStrongRef() const
   {
      return SharedPointer<T>(*this);
   }
   
   inline SharedPointer<T> lock() const
   {
      return toStrongRef();
   }
   
private:
#if defined(PDK_NO_TEMPLATE_FRIENDS)
public:
#endif
   template <class X> friend class SharedPointer;
   template <class X> friend class Pointer;
   
   template <class X>
   inline WeakPointer &assign(X* ptr)
   {
      return *this = WeakPointer<X>(ptr, true);
   }
   
   template <class X>
   inline WeakPointer(X *ptr, bool) 
      : m_refcountData(ptr ? RefCountData::getAndRef(ptr) : nullptr),
        m_value(ptr)
   {}
   
   inline void internalSet(RefCountData *refcountData, T *actual)
   {
      if (m_refcountData == refcountData) {
         return;
      }
      if (refcountData) {
         refcountData->m_weakRef.ref();
      }
      if (m_refcountData && !m_refcountData->m_weakRef.deref()) {
         delete m_refcountData;
      }
      m_refcountData = refcountData;
      m_value = actual;
   }
   
   RefCountData *m_refcountData;
   T *m_value;
};

template <typename T>
class EnableSharedFromThis
{
protected:
   EnableSharedFromThis() = default;
   EnableSharedFromThis(const EnableSharedFromThis &)
   {}
   EnableSharedFromThis &operator =(const EnableSharedFromThis &)
   {
      return *this;
   }
   
#ifndef PDK_NO_TEMPLATE_FRIENDS
private:
   template <typename X>
   friend class SharedPointer;
#else
public:
#endif
   template <typename X>
   inline void initializeFromSharedPointer(const SharedPointer<X> &ptr) const
   {
      m_weakPointer = ptr;
   }
   
   mutable WeakPointer<T> m_weakPointer;
};

template <class T>
inline WeakPointer<T> SharedPointer<T>::toWeakRef() const
{
    return WeakPointer<T>(*this);
}


template <class T>
inline void swap(SharedPointer<T> &lhs, SharedPointer<T> &rhs)
{
    lhs.swap(rhs);
}

namespace internal {

template <typename TargetType, typename SourceType>
inline SharedPointer<TargetType> copy_and_set_pointer(TargetType *ptr, const SharedPointer<SourceType> &src)
{
   SharedPointer<TargetType> result;
   result.internalSet(src.m_refCountData, ptr);
   return result;
}

} // internal

template <typename TargetType, typename SourceType>
inline SharedPointer<TargetType> shared_pointer_cast(const SharedPointer<SourceType> &src)
{
   TargetType *ptr = static_cast<TargetType *>(src.getData());
   return internal::copy_and_set_pointer(ptr, src);
}

template <typename TargetType, typename SourceType>
inline SharedPointer<TargetType> shared_pointer_cast(const WeakPointer<SourceType> &src)
{
   return shared_pointer_cast<TargetType, SourceType>(src.toStrongRef());
}

template <typename TargetType, typename SourceType>
inline SharedPointer<TargetType> shared_pointer_dynamiccast(const SharedPointer<SourceType> &src)
{
   TargetType *ptr = dynamic_cast<TargetType *>(src.getData());
   if (!ptr) {
      return SharedPointer<TargetType>();
   }
   return internal::copy_and_set_pointer(ptr, src);
}

template <typename TargetType, typename SourceType>
inline SharedPointer<TargetType> shared_pointer_dynamiccast(const WeakPointer<SourceType> &src)
{
   return shared_pointer_dynamiccast<TargetType, SourceType>(src.toStrongRef());
}

template <typename TargetType, typename SourceType>
inline SharedPointer<TargetType> shared_pointer_constcast(const SharedPointer<SourceType> &src)
{
   TargetType *ptr = const_cast<TargetType *>(src.getData());
   return internal::copy_and_set_pointer(ptr, src);
}

template <typename TargetType, typename SourceType>
inline SharedPointer<TargetType> shared_pointer_constcast(const WeakPointer<SourceType> &src)
{
   return shared_pointer_constcast<TargetType, SourceType>(src.toStrongRef());
}

template <typename TargetType, typename SourceType>
inline WeakPointer<TargetType> weak_pointer_cast(const SharedPointer<SourceType> &src)
{
   return shared_pointer_cast<TargetType, SourceType>(src).toWeakRef();
}

template <class T, class X>
bool operator==(const SharedPointer<T> &lhs, const SharedPointer<X> &rhs) noexcept
{
    return lhs.getData() == rhs.getData();
}

template <class T, class X>
bool operator!=(const SharedPointer<T> &lhs, const SharedPointer<X> &rhs) noexcept
{
    return lhs.getData() != rhs.getData();
}

template <class T, class X>
bool operator==(const SharedPointer<T> &lhs, const X *rhs) noexcept
{
    return lhs.getData() == rhs;
}

template <class T, class X>
bool operator==(const T *lhs, const SharedPointer<X> &rhs) noexcept
{
    return lhs == rhs.getData();
}

template <class T, class X>
bool operator!=(const SharedPointer<T> &lhs, const X *rhs) noexcept
{
    return !(lhs == rhs);
}

template <class T, class X>
bool operator!=(const T *lhs, const SharedPointer<X> &rhs) noexcept
{
    return !(rhs == lhs);
}

template <typename T, typename X>
bool operator ==(const SharedPointer<T> &lhs, const WeakPointer<X> &rhs) noexcept
{
   return rhs == lhs;
}

template <typename T, typename X>
bool operator !=(const SharedPointer<T> &lhs, const WeakPointer<X> &rhs) noexcept
{
   return rhs != lhs;
}

template <typename T>
bool operator ==(const SharedPointer<T> &lhs, std::nullptr_t) noexcept
{
   return lhs.isNull();
}

template <typename T>
bool operator !=(const SharedPointer<T> &lhs, std::nullptr_t) noexcept
{
   return !lhs.isNull();
}

template <typename T>
bool operator ==(std::nullptr_t, const SharedPointer<T> &rhs) noexcept
{
   return rhs.isNull();
}

template <typename T>
bool operator !=(std::nullptr_t, const SharedPointer<T> &rhs) noexcept
{
   return !rhs.isNull();
}

template <typename T>
bool operator ==(const WeakPointer<T> &lhs, std::nullptr_t) noexcept
{
   return lhs.isNull();
}

template <typename T>
bool operator !=(const WeakPointer<T> &lhs, std::nullptr_t) noexcept
{
   return !lhs.isNull();
}

template <typename T>
bool operator ==(std::nullptr_t, const WeakPointer<T> &rhs) noexcept
{
   return rhs.isNull();
}

template <typename T>
bool operator !=(std::nullptr_t, const WeakPointer<T> &rhs) noexcept
{
   return !rhs.isNull();
}

template <typename T, typename X>
inline typename SharedPointer<T>::DifferenceType operator-(const SharedPointer<T> &lhs, const SharedPointer<X> &rhs)
{
    return lhs.getData() - rhs.getData();
}

template <typename T, typename X>
inline typename SharedPointer<T>::DifferenceType operator-(const SharedPointer<T> &lhs, X *rhs)
{
    return lhs.getData() - rhs;
}

template <typename T, typename X>
inline typename SharedPointer<T>::DifferenceType operator-(T *lhs, const SharedPointer<X> &rhs)
{
    return lhs - rhs.getData();
}

template <class T, class X>
inline bool operator<(const SharedPointer<T> &lhs, const SharedPointer<X> &rhs)
{
    return lhs.getData() < rhs.getData();
}
template <class T, class X>
inline bool operator<(const SharedPointer<T> &lhs, X *rhs)
{
    return lhs.getData() < rhs;
}
template <class T, class X>
inline bool operator<(T *lhs, const SharedPointer<X> &rhs)
{
    return lhs < rhs.getData();
}

template <typename T>
inline uint hash(const SharedPointer<T> &ptr, uint seed = 0)
{
   return pdk::hash(ptr.getData(), seed);
}

} // utils
} // pdk

template<typename T> PDK_DECLARE_TYPEINFO_BODY(pdk::utils::WeakPointer<T>, PDK_MOVABLE_TYPE);
template<typename T> PDK_DECLARE_TYPEINFO_BODY(pdk::utils::SharedPointer<T>, PDK_MOVABLE_TYPE);

#endif // PDK_UTILS_SHARED_POINTER_H
