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
// Created by softboy on 2018/02/07.

#include "pdk/global/Random.h"
#include "pdk/global/internal/RandomPrivate.h"
#include "pdk/base/os/thread/ThreadStorage.h"
#include "pdk/kernel/DeadlineTimer.h"
#include "pdk/global/Logging.h"
#include "pdk/stdext/typetraits/FunctionTraits.h"
#include "pdk/global/GlobalStatic.h"
#include <errno.h>

#if PDK_CONFIG(getentropy)
#  include <sys/random.h>
#elif !defined(PDK_OS_BSD4) && !defined(PDK_OS_WIN)
#include "pdk/kernel/DeadlineTimer.h"
#include "pdk/kernel/HashFuncs.h"
#  if PDK_CONFIG(getauxval)
#     include <sys/auxv.h>
#  endif
#endif // PDK_CONFIG(getentropy)

#ifdef PDK_OS_UNIX
#  include <fcntl.h>
#  include "pdk/kernel/CoreUnix.h"
#else
#  include "pdk/global/Windows.h"

// RtlGenRandom is not exported by its name in advapi32.dll, but as SystemFunction036
// See https://msdn.microsoft.com/en-us/library/windows/desktop/aa387694(v=vs.85).aspx
// Implementation inspired on https://hg.mozilla.org/mozilla-central/file/722fdbff1efc/security/nss/lib/freebl/win_rand.c#l146
// Argument why this is safe to use: https://bugzilla.mozilla.org/show_bug.cgi?id=504270
extern "C" {
DECLSPEC_IMPORT BOOLEAN WINAPI SystemFunction036(PVOID RandomBuffer, ULONG RandomBufferLength);
}
#endif

// This file is too low-level for regular PDK_ASSERT (the logging framework may
// recurse back), so use regular assert()
#undef NDEBUG
#undef PDK_ASSERT_X
#undef PDK_ASSERT
#define PDK_ASSERT(cond) assert(cond)
#define PDK_ASSERT_X(cond, x, msg) assert(cond && msg)
#if defined(PDK_NO_DEBUG) && !defined(PDK_FORCE_ASSERTS)
#  define NDEBUG    1
#endif
#include <assert.h>

namespace pdk {

using pdk::os::thread::BasicAtomicInt;
using pdk::os::thread::BasicAtomicInteger;
using pdk::kernel::DeadlineTimer;
using pdk::os::thread::ThreadStorage;

namespace {
#if defined(PDK_PROCESSOR_X86) && PDK_COMPILER_SUPPORTS_HERE(RDRND)
pdk::sizetype pdk_random_cpu(void *buffer, pdk::sizetype count) noexcept;

#  ifdef PDK_PROCESSOR_X86_64
#    define _rdrandXX_step _rdrand64_step
#  else
#    define _rdrandXX_step _rdrand32_step
#  endif

PDK_FUNCTION_TARGET(RDRND) pdk::sizetype pdk_random_cpu(void *buffer, pdk::sizetype count) noexcept
{
   unsigned *ptr = reinterpret_cast<unsigned *>(buffer);
   unsigned *end = ptr + count;
   
   while (ptr + sizeof(pdk::registeruint)/sizeof(*ptr) <= end) {
      if (_rdrandXX_step(reinterpret_cast<pdk::registeruint *>(ptr)) == 0) {
         goto out;
      }
      ptr += sizeof(pdk::registeruint)/sizeof(*ptr);
   }
   
   if (sizeof(*ptr) != sizeof(pdk::registeruint) && ptr != end) {
      if (_rdrand32_step(ptr)) {
         goto out;
      }
      ++ptr;
   }
out:
   return ptr - reinterpret_cast<unsigned *>(buffer);
}
#else
pdk::sizetype pdk_random_cpu(void *, pdk::sizetype)
{
   return 0;
}
#endif
} // anonymous namespace

enum {
   // may be "overridden" by a member enum
   FillBufferNoexcept = true
};

struct RandomGenerator::SystemGenerator
{
#if PDK_CONFIG(getentropy)
   static pdk::sizetype fillBuffer(void *buffer, pdk::sizetype count) noexcept
   {
      // getentropy can read at most 256 bytes, so break the reading
      pdk::sizetype read = 0;
      while (count - read > 256) {
         // getentropy can't fail under normal circumstances
         int ret = getentropy(reinterpret_cast<uchar *>(buffer) + read, 256);
         PDK_ASSERT(ret == 0);
         PDK_UNUSED(ret);
         read += 256;
      }
      
      int ret = getentropy(reinterpret_cast<uchar *>(buffer) + read, count - read);
      PDK_ASSERT(ret == 0);
      PDK_UNUSED(ret);
      return count;
   }
   
#elif defined(PDK_OS_UNIX)
   enum { FillBufferNoexcept = false };
   
