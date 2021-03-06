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
// Created by softboy on 2018/01/23.

#ifndef PDK_KERNEL_POINTER_H
#define PDK_KERNEL_POINTER_H

#include "pdk/global/Global.h"
#include "pdk/utils/SharedPointer.h"

namespace pdk {
namespace kernel {

using pdk::utils::WeakPointer;

template <typename T>
class Pointer
{
   PDK_STATIC_ASSERT_X(!std::is_pointer<T>::value, "pdk::kernel::Pointer's template type "
                                                   "must not be a pointer type");
   template <typename U>
   struct TypeSelector
   {
      using Type = Object;
   };
   
   template <typename U>
   struct TypeSelector <const U>
   {
      using Type = const Object;
   };
   
   using ObjectType = typename TypeSelector<T>::Type;
   
public:
   inline Pointer()
   {}
   
   inline Pointer(T *p) 
      : m_wptr(p, true)
   {}
   
   inline Pointer<T> &operator=(T* p)
   {
      m_wptr.assign(static_cast<ObjectType *>(p));
      return *this;
   }
   
   inline void swap(Pointer &other)
   { 
      m_wptr.swap(other.m_wptr); 
   }
   
   inline T *getData() const
   {
      return static_cast<T *>(m_wptr.getData());
   }
   
   inline T *operator ->() const
   {
      return getData();
   }
   
   inline T &operator *() const
   {
      return *getData();
   }
   
   inline operator T*() const
   {
      return getData();
   }
   
   inline bool isNull() const
   {
      return m_wptr.isNull();
   }
   
   inline void clear()
   {
      m_wptr.clear();
   }
   
   ~Pointer() = default;
   
private:
   WeakPointer<ObjectType> m_wptr;
};

template <class T>
inline bool operator==(const T *o, const Pointer<T> &p)
{
   return o == p.operator->();
}

template<class T>
inline bool operator==(const Pointer<T> &p, const T *o)
{
   return p.operator->() == o;
}

template <class T>
inline bool operator==(T *o, const Pointer<T> &p)
{
   return o == p.operator->();
}

template<class T>
inline bool operator==(const Pointer<T> &p, T *o)
{
   return p.operator->() == o;
}

template<class T>
inline bool operator==(const Pointer<T> &p1, const Pointer<T> &p2)
{
   return p1.operator->() == p2.operator->();
}

template <class T>
inline bool operator!=(const T *o, const Pointer<T> &p)
{
   return o != p.operator->();
}

template<class T>
inline bool operator!= (const Pointer<T> &p, const T *o)
{
   return p.operator->() != o;
}

template <class T>
inline bool operator!=(T *o, const Pointer<T> &p)
{
   return o != p.operator->();
}

template<class T>
inline bool operator!= (const Pointer<T> &p, T *o)
{
   return p.operator->() != o;
}

template<class T>
inline bool operator!= (const Pointer<T> &p1, const Pointer<T> &p2)
{
   return p1.operator->() != p2.operator->();
}

} // kernel
} // pdk

template <class T> 
PDK_DECLARE_TYPEINFO_BODY(::pdk::kernel::Pointer<T>, PDK_MOVABLE_TYPE);

#endif // PDK_KERNEL_POINTER_H
