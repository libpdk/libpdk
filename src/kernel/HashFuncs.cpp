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

namespace pdk {

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

inline uint hash(const uchar *p, int length, uint seed) noexcept
{
   uint h = seed;
   if (has_fast_crc32()) {
      return crc32(p, static_cast<uint>(length), h);
   }
   for (int i = 0; i < length; ++i) {
      h = 31 * h + p[i];
   }
   return h;
}

}

uint hash_bits(const void *p, size_t length, uint seed) noexcept
{
   return hash(static_cast<const uchar *>(p), static_cast<int>(length), seed);
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

constexpr uint hash(float key, uint seed) noexcept
{
   return key != 0.0f ? hash(reinterpret_cast<const uchar *>(&key), sizeof(key), seed) : seed;
}

constexpr uint hash(double key, uint seed) noexcept
{
   return key != 0.0  ? hash(reinterpret_cast<const uchar *>(&key), sizeof(key), seed) : seed;
}

#ifndef PDK_OS_DARWIN

uint hash(long double key, uint seed) noexcept
{
   return key != 0.0L ? hash(reinterpret_cast<const uchar *>(&key), sizeof(key), seed) : seed;
}

#endif

} // pdk
