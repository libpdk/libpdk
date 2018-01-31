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
// Created by softboy on 2017/11/14.

#ifndef PDK_GLOBAL_TYPEINFO_H
#define PDK_GLOBAL_TYPEINFO_H

#include "pdk/global/Global.h"

namespace pdk 
{

template<typename T>
class TypeInfo
{
public:
   enum {
      isSpecilized = std::is_enum<T>::value,
      isPointer = false,
      isIntegral = std::is_integral<T>::value,
      isComplex = !isIntegral && !std::is_enum<T>::value,
      isStatic = true,
      isRelocatable = std::is_enum<T>::value,
      isLarge = (sizeof(T)>sizeof(void*)),
      sizeOf = sizeof(T)
   };
};

template<>
class TypeInfo<void>
{
public:
   enum {
      isSpecilized = true,
      isPointer = false,
      isIntegral = false,
      isComplex = false,
      isStatic = false,
      isRelocatable = false,
      isLarge = false,
      sizeOf = 0
   };
};

template<typename T>
class TypeInfo<T*>
{
public:
   enum {
      isSpecilized = true,
      isPointer = true,
      isIntegral = false,
      isComplex = false,
      isStatic = false,
      isRelocatable = true,
      isLarge = false,
      sizeOf = sizeof(T*)
   };
};

template <typename T, typename = void>
struct TypeInfoQuery : public TypeInfo<T>
{
   enum {
      isRelocatable = !TypeInfo<T>::isStatic
   };
};

template <typename T>
struct TypeInfoQuery<T, typename std::enable_if<TypeInfo<T>::isRelocatable || true>::type> 
      : public TypeInfo<T>
{};

template <class T, class T1, class T2 = T1, class T3 = T1, class T4 = T1>
class TypeInfoMerger
{
public:
   enum {
      isSpecialized = true,
      isComplex = TypeInfoQuery<T1>::isComplex || TypeInfoQuery<T2>::isComplex
      || TypeInfoQuery<T3>::isComplex || TypeInfoQuery<T4>::isComplex,
      isStatic = TypeInfoQuery<T1>::isStatic || TypeInfoQuery<T2>::isStatic
      || TypeInfoQuery<T3>::isStatic || TypeInfoQuery<T4>::isStatic,
      isRelocatable = TypeInfoQuery<T1>::isRelocatable && TypeInfoQuery<T2>::isRelocatable
      && TypeInfoQuery<T3>::isRelocatable && TypeInfoQuery<T4>::isRelocatable,
      isLarge = sizeof(T) > sizeof(void*),
      isPointer = false,
      isIntegral = false,
      sizeOf = sizeof(T)
   };
};

} // pdk

constexpr const int PDK_COMPLEX_TYPE = 0;
constexpr const int PDK_PRIMITIVE_TYPE = 0x1;
constexpr const int PDK_STATIC_TYPE = 0;
constexpr const int PDK_MOVABLE_TYPE = 0x2;
constexpr const int PDK_RELOCATABLE_TYPE = 0x4;

#define PDK_DECLARE_MOVABLE_CONTAINER(CONTAINER) \
template <typename T> \
class pdk::TypeInfo< CONTAINER<T> > \
{ \
public: \
    enum { \
        isSpecialized = true, \
        isPointer = false, \
        isIntegral = false, \
        isComplex = true, \
        isRelocatable = true, \
        isStatic = false, \
        isLarge = (sizeof(CONTAINER<T>) > sizeof(void*)), \
        sizeOf = sizeof(CONTAINER<T>) \
    }; \
}

#include <vector>
#include <list>
#include <array>
#include <queue>
#include <stack>

PDK_DECLARE_MOVABLE_CONTAINER(std::vector);
PDK_DECLARE_MOVABLE_CONTAINER(std::list);
PDK_DECLARE_MOVABLE_CONTAINER(std::stack);
PDK_DECLARE_MOVABLE_CONTAINER(std::queue);
PDK_DECLARE_MOVABLE_CONTAINER(std::priority_queue);

#undef PDK_DECLARE_MOVABLE_CONTAINER

