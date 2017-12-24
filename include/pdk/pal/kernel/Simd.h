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
// Created by softboy on 2017/12/24.

#ifndef PDK_PAL_KERNEL_SIMD_H
#define PDK_PAL_KERNEL_SIMD_H

#include "pdk/global/Global.h"
#include "pdk/base/os/thread/Atomic.h"

/*
 * defines the PDK_COMPILER_SUPPORTS_XXX macros.
 * They mean the compiler supports the necessary flags and the headers
 * for the x86 and ARM intrinsics:
 *  - GCC: the -mXXX or march=YYY flag is necessary before #include
 *    up to 4.8; GCC >= 4.9 can include unconditionally
 *  - Intel CC: #include can happen unconditionally
 *  - MSVC: #include can happen unconditionally
 *  - RVCT: ???
 *
 * We will try to include all headers possible under this configuration.
 *
 * MSVC does not define __SSE2__ & family, so we will define them. MSVC 2013 &
 * up do define __AVX__ if the -arch:AVX option is passed on the command-line.
 *
 * Supported XXX are:
 *   Flag    | Arch |  GCC  | Intel CC |  MSVC  |
 *  ARM_NEON | ARM  | I & C | None     |   ?    |
 *  SSE2     | x86  | I & C | I & C    | I & C  |
 *  SSE3     | x86  | I & C | I & C    | I only |
 *  SSSE3    | x86  | I & C | I & C    | I only |
 *  SSE4_1   | x86  | I & C | I & C    | I only |
 *  SSE4_2   | x86  | I & C | I & C    | I only |
 *  AVX      | x86  | I & C | I & C    | I & C  |
 *  AVX2     | x86  | I & C | I & C    | I only |
 *  AVX512xx | x86  | I & C | I & C    | I only |
 * I = intrinsics; C = code generation
 *
 * Code can use the following constructs to determine compiler support & status:
 * - #ifdef __XXX__      (e.g: #ifdef __AVX__  or #ifdef __ARM_NEON__)
 *   If this test passes, then the compiler is already generating code for that
 *   given sub-architecture. The intrinsics for that sub-architecture are
 *   #included and can be used without restriction or runtime check.
 *
 * - #if PDK_COMPILER_SUPPORTS(XXX)
 *   If this test passes, then the compiler is able to generate code for that
 *   given sub-architecture in another translation unit, given the right set of
 *   flags. Use of the intrinsics is not guaranteed. This is useful with
 *   runtime detection (see below).
 *
 * - #if PDK_COMPILER_SUPPORTS_HERE(XXX)
 *   If this test passes, then the compiler is able to generate code for that
 *   given sub-architecture in this translation unit, even if it is not doing
 *   that now (it might be). Individual functions may be tagged with
 *   PDK_FUNCTION_TARGET(XXX) to cause the compiler to generate code for that
 *   sub-arch. Only inside such functions is the use of the intrisics
 *   guaranteed to work. This is useful with runtime detection (see below).
 *
 * Runtime detection of a CPU sub-architecture can be done with the
 * cpu_has_feature(XXX) function. There are two strategies for generating
 * optimized code like that:
 *
 * 1) place the optimized code in a different translation unit (C or assembly
 * sources) and pass the correct flags to the compiler to enable support. Those
 * sources must not include qglobal.h, which means they cannot include this
 * file either. The dispatcher function would look like this:
 *
 *      void foo()
 *      {
 *      #if PDK_COMPILER_SUPPORTS(XXX)
 *          if (cpu_has_feature(XXX)) {
 *              foo_optimized_xxx();
 *              return;
 *          }
 *      #endif
 *          foo_plain();
 *      }
 *
 * 2) place the optimized code in a function tagged with PDK_FUNCTION_TARGET and
 * surrounded by #if PDK_COMPILER_SUPPORTS_HERE(XXX). That code can freely use
 * other pdk code. The dispatcher function would look like this:
 *
 *      void foo()
 *      {
 *      #if PDK_COMPILER_SUPPORTS_HERE(XXX)
 *          if (cpu_has_feature(XXX)) {
 *              foo_optimized_xxx();
 *              return;
 *          }
 *      #endif
 *          foo_plain();
 *      }
 */

#if defined(__MINGW64_VERSION_MAJOR) || defined(PDK_CC_MSVC)
#include <intrin.h>
#endif

#define PDK_COMPILER_SUPPORTS(x) (PDK_COMPILER_SUPPORTS_ ## x - 0)