   BasicAtomicInt fdp1;   // "file descriptor plus 1"
   int openDevice()
   {
      int fd = fdp1.loadAcquire() - 1;
      if (fd != -1) {
         return fd;
      }
      fd = pdk::kernel::safe_open("/dev/urandom", O_RDONLY);
      if (fd == -1) {
         fd = pdk::kernel::safe_open("/dev/random", O_RDONLY | O_NONBLOCK);
      }
      if (fd == -1) {
         // failed on both, set to -2 so we won't try again
         fd = -2;
      }
      
      int opened_fdp1;
      if (fdp1.testAndSetOrdered(0, fd + 1, opened_fdp1)) {
         return fd;
      }
      // failed, another thread has opened the file descriptor
      if (fd >= 0) {
         pdk::kernel::safe_close(fd);
      }
      return opened_fdp1 - 1;
   }
   
#ifdef PDK_CC_GNU
   // If it's not GCC or GCC-like, then we'll leak the file descriptor
   __attribute__((destructor))
#endif
   static void closeDevice()
   {
      int fd = self().fdp1.load() - 1;
      if (fd >= 0) {
         pdk::kernel::safe_close(fd);
      }
   }
   
   constexpr SystemGenerator()
      : fdp1 PDK_BASIC_ATOMIC_INITIALIZER(0)
   {}
   
   pdk::sizetype fillBuffer(void *buffer, pdk::sizetype count)
   {
      int fd = openDevice();
      if (PDK_UNLIKELY(fd < 0)) {
         return 0;
      }
      pdk::pint64 n = pdk::kernel::safe_read(fd, buffer, count);
      return std::max<pdk::sizetype>(n, 0);        // ignore any errors
   }
   
#elif defined(PDK_OS_WIN)
   pdk::sizetype fillBuffer(void *buffer, pdk::sizetype count) noexcept
   {
      auto RtlGenRandom = SystemFunction036;
      return RtlGenRandom(buffer, ULONG(count)) ? count: 0;
   }
#endif // PDK_OS_WIN
   
   static SystemGenerator &self();
   void generate(pdk::puint32 *begin, pdk::puint32 *end) noexcept(FillBufferNoexcept);
   
