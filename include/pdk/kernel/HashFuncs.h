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
// Created by softboy on 2017/12/30.

#ifndef PDK_KERNEL_HASH_FUNCS_H
#define PDK_KERNEL_HASH_FUNCS_H

#include "pdk/global/Global.h"
#include "pdk/base/lang/Character.h"
#include <numeric>
#include <utility>

#if defined(PDK_CC_MSVC)
#pragma warning(push)
#pragma warning(disable : 4311) // disable pointer truncation warning
#pragma warning(disable : 4127) // conditional expression is constant
#endif

namespace pdk {

// forward declare class with namespace
namespace ds {
class ByteArray;
} // ds

PDK_CORE_EXPORT int retrieve_global_hash_seed();
PDK_CORE_EXPORT void set_global_hash_seed(int newSeed);

PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION uint hash_bits(const void *p, size_t length, uint seed = 0) noexcept;

PDK_DECL_CONST_FUNCTION constexpr inline uint hash(char key, uint seed = 0) noexcept
{
   return static_cast<uint>(key) ^ seed;
}

PDK_DECL_CONST_FUNCTION constexpr inline uint hash(uchar key, uint seed = 0) noexcept
{
   return static_cast<uint>(key) ^ seed;
}

PDK_DECL_CONST_FUNCTION constexpr inline uint hash(signed char key, uint seed = 0) noexcept
{
   return static_cast<uint>(key) ^ seed;
}

PDK_DECL_CONST_FUNCTION constexpr inline uint hash(short key, uint seed = 0) noexcept
{
   return static_cast<uint>(key) ^ seed;
}

PDK_DECL_CONST_FUNCTION constexpr inline uint hash(ushort key, uint seed = 0) noexcept
{
   return static_cast<uint>(key) ^ seed;
}

PDK_DECL_CONST_FUNCTION constexpr inline uint hash(int key, uint seed = 0) noexcept
{
   return static_cast<uint>(key) ^ seed;
}

PDK_DECL_CONST_FUNCTION constexpr inline uint hash(uint key, uint seed = 0) noexcept
{
   return key ^ seed;
}

PDK_DECL_CONST_FUNCTION constexpr inline uint hash(ulong key, uint seed = 0) noexcept
{
   return (sizeof(ulong) > sizeof(uint))
         ? (static_cast<uint>(((key >> (8 * sizeof(uint) - 1)) ^ key) & (~0U)) ^ seed)
         : (static_cast<uint>(key & (~0U)) ^ seed);
}

PDK_DECL_CONST_FUNCTION constexpr inline uint hash(long key, uint seed = 0) noexcept
{
   return hash(static_cast<ulong>(key), seed);
}

PDK_DECL_CONST_FUNCTION constexpr inline uint hash(puint64 key, uint seed = 0) noexcept
{
   return static_cast<uint>(((key >> (8 * sizeof(uint) - 1)) ^ key) & (~0U)) ^ seed;
}

PDK_DECL_CONST_FUNCTION constexpr inline uint hash(pint64 key, uint seed = 0) noexcept
{
   return hash(static_cast<puint64>(key), seed);;
}

PDK_DECL_CONST_FUNCTION constexpr inline uint hash(float key, uint seed = 0) noexcept;
PDK_DECL_CONST_FUNCTION constexpr inline uint hash(double key, uint seed = 0) noexcept;

#ifndef PDK_OS_DARWIN
PDK_DECL_CONST_FUNCTION constexpr inline uint hash(long double key, uint seed = 0) noexcept;
#endif

PDK_DECL_CONST_FUNCTION constexpr inline uint hash(lang::Character key, uint seed = 0) noexcept
{
   return hash(key.unicode(), seed);;
}

PDK_CORE_EXPORT PDK_DECL_PURE_FUNCTION uint hash(const ds::ByteArray &key, uint seed = 0) noexcept;

template <typename T>
inline uint hash(const T *key, uint seed = 0) noexcept
{
   return hash(reinterpret_cast<pdk::uintptr>(key), seed);
}

template <typename T>
inline uint hash(const T &key, uint seed) noexcept(noexcept(hash(key)))
{
   return hash(key) ^ seed;
}

namespace internal {

struct HashCombine
{
   using result_type = uint;
   template <typename T>
   // combiner taken from N3876 / boost::hash_combine
   constexpr result_type operator()(const T &value, uint seed) const noexcept(noexcept(hash(value)))
   {
      return seed ^ (hash(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
   }
};

struct HashCombineCommutative
{
   // HashCombine is a good hash combiner, but is not commutative,
   // ie. it depends on the order of the input elements. That is
   // usually what we want: {0,1,3} should hash differently than
   // {1,3,0}. Except when it isn't. Therefore, provide a commutative 
   // combiner, too.
   using result_type = uint;
   template <typename T>
   constexpr result_type operator()(const T &value, uint seed) const noexcept(noexcept(hash(value)))
   {
      return hash(value) + seed;
   }
};

} // internal


template <typename InputIterator>
inline uint hash_range(InputIterator first, InputIterator last, uint seed = 0) noexcept(noexcept(hash(*first)))
{
   return std::accumulate(first, last, seed, internal::HashCombine());
}

template <typename InputIterator>
inline uint hash_range_commutative(InputIterator first, InputIterator last, uint seed = 0) noexcept(noexcept(hash(*first)))
{
   return std::accumulate(first, last, seed, internal::HashCombineCommutative());
}

template <typename T1, typename T2>
inline uint hash(const std::pair<T1, T2> &key, uint seed = 0)
noexcept(noexcept(hash(key.first, seed)) && noexcept(hash(key.first, seed)))
{
   internal::HashCombine hash;
   seed = hash(seed, key.first);
   seed = hash(seed, key.second);
   return seed;
}

} // pdk

#if defined(PDK_CC_MSVC)
#pragma warning(pop)
#endif

#endif // PDK_KERNEL_HASH_FUNCS_H