#if defined(PDK_PROCESSOR_ARM)
#  define PDK_COMPILER_SUPPORTS_HERE(x) (__ARM_FEATURE_ ## x)
#  if defined(PDK_CC_GNU) && !defined(PDK_CC_INTEL) && PDK_CC_GNU >= 600
/* GCC requires attributes for a function */
#    define PDK_FUNCTION_TARGET(x) __attribute__((__target__(PDK_FUNCTION_TARGET_STRING_ ## x)))
#  else
#    define PDK_FUNCTION_TARGET(x)
#  endif
#  if !defined(__ARM_FEATURE_NEON) && defined(__ARM_NEON__)
#     define __ARM_FEATURE_NEON
#  endif
#elif (defined(PDK_CC_INTEL) || defined(PDK_CC_MSVC) \
   || (defined(PDK_CC_GNU) && !defined(PDK_CC_CLANG) && (__GNUC__-0) * 100 + (__GNUC_MINOR__-0) >= 409))
#  define PDK_COMPILER_SUPPORTS_SIMD_ALWAYS
#  define PDK_COMPILER_SUPPORTS_HERE(x) PDK_COMPILER_SUPPORTS(x)
#  if defined(PDK_CC_GNU) !defined(PDK_CC_INTEL)
/* GCC requires attributes for a function */
#    define PDK_FUNCTION_TARGET(x) __attribute__((__target__(PDK_FUNCTION_TARGET_STRING_ ## x)))
#  else
#    define PDK_DUNCTION_TARGET(x)
#  endif
#else
#  define PDK_COMPILER_SUPPORTS_HERE(x) (__ ## x ## __)
#  define PDK_FUNCTION_TARGET(x)
#endif

// SSE intrinsics
#define PDK_FUNCTION_TARGET_STZRING_SSE2 "sse2"
#if defined(__SSE2__) || (defined(PDK_COMPILER_SUPPORTS_SSE2) && defined(PDK_COMPILER_SUPPORTS_SIMD_ALWAYS))
#  if defined(PDK_LINUXBASE)
/// this is an evil hack - the posix_memalign declaration in LSB
/// is wrong - see http://bugs.linuxbase.org/show_bug.cgi?id=2431
#     define posix_memalign _lsb_hack_posix_memalign
#     include <emmintrin.h>
#     undef posix_memalign
#  else
#     include <emmintrin.h>
#  endif
#  if defined(PDK_CC_MSVC) && (defined(_M_X64) || _M_IX86_FP >= 2)
#    define __SSE__ 1
#    define __SSE2__ 1 
#  endif
#endif

// SSE3 intrinsics
#define PDK_FUNCTION_TARGET_STZRING_SSE3 "sse3"
#if defined(__SSE3__) || (defined(PDK_COMPILER_SUPPORTS_SSE3) && defined(PDK_COMPILER_SUPPORTS_SIMD_ALWAYS))
#  include <pmmintrin.h>
#endif

// SSSE3 intrinsics
#define PDK_FUNCTION_TARGET_STRING_SSSE3 "ssse3"
#if defined(__SSSE3__) || (defined(PDK_COMPILER_SUPPORTS_SSSE3) && defined(PDK_COMPILER_SUPPORTS_SIMD_ALWAYS))
#  include <tmmintrin.h>
#endif

// SSE4.1 intrinsics
#define PDK_FUNCTION_TARGET_STRING_SSE4_1 "sse4.1"
#if defined(__SSE4_1__) || (defined(PDK_COMPILER_SUPPORTS_SSE4_1) && defined(PDK_COMPILER_SUPPORTS_SIMD_ALWAYS))
#  include <smmintrin.h>
#endif

#define PDK_FUNCTION_TARGET_STRING_SSE4_2 "sse4.2"
#if defined(__SSE4_2__) || (defined(PDK_COMPILER_SUPPORTS_SSE4_2) && defined(PDK_COMPILER_SUPPORTS_SIMD_ALWAYS))
#  include <nmmintrin.h>
#endif

// AVX intrinsics
#define PDK_FUNCTION_TARGET_STRING_AVX "avx"
#define PDK_FUNCTION_TARGET_STRING_AVX2 "avx2"
#if defined(__AVX__) || (defined(PDK_COMPILER_SUPPORTS_AVX) && defined(PDK_COMPILER_SUPPORTS_SIMD_ALWAYS))
// immintrin.h is the ultimate header, we don't need anything else after this
#  include <immintrin.h>
#  if defined(PDK_CC_MSVC) && (defined(_M_AVX) || defined(__AVX__))
// MS Visual Studio 2010 has no macro pre-defined to identify the use of /arch:AVX
// MS Visual Studio 2013 adds it: __AVX__
// See: http://connect.microsoft.com/VisualStudio/feedback/details/605858/arch-avx-should-define-a-predefined-macro-in-x64-and-set-a-unique-value-for-m-ix86-fp-in-win32
#    define __SSE3__ 1
#    define __SSSE3__ 1
// no Intel CPU supports SSE4a, so don't define it
#    define __SSE4_1__ 1
#    define __SSE4_2__ 1
#    ifndef __AVX__
#       define __AVX__ 1
#    endif
#  endif
#endif