   // For std::mersenne_twister_engine implementations that use something
   // other than pdk::puint32 (unsigned int) to fill their buffers.
   template <typename T> void generate(T *begin, T *end)
   {
      PDK_STATIC_ASSERT(sizeof(T) >= sizeof(pdk::puint32));
      if (sizeof(T) == sizeof(pdk::puint32)) {
         // Microsoft Visual Studio uses unsigned long, but that's still 32-bit
         generate(reinterpret_cast<pdk::puint32 *>(begin), reinterpret_cast<pdk::puint32 *>(end));
      } else {
         // Slow path. Fix your C++ library.
         std::generate(begin, end, [this]() {
            pdk::puint32 datum;
            generate(&datum, &datum + 1);
            return datum;
         });
      }
   }
};

namespace {
#if defined(PDK_OS_WIN)
void fallback_update_seed(unsigned) {}
void fallback_fill(pdk::puint32 *ptr, pdk::sizetype left) noexcept
{
   // on Windows, rand_s is a high-quality random number generator
   // and it requires no seeding
   std::generate(ptr, ptr + left, []() {
      unsigned value;
      rand_s(&value);
      return value;
   });
}
#elif PDK_CONFIG(getentropy)
void fallback_update_seed(unsigned) {}
void fallback_fill(pdk::puint32 *, pdk::sizetype) noexcept
{
   // no fallback necessary, getentropy cannot fail under normal circumstances
   PDK_UNREACHABLE();
}
#elif defined(PDK_OS_BSD4)
void fallback_update_seed(unsigned) {}
void fallback_fill(pdk::puint32 *ptr, pdk::sizetype left) noexcept
{
   // BSDs have arc4random(4) and these work even in chroot(2)
   arc4random_buf(ptr, left * sizeof(*ptr));
}
#else
static BasicAtomicInteger<unsigned> sg_seed = PDK_BASIC_ATOMIC_INITIALIZER(0U);
void fallback_update_seed(unsigned value)
{
   // Update the seed to be used for the fallback mechansim, if we need to.
   // We can't use QtPrivate::QHashCombine here because that is not an atomic
   // operation. A simple XOR will have to do then.
   sg_seed.fetchAndXorRelaxed(value);
}

PDK_NEVER_INLINE
#ifdef PDK_CC_GNU
__attribute__((cold))   // this function is pretty big, so optimize for size
#endif
void fallback_fill(pdk::puint32 *ptr, pdk::sizetype left) noexcept
{
   pdk::puint32 scratch[12];    // see element count below
   pdk::puint32 *end = scratch;
   
   auto foldPointer = [](pdk::uintptr v) {
      if (sizeof(pdk::uintptr) == sizeof(pdk::puint32)) {
         // For 32-bit systems, we simply return the pointer.
         return pdk::puint32(v);
      } else {
         // For 64-bit systems, we try to return the variable part of the
         // pointer. On current x86-64 and AArch64, the top 17 bits are
         // architecturally required to be the same, but in reality the top
         // 24 bits on Linux are likely to be the same for all processes.
         return pdk::puint32(v >> (32 - 24));
      }
   };
   
   PDK_ASSERT(left);
   
   *end++ = foldPointer(pdk::uintptr(&seed));          // 1: variable in this library/executable's .data
   *end++ = foldPointer(pdk::uintptr(&scratch));       // 2: variable in the stack
   *end++ = foldPointer(pdk::uintptr(&errno));         // 3: veriable either in libc or thread-specific
   *end++ = foldPointer(pdk::uintptr(reinterpret_cast<void*>(strerror)));   // 4: function in libc (and unlikely to be a macro)
   
   pdk::puint64 nsecs = DeadlineTimer::getCurrent(pdk::TimerType::PreciseTimer).getDeadline();
   *end++ = pdk::puint32(nsecs);    // 5
   
   if (pdk::puint32 v = sg_seed.load()) {
      *end++ = v; // 6
   }
#if PDK_CONFIG(getauxval)
   // works on Linux -- all modern libc have getauxval
#  ifdef AT_RANDOM
   // ELF's auxv AT_RANDOM has 16 random bytes
   // (other ELF-based systems don't seem to have AT_RANDOM)
   ulong auxvSeed = getauxval(AT_RANDOM);
   if (auxvSeed) {
      memcpy(end, reinterpret_cast<void *>(auxvSeed), 16);
      end += 4;   // 7 to 10
   }
#  endif
   
   // Both AT_BASE and AT_SYSINFO_EHDR have some randomness in them due to the
   // system's ASLR, even if many bits are the same. They also have randomness
   // between them.
#  ifdef AT_BASE
   // present at least on the BSDs too, indicates the address of the loader
   ulong base = getauxval(AT_BASE);
   if (base)
      *end++ = foldPointer(base); // 11
#  endif
#  ifdef AT_SYSINFO_EHDR
   // seems to be Linux-only, indicates the global page of the sysinfo
   ulong sysinfo_ehdr = getauxval(AT_SYSINFO_EHDR);
   if (sysinfo_ehdr)
      *end++ = foldPointer(sysinfo_ehdr); // 12
#  endif
#endif
   
   PDK_ASSERT(end <= std::end(scratch));
   
   // this is highly inefficient, we should save the generator across calls...
   std::seed_seq sseq(scratch, end);
   std::mt19937 generator(sseq);
   std::generate(ptr, ptr + left, generator);
   
   fallback_update_seed(*ptr);
}
#endif
} // anonymous namespace

using internal::RandomGeneratorControl;
using internal::RNGType;

PDK_NEVER_INLINE void RandomGenerator::SystemGenerator::generate(pdk::puint32 *begin, pdk::puint32 *end)
noexcept(FillBufferNoexcept)
{
   pdk::puint32 *buffer = begin;
   pdk::sizetype count = end - begin;
   
   if (PDK_UNLIKELY(uint(internal::sg_randomdeviceControl) & pdk::as_integer<RandomGeneratorControl>(RandomGeneratorControl::SetRandomData))) {
      uint value = uint(internal::sg_randomdeviceControl) & pdk::as_integer<RandomGeneratorControl>(RandomGeneratorControl::RandomDataMask);
      std::fill_n(buffer, count, value);
      return;
   }
   
   pdk::sizetype filled = 0;
   if (internal::pdk_has_hwrng() && (uint(internal::sg_randomdeviceControl) & 
                                     pdk::as_integer<RandomGeneratorControl>(RandomGeneratorControl::SkipHWRNG)) == 0)
      filled += pdk_random_cpu(buffer, count);
   
   if (filled != count && (uint(internal::sg_randomdeviceControl) &
                           pdk::as_integer<RandomGeneratorControl>(RandomGeneratorControl::SkipSystemRNG)) == 0) {
      pdk::sizetype bytesFilled =
            fillBuffer(buffer + filled, (count - filled) * pdk::sizetype(sizeof(*buffer)));
      filled += bytesFilled / pdk::sizetype(sizeof(*buffer));
   }
   if (filled) {
      fallback_update_seed(*buffer);
   }
   if (PDK_UNLIKELY(filled != count)) {
      // failed to fill the entire buffer, try the faillback mechanism
      fallback_fill(buffer + filled, count - filled);
   }
}

struct RandomGenerator::SystemAndGlobalGenerators
{
   // Construction notes:
   // 1) The global PRNG state is in a different cacheline compared to the
   //    mutex that protects it. This avoids any false cacheline sharing of
   //    the state in case another thread tries to lock the mutex. It's not
   //    a common scenario, but since sizeof(RandomGenerator) >= 2560, the
   //    overhead is actually acceptable.
   // 2) We use both PDK_DECL_ALIGN and std::aligned_storage<..., 64> because
   //    some implementations of std::aligned_storage can't align to more
   //    than a primitive type's alignment.
   // 3) We don't store the entire system RandomGenerator, only the space
   //    used by the RandomGenerator::type member. This is fine because we
   //    (ab)use the common initial sequence exclusion to aliasing rules.
   std::mutex m_globalPRNGMutex;
   struct ShortenedSystem { uint m_type; } m_system;
   RandomGenerator::SystemGenerator m_sys;
   PDK_DECL_ALIGN(64) std::aligned_storage<sizeof(RandomGenerator64), 64>::type m_global;
   
