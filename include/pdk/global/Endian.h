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
// Created by softboy on 2017/12/25.

#ifndef PDK_GLOBAL_ENDIAN_H
#define PDK_GLOBAL_ENDIAN_H

#include "pdk/global/Global.h"

namespace pdk
{

// Used to implement a type-safe and alignment-safe copy operation
// If you want to avoid the memcpy, you must write specializations for these functions
template <typename T>
PDK_ALWAYS_INLINE void to_unaligned(void *dest, const T src)
{
   // Using sizeof(T) inside memcpy function produces internal compiler error with
   // MSVC2008/ARM in tst_endian -> use extra indirection to resolve size of T.
   const size_t size = sizeof(T);
#if PDK_HAS_BUILTIN(__builtin_memcpy)
   __builtin_memcpy
      #else
   std::memcpy
      #endif
         (dest, &src, size);
}

template <typename T>
PDK_ALWAYS_INLINE T from_unaligned(const void *src)
{
   T dest;
   const size_t size = sizeof(T);
#if PDK_HAS_BUILTIN(__builtin_memcpy)
   __builtin_memcpy
      #else
   std::memcpy
      #endif
         (&dest, src, size);
   return dest;
}

/*
 * T bswap(T source).
 * Changes the byte order of a value from big endian to little endian or vice versa.
 * This function can be used if you are not concerned about alignment issues,
 * and it is therefore a bit more convenient and in most cases more efficient.
*/
template <typename T>
constexpr T bswap(T source);

// These definitions are written so that they are recognized by most compilers
// as bswap and replaced with single instruction builtins if available.
template <>
inline constexpr puint64 bswap<puint64>(puint64 source)
{
   return 0
         | ((source & PDK_UINT64_C(0x00000000000000ff)) << 56)
         | ((source & PDK_UINT64_C(0x000000000000ff00)) << 40)
         | ((source & PDK_UINT64_C(0x0000000000ff0000)) << 24)
         | ((source & PDK_UINT64_C(0x00000000ff000000)) << 8)
         | ((source & PDK_UINT64_C(0x000000ff00000000)) >> 8)
         | ((source & PDK_UINT64_C(0x0000ff0000000000)) >> 24)
         | ((source & PDK_UINT64_C(0x00ff000000000000)) >> 40)
         | ((source & PDK_UINT64_C(0xff00000000000000)) >> 56);
}

template <>
inline constexpr puint32 bswap< puint32>(puint32 source)
{
   return 0
         | ((source & 0x000000ff) << 24)
         | ((source & 0x0000ff00) << 8)
         | ((source & 0x00ff0000) >> 8)
         | ((source & 0xff000000) >> 24);
}

template <>
inline constexpr puint16 bswap<puint16>(puint16 source)
{
   return puint16( 0
                        | ((source & 0x00ff) << 8)
                        | ((source & 0xff00) >> 8) );
}

template <>
inline constexpr puint8 bswap<puint8>(puint8 source)
{
    return source;
}

// signed specializations
template <>
inline constexpr pint64 bswap<pint64>(pint64 source)
{
    return bswap<puint64>(static_cast<puint64>(source));
}

template <>
inline constexpr pint32 bswap<pint32>(pint32 source)
{
    return bswap<puint32>(static_cast<puint32>(source));
}

template <>
inline constexpr pint16 bswap<pint16>(pint16 source)
{
    return bswap<puint16>(static_cast<puint16>(source));
}

template <>
inline constexpr pint8 bswap<pint8>(pint8 source)
{
    return source;
}

/*
 * bswap(const T src, const void *dest);
 * Changes the byte order of \a src from big endian to little endian or vice versa
 * and stores the result in \a dest.
 * There is no alignment requirements for \a dest.
*/
template <typename T>
inline void bswap(const T src, void *dest)
{
    to_unaligned<T>(bswap<T>(src), dest);
}

#if PDK_BYTE_ORDER == PDK_BIG_ENDIAN

template <typename T>
inline constexpr T to_big_endian(T source)
{
   return source;
}

template <typename T>
inline constexpr T from_big_endian(T source)
{
   return source;
}

template <typename T>
inline constexpr T to_little_endian(T source)
{
   return bswap<T>(source);
}

template <typename T>
inline constexpr T from_little_endian(T source)
{
   return bswap<T>(source);
}

template <typename T>
inline void to_big_endian(T src, void *dest)
{
   to_unaligned<T>(src, dest);
}

template <typename T>
inline void to_little_endian(T src, void *dest)
{
   bswap<T>(src, dest);
}

#else // PDK_LITTLE_ENDIAN

template <typename T>
inline constexpr T to_big_endian(T source)
{
   return bswap<T>(source);
}

template <typename T>
inline constexpr T from_big_endian(T source)
{
   return bswap<T>(source);
}

template <typename T>
inline constexpr T to_little_endian(T source)
{
   return source;
}

template <typename T>
inline constexpr T from_little_endian(T source)
{
   return source;
}

template <typename T>
inline void to_big_endian(T src, void *dest)
{
   bswap<T>(src, dest);
}

template <typename T>
inline void to_little_endian(T src, void *dest)
{
   to_unaligned<T>(src, dest);
}

#endif // PDK_BYTE_ORDER == PDK_BIG_ENDIAN

/* T from_little_endian(const void *src)
 * This function will read a little-endian encoded value from \a src
 * and return the value in host-endian encoding.
 * There is no requirement that \a src must be aligned.
*/
template <typename T> inline T from_little_endian(const void *src)
{
    return from_little_endian(from_unaligned<T>(src));
}

template <>
inline puint8 from_little_endian<puint8>(const void *src)
{
   return static_cast<const puint8 *>(src)[0];
}

template <>
inline pint8 from_little_endian<pint8>(const void *src)
{
   return static_cast<const pint8 *>(src)[0];
}

/* This function will read a big-endian (also known as network order) encoded value from \a src
 * and return the value in host-endian encoding.
 * There is no requirement that \a src must be aligned.
*/
template <typename T>
inline T from_big_endian(const void *src)
{
    return from_big_endian(from_unaligned<T>(src));
}

template <>
inline puint8 from_big_endian<puint8>(const void *src)
{
   return static_cast<const puint8 *>(src)[0];
}

template <>
inline pint8 from_big_endian<pint8>(const void *src)
{
   return static_cast<const pint8 *>(src)[0];
}

} // pdk

#endif // PDK_GLOBAL_ENDIAN_H