#define PDK_FUNCTION_TARGET_STRING_AVX512F       "avx512f"
#define PDK_FUNCTION_TARGET_STRING_AVX512CD      "avx512cd"
#define PDK_FUNCTION_TARGET_STRING_AVX512ER      "avx512er"
#define PDK_FUNCTION_TARGET_STRING_AVX512PF      "avx512pf"
#define PDK_FUNCTION_TARGET_STRING_AVX512BW      "avx512bw"
#define PDK_FUNCTION_TARGET_STRING_AVX512DQ      "avx512dq"
#define PDK_FUNCTION_TARGET_STRING_AVX512VL      "avx512vl"
#define PDK_FUNCTION_TARGET_STRING_AVX512IFMA    "avx512ifma"
#define PDK_FUNCTION_TARGET_STRING_AVX512VBMI    "avx512vbmi"

#define PDK_FUNCTION_TARGET_STRING_F16C          "f16c"
#define PDK_FUNCTION_TARGET_STRING_RDRAND        "rdrnd"
#define PDK_FUNCTION_TARGET_STRING_BMI           "bmi"
#define PDK_FUNCTION_TARGET_STRING_BMI2          "bmi2"
#define PDK_FUNCTION_TARGET_STRING_RDSEED        "rdseed"
#define PDK_FUNCTION_TARGET_STRING_SHA           "sha"

// other x86 intrinsics
#if defined(PDK_PROCESSOR_X86) && ((defined(PDK_CC_GNU) && (PDK_CC_GNU >= 404)) \
   || (defined(PDK_CC_CLANG) && (PDK_CC_CLANG >= 208)) \
   || defined(PDK_CC_INTEL))
#  define PDK_COMPILER_SUPPORTS_X86INTRIN
#  ifdef PDK_CC_INTEL
// The Intel compiler has no <x86intrin.h> -- all intrinsics are in <immintrin.h>;
#    include <immintrin.h>
#  else
// GCC 4.4 and Clang 2.8 added a few more intrinsics there
#    include <x86intrin.h>
#  endif
#endif

// Clang compiler fix, see http://lists.llvm.org/pipermail/cfe-commits/Week-of-Mon-20160222/151168.html
// This should be tweaked with an "upper version" of clang once we know which release fixes the
// issue. At that point we can rely on __ARM_FEATURE_CRC32 again.
#if defined(PDK_CC_CLANG) && defined(PDK_OS_DARWIN) && defined(__ARM_FEATURE_CRC32)
#  undef __ARM_FEATURE_CRC32
#endif

// NEON intrinsics
// note: as of GCC 4.9, does not support function targets for ARM
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#  include <arm_neon.h>
#  define PDK_FUNCTION_TARGET_STRING_NEON "+neon"
// unused: gcc doesn't support function targets on non-aarch64, and on Aarch64 NEON is always available.
#  ifndef __ARM_NEON
// __ARM_NEON__ is not defined on AArch64, but we need it in our NEON detection.
#     define __ARM_NEON__
#  endif
#endif

// AArch64/ARM64
#if defined(PDK_PROCESSOR_ARM_V8) && defined(__ARM_FEATURE_CRC32)
#  define PDK_FUNCTION_TARGET_STRING_CRC32 "+crc"
#  include <arm_acle.h>
#endif

#undef PDK_COMPILER_SUPPORTS_SIMD_ALWAYS

namespace pdk {
namespace pal {
namespace kernel {

enum class CPUFeatures : long
{
#if defined(PDK_PROCESSOR_ARM)
   NEON = 0,
   ARM_NEON = NEON,
   CRC32 = 1,
#elif defined(PDK_PROCESSOR_MIPS)
   DSP = 0,
   DSPR2 = 1,
#elif defined(PDK_PROCESSOR_X86)
   // The order of the flags is jumbled so it matches most closely the bits in CPUID
   // Out of order:
   SSE2          = 1,
   SSE3          = (0 + 0),
   SSSE3         = (0 + 9),
   SSE4_1        = (0 + 19),
   SSE4_2        = (0 + 20),
   MOVBE         = (0 + 22),
   POPCNT        = (0 + 23),
   AES           = (0 + 25),
   AVX           = (0 + 28),
   F16C          = (0 + 29),
   RDRAND        = (0 + 30),
   // 31 is always zero and we've used it for the SimdInitialized
   
