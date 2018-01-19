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

/* Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef PDK_UTILS_SCOPED_OBJECT_GUARD_H
#define PDK_UTILS_SCOPED_OBJECT_GUARD_H

#include <type_traits>

#if defined(_MSC_VER)
#pragma once
#endif

namespace pdk {
namespace utils {

/* Until some official version of the ScopedGuard idiom makes it into Boost,
 * we locally define our own. This is a merely reformated version of
 * ScopedGuard.h as defined in:
 *   Alexandrescu, A., Marginean, P.:"Generic<Programming>: Change the Way You
 *     Write Exception-Safe Code - Forever", C/C++ Users Jornal, Dec 2000,
 *     http://www.drdobbs.com/184403758
 * with the following modifications:
 *   - General pretty formatting (pretty to my taste at least.)
 *   - Naming style changed to standard C++ library requirements.
 *   - Added scope_guard_impl4 and obj_scope_guard_impl3, (Boost.MultiIndex
 *     needs them). A better design would provide guards for many more
 *     arguments through the Boost Preprocessor Library.
 *   - Added scope_guard_impl_base::touch (see below.)
 *   - Removed RefHolder and ByRef, whose functionality is provided
 *     already by Boost.Ref.
 *   - Removed static make_guard's and make_obj_guard's, so that the code
 *     will work even if BOOST_NO_MEMBER_TEMPLATES is defined. This forces
 *     us to move some private ctors to public, though.
 *
 * NB: CodeWarrior Pro 8 seems to have problems looking up safe_execute
 * without an explicit qualification.
 * 
 * We also define the following variants of the idiom:
 * 
 *   - make_guard_if_c<bool>( ... )
 *   - make_guard_if<IntegralConstant>( ... )
 *   - make_obj_guard_if_c<bool>( ... )
 *   - make_obj_guard_if<IntegralConstant>( ... )
 * which may be used with a compile-time constant to yield
 * a "null_guard" if the boolean compile-time parameter is false,
 * or conversely, the guard is only constructed if the constant is true.
 * This is useful to avoid extra tagging, because the returned
 * null_guard can be optimzed comlpetely away by the compiler.
 */
class ScopedGuardImplBase
{
public:
   ScopedGuardImplBase()
      : m_dismissed(false)
   {}
   
   void dismiss() const
   {
      m_dismissed = true;
   }
   
   /* This helps prevent some "unused variable" warnings under, for instance,
    * GCC 3.2.
    */
   void touch() const
   {}
   
protected:
   ~ScopedGuardImplBase()
   {}
   
   ScopedGuardImplBase(const ScopedGuardImplBase &other)
      : m_dismissed(other.m_dismissed)
   {
      other.dismiss();
   }
   
   template <typename J>
   static void safeExecute(J &j)
   {
      try {
         if (!j.m_dismissed) {
            j.execute();
         }
      } catch (...) {}
   }
   
   mutable bool m_dismissed;
   
private:
   ScopedGuardImplBase &operator=(const ScopedGuardImplBase&);
};

using ScopedGuard = const ScopedGuardImplBase &;

struct NullGuard : public ScopedGuardImplBase
{
   template <typename T1>
   NullGuard(const T1 &)
   {}
   
   template <typename T1, typename T2>
   NullGuard(const T1 &)
   {}
   
   template <typename T1, typename T2, typename T3>
   NullGuard(const T1 &, const T2 &, const T3 &)
   {}
   
   template <typename T1, typename T2, typename T3, typename T4>
   NullGuard(const T1 &, const T2 &, const T3 &, const T4 &)
   {}
   
   template <typename T1, typename T2, typename T3, typename T4, typename T5>
   NullGuard(const T1 &, const T2 &, const T3 &, const T4 &, const T5 &)
   {}
};

template <bool cond, typename T>
struct NullGuardReturn
{
   using type = typename std::conditional<cond, T, NullGuard>::type;
};