   constexpr SystemAndGlobalGenerators()
      : m_globalPRNGMutex{},
        m_system{0},
        m_sys{},
        m_global{}
   {}
   
   void confirmLiteral()
   {
      //#if defined(PDK_COMPILER_CONSTEXPR) && !defined(PDK_CC_MSVC) && !defined(PDK_OS_INTEGRITY)
      //      // Currently fails to compile with MSVC 2017, saying std::mutex is not
      //      // a literal type. Disassembly with MSVC 2013 and 2015 shows it is
      //      // actually a literal; MSVC 2017 has a bug relating to this, so we're
      //      // withhold judgement for now.  Integrity's compiler is unable to
      //      // guarantee g's alignment for some reason.
      
      //      constexpr SystemAndGlobalGenerators g = {};
      //      PDK_UNUSED(g);
      //      PDK_STATIC_ASSERT(std::is_literal_type<SystemAndGlobalGenerators>::value);
      //#endif
   }
   
   static SystemAndGlobalGenerators *self()
   {
      static SystemAndGlobalGenerators g;
      PDK_STATIC_ASSERT(sizeof(g) > sizeof(RandomGenerator64));
      return &g;
   }
   
   static RandomGenerator64 *system()
   {
      // Though we never call the constructor, the system RandomGenerator is
      // properly initialized by the zero initialization performed in self().
      // Though RandomGenerator is has non-vacuous initialization, we
      // consider it initialized because of the common initial sequence.
      return reinterpret_cast<RandomGenerator64 *>(&self()->m_system);
   }
   
   static RandomGenerator64 *globalNoInit()
   {
      // This function returns the pointer to the global RandomGenerator,
      // but does not initialize it. Only call it directly if you meant to do
      // a pointer comparison.
      return reinterpret_cast<RandomGenerator64 *>(&self()->m_global);
   }
   
