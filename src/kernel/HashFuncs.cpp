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

#include <cstdlib>
#include <ctime>
#include <climits>

#include "pdk/kernel/HashFuncs.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/pal/kernel/Simd.h"
#include "pdk/global/Endian.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/lang/StringView.h"
#include "pdk/base/lang/Character.h"
#include "pdk/base/ds/BitArray.h"

namespace pdk {

using lang::String;
using lang::StringRef;
using lang::StringView;
using lang::Latin1String;
using lang::Character;
using ds::ByteArray;
using ds::BitArray;

/*
    The Java's hashing algorithm for strings is a variation of D. J. Bernstein
    hashing algorithm appeared here http://cr.yp.to/cdb/cdb.txt
    and informally known as DJB33XX - DJB's 33 Times Xor.
    Java uses DJB31XA, that is, 31 Times Add.
    
    The original algorithm was a loop around
        (h << 5) + h ^ c
    (which is indeed h*33 ^ c); it was then changed to
        (h << 5) - h ^ c
    (so h*31^c: DJB31XX), and the XOR changed to a sum:
        (h << 5) - h + c
    (DJB31XA), which can save some assembly instructions.
    
    Still, we can avoid writing the multiplication as "(h << 5) - h"
    -- the compiler will turn it into a shift and an addition anyway
    (for instance, gcc 4.4 does that even at -O0).
*/

PDK_CORE_EXPORT pdk::os::thread::BasicAtomicInt HashFuncSeed = PDK_BASIC_ATOMIC_INITIALIZER(-1);

namespace
{
#if PDK_COMPILER_SUPPORTS_HERE(SSE4_2)
inline bool hash_fast_crc32()
{
   return CPU_HAS_FEATURE(SSE4_2);
}

template <typename Char>
PDK_FUNCTION_TARGET(SSE4_2) uint crc32(const Char *ptr, size_t len, uint h)
{
   // The CRC32 instructions from Nehalem calculate a 32-bit CRC32 checksum
   const uchar *p = reinterpret_cast<const uchar *>(ptr);
   const uchar *const e = p + (len * sizeof(Char));
#  ifdef PDK_PROCESSOR_X86_64
   // The 64-bit instruction still calculates only 32-bit, but without this
   // variable GCC 4.9 still tries to clear the high bits on every loop
   pulonglong h2 = h;
   
   p += 8;
   for ( ; p <= e; p += 8)
      h2 = _mm_crc32_u64(h2, pdk::from_unaligned<plonglong>(p - 8));
   h = h2;
   p -= 8;
   
   len = e - p;
   if (len & 4) {
      h = _mm_crc32_u32(h, from_unaligned<uint>(p));
      p += 4;
   }
#  else
   p += 4;
   for ( ; p <= e; p += 4)
      h = _mm_crc32_u32(h, from_unaligned<uint>(p - 4));
   p -= 4;
   len = e - p;
#  endif
   if (len & 2) {
      h = _mm_crc32_u16(h, from_unaligned<ushort>(p));
      p += 2;
   }
   if (sizeof(Char) == 1 && len & 1)
      h = _mm_crc32_u8(h, *p);
   return h;
}
#elif defined(__ARM_FEATURE_CRC32)

inline bool has_fast_crc32()
{
   return CPU_HAS_FEATURE(CRC32);
}

template <typename Char>
PDK_FUNCTION_TARGET(CRC32) uint crc32(const Char *ptr, size_t len, uint h)
{
   // The crc32[whbd] instructions on Aarch64/Aarch32 calculate a 32-bit CRC32 checksum
   const uchar *p = reinterpret_cast<const uchar *>(ptr);
   const uchar *const e = p + (len * sizeof(Char));
   
#  ifndef __ARM_FEATURE_UNALIGNED
   if (PDK_UNLIKELY(reinterpret_cast<quintptr>(p) & 7)) {
      if ((sizeof(Char) == 1) && (reinterpret_cast<pdk::uintptr>(p) & 1) && (e - p > 0)) {
         h = __crc32b(h, *p);
         ++p;
      }
      if ((reinterpret_cast<pdk::uintptr>(p) & 2) && (e >= p + 2)) {
         h = __crc32h(h, *reinterpret_cast<const uint16_t *>(p));
         p += 2;
      }
      if ((reinterpret_cast<pdk::uintptr>(p) & 4) && (e >= p + 4)) {
         h = __crc32w(h, *reinterpret_cast<const uint32_t *>(p));
         p += 4;
      }
   }
#  endif
   
   for ( ; p + 8 <= e; p += 8)
      h = __crc32d(h, *reinterpret_cast<const uint64_t *>(p));
   
   len = e - p;
   if (len == 0)
      return h;
   if (len & 4) {
      h = __crc32w(h, *reinterpret_cast<const uint32_t *>(p));
      p += 4;
   }
   if (len & 2) {
      h = __crc32h(h, *reinterpret_cast<const uint16_t *>(p));
      p += 2;
   }
   if (sizeof(Char) == 1 && len & 1)
      h = __crc32b(h, *p);
   return h;
}
#else
inline bool has_fast_crc32()
{
   return false;
}

uint crc32(...)
{
   PDK_UNREACHABLE();
   return 0;   
}

#endif
void initialize_hash_func_seed()
{
   if (HashFuncSeed.load() == -1) {
      std::srand(std::time(nullptr));
      int seed = (std::rand() & INT_MAX);
      HashFuncSeed.testAndSetRelaxed(-1, seed);
   }
}

inline uint do_hash(const uchar *p, int length, uint seed) noexcept
{
   uint h = seed;
   if (seed && has_fast_crc32()) {
      return crc32(p, static_cast<uint>(length), h);
   }
   for (int i = 0; i < length; ++i) {
      h = 31 * h + p[i];
   }
   return h;
}

inline uint do_hash(const Character *p, int length, uint seed) noexcept
{
   uint h = seed;
   
   if (seed && has_fast_crc32()) {
      return crc32(p, static_cast<uint>(length), h);
   }
   
   for (size_t i = 0; i < static_cast<size_t>(length); ++i) {
      h = 31 * h + p[i].unicode();
   }
   return h;
}

}

uint hash_bits(const void *p, size_t length, uint seed) noexcept
{
   return do_hash(static_cast<const uchar *>(p), static_cast<int>(length), seed);
}

int retrieve_global_hash_seed()
{
   return HashFuncSeed.load();
}

void set_global_hash_seed(int newSeed)
{
   if (-1 == newSeed) {
      std::srand(std::time(nullptr));
      int seed = (std::rand() & INT_MAX);
      HashFuncSeed.store(seed);
   } else {
      HashFuncSeed.store(newSeed & INT_MAX);
   }
}

constexpr uint pdk_hash(float key, uint seed) noexcept
{
   return key != 0.0f ? do_hash(reinterpret_cast<const uchar *>(&key), sizeof(key), seed) : seed;
}

constexpr uint pdk_hash(double key, uint seed) noexcept
{
   return key != 0.0  ? do_hash(reinterpret_cast<const uchar *>(&key), sizeof(key), seed) : seed;
}

#ifndef PDK_OS_DARWIN

uint pdk_hash(long double key, uint seed) noexcept
{
   return key != 0.0L ? do_hash(reinterpret_cast<const uchar *>(&key), sizeof(key), seed) : seed;
}

#endif

uint pdk_hash(const ByteArray &key, uint seed) noexcept
{
   return do_hash(reinterpret_cast<const uchar *>(key.getConstRawData()), size_t(key.size()), seed);
}

#if PDK_STRINGVIEW_LEVEL < 2
uint pdk_hash(const String &key, uint seed) noexcept
{
   return do_hash(key.unicode(), size_t(key.size()), seed);
}

uint pdk_hash(const StringRef &key, uint seed) noexcept
{
   return do_hash(key.unicode(), size_t(key.size()), seed);
}
#endif

uint pdk_hash(StringView key, uint seed) noexcept
{
   return do_hash(key.data(), key.size(), seed);
}

uint pdk_hash(const BitArray &bitArray, uint seed) noexcept
{
   int m = bitArray.m_data.size() - 1;
   uint result = do_hash(reinterpret_cast<const uchar *>(bitArray.m_data.getConstRawData()),
                         size_t(std::max(0, m)), seed);
   
   // deal with the last 0 to 7 bits manually, because we can't trust that
   // the padding is initialized to 0 in bitArray.d
   int n = bitArray.size();
   if (n & 0x7) {
      result = ((result << 4) + bitArray.m_data.at(m)) & ((1 << n) - 1);
   }
   return result;
}

uint pdk_hash(Latin1String key, uint seed) noexcept
{
   return do_hash(reinterpret_cast<const uchar *>(key.getRawData()), size_t(key.size()), seed);
}

/*!
  
    Private copy of the implementation of the Qt 4 qHash algorithm for strings,
    (that is, QChar-based arrays, so all String-like classes),
    to be used wherever the result is somehow stored or reused across multiple
    Qt versions. The public qHash implementation can change at any time,
    therefore one must not rely on the fact that it will always give the same
    results.
    
    The pdk_hash functions must *never* change their results.
    
    This function can hash discontiguous memory by invoking it on each chunk,
    passing the previous's result in the next call's \a chained argument.
*/
uint pdk_internal_hash(StringView key, uint chained) noexcept
{
   auto n = key.size();
   auto p = key.utf16();
   
   uint h = chained;
   while (n--) {
      h = (h << 4) + *p++;
      h ^= (h & 0xf0000000) >> 23;
      h &= 0x0fffffff;
   }
   return h;
}

} // pdk