#define PDK_DECLARE_MOVABLE_CONTAINER(CONTAINER) \
template <typename K, typename V> \
class pdk::TypeInfo< CONTAINER<K, V> > \
{ \
public: \
    enum { \
        isSpecialized = true, \
        isPointer = false, \
        isIntegral = false, \
        isComplex = true, \
        isStatic = true, \
        isRelocatable = true, \
        isLarge = (sizeof(CONTAINER<K, V>) > sizeof(void*)), \
        sizeOf = sizeof(CONTAINER<K, V>) \
    }; \
}

#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

PDK_DECLARE_MOVABLE_CONTAINER(std::set);
PDK_DECLARE_MOVABLE_CONTAINER(std::map);
PDK_DECLARE_MOVABLE_CONTAINER(std::multiset);
PDK_DECLARE_MOVABLE_CONTAINER(std::multimap);
PDK_DECLARE_MOVABLE_CONTAINER(std::unordered_set);
PDK_DECLARE_MOVABLE_CONTAINER(std::unordered_map);
PDK_DECLARE_MOVABLE_CONTAINER(std::unordered_multiset);
PDK_DECLARE_MOVABLE_CONTAINER(std::unordered_multimap);

#undef PDK_DECLARE_MOVABLE_CONTAINER

namespace pdk {

#define PDK_DECLARE_TYPEINFO_BODY(TYPE, FLAGS)\
   class ::pdk::TypeInfo<TYPE>\
{\
   public: \
   enum { \
   isSpecialized = true, \
   isComplex = (((FLAGS) & PDK_PRIMITIVE_TYPE) == 0), \
   isStatic = (((FLAGS) & (PDK_MOVABLE_TYPE | PDK_PRIMITIVE_TYPE)) == 0), \
   isRelocatable = !isStatic || ((FLAGS) & PDK_RELOCATABLE_TYPE), \
   isLarge = (sizeof(TYPE)>sizeof(void*)), \
   isPointer = false, \
   isIntegral = std::is_integral<TYPE>::value, \
   sizeOf = sizeof(TYPE) \
}; \
   static inline const char *getName() { return #TYPE; } \
}

#define PDK_DECLARE_TYPEINFO(TYPE, FLAGS)\
   template<> \
   PDK_DECLARE_TYPEINFO_BODY(TYPE, FLAGS)

template<typename T> class Flags;
template<typename T>
PDK_DECLARE_TYPEINFO_BODY(Flags<T>, PDK_PRIMITIVE_TYPE);

/*
 * Specialize a shared type with:
 * PDK_DECLARE_SHARED(type)
 
 * where 'type' is the name of the type to specialize.  NOTE: shared
 * types must define a member-swap, and be defined in the same
 * namespace as PDK for this to work.
 
 * If the type was already released without PDK_DECLARE_SHARED applied,
 * _and_ without an explicit PDK_DECLARE_TYPEINFO(type, PDK_MOVABLE_TYPE)
 */
#define PDK_DECLARE_SHARED_IMPL(TYPE, FLAGS)\
   PDK_DECLARE_TYPEINFO(TYPE, FLAGS);\
   inline void swap(TYPE &lhs, TYPE &rhs) noexcept(noexcept(lhs.swap(rhs)))\
{return lhs.swap(rhs);}

#define PDK_DECLARE_SHARED(TYPE) PDK_DECLARE_SHARED_IMPL(TYPE, PDK_MOVABLE_TYPE)

/*
 * TypeInfo primitive specializations
 */
PDK_DECLARE_TYPEINFO(bool, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(char, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(signed char, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(uchar, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(short, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(ushort, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(int, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(uint, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(long, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(ulong, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(pint64, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(puint64, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(float, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(double, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(long double, PDK_PRIMITIVE_TYPE);

PDK_DECLARE_TYPEINFO(char16_t, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(char32_t, PDK_PRIMITIVE_TYPE);

#if !defined(PDK_CC_MSVC) || defined(_NATIVE_WCHAR_T_DEFINED)
PDK_DECLARE_TYPEINFO(wchar_t, PDK_PRIMITIVE_TYPE);
#endif

} // pdk

#endif // PDK_GLOBAL_TYPEINFO_H