   static void securelySeed(RandomGenerator *rng)
   {
      // force reconstruction, just to be pedantic
      new (rng) RandomGenerator{RandomGenerator::System{}};
      
      rng->m_type = pdk::as_integer<RNGType>(RNGType::MersenneTwister);
      new (&rng->m_storage.engine()) RandomGenerator::RandomEngine(self()->m_sys);
   }
   
   struct PRNGLocker {
      const bool locked;
      PRNGLocker(const RandomGenerator *that)
         : locked(that == globalNoInit())
      {
         if (locked) {
            self()->m_globalPRNGMutex.lock();
         }
      }
      ~PRNGLocker()
      {
         if (locked) {
            self()->m_globalPRNGMutex.unlock();
         }
      }
   };
};

inline RandomGenerator::SystemGenerator &RandomGenerator::SystemGenerator::self()
{
   return SystemAndGlobalGenerators::self()->m_sys;
}

constexpr RandomGenerator::Storage::Storage()
   : m_dummy(0)
{
   // nothing
}

inline RandomGenerator64::RandomGenerator64(System s)
   : RandomGenerator(s)
{
}

RandomGenerator64 *RandomGenerator64::system()
{
   auto self = SystemAndGlobalGenerators::system();
   PDK_ASSERT(self->m_type == pdk::as_integer<RNGType>(RNGType::SystemRNG));
   return self;
}

RandomGenerator64 *RandomGenerator64::global()
{
   auto self = SystemAndGlobalGenerators::globalNoInit();
   
   // Yes, this is a double-checked lock.
   // We can return even if the type is not completely initialized yet:
   // any thread trying to actually use the contents of the random engine
   // will necessarily wait on the lock.
   if (PDK_LIKELY(self->m_type !=  pdk::as_integer<RNGType>(RNGType::SystemRNG))) {
      return self;
   }
   
   SystemAndGlobalGenerators::PRNGLocker locker(self);
   if (self->m_type == pdk::as_integer<RNGType>(RNGType::SystemRNG)) {
      SystemAndGlobalGenerators::securelySeed(self);
   }
   return self;
}

RandomGenerator64 RandomGenerator64::securelySeeded()
{
   RandomGenerator64 result(System{});
   SystemAndGlobalGenerators::securelySeed(&result);
   return result;
}

inline RandomGenerator::RandomGenerator(System)
   : m_type(pdk::as_integer<RNGType>(RNGType::SystemRNG))
{
   // don't touch storage
}

RandomGenerator::RandomGenerator(const RandomGenerator &other)
   : m_type(other.m_type)
{
   PDK_ASSERT(this != system());
   PDK_ASSERT(this != SystemAndGlobalGenerators::globalNoInit());
   
   if (m_type != pdk::as_integer<RNGType>(RNGType::SystemRNG)) {
      SystemAndGlobalGenerators::PRNGLocker lock(&other);
      m_storage.engine() = other.m_storage.engine();
   }
}

RandomGenerator &RandomGenerator::operator=(const RandomGenerator &other)
{
   if (PDK_UNLIKELY(this == system()) || PDK_UNLIKELY(this == SystemAndGlobalGenerators::globalNoInit()))
      fatal_stream("Attempted to overwrite a RandomGenerator to system() or global().");
   
   if ((m_type = other.m_type) != pdk::as_integer<RNGType>(RNGType::SystemRNG)) {
      SystemAndGlobalGenerators::PRNGLocker lock(&other);
      m_storage.engine() = other.m_storage.engine();
   }
   return *this;
}

RandomGenerator::RandomGenerator(std::seed_seq &sseq) noexcept
   : m_type(pdk::as_integer<RNGType>(RNGType::MersenneTwister))
{
   PDK_ASSERT(this != system());
   PDK_ASSERT(this != SystemAndGlobalGenerators::globalNoInit());
   
   new (&m_storage.engine()) RandomEngine(sseq);
}

RandomGenerator::RandomGenerator(const pdk::puint32 *begin, const pdk::puint32 *end)
   : m_type(pdk::as_integer<RNGType>(RNGType::MersenneTwister))
{
   PDK_ASSERT(this != system());
   PDK_ASSERT(this != SystemAndGlobalGenerators::globalNoInit());
   
   std::seed_seq s(begin, end);
   new (&m_storage.engine()) RandomEngine(s);
}

void RandomGenerator::discard(unsigned long long z)
{
   if (PDK_UNLIKELY(m_type == pdk::as_integer<RNGType>(RNGType::SystemRNG))) {
      return;
   }
   SystemAndGlobalGenerators::PRNGLocker lock(this);
   m_storage.engine().discard(z);
}

bool operator==(const RandomGenerator &rng1, const RandomGenerator &rng2)
{
   if (rng1.m_type != rng2.m_type) {
      return false;
   } 
   if (rng1.m_type == pdk::as_integer<RNGType>(RNGType::SystemRNG)) {
      return true;
   }
   // Lock global() if either is it (otherwise this locking is a no-op)
   using PRNGLocker = RandomGenerator::SystemAndGlobalGenerators::PRNGLocker;
   PRNGLocker locker(&rng1 == RandomGenerator::global() ? &rng1 : &rng2);
   return rng1.m_storage.engine() == rng2.m_storage.engine();
}

void RandomGenerator::doFillRange(void *buffer, void *bufferEnd)
{
   // Verify that the pointers are properly aligned for 32-bit
   PDK_ASSERT(pdk::uintptr(buffer) % sizeof(pdk::puint32) == 0);
   PDK_ASSERT(pdk::uintptr(bufferEnd) % sizeof(pdk::puint32) == 0);
   pdk::puint32 *begin = static_cast<pdk::puint32 *>(buffer);
   pdk::puint32 *end = static_cast<pdk::puint32 *>(bufferEnd);
   
   if (m_type == pdk::as_integer<RNGType>(RNGType::SystemRNG) || 
       PDK_UNLIKELY(uint(internal::sg_randomdeviceControl) & (pdk::as_integer<RandomGeneratorControl>(RandomGeneratorControl::UseSystemRNG)|
                                                              pdk::as_integer<RandomGeneratorControl>(RandomGeneratorControl::SetRandomData))))
      return SystemGenerator::self().generate(begin, end);
   
   SystemAndGlobalGenerators::PRNGLocker lock(this);
   std::generate(begin, end, [this]() { return m_storage.engine()(); });
}

#if defined(PDK_OS_UNIX) && !defined(PDK_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)
using SeedStorageType = typename pdk::stdext::FunctionPointer<decltype(&srand)>::arg<0>::type;

typedef ThreadStorage<SeedStorageType *> SeedStorage;
PDK_GLOBAL_STATIC(SeedStorage, sg_randTLS);  // Thread Local Storage for seed value

#endif

void srand(uint seed)
{
#if defined(PDK_OS_UNIX) && !defined(PDK_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)
   SeedStorage *seedStorage = sg_randTLS();
   if (seedStorage) {
      SeedStorageType *pseed = seedStorage->localData();
      if (!pseed)
         seedStorage->setLocalData(pseed = new SeedStorageType);
      *pseed = seed;
   } else {
      //global static seed storage should always exist,
      //except after being deleted by QGlobalStaticDeleter.
      //But since it still can be called from destructor of another
      //global static object, fallback to srand(seed)
      srand(seed);
   }
#else
   // On Windows srand() and rand() already use Thread-Local-Storage
   // to store the seed between calls
   // this is also valid for PDK_NO_THREAD
   srand(seed);
#endif
}

int rand()
{
#if defined(PDK_OS_UNIX) && !defined(PDK_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && (_POSIX_THREAD_SAFE_FUNCTIONS - 0 > 0)
   SeedStorage *seedStorage = sg_randTLS();
   if (seedStorage) {
      SeedStorageType *pseed = seedStorage->localData();
      if (!pseed) {
         seedStorage->setLocalData(pseed = new SeedStorageType);
         *pseed = 1;
      }
      return rand_r(pseed);
   } else {
      //global static seed storage should always exist,
      //except after being deleted by QGlobalStaticDeleter.
      //But since it still can be called from destructor of another
      //global static object, fallback to rand()
      return rand();
   }
#else
   // On Windows srand() and rand() already use Thread-Local-Storage
   // to store the seed between calls
   // this is also valid for PDK_NO_THREAD
   return rand();
#endif
}

} // pdk