template <typename FuncType>
class ScopedGuardImpl0 : public ScopedGuardImplBase
{
public:
   ScopedGuardImpl0(FuncType func)
      : m_func(func)
   {}
   
   ~ScopedGuardImpl0()
   {
      ScopedGuardImplBase::safeExecute(*this);
   }
   
   void execute()
   {
      m_func();
   }
   
protected:
   FuncType m_func;
};

template <typename FuncType>
inline ScopedGuardImpl0<FuncType> make_guard(FuncType func)
{
   return ScopedGuardImpl0<FuncType>(func);
}

template <bool cond, typename FuncType>
inline typename NullGuardReturn<cond, ScopedGuardImpl0<FuncType>>::type
make_guard_if_cond(FuncType func)
{
   return typename NullGuardReturn<cond, ScopedGuardImpl0<FuncType>>::type(func);
}

template<typename C, typename FuncType>
inline typename NullGuardReturn<C::value, ScopedGuardImpl0<FuncType>>::type
make_guard_if(FuncType func)
{
   return make_guard_if<C::value>(func);
}

template <typename F, typename P1>
class ScopedGuardImpl1 : public ScopedGuardImplBase
{
public:
   ScopedGuardImpl1(F func, P1 p1)
      : m_func(func),
        m_p1(p1)
   {}
   
   ~ScopedGuardImpl1()
   {
      ScopedGuardImplBase::safeExecute(*this);
   }
   
   void execute()
   {
      m_func(m_p1);
   }
protected:
   F m_func;
   const P1 m_p1;
};

template <typename FuncType, typename P1>
inline ScopedGuardImpl1<FuncType, P1> make_guard(FuncType func, P1 p1)
{
   return ScopedGuardImpl1<FuncType, P1>(func, p1);
}

template <bool cond, typename FuncType, typename P1>
inline typename NullGuardReturn<cond, ScopedGuardImpl1<FuncType, P1>>::type
make_guard_if_cond(FuncType func, P1 p1)
{
   return typename NullGuardReturn<cond, ScopedGuardImpl1<FuncType, P1>>::type(func, p1);
}

template <typename C, typename FuncType, typename P1>
inline typename NullGuardReturn<C::value, ScopedGuardImpl1<F, P1>>::type
make_guard_if(FuncType func, P1 p1)
{
   return make_guard_if_cond<C::value>(func, p1);
}

template <typename FuncType, typename P1, typename P2>
class ScopedGuardImpl2 : public ScopedGuardImplBase
{
public:
   ScopedGuardImpl2(FuncType func, P1 p1, P2 p2)
      : m_func(func),
        m_p1(p1),
        m_p2(p2)
   {}
   
   ~ScopedGuardImpl2()
   {
      ScopedGuardImplBase::safeExecute(*this);
   }
   
   void execute()
   {
      m_func(m_p1, m_p2);
   }
protected:
   FuncType m_func;
   const P1 m_p1;
   const P2 m_p2;
};

template<typename FuncType,typename P1, typename P2>
inline ScopedGuardImpl2<FuncType, P1, P2> make_guard(FuncType func, P1 p1, P2 p2)
{
   return ScopedGuardImpl2<FuncType, P1, P2>(func, p1, p2);
}

template<bool cond, typename FuncType, typename P1, typename P2>
inline typename NullGuardReturn<cond, ScopedGuardImpl2<FuncType, P1, P2>>::type
make_guard_if_cond(FuncType func, P1 p1, P2 p2)
{
   return typename NullGuardReturn<cond, ScopedGuardImpl2<FuncType, P1, p2>>::type(func, p1, p2);
}

template<typename C, typename FuncType, typename P1, typename P2>
inline typename NullGuardReturn<C::value, ScopedGuardImpl2<FuncType, P1, P2>>::type
make_guard_if(FuncType func, P1 p1, P2 p2)
{
   return make_guard_if_cond<C::value>(func, p1, p2);
}