   // in level 7, leaf 0, EBX
   BMI           = (32 + 3),
   HLE           = (32 + 4),
   AVX2          = (32 + 5),
   BMI2          = (32 + 8),
   RTM           = (32 + 11),
   AVX512F       = (32 + 16),
   AVX512DQ      = (32 + 17),
   RDSEED        = (32 + 18),
   AVX512IFMA    = (32 + 21),
   AVX512PF      = (32 + 26),
   AVX512ER      = (32 + 27),
   AVX512CD      = (32 + 28),
   SHA           = (32 + 29),
   AVX512BW      = (32 + 30),
   AVX512VL      = (32 + 31),
   // in level 7, leaf 0, ECX (out of order, for now)
   AVX512VBMI    = 2,// uses the bit for DTES64
#endif
   // used only to indicate that the CPU detection was initialised
   SimdInitialized = 0x80000000
};

static const puint64 COMPILER_CPU_FEATURE = 0
      #if defined(__SHA__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::SHA))
      #endif
      #if defined(__AES__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::AES))
      #endif
      #if defined(__RTM__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::RTM))
      #endif
      #if defined(__RDRND__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::RDRAND))
      #endif
      #if defined(__RDSEED__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::RDSEED))
      #endif
      #if defined(__BMI__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::BMI))
      #endif
      #if defined(__BMI2__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::BMI2))
      #endif
      #if defined(__F16C__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::F16C))
      #endif
      #if defined(__POPCNT__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::POPCNT))
      #endif
      #if defined(__MOVBE__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::MOVBE))
      #endif
      #if defined(__AVX512F__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::AVX512F))
      #endif
      #if defined(__AVX512CD__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::AVX512CD))
      #endif
      #if defined(__AVX512ER__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::AVX512ER))
      #endif
      #if defined(__AVX512PF__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::AVX512PF))
      #endif
      #if defined(__AVX512BW__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::AVX512BW))
      #endif
      #if defined(__AVX512DQ__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::AVX512DQ))
      #endif
      #if defined(__AVX512VL__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::AVX512VL))
      #endif
      #if defined(__AVX512IFMA__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::AVX512IFMA))
      #endif
      #if defined(__AVX512VBMI__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::AVX512VBMI))
      #endif
      #if defined(__AVX2__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::AVX2))
      #endif
      #if defined(__AVX__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::AVX))
      #endif
      #if defined(__SSE4_2__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::SSE4_2))
      #endif
      #if defined(__SSE4_1__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::SSE4_1))
      #endif
      #if defined(__SSSE3__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::SSSE3))
      #endif
      #if defined(__SSE3__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::SSE3))
      #endif
      #if defined(__SSE2__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::SSE2))
      #endif
      #if defined(__ARM_NEON__)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::NEON))
      #endif
      #if defined(__ARM_FEATURE_CRC32)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::CRC32))
      #endif
      #if defined(__mips_dsp)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::DSP))
      #endif
      #if defined(__mips_dspr2)
      | (PDK_UINT64_C(1) << static_cast<unsigned long long>(CPUFeatures::DSPR2))
      #endif
      ;

#ifdef PDK_ATOMIC_INT64_IS_SUPPORTED
extern PDK_CORE_EXPORT pdk::os::thread::BasicAtomicInteger<puint64> pdk_cpu_features[1];
#else
extern PDK_CORE_EXPORT pdk::os::thread::BasicAtomicInteger<unsigned int> pdk_cpu_features[2];
#endif

PDK_CORE_EXPORT void detect_cpu_features();

namespace
{
inline puint64 cpu_features()
{
   puint64 features = pdk_cpu_features[0].load();
#ifndef PDK_ATOMIC_INT64_IS_SUPPORTED
   features |= static_cast<puint64>(pdk_cpu_features[1].load()) << 32;
#endif
   if (PDK_UNLIKELY(features == 0)) {
      detect_cpu_features();
      features = pdk_cpu_features[0].load();
#ifndef PDK_ATOMIC_INT64_IS_SUPPORTED
      features |= static_cast<puint64>(pdk_cpu_features[1].load()) << 32;
#endif
      PDK_ASSUME(features != 0);
   }
   return features;
}
}

#define CPU_HAS_FEATURE(feature) ((COMPILER_CPU_FEATURE & (PDK_UINT64_C(1) << CPUFeatures::##feature))\
   || (cpu_features() & (PDK_UINT64_C(1) << CPUFeatures::feature)))

#define ALIGNMENT_PROLOGUE_16BYTES(ptr, i, length) \
   for (; i < static_cast<int>(std::min(static_cast<pdk::uintptr>(length), ((4 - ((reinterpret_cast<pdk::uintptr>(ptr) >> 2) & 0x3)) & 0x3))); ++i)

#define ALIGNMENT_PROLOGUE_32BYTES(ptr, i, length) \
   for (; i < static_cast<int>(std::min(static_cast<pdk::uintptr>(length), ((8 - ((reinterpret_cast<pdk::uintptr>(ptr) >> 2) & 0x3)) & 0x7))); ++i)

#define SIMD_EPILOGUE(i, length, max) \
   for (int _i = 0; _i < max && i < length; ++i, ++_i)

} // kernel
} // pal
} // pdk

#endif // PDK_PAL_KERNEL_SIMD_H
