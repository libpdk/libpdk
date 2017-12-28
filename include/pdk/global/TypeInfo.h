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
      isPointer = false,
      isIntegral = std::is_integral<T>::value,
      isComplex = true,
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

}

constexpr int PDK_COMPLEX_TYPE = 0;
constexpr int PDK_PRIMITIVE_TYPE = 0x1;
constexpr int PDK_STATIC_TYPE = 0;
constexpr int PDK_MOVABLE_TYPE = 0x2;
constexpr int PDK_RELOCATABLE_TYPE = 0x4;

namespace pdk {

#define PDK_DECLARE_TYPEINFO_BODY(TYPE, FLAGS)\
   class pdk::TypeInfo<TYPE>\
{\
   public: \
   enum { \
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
#ifndef PDK_OS_DARWIN
PDK_DECLARE_TYPEINFO(long double, PDK_PRIMITIVE_TYPE);
#endif


} // pdk

#endif // PDK_GLOBAL_TYPEINFO_H