template<typename FuncType, typename P1, typename P2, typename P3>
class ScopedGuardImpl3 : public ScopedGuardImplBase
{
public:
   ScopedGuardImpl3(FuncType func, P1 p1, P2 p2, P3 p3)
      : m_func(func),
        m_p1(p1),
        m_p2(p2),
        m_p3(p3)
   {}
   
   ~ScopedGuardImpl3()
   {
      ScopedGuardImplBase::safeExecute(*this);
   }
   
   void execute()
   {
      m_func(m_p1, m_p2, m_p3);
   }
protected:
   FuncType m_func;
   const P1 m_p1;
   const P2 m_p2;
   const P3 m_p3;
};

template <typename FuncType, typename P1, typename P2, typename P3>
inline ScopedGuardImpl3<FuncType, p1, p2, p3> make_guard(FuncType func, P1 p1, P2 p2, P3 p3)
{
   return ScopedGuardImpl3<FuncType, P1, P2, P3>(func, p1, p2, p3);
}

template <bool cond, typename FuncType, typename P1, typename P2, typename P3>
inline typename NullGuardReturn<cond, ScopedGuardImpl3<FuncType, P1, P2, P3>>::type 
make_guard_if_cond(FuncType func, P1 p1, P2 p2, P3 p3)
{
   return typename NullGuardReturn<cond, ScopedGuardImpl3<FuncType, P1, P2, P3>>::type(func, p1, p2, p3);
}

template<typename C, typename FuncType, typename P1, typename P2, typename P3>
inline typename NullGuardReturn<C::value, ScopedGuardImpl3<FuncType, P1, P2, P3> >::type
make_guard_if(FuncType func, P1 p1, P2 p2, P3 p3)
{
   return make_guard_if_cond<C::value>(func, p1, p2, p3);
}

template <typename FuncType, typename P1, typename P2, typename P3, typename P4>
class ScopedGuardImpl4 : public ScopedGuardImplBase
{
public:
   ScopedGuardImpl4(FuncType func, P1 p1, P2 p2, P3 p3, P4 p4)
      : m_func(func),
        m_p1(p1),
        m_p2(p2),
        m_p3(p3),
        m_p4(p4)
   {}
   
   ~ScopedGuardImpl4()
   {
      ScopedGuardImplBase::safeExecute(*this);
   }
   
   void execute()
   {
      m_func(m_p1, m_p2, m_p3, m_p4);
   }
   
protected:
   FuncType m_func;
   const P1 m_p1;
   const P2 m_p2;
   const P3 m_p3;
   const P4 m_p4;
};

template <typename FuncType, typename P1, typename P2, typename P3, typename P4>
inline ScopedGuardImpl4<FuncType, P1, P2, P3, P4> 
make_guard(FuncType func, P1 p1, P2 p2, P3 p3, P4 p4)
{
   return ScopedGuardImpl4<FuncType, P1, P2, P3, P4>(func, p1, p2, p3, p4);
}

template <bool cond, typename FuncType, typename P1, typename P2, typename P3, typename P4>
inline typename NullGuardReturn<cond, ScopedGuardImpl4<FuncType, P1, P2, P3, P4>>::type
make_guard_if_cond(FuncType func, P1 p1, P2 p2, P3 p3, P4 p4)
{
   return typename NullGuardReturn<cond, ScopedGuardImpl4<FuncType, P1, P2, P3, P4>>::type(func, p1, p2, p3, p4);
}

template<typename C, typename FuncType,typename P1,typename P2,typename P3,typename P4>
inline typename NullGuardReturn<C::value, ScopedGuardImpl4<FuncType, P1, P2, P3, P4>>::type
make_guard_if(FuncType func,P1 p1, P2 p2, P3 p3, P4 p4)
{
   return make_guard_if_cond<C::value>(func, p1, p2, p3, p4);
}

template <typename ObjectType, typename MemFunc>
class ObjectScopedGuardImpl0 : public ScopedGuardImplBase
{
public:
   ObjectScopedGuardImpl0(ObjectType &obj, MemFunc memFunc)
      : m_obj(obj),
        m_memFunc(memFunc)
   {}
   
   ~ObjectScopedGuardImpl0()
   {
      ScopedGuardImplBase::safeExecute(*this);
   }
   
   void execute()
   {
      (m_obj.*memFunc)();         
   }
protected:
  ObjectType &m_obj;
  MemFunc m_memFunc;
};

template<typename ObjectType, typename MemFunc>
inline ObjectScopedGuardImpl0<ObjectType, MemFunc> make_object_guard(ObjectType &obj, MemFunc memFunc)
{
   return ObjectScopedGuardImpl0<ObjectType, MemFunc>(obj, memFunc);
}

template<bool cond, typename ObjectType, typename MemFunc>
inline typename NullGuardReturn<cond, ObjectScopedGuardImpl0<ObjectType, MemFunc>>::type
make_object_guard_if_cond(ObjectType &object, MemFunc memFunc)
{
   return typename NullGuardReturn<cond, ObjectScopedGuardImpl0<ObjectType, MemFunc>>::type(object, memFunc);
}

template<typename C, typename ObjectType, typename MemFunc>
inline typename NullGuardReturn<C::value, ObjectScopedGuardImpl0<ObjectType, MemFunc>>::type 
make_object_guard_if(ObjectType &object, MemFunc memFunc)
{
   return make_object_guard_if_cond<C::value>(object, memFunc);
}

template <typename ObjectType, typename MemFunc, typename P1>
class ObjectScopedGuardImpl1 : public ScopedGuardImplBase
{
public:
   ObjectScopedGuardImpl1(ObjectType &obj, MemFunc memFunc, P1 p1)
      : m_obj(obj),
        m_memFunc(memFunc),
        m_p1(p1)
   {}
   
   ~ObjectScopedGuardImpl1()
   {
      ScopedGuardImplBase::safeExecute(*this);
   }
   
   void execute()
   {
      (m_obj.*memFunc)(m_p1);         
   }
protected:
  ObjectType &m_obj;
  MemFunc m_memFunc;
  const P1 m_p1;
};

template <typename ObjectType, typename MemFunc, typename P1>
inline ObjectScopedGuardImpl1<ObjectType, MemFunc, P1>
make_object_guard(ObjectType &obj, MemFunc memFunc, P1 p1)
{
   return ObjectScopedGuardImpl1<ObjectType, MemFunc, P1>(obj, memFunc, p1);
}

template<bool cond, typename ObjectType, typename MemFunc, typename P1>
inline typename NullGuardReturn<cond, ObjectScopedGuardImpl1<ObjectType, MemFunc, P1>>::type
make_object_guard_if_cond(ObjectType &obj, MemFunc memFunc, P1 p1)
{
   return typename NullGuardReturn<cond, ObjectScopedGuardImpl1<ObjectType, MemFunc, P1>>::type(obj, memFunc, p1);
}

template<typename C, class ObjectType, typename MemFunc, typename P1>
inline typename NullGuardReturn<C::value, ObjectScopedGuardImpl1<ObjectType, MemFunc, P1>>::type
make_object_guard_if(ObjectType &obj, MemFunc memFun, P1 p1)
{
  return make_object_guard_if_cond<C::value>(obj, memFun, p1);
}

template <typename ObjectType, typename MemFunc, typename P1, typename P2>
class ObjectScopedGuardImpl2 : public ScopedGuardImplBase
{
public:
   ObjectScopedGuardImpl2(ObjectType &obj, MemFunc memFunc, P1 p1, P2 p2)
      : m_obj(obj),
        m_memFunc(memFunc),
        m_p1(p1),
        m_p2(p2)
   {}
   
   ~ObjectScopedGuardImpl2()
   {
      ScopedGuardImplBase::safeExecute(*this);
   }
   
   void execute()
   {
      (m_obj.*memFunc)(m_p1, m_p2);         
   }
protected:
  ObjectType &m_obj;
  MemFunc m_memFunc;
  const P1 m_p1;
  const P2 m_p2;
};

template <typename ObjectType, typename MemFunc, typename P1, typename P2>
inline ObjectScopedGuardImpl2<ObjectType, MemFunc, P1, P2>
make_object_guard(ObjectType &obj, MemFunc memFunc, P1 p1, P2 p2)
{
   return ObjectScopedGuardImpl2<ObjectType, MemFunc, P1>(obj, memFunc, p1, p2);
}

template<bool cond, typename ObjectType, typename MemFunc, typename P1, typename P2>
inline typename NullGuardReturn<cond, ObjectScopedGuardImpl2<ObjectType, MemFunc, P1, P2>>::type
make_object_guard_if_cond(ObjectType &obj, MemFunc memFunc, P1 p1, P2 p2)
{
   return typename NullGuardReturn<cond, ObjectScopedGuardImpl2<ObjectType, MemFunc, P1, P2>>::type(obj, memFunc, p1, p2);
}

template<typename C, class ObjectType, typename MemFunc, typename P1, typename P2>
inline typename NullGuardReturn<C::value, ObjectScopedGuardImpl2<ObjectType, MemFunc, P1, P2>>::type
make_object_guard_if(ObjectType &obj, MemFunc memFun, P1 p1, P2 p2)
{
  return make_object_guard_if_cond<C::value>(obj, memFun, p1, p2);
}

template <typename ObjectType, typename MemFunc, typename P1, typename P2, typename P3>
class ObjectScopedGuardImpl3 : public ScopedGuardImplBase
{
public:
   ObjectScopedGuardImpl3(ObjectType &obj, MemFunc memFunc, P1 p1, P2 p2, P3 p3)
      : m_obj(obj),
        m_memFunc(memFunc),
        m_p1(p1),
        m_p2(p2),
        m_p3(p3)
   {}
   
   ~ObjectScopedGuardImpl3()
   {
      ScopedGuardImplBase::safeExecute(*this);
   }
   
   void execute()
   {
      (m_obj.*memFunc)(m_p1, m_p2, m_p3);         
   }
protected:
  ObjectType &m_obj;
  MemFunc m_memFunc;
  const P1 m_p1;
  const P2 m_p2;
  const P3 m_p3;
};

template <typename ObjectType, typename MemFunc, typename P1, typename P2, typename P3>
inline ObjectScopedGuardImpl3<ObjectType, MemFunc, P1, P2, P3>
make_object_guard(ObjectType &obj, MemFunc memFunc, P1 p1, P2 p2, P3 p3)
{
   return ObjectScopedGuardImpl3<ObjectType, MemFunc, P1>(obj, memFunc, p1, p2, p3);
}

template<bool cond, typename ObjectType, typename MemFunc, typename P1, typename P2, typename P3>
inline typename NullGuardReturn<cond, ObjectScopedGuardImpl3<ObjectType, MemFunc, P1, P2, P3>>::type
make_object_guard_if_cond(ObjectType &obj, MemFunc memFunc, P1 p1, P2 p2, P3 p3)
{
   return typename NullGuardReturn<cond, ObjectScopedGuardImpl3<ObjectType, MemFunc, P1, P2, P3>>::type(obj, memFunc, p1, p2, p3);
}

template<typename C, class ObjectType, typename MemFunc, typename P1, typename P2, typename P3>
inline typename NullGuardReturn<C::value, ObjectScopedGuardImpl3<ObjectType, MemFunc, P1, P2, P3>>::type
make_object_guard_if(ObjectType &obj, MemFunc memFun, P1 p1, P2 p2, P3 p3)
{
  return make_object_guard_if_cond<C::value>(obj, memFun, p1, p2, p3);
}

} // utils
} // pdk

#endif // PDK_UTILS_SCOPED_OBJECT_GUARD_H
