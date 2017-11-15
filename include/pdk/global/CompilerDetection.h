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
// Created by softboy on 2017/11/13.

#ifndef PDK_GLOBAL_COMPILERDETECTION_H
#define PDK_GLOBAL_COMPILERDETECTION_H

/*
   The compiler, must be one of: (PDK_CC_x)
   
     SYM      - Digital Mars C/C++ (used to be Symantec C++)
     MSVC     - Microsoft Visual C/C++, Intel C++ for Windows
     BOR      - Borland/Turbo C++
     WAT      - Watcom C++
     GNU      - GNU C++
     COMEAU   - Comeau C++
     EDG      - Edison Design Group C++
     OC       - CenterLine C++
     SUN      - Forte Developer, or Sun Studio C++
     MIPS     - MIPSpro C++
     DEC      - DEC C++
     HPACC    - HP aC++
     USLC     - SCO OUDK and UDK
     CDS      - Reliant C++
     KAI      - KAI C++
     INTEL    - Intel C++ for Linux, Intel C++ for Windows
     HIGHC    - MetaWare High C/C++
     PGI      - Portland Group C++
     GHS      - Green Hills Optimizing C++ Compilers
     RVCT     - ARM Realview Compiler Suite
     CLANG    - C++ front-end for the LLVM compiler
     
     
   Should be sorted most to least authoritative.
*/

/* Symantec C++ is now Digital Mars */
#if defined(__DMC__) || defined(__SC__)
#   define PDK_CC_SYM
/* "explicit" semantics implemented in 8.1e but keyword recognized since 7.5 */
#   if defined(__SC__) && __SC__ < 0x750
#      error "Compiler not supported"
#   endif
#elif defined(_MSC_VER)
#   ifdef __clang__
#      define PDK_CC_CLANG ((__clang_major__ * 100) + __clang_minor__)
#   endif
#   define PDK_CC_MSVC (_MSC_VER)
#   define PDK_CC_MSVC_NET
#   define PDK_OUTOFLINE_TEMPLATE inline
#   if _MSC_VER < 1600
#      define PDK_NO_TEMPLATE_FRIENDS
#   endif
#   define PDK_COMPILER_MANGLES_RETURN_TYPE
#   define PDK_FUNC_INFO __FUNCSIG__
#   define PDK_ALIGNOF(type) __alignof(type)
#   define PDK_DECL_ALIGN(n) __declspec(align(n))
#   define PDK_ASSUME_IMPL() __assume(0)
#   define PDK_NORETURN __declspec(noreturn)
#   define PDK_DECL_DEPRECATED __declspec(deprecated)
#   ifndef PDK_CC_CLANG
#      define PDK_DECL_DEPRECATED_X(text) __declspec(deprecated(text))
#   endif
#   define PDK_DECL_EXPORT __declspec(dllexport)
#   define PDK_DECL_IMPORT __declspec(dllimport)
#   if _MSC_VER >= 1800
#      define PDK_MAKE_UNCHECKED_ARRAY_ITERATOR(x) stdext::make_unchecked_array_iterator(x)
#   endif
#   if _MSC_VER >= 1500
#      define PDK_MAKE_CHECKED_ARRAY_ITERATOR(x, N) stdext::make_checked_array_iterator(x, size_t(N))
#   endif
/* Intel C++ disguising as Visual C++: the `using' keyword avoids warnings */
#   if defined(__INTEL_COMPILER)
#      define PDK_DECL_VARIABLE_DEPRECATED
#      define PDK_CC_INTEL __INTEL_COMPILER
#   endif

/* only defined for MSVC since that's the only compiler that actually optimizes for this */
/* might get overridden further down when PDK_COMPILER_NOEXCEPT is detected */
#   ifdef __cplusplus
#      define PDK_DECL_NOTHROW  throw()
#   endif
#elif defined(__BORLANDC__) || defined(__TURBOC__)
#   define PDK_CC_BORLAND
#   define PDK_INLINE_TEMPLATE
#   if __BORLANDC__ < 0x502
#      error "Compiler not supported"
#   endif
#elif defined(__WATCOMC__)
#   define PDK_CC_WAT

/* ARM Realview Compiler Suite
   RVCT compiler also defines __EDG__ and __GNUC__ (if --gnu flag is given),
   so check for it before that */
#elif defined(__ARMCC__) || defined(__CC_ARM)
#   define PDK_CC_RVCT
/* work-around for missing compiler intrinsics */
#   define __is_empty(x) false
#   define __is_pod(x) false
#   define PDK_DECL_DEPRECATED __attribute__ ((__deprecated__))
#   ifdef PDK_OS_LINUX
#      define PDK_DECL_EXPORT __attribute__((visibility("default")))
#      define PDK_DECL_IMPORT __attribute__((visibility("default")))
#      define PDK_DECL_HIDDEN __attribute__((visibility("hidden")))
#   else 
#      define PDK_DECL_EXPORT __declspec(dllexport)
#      define PDK_DECL_EXPORT __declspec(dllimport)
#   endif
#elif defined(__GNUC__)
#   define PDK_CC_GNU (__GNUC__ * 100 + __GNUC_MINOR__)
#   if defined(__MINGW32__)
#      define PDK_CC_MINGW
#   endif
#   if defined(__INTEL_COMPILER)
/* Intel C++ also masquerades as GCC */
#      define PDK_CC_INTEL (__INTEL_COMPILER)
#      ifdef __clang__
#         define PDK_CC_CLANG 305
#      endif
#      define PDK_ASSUME_IMPL(expr) __assume(expr)
#      define PDK_UNREACHABLE_IMPL() __builtin_unreachable()
#      if __INTEL_COMPILER >= 1300 && !defined(__APPLE__)
#         define PDK_DECL_DEPRECATED_X(text) __attribute__ ((__deprecated__(text)))
#      endif
#   elif defined(__clang__)
/* Clang also masquerades as GCC */
#      if defined(__appple_build_version__)
/* http://en.wikipedia.org/wiki/Xcode#Toolchain_Versions */
#         if __appple_build_version__ >= 7000053
#            define PDK_CC_CLANG 306
#         elif __appple_build_version__ >= 6000051
#            define PDK_CC_CLANG 305
#         elif __appple_build_version__ >= 5030038
#            define PDK_CC_CLANG 304
#         elif __appple_build_version__ >= 5000275
#            define PDK_CC_CLANG 303
#         elif __appple_build_version__ >= 4250024
#            define PDK_CC_CLANG 302
#         elif __appple_build_version__ >= 3180045
#            define PDK_CC_CLANG 301
#         elif __appple_build_version__ >= 2111001
#            define PDK_CC_CLANG 300
#         else
#            error "Unknown Apple Clang version"
#         endif
#      else 
#         define PDK_CC_CLANG ((__clang_major__ * 100) + __clang_minor__)
#      endif
#      if __has_builtin(__builtin_assume)
#         define PDK_ASSUME_IMPL(expr) __builtin_assume(expr)
#      else
#         define PDK_ASSUME_IMPL(expr) if (expr){} else __builtin_unreachable()
#      endif
#      define PDK_UNREACHABLE_IMPL() __builtin_unreachable()
#      if !defined(__has_extension)
#         /* Compatibility with older Clang versions */
#         define __has_extension __has_feature
#      endif
#      if defined(__APPLE__)
/* Apple/clang specific features */
#         define PDK_DECL_CF_RETURNS_RETAINED __attribute__((cf_returns_retained))
#         ifdef __OBJC__
#            define PDK_DECL_NS_RETURNS_AUTORELEASED __attribute__((ns_returns_autoreleased))
#         endif
#      endif
#   else
/* Plain GCC */
#      if PDK_CC_GNU >= 405
#         define PDK_ASSUME_IMPL(expr)  if (expr){} else __builtin_unreachable()
#         define PDK_UNREACHABLE_IMPL() __builtin_unreachable()
#         define PDK_DECL_DEPRECATED_X(text) __attribute__ ((__deprecated__(text)))
#      endif
#   endif
#   ifdef PDK_OS_WIN
#      define PDK_DECL_EXPORT __declspec(dllexport)
#      define PDK_DECL_IMPORT __declspec(dllimport)
#   elif defined(PDK_VISIBILITY_AVAILABLE)
#      define PDK_DECL_EXPORT __attribute__((visibility("default")))
#      define PDK_DECL_IMPORT __attribute__((visibility("default")))
#      define PDK_DECL_HIDDEN __attribute__((visibility("hidden")))
#   endif
#   define PDK_FUNC_INFO __PRETTY_FUNCTION__
#   define PDK_ALIGNOF(type) __alignof__(type)
#   define PDK_TYPEOF(expr) __typeof(expr)
#   define PDK_DECL_DEPRECATED __attribute__ ((__deprecated__))
#   define PDK_DECL_ALIGN(n) __attribute__((__aligned__(n)))
#   define PDK_DECL_UNUSED __attribute__((__unused__))
#   define PDK_LIKELY(expr) __builtin_expect(!!(expr), true)
#   define PDK_UNLIKELY(expr)  __builtin_expect(!!(expr), false)
#   define PDK_NORETURN __attribute__((__noreturn__))
#   define PDK_REQUIRED_RESULT __attribute__ ((__warn_unused_result__))
#   define PDK_DECL_PURE_FUNCTION __attribute__((pure))
#   define PDK_DECL_CONST_FUNCTION __attribute__((const))
#   define PDK_PACKED __attribute__ ((__packed__))
#   ifndef __ARM_EABI__
#      define PDK_NO_ARM_EABI
#   endif
#   if PDK_CC_GNU >= 403 && !defined(PDK_CC_CLANG)
#      define PDK_ALLOC_SIZE(x) __attribute__((alloc_size(x)))
#   endif
/* IBM compiler versions are a bit messy. There are actually two products:
   the C product, and the C++ product. The C++ compiler is always packaged
   with the latest version of the C compiler. Version numbers do not always
   match. This little table (I'm not sure it's accurate) should be helpful:
   
   C++ product                C product
   
   C Set 3.1                  C Compiler 3.0
   ...                        ...
   C++ Compiler 3.6.6         C Compiler 4.3
   ...                        ...
   Visual Age C++ 4.0         ...
   ...                        ...
   Visual Age C++ 5.0         C Compiler 5.0
   ...                        ...
   Visual Age C++ 6.0         C Compiler 6.0
   
   Now:
   __xlC__    is the version of the C compiler in hexadecimal notation
              is only an approximation of the C++ compiler version
   __IBMCPP__ is the version of the C++ compiler in decimal notation
              but it is not defined on older compilers like C Set 3.1 */
#elif defined(__xlC__)
#   define PDK_CC_XLC
#   define PDK_FULL_TEMPLATE_INSTANTIATION
#   if __xlC__ < 0x400
#      error "Compiler not supported"
#   elif __xlC__ >= 0x0600
#      define PDK_ALGINOF(type) __alignof__(type)
#      define PDK_TYPEOF __typeof__(expr)
#      define PDK_DECL_ALIGN(n) __attribute__((__aligned__(n)))
#      define PDK_PACKED __attribute__((__packed__))
#   endif

/* Older versions of DEC C++ do not define __EDG__ or __EDG - observed
   on DEC C++ V5.5-004. New versions do define  __EDG__ - observed on
   Compaq C++ V6.3-002.
   This compiler is different enough from other EDG compilers to handle
   it separately anyway. */
#elif defined(__DECCXX) || defined(__DECC)
#   define PDK_CC_DEC
/* Compaq C++ V6 compilers are EDG-based but I'm not sure about older
   DEC C++ V5 compilers. */
#   if defined(__EDG__)
#      define PDK_CC_EDG
#   endif
/* Compaq has disabled EDG's _BOOL macro and uses _BOOL_EXISTS instead
   - observed on Compaq C++ V6.3-002.
   In any case versions prior to Compaq C++ V6.0-005 do not have bool. */
#   if !defined(_BOOL_EXISTS)
#      error "Compiler not supported"
#   endif
/* Spurious (?) error messages observed on Compaq C++ V6.5-014. */
/* Apply to all versions prior to Compaq C++ V6.0-000 - observed on
   DEC C++ V5.5-004. */
#   if __DECCXX_VER < 60060000
#      define PDK_BROKEN_TEMPLATE_SPECIALIZATION
#   endif
/* avoid undefined symbol problems with out-of-line template members */
#   define PDK_OUTOFLINE_TEMPLATE inline

/* The Portland Group C++ compiler is based on EDG and does define __EDG__
   but the C compiler does not */
#elif defined(__PGI)
#   define PDK_CC_PGI
#   if defined(__EDG__)
#      define PDK_CC_EDG
#   endif

/* Compilers with EDG front end are similar. To detect them we test:
   __EDG documented by SGI, observed on MIPSpro 7.3.1.1 and KAI C++ 4.0b
   __EDG__ documented in EDG online docs, observed on Compaq C++ V6.3-002
   and PGI C++ 5.2-4 */
#elif !defined(PDK_OS_HPUX) && (defined(__EDG) || defined(__EDG__))
#   define PDK_CC_EDG
/* From the EDG documentation (does not seem to apply to Compaq C++ or GHS C):
   _BOOL
        Defined in C++ mode when bool is a keyword. The name of this
        predefined macro is specified by a configuration flag. _BOOL
        is the default.
   __BOOL_DEFINED
        Defined in Microsoft C++ mode when bool is a keyword. */
#   if !defined(_BOOL) && !defined(__BOOL_DEFINED) && !defined(__ghs)
#      error "Compiler not supported"
#   endif

/* The Comeau compiler is based on EDG and does define __EDG__ */
#   if defined(__COMO__)
#      define PDK_CC_COMEAU
/* The `using' keyword was introduced to avoid KAI C++ warnings
   but it's now causing KAI C++ errors instead. The standard is
   unclear about the use of this keyword, and in practice every
   compiler is using its own set of rules. Forget it. */
#   elif defined(__KCC)
#      define PDK_CC_KAI
/* Using the `using' keyword avoids Intel C++ for Linux warnings */
#   elif defined(__INTEL_COMPILER)
#      define PDK_CC_INTEL (__INTEL_COMPILER)
/* Uses CFront, make sure to read the manual how to tweak templates. */
#   elif defined(__ghs)
#      define PDK_CC_GHS
#      define PDK_DECL_DEPRECATED __attribute__((__deprecated__))
#      define PDK_FUNC_INFO  __PRETTY_FUNCTION__
#      define PDK_TYPEOF(expr) __typeof__(expr)
#      define PDK_ALIGNOF(type) __alignof__(type)
#      define PDK_UNREACHABLE_IMPL()
#      if defined(__cplusplus)
#         define PDK_COMPILER_AUTO_TYPE
#         define PDK_COMPILER_STATIC_ASSERT
#         define PDK_COMPILER_RANGE_FOR
#         if __GHS_VERSION_NUMBER >= 201505
#            define PDK_COMPILER_ALIGNAS
#            define PDK_COMPILER_ALIGNOF
#            define PDK_COMPILER_ATOMICS
#            define PDK_COMPILER_ATTRIBUTES
#            define PDK_COMPILER_AUTO_FUNCTION
#            define PDK_COMPILER_CLASS_ENUM
#            define PDK_COMPILER_CONSTEXPR
#            define PDK_COMPILER_DECLTYPE
#            define PDK_COMPILER_DEFAULT_MEMBERS
#            define PDK_COMPILER_DELETE_MEMBERS
#            define PDK_COMPILER_DELEGATING_CONSTRUCTORS
#            define PDK_COMPILER_EXPLICIT_CONVERSIONS
#            define PDK_COMPILER_EXPLICIT_OVERRIDES
#            define PDK_COMPILER_EXTERN_TEMPLATES
#            define PDK_COMPILER_INHERITING_CONSTRUCTORS
#            define PDK_COMPILER_INITIALIZER_LISTS
#            define PDK_COMPILER_LAMBDA
#            define PDK_COMPILER_NONSTATIC_MEMBER_INIT
#            define PDK_COMPILER_NOEXCEPT
#            define PDK_COMPILER_NULLPTR
#            define PDK_COMPILER_RANGE_FOR
#            define PDK_COMPILER_RAW_STRINGS
#            define PDK_COMPILER_REF_QUALIFIERS
#            define PDK_COMPILER_RVALUE_REFS
#            define PDK_COMPILER_STATIC_ASSERT
#            define PDK_COMPILER_TEMPLATE_ALIAS
#            define PDK_COMPILER_THREAD_LOCAL
#            define PDK_COMPILER_THREADSAFE_STATICS
#            define PDK_COMPILER_UDL
#            define PDK_COMPILER_UNICODE_STRINGS
#            define PDK_COMPILER_UNIFORM_INIT
#            define PDK_COMPILER_UNRESTRICTED_UNIONS
#            define PDK_COMPILER_VARIADIC_MACROS
#            define PDK_COMPILER_VARIADIC_TEMPLATES
#         endif
#      endif //__cplusplus
#   elseif defined(__DCC__)
#      define PDK_CC_DIAB
#      if !defined(__bool)
#         error "Compiler not supported"
#      endif
/* The UnixWare 7 UDK compiler is based on EDG and does define __EDG__ */
#   elif defined(__USLC__) && defined(__SCO_VERSION__)
#      define PDK_CC_USLC
/* The latest UDK 7.1.1b does not need this, but previous versions do */
#      if !defined(__SCO_VERSION__) || (__SCO_VERSION__ < 302200010)
#         define PDK_OUTOFLINE_TEMPLATE inline
#      endif
/* Never tested! */
#   elif defined(CENTERLINE_CLPP) || defined(OBJECTCENTER)
#      define PDK_CC_OC
/* CDS++ defines __EDG__ although this is not documented in the Reliant
   documentation. It also follows conventions like _BOOL and this documented */
#   elif defined(sinix)
#      define PDK_CC_CDS
/* The MIPSpro compiler defines __EDG */
#   elif defined(__sgi)
#      define PDK_CC_MIPS
#      define PDK_NO_TEMPLATE_FRIENDS
#      if defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 740)
#         define PDK_OUTOFLINE_TEMPLATE inline
#         pragma set woff 3624,3625,3649 /* turn off some harmless warnings */
#      endif
#   endif

/* VxWorks' DIAB toolchain has an additional EDG type C++ compiler
   (see __DCC__ above). This one is for C mode files (__EDG is not defined) */
#elif defined(_DIAB_TOOL)
#   define PDK_CC_DIAB
#   define PDK_FUNC_INFO __PRETTY_FUNCTION__

/* Never tested! */
#elif defined(__HIGHC__)
#   define PDK_CC_HIGHC

#elif defined(__SUNPRO_CC) || defined(__SUNPRO_C)
#   define PDK_CC_SUN
#   define PDK_COMPILER_MANGLES_RETURN_TYPE
/* 5.0 compiler or better
    'bool' is enabled by default but can be disabled using -features=nobool
    in which case _BOOL is not defined
        this is the default in 4.2 compatibility mode triggered by -compat=4 */
#   if __SUNPRO_CC >= 0x500
#      define PDK_NO_TEMPLATE_TEMPLATE_PARAMETERS
/* see http://developers.sun.com/sunstudio/support/Ccompare.html */
#      if __SUNPRO_CC >= 0x590
#         define PDK_ALIGNOF(type) __alignof__(type)
#         define PDK_TYPEOF(expr) __typeof__(expr)
#         define PDK_DECL_ALIGN(n) __attribute__((__aligned__(n)))
#      endif
#      if __SUNPRO_CC >= 0x550
#         define PDK_DECL_EXPORT __global
#      endif
#      if __SUNPRO_CC < 0x5a0
#         define PDK_NO_TEMPLATE_FRIENDS
#      endif
#      if !defined(_BOOL)
#         error "Compiler not supported"
#      endif
/* 4.2 compiler or older */
#   else
#      error "Compiler not supported"
#   endif

/* CDS++ does not seem to define __EDG__ or __EDG according to Reliant
   documentation but nevertheless uses EDG conventions like _BOOL */
#elif defined(sinix)
#   define PDK_CC_EDG
#   define PDK_CC_CDS
#   if !defined(_BOOL)
#      error "Compiler not supported"
#   endif
#   define PDK_BROKEN_TEMPLATE_SPECIALIZATION

#elif defined(PDK_OS_HPUX)
/* __HP_aCC was not defined in first aCC releases */
#   if defined(__HP_aCC) || __cplusplus >= 199707L
#      define PDK_NO_TEMPLATE_FRIENDS
#      define PDK_CC_HPACC
#      define PDK_FUNC_INFO __PRETTY_FUNCTION__
#      if __HP_aCC-0 < 060000
#         define PDK_NO_TEMPLATE_TEMPLATE_PARAMETERS
#         define PDK_DECL_EXPORT __declspec(dllexport)
#         define PDK_DECL_IMPORT __declspec(dllimport)
#      endif
#      if __HP_aCC-0 >= 061200
#         define PDK_DECL_ALIGN(n) __attribute__((aligned(n)))
#      endif
#      if __HP_aCC-0 >= 062000
#         define PDK_DECL_EXPORT __attribute__((visibility("default")))
#         define PDK_DECL_HIDDEN __attribute__((visibility("hidden")))
#         define PDK_DECL_IMPORT PDK_DECL_EXPORT
#      endif
#   else
#      error "Compiler not supported"
#   endif
#else
#   error "libPDK has not been tested with this compiler - see http://www.libPDK.org/"
#endif

/*
 * C++11 support
 *
 *  Paper           Macro                               SD-6 macro
 *  N2341           PDK_COMPILER_ALIGNAS
 *  N2341           PDK_COMPILER_ALIGNOF
 *  N2427           PDK_COMPILER_ATOMICS
 *  N2761           PDK_COMPILER_ATTRIBUTES               __cpp_attributes = 200809
 *  N2541           PDK_COMPILER_AUTO_FUNCTION
 *  N1984 N2546     PDK_COMPILER_AUTO_TYPE
 *  N2437           PDK_COMPILER_CLASS_ENUM
 *  N2235           PDK_COMPILER_CONSTEXPR                __cpp_constexpr = 200704
 *  N2343 N3276     PDK_COMPILER_DECLTYPE                 __cpp_decltype = 200707
 *  N2346           PDK_COMPILER_DEFAULT_MEMBERS
 *  N2346           PDK_COMPILER_DELETE_MEMBERS
 *  N1986           PDK_COMPILER_DELEGATING_CONSTRUCTORS
 *  N2437           PDK_COMPILER_EXPLICIT_CONVERSIONS
 *  N3206 N3272     PDK_COMPILER_EXPLICIT_OVERRIDES
 *  N1987           PDK_COMPILER_EXTERN_TEMPLATES
 *  N2540           PDK_COMPILER_INHERITING_CONSTRUCTORS
 *  N2672           PDK_COMPILER_INITIALIZER_LISTS
 *  N2658 N2927     PDK_COMPILER_LAMBDA                   __cpp_lambdas = 200907
 *  N2756           PDK_COMPILER_NONSTATIC_MEMBER_INIT
 *  N2855 N3050     PDK_COMPILER_NOEXCEPT
 *  N2431           PDK_COMPILER_NULLPTR
 *  N2930           PDK_COMPILER_RANGE_FOR
 *  N2442           PDK_COMPILER_RAW_STRINGS              __cpp_raw_strings = 200710
 *  N2439           PDK_COMPILER_REF_QUALIFIERS
 *  N2118 N2844 N3053 PDK_COMPILER_RVALUE_REFS            __cpp_rvalue_references = 200610
 *  N1720           PDK_COMPILER_STATIC_ASSERT            __cpp_static_assert = 200410
 *  N2258           PDK_COMPILER_TEMPLATE_ALIAS
 *  N2659           PDK_COMPILER_THREAD_LOCAL
 *  N2660           PDK_COMPILER_THREADSAFE_STATICS
 *  N2765           PDK_COMPILER_UDL                      __cpp_user_defined_literals = 200809
 *  N2442           PDK_COMPILER_UNICODE_STRINGS          __cpp_unicode_literals = 200710
 *  N2640           PDK_COMPILER_UNIFORM_INIT
 *  N2544           PDK_COMPILER_UNRESTRICTED_UNIONS
 *  N1653           PDK_COMPILER_VARIADIC_MACROS
 *  N2242 N2555     PDK_COMPILER_VARIADIC_TEMPLATES       __cpp_variadic_templates = 200704
 *
 * For any future version of the C++ standard, we use only the SD-6 macro.
 * For full listing, see
 *  http://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations
 *
 * C++ extensions:
 *  PDK_COMPILER_RESTRICTED_VLA       variable-length arrays, prior to __cpp_runtime_arrays
 */

#ifdef __cplusplus
#   if __cplusplus < 201103L && !(defined(PDK_CC_MSVC) && PDK_CC_MSVC >= 1800)
#      error libPDK requires a C++11 compiler and yours does not seem to be that.
#   endif
#endif

#ifdef PDK_CC_INTEL
#   define PDK_COMPILER_RESTRICTED_VLA
#   define PDK_COMPILER_VARIADIC_MACROS  // C++11 feature supported as an extension in other modes, too
#   define PDK_COMPILER_THREADSAFE_STATICS
#   if __INTEL_COMPILER < 1200
#      define PDK_NO_TEMPLATE_FRIENDS
#   endif
#   if __INTEL_COMPILER >= 1310 && !defined(_WIN32)
//    ICC supports C++14 binary literals in C, C++98, and C++11 modes
//    at least since 13.1, but I can't test further back
#      define PDK_COMPILER_BINARY_LITERALS
#   endif
#   if __cplusplus >= 201103L || defined(__INTEL_CXX11_MODE__)
#      if __INTEL_COMPILER >= 1200
#         define PDK_COMPILER_AUTO_TYPE
#         define PDK_COMPILER_CLASS_ENUM
#         define PDK_COMPILER_DECLTYPE
#         define PDK_COMPILER_DEFAULT_MEMBERS
#         define PDK_COMPILER_DELETE_MEMBERS
#         define PDK_COMPILER_EXTERN_TEMPLATES
#         define PDK_COMPILER_LAMBDA
#         define PDK_COMPILER_RVALUE_REFS
#         define PDK_COMPILER_STATIC_ASSERT
#         define PDK_COMPILER_VARIADIC_MACROS
#      endif
#      if __INTEL_COMPILER >= 1210
#         define PDK_COMPILER_ATTRIBUTES
#         define PDK_COMPILER_AUTO_FUNCTION
#         define PDK_COMPILER_NULLPTR
#         define PDK_COMPILER_TEMPLATE_ALIAS
#         ifndef _CHAR16T      // MSVC headers
#            define PDK_COMPILER_UNICODE_STRINGS
#         endif
#         define PDK_COMPILER_VARIADIC_TEMPLATES
#      endif
#      if __INTEL_COMPILER >= 1300
#         define PDK_COMPILER_ATOMICS
//        constexpr support is only partial
//#       define PDK_COMPILER_CONSTEXPR
#         define PDK_COMPILER_INITIALIZER_LISTS
#         define PDK_COMPILER_UNIFORM_INIT
#         define PDK_COMPILER_NOEXCEPT
#      endif
#      if __INTEL_COMPILER >= 1400
//       causes issues with QArrayData and PDK_Private::RefCount - Intel issue ID 6000056211, bug DPD200534796
//#       define PDK_COMPILER_CONSTEXPR
#         define PDK_COMPILER_DELEGATING_CONSTRUCTORS
#         define PDK_COMPILER_EXPLICIT_CONVERSIONS
#         define PDK_COMPILER_EXPLICIT_OVERRIDES
#         define PDK_COMPILER_NONSTATIC_MEMBER_INIT
#         define PDK_COMPILER_RANGE_FOR
#         define PDK_COMPILER_RAW_STRINGS
#         define PDK_COMPILER_REF_QUALIFIERS
#         define PDK_COMPILER_UNICODE_STRINGS
#         define PDK_COMPILER_UNRESTRICTED_UNIONS
#      endif
#      if __INTEL_COMPILER >= 1500
#         if __INTEL_COMPILER * 100 + __INTEL_COMPILER_UPDATE >= 150001
//        the bug mentioned above is fixed in 15.0.1
#            define PDK_COMPILER_CONSTEXPR
#         endif
#         define PDK_COMPILER_ALIGNAS
#         define PDK_COMPILER_ALIGNOF
#         define PDK_COMPILER_INHERITING_CONSTRUCTORS
#         ifndef PDK_OS_OSX
//        C++11 thread_local is broken on OS X (Clang doesn't support it either)
#            define PDK_COMPILER_THREAD_LOCAL
#         endif
#         define PDK_COMPILER_UDL
#      endif
#      ifdef _MSC_VER
#         if _MSC_VER == 1700
//       <initializer_list> is missing with MSVC 2012 (it's present in 2010, 2013 and up)
#            undef PDK_COMPILER_INITIALIZER_LISTS
#         endif
#         if _MSC_VER < 1900
//        ICC disables unicode string support when compatibility mode with MSVC 2013 or lower is active
#            undef PDK_COMPILER_UNICODE_STRINGS
//        Even though ICC knows about ref-qualified members, MSVC 2013 or lower doesn't, so
//        certain member functions (like QString::toUpper) may be missing from the DLLs.
#            undef PDK_COMPILER_REF_QUALIFIERS
//       Disable constexpr unless the MS headers have constexpr in all the right places too
//       (like std::numeric_limits<T>::max())
#            undef PDK_COMPILER_CONSTEXPR
#         endif
#      endif
#   endif
#endif

#if defined(PDK_CC_CLANG) && !defined(PDK_CC_INTEL)
/* General C++ features */
#   define PDK_COMPILER_RESTRICTED_VLA
#   define PDK_COMPILER_THREADSAFE_STATICS
#   if __has_feature(attribute_deprecated_with_message)
#      define PDK_DECL_DEPRECATED_X(text) __attribute__ ((__deprecated__(text)))
#   endif

// Clang supports binary literals in C, C++98 and C++11 modes
// It's been supported "since the dawn of time itself" (cf. commit 179883)
#   if __has_extension(cxx_binary_literals)
#      define PDK_COMPILER_BINARY_LITERALS
#   endif

// Variadic macros are supported for gnu++98, c++11, c99 ... since 2.9
#   if PDK_CC_CLANG >= 209
#      if !defined(__STRICT_ANSI__) || defined(__GXX_EXPERIMENTAL_CXX0X__) \
   || (defined(__cplusplus) && (__cplusplus >= 201103L)) \
   || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
#         define PDK_COMPILER_VARIADIC_MACROS
#      endif
#   endif

/* C++11 features, see http://clang.llvm.org/cxx_status.html */
#   if __cplusplus >= 201103L  || defined(__GXX_EXPERIMENTAL_CXX0X__)
/* Detect C++ features using __has_feature(), see http://clang.llvm.org/docs/LanguageExtensions.html#cxx11 */
#      if __has_feature(cxx_alignas)
#         define PDK_COMPILER_ALIGNAS
#         define PDK_COMPILER_ALIGNOF
#      endif
#      if __has_feature(cxx_atomic) && __has_include(<atomic>)
#         define PDK_COMPILER_ATOMICS
#      endif
#      if __has_feature(cxx_attributes)
#         define PDK_COMPILER_ATTRIBUTES
#      endif
#      if __has_feature(cxx_auto_type)
#         define PDK_COMPILER_AUTO_FUNCTION
#         define PDK_COMPILER_AUTO_TYPE
#      endif
#      if __has_feature(cxx_strong_enums)
#         define PDK_COMPILER_CLASS_ENUM
#      endif
#      if __has_feature(cxx_constexpr) && PDK_CC_CLANG > 302 /* CLANG 3.2 has bad/partial support */
#         define PDK_COMPILER_CONSTEXPR
#      endif
#      if __has_feature(cxx_decltype) /* && __has_feature(cxx_decltype_incomplete_return_types) */
#         define PDK_COMPILER_DECLTYPE
#      endif
#      if __has_feature(cxx_defaulted_functions)
#         define PDK_COMPILER_DEFAULT_MEMBERS
#      endif
#      if __has_feature(cxx_deleted_functions)
#         define PDK_COMPILER_DELETE_MEMBERS
#      endif
#      if __has_feature(cxx_delegating_constructors)
#         define PDK_COMPILER_DELEGATING_CONSTRUCTORS
#      endif
#      if __has_feature(cxx_explicit_conversions)
#         define PDK_COMPILER_EXPLICIT_CONVERSIONS
#      endif
#      if __has_feature(cxx_override_control)
#         define PDK_COMPILER_EXPLICIT_OVERRIDES
#      endif
#      if __has_feature(cxx_inheriting_constructors)
#         define PDK_COMPILER_INHERITING_CONSTRUCTORS
#      endif
#      if __has_feature(cxx_generalized_initializers)
#         define PDK_COMPILER_INITIALIZER_LISTS
#         define PDK_COMPILER_UNIFORM_INIT /* both covered by this feature macro, according to docs */
#      endif
#      if __has_feature(cxx_lambdas)
#         define PDK_COMPILER_LAMBDA
#      endif
#      if __has_feature(cxx_noexcept)
#         define PDK_COMPILER_NOEXCEPT
#      endif
#      if __has_feature(cxx_nonstatic_member_init)
#         define PDK_COMPILER_NONSTATIC_MEMBER_INIT
#      endif
#      if __has_feature(cxx_nullptr)
#         define PDK_COMPILER_NULLPTR
#      endif
#      if __has_feature(cxx_range_for)
#         define PDK_COMPILER_RANGE_FOR
#      endif
#      if __has_feature(cxx_raw_string_literals)
#         define PDK_COMPILER_RAW_STRINGS
#      endif
#      if __has_feature(cxx_reference_qualified_functions)
#         define PDK_COMPILER_REF_QUALIFIERS
#      endif
#      if __has_feature(cxx_rvalue_references)
#         define PDK_COMPILER_RVALUE_REFS
#      endif
#      if __has_feature(cxx_static_assert)
#         define PDK_COMPILER_STATIC_ASSERT
#      endif
#      if __has_feature(cxx_alias_templates)
#         define PDK_COMPILER_TEMPLATE_ALIAS
#      endif
#      if __has_feature(cxx_thread_local)
#         if !defined(__FreeBSD__) /* FreeBSD clang fails on __cxa_thread_atexit */
#            define PDK_COMPILER_THREAD_LOCAL
#         endif
#      endif
#      if __has_feature(cxx_user_literals)
#         define PDK_COMPILER_UDL
#      endif
#      if __has_feature(cxx_unicode_literals)
#         define PDK_COMPILER_UNICODE_STRINGS
#      endif
#      if __has_feature(cxx_unrestricted_unions)
#         define PDK_COMPILER_UNRESTRICTED_UNIONS
#      endif
#      if __has_feature(cxx_variadic_templates)
#         define PDK_COMPILER_VARIADIC_TEMPLATES
#      endif
/* Features that have no __has_feature() check */
#      if PDK_CC_CLANG >= 209 /* since clang 2.9 */
#         define PDK_COMPILER_EXTERN_TEMPLATES
#      endif
#   endif
/* C++1y features, deprecated macros. Do not update this list. */
#   if __cplusplus > 201103L
#      if __has_feature(cxx_generic_lambda)
#         define PDK_COMPILER_GENERIC_LAMBDA
#      endif
#      if __has_feature(cxx_init_capture)
#         define PDK_COMPILER_LAMBDA_CAPTURES
#      endif
#      if __has_feature(cxx_relaxed_constexpr)
#         define PDK_COMPILER_RELAXED_CONSTEXPR_FUNCTIONS
#      endif
#      if __has_feature(cxx_decltype_auto) && __has_feature(cxx_return_type_deduction)
#         define PDK_COMPILER_RETURN_TYPE_DEDUCTION
#      endif
#      if __has_feature(cxx_variable_templates)
#         define PDK_COMPILER_VARIABLE_TEMPLATES
#      endif
#      if __has_feature(cxx_runtime_array)
#         define PDK_COMPILER_VLA
#      endif
#   endif

#   if defined(__has_warning)
#      if __has_warning("-Wunused-private-field")
#         define PDK_DECL_UNUSED_MEMBER PDK_DECL_UNUSED
#      endif
#   endif
#endif // PDK_CC_CLANG

#if defined(PDK_CC_GNU) && !defined(PDK_CC_INTEL) && !defined(PDK_CC_CLANG)
#   define PDK_COMPILER_RESTRICTED_VLA
#   define PDK_COMPILER_THREADSAFE_STATICS
#   if PDK_CC_GNU >= 403
//   GCC supports binary literals in C, C++98 and C++11 modes
#      define PDK_COMPILER_BINARY_LITERALS
#   endif
#   if !defined(__STRICT_ANSI__) || defined(__GXX_EXPERIMENTAL_CXX0X__) \
   || (defined(__cplusplus) && (__cplusplus >= 201103L)) \
   || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
// Variadic macros are supported for gnu++98, c++11, C99 ... since forever (gcc 2.97)
#      define PDK_COMPILER_VARIADIC_MACROS
#   endif
#   if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#      if PDK_CC_GNU >= 403
/* C++11 features supported in GCC 4.3: */
#         define PDK_COMPILER_DECLTYPE
#         define PDK_COMPILER_RVALUE_REFS
#         define PDK_COMPILER_STATIC_ASSERT
#      endif
#      if PDK_CC_GNU >= 404
#         /* C++11 features supported in GCC 4.4: */
#         define PDK_COMPILER_AUTO_FUNCTION
#         define PDK_COMPILER_AUTO_TYPE
#         define PDK_COMPILER_EXTERN_TEMPLATES
#         define PDK_COMPILER_UNIFORM_INIT
#         define PDK_COMPILER_UNICODE_STRINGS
#         define PDK_COMPILER_VARIADIC_TEMPLATES
#      endif
#      if PDK_CC_GNU >= 405
#         /* C++11 features supported in GCC 4.5: */
#         define PDK_COMPILER_EXPLICIT_CONVERSIONS
/* GCC 4.4 implements initializer_list but does not define typedefs required by the standard. */
#         define PDK_COMPILER_INITIALIZER_LISTS
#         define PDK_COMPILER_LAMBDA
#         define PDK_COMPILER_RAW_STRINGS
#         define PDK_COMPILER_CLASS_ENUM
#      endif
#      if PDK_CC_GNU >= 406
/* Pre-4.6 compilers implement a non-final snapshot of N2346, hence default and delete
        * functions are supported only if they are public. Starting from 4.6, GCC handles
        * final version - the access modifier is not relevant. */
#         define PDK_COMPILER_DEFAULT_MEMBERS
#         define PDK_COMPILER_DELETE_MEMBERS
/* C++11 features supported in GCC 4.6: */
#         define PDK_COMPILER_CONSTEXPR
#         define PDK_COMPILER_NULLPTR
#         define PDK_COMPILER_UNRESTRICTED_UNIONS
#         define PDK_COMPILER_RANGE_FOR
#      endif
#      if PDK_CC_GNU >= 407
/* GCC 4.4 implemented <atomic> and std::atomic using its old intrinsics.
 * However, the implementation is incomplete for most platforms until GCC 4.7:
 * instead, std::atomic would use an external lock. Since we need an std::atomic
 * that is behavior-compatible with QBasicAtomic, we only enable it here */
#         define PDK_COMPILER_ATOMICS
/* GCC 4.6.x has problems dealing with noexcept expressions,
        * so turn the feature on for 4.7 and above, only */
#         define PDK_COMPILER_NOEXCEPT
/* C++11 features supported in GCC 4.7: */
#         define PDK_COMPILER_NONSTATIC_MEMBER_INIT
#         define PDK_COMPILER_DELEGATING_CONSTRUCTORS
#         define PDK_COMPILER_EXPLICIT_OVERRIDES
#         define PDK_COMPILER_TEMPLATE_ALIAS
#         define PDK_COMPILER_UDL
#      endif
#      if PDK_CC_GNU >= 408
#         define PDK_COMPILER_ATTRIBUTES
#         define PDK_COMPILER_ALIGNAS
#         define PDK_COMPILER_ALIGNOF
#         define PDK_COMPILER_INHERITING_CONSTRUCTORS
#         define PDK_COMPILER_THREAD_LOCAL
#         if PDK_CC_GNU > 408 || __GNUC_PATCHLEVEL__ >= 1
#            define PDK_COMPILER_REF_QUALIFIERS
#         endif
#      endif
/* C++11 features are complete as of GCC 4.8.1 */
#   endif
#   if __cplusplus > 201103L
#      if PDK_CC_GNU >= 409
#         define PDK_COMPILER_LAMBDA_CAPTURES
#         define PDK_COMPILER_RETURN_TYPE_DEDUCTION
#      endif
#   endif
#endif

#if defined(PDK_CC_MSVC) && !defined(PDK_CC_INTEL)
#   if defined(__cplusplus)
#      if _MSC_VER >= 1400
/* C++11 features supported in VC8 = VC2005: */
#         define PDK_COMPILER_VARIADIC_MACROS
#         ifndef __cplusplus_cli
/* 2005 supports the override and final contextual keywords, in
           the same positions as the C++11 variants, but 'final' is
           called 'sealed' instead:
           http://msdn.microsoft.com/en-us/library/0w2w91tf%28v=vs.80%29.aspx
           The behavior is slightly different in C++/CLI, which requires the
           "virtual" keyword to be present too, so don't define for that.
           So don't define PDK_COMPILER_EXPLICIT_OVERRIDES (since it's not
           the same as the C++11 version), but define the PDK_DECL_* flags
           accordingly: */
#            define PDK_DECL_OVERRIDE override
#            define PDK_DECL_FINAL sealed
#         endif
#      endif
#      if _MSC_VER >= 1600
/* C++11 features supported in VC10 = VC2010: */
#         define PDK_COMPILER_AUTO_FUNCTION
#         define PDK_COMPILER_AUTO_TYPE
#         define PDK_COMPILER_DECLTYPE
#         define PDK_COMPILER_EXTERN_TEMPLATES
#         define PDK_COMPILER_LAMBDA
#         define PDK_COMPILER_NULLPTR
#         define PDK_COMPILER_RVALUE_REFS
#         define PDK_COMPILER_STATIC_ASSERT
//  MSVC's library has std::initializer_list, but the compiler does not support the braces initialization
//#       define PDK_COMPILER_INITIALIZER_LISTS
//#       define PDK_COMPILER_UNIFORM_INIT
#      endif
#      if _MSC_VER >= 1700
/* C++11 features supported in VC11 = VC2012: */
#         undef PDK_DECL_OVERRIDE               /* undo 2005/2008 settings... */
#         undef PDK_DECL_FINAL                  /* undo 2005/2008 settings... */
#         define PDK_COMPILER_EXPLICIT_OVERRIDES /* ...and use std C++11 now   */
#         define PDK_COMPILER_CLASS_ENUM
#         define PDK_COMPILER_ATOMICS
#      endif /* VC 11 */
#      if _MSC_VER >= 1800
/* C++11 features in VC12 = VC2013 */
/* Implemented, but can't be used on move special members */
/* #      define PDK_COMPILER_DEFAULT_MEMBERS */
#         define PDK_COMPILER_DELETE_MEMBERS
#         define PDK_COMPILER_DELEGATING_CONSTRUCTORS
#         define PDK_COMPILER_EXPLICIT_CONVERSIONS
#         define PDK_COMPILER_NONSTATIC_MEMBER_INIT
// implemented, but nested initialization fails (eg tst_qvector): http://connect.microsoft.com/VisualStudio/feedback/details/800364/initializer-list-calls-object-destructor-twice
//       #define PDK_COMPILER_INITIALIZER_LISTS
// implemented in principle, but has a bug that makes it unusable: http://connect.microsoft.com/VisualStudio/feedback/details/802058/c-11-unified-initialization-fails-with-c-style-arrays
//       #define PDK_COMPILER_UNIFORM_INIT
#         define PDK_COMPILER_RAW_STRINGS
#         define PDK_COMPILER_TEMPLATE_ALIAS
#         define PDK_COMPILER_VARIADIC_TEMPLATES
#      endif /* VC 12 */
#      if _MSC_FULL_VER >= 180030324 // VC 12 SP 2 RC
#         define PDK_COMPILER_INITIALIZER_LISTS
#      endif /* VC 12 SP 2 RC */
#      if _MSC_VER >= 1900
/* C++11 features in VC14 = VC2015 */
#         define PDK_COMPILER_DEFAULT_MEMBERS
#         define PDK_COMPILER_ALIGNAS
#         define PDK_COMPILER_ALIGNOF
// Partial support, insufficient for PDK_
//#       define PDK_COMPILER_CONSTEXPR
#         define PDK_COMPILER_INHERITING_CONSTRUCTORS
#         define PDK_COMPILER_NOEXCEPT
#         define PDK_COMPILER_RANGE_FOR
#         define PDK_COMPILER_REF_QUALIFIERS
#         define PDK_COMPILER_THREAD_LOCAL
// Broken, see PDK_BUG-47224 and https://connect.microsoft.com/VisualStudio/feedback/details/1549785
//#       define PDK_COMPILER_THREADSAFE_STATICS
#         define PDK_COMPILER_UDL
#         define PDK_COMPILER_UNICODE_STRINGS
// Uniform initialization is not working yet -- build errors with QUuid
//#       define PDK_COMPILER_UNIFORM_INIT
#         define PDK_COMPILER_UNRESTRICTED_UNIONS
#      endif
#      if _MSC_FULL_VER >= 190023419
#         define PDK_COMPILER_ATTRIBUTES
// Almost working, see https://connect.microsoft.com/VisualStudio/feedback/details/2011648
//#       define PDK_COMPILER_CONSTEXPR
#         define PDK_COMPILER_THREADSAFE_STATICS
#         define PDK_COMPILER_UNIFORM_INIT
#      endif
#      if _MSC_VER >= 1910
#         define PDK_COMPILER_CONSTEXPR
#      endif
#   endif /* __cplusplus */
#endif  /* PDK_CC_MSVC */

#ifdef __cplusplus
#   include <utility>
// By default, QNX 7.0 uses libc++ (from LLVM) and
// QNX 6.X uses Dinkumware's libcpp. In all versions,
// it is also possible to use GNU libstdc++.

// For Dinkumware, some features must be disabled
// (mostly because of library problems).
// Dinkumware is assumed when __GLIBCXX__ (GNU libstdc++)
// and _LIBCPP_VERSION (LLVM libc++) are both absent.
#   if defined(PDK_OS_QNX)
// By default, QNX 7.0 uses libc++ (from LLVM) and
// QNX 6.X uses Dinkumware's libcpp. In all versions,
// it is also possible to use GNU libstdc++.

// For Dinkumware, some features must be disabled
// (mostly because of library problems).
// Dinkumware is assumed when __GLIBCXX__ (GNU libstdc++)
// and _LIBCPP_VERSION (LLVM libc++) are both absent.
#      if !defined(__GLIBCXX__) && !defined(_LIBCPP_VERSION)
// Older versions of libcpp (QNX 650) do not support C++11 features
// _HAS_* macros are set to 1 by toolchains that actually include
// Dinkum C++11 libcpp.
#         if !defined(_HAS_CPP0X) || !_HAS_CPP0X
#            undef PDK_COMPILER_INITIALIZER_LISTS
#            undef PDK_COMPILER_RVALUE_REFS
#            undef PDK_COMPILER_REF_QUALIFIERS
#            undef PDK_COMPILER_UNICODE_STRINGS
#            undef PDK_COMPILER_NOEXCEPT
#         endif // !_HAS_CPP0X
#         if !defined(_HAS_NULLPTR_T) || !_HAS_NULLPTR_T
#            undef PDK_COMPILER_NULLPTR
#         endif
#         if !defined(_HAS_CONSTEXPR) || !_HAS_CONSTEXPR
// The libcpp is missing constexpr keywords on important functions like std::numeric_limits<>::min()
// Disable constexpr support on QNX even if the compiler supports it
#            undef PDK_COMPILER_CONSTEXPR
#         endif // !_HAS_CONSTEXPR
#      endif // !__GLIBCXX__ && !_LIBCPP_VERSION
#   endif // PDK_OS_QNX
#   if (defined(PDK_CC_CLANG) || defined(PDK_CC_INTEL)) && defined(PDK_OS_MAC) && defined(__GNUC_LIBSTD__) \
   && ((__GNUC_LIBSTD__-0) * 100 + __GNUC_LIBSTD_MINOR__-0 <= 402)
// Apple has not updated libstdc++ since 2007, which means it does not have
// <initializer_list> or std::move. Let's disable these features
#      undef PDK_COMPILER_INITIALIZER_LISTS
#      undef PDK_COMPILER_RVALUE_REFS
#      undef PDK_COMPILER_REF_QUALIFIERS
// Also disable <atomic>, since it's clearly not there
#      undef PDK_COMPILER_ATOMICS
#   endif
#   if defined(PDK_CC_CLANG) && defined(PDK_CC_INTEL) && PDK_CC_INTEL >= 1500
// ICC 15.x and 16.0 have their own implementation of std::atomic, which is activated when in Clang mode
// (probably because libc++'s <atomic> on OS X failed to compile), but they're missing some
// critical definitions. (Reported as Intel Issue ID 6000117277)
#      define __USE_CONSTEXPR 1
#      define __USE_NOEXCEPT 1
#   endif
#   if defined(PDK_CC_MSVC) && defined(PDK_CC_CLANG)
// Clang and the Intel compiler support more C++ features than the Microsoft compiler
// so make sure we don't enable them if the MS headers aren't properly adapted.
#      ifndef _HAS_CONSTEXPR
#         undef PDK_COMPILER_CONSTEXPR
#      endif
#      ifndef _HAS_DECLTYPE
#         undef PDK_COMPILER_DECLTYPE
#      endif
#      ifndef _HAS_INITIALIZER_LISTS
#         undef PDK_COMPILER_INITIALIZER_LISTS
#      endif
#      ifndef _HAS_NULLPTR_T
#         undef PDK_COMPILER_NULLPTR
#      endif
#      ifndef _HAS_RVALUE_REFERENCES
#         undef PDK_COMPILER_RVALUE_REFS
#      endif
#      ifndef _HAS_SCOPED_ENUM
#         undef PDK_COMPILER_CLASS_ENUM
#      endif
#      ifndef _HAS_TEMPLATE_ALIAS
#         undef PDK_COMPILER_TEMPLATE_ALIAS
#      endif
#      ifndef _HAS_VARIADIC_TEMPLATES
#         undef PDK_COMPILER_VARIADIC_TEMPLATES
#      endif
#   endif
#   if defined(PDK_COMPILER_THREADSAFE_STATICS) && defined(PDK_OS_MAC)
// Apple's low-level implementation of the C++ support library
// (libc++abi.dylib, shared between libstdc++ and libc++) has deadlocks. The
// C++11 standard requires the deadlocks to be removed, so this will eventually
// be fixed; for now, let's disable this.
#      undef PDK_COMPILER_THREADSAFE_STATICS
#   endif
#endif

/*
 * C++11 keywords and expressions
 */
#ifdef PDK_COMPILER_NULLPTR
#   define PDK_NULLPTR nullptr
#else
#   define PDK_NULLPTR NULL
#endif

#ifdef PDK_COMPILER_DEFAULT_MEMBERS
#   define PDK_DECL_EPDK_DEFAULT = default
#else
#   define PDk_DECL_EPDK_DEFAULT
#endif

#ifdef PDK_COMPILER_DELETE_MEMBERS
#   define PDK_DECL_EPDK_DELETE = delete
#else
#   define PDK_DECL_EPDK_DELETE
#endif

// Don't break code that is already using PDK_COMPILER_DEFAULT_DELETE_MEMBERS
#if defined(PDK_COMPILER_DEFAULT_MEMBERS) && defined(PDK_COMPILER_DELETE_MEMBERS)
#   define PDK_COMPILER_DEFAULT_DELETE_MEMBERS
#endif

#if defined(PDK_COMPILER_CONSTEXPR)
#   if defined(__cpp_constexpr) && __cpp_constexpr-0 >= 201304
#      define PDK_DECL_CONSTEXPR constexpr
#      define PDK_DECL_RELAXED_CONSTEXPR constexpr
#      define PDK_CONSTEXPR constexpr
#      define PDK_RELAXED_CONSTEXPR constexpr
#   else
#      define PDK_DECL_CONSTEXPR constexpr
#      define PDK_DECL_RELAXED_CONSTEXPR
#      define PDK_CONSTEXPR constexpr
#      define PDK_RELAXED_CONSTEXPR const
#   endif
#else
#   define PDK_DECL_CONSTEXPR
#   define PDK_DECL_RELAXED_CONSTEXPR
#   define PDK_CONSTEXPR const
#   define PDK_RELAXED_CONSTEXPR const
#endif

#ifdef PDK_COMPILER_EXPLICIT_OVERRIDES
#   define PDK_DECL_OVERRIDE override
#   define PDK_DECL_FINAL final
#else
#   ifndef PDK_DECL_OVERRIDE
#      define PDK_DECL_OVERRIDE
#   endif
#   ifndef PDK_DECL_FINAL
#      define PDK_DECL_FINAL
#   endif
#endif

#ifdef PDK_COMPILER_NOEXCEPT
#   define PDK_DECL_NOEXCEPT noexcept
#   define PDK_DECL_NOEXCEPT_EXPR(x) noexcept(x)
#   ifdef PDK_DECL_NOTHROW
#      undef PDK_DECL_NOTHROW /* override with C++11 noexcept if available */
#   endif
#else
#   define PDK_DECL_NOEXCEPT
#   define PDK_DECL_NOEXCEPT_EXPR(x)
#endif

#ifndef PDK_DECL_NOTHROW
#   define PDK_DECL_NOTHROW PDK_DECL_NOEXCEPT
#endif

#if defined(PDK_COMPILER_ALIGNOF)
#   undef PDK_ALIGNOF
#   define PDK_ALIGNOF(x) alignof(x)
#endif

#if defined(PDK_COMPILER_ALIGNAS)
#   undef PDK_DECL_ALIGN
#   define PDK_DECL_ALIGN(n) alignas(n)
#endif

/*
 * Fallback macros to certain compiler features
 */
#ifndef PDK_NORETURN
#   define PDK_NORETURN
#endif

#ifndef PDK_LIKELY
#   define PDK_LIKELY(x) (x)
#endif

#ifndef  PDK_UNLIKELY
#   define PDK_UNLIKELY(x) (x)
#endif

#ifndef PDK_ASSUME_IMPL
#   define PDK_ASSUME_IMPL(expr) PDK_noop()
#endif

#ifndef PDK_UNREACHABLE_IMPL
#   define PDK_UNREACHABLE_IMPL() PDK_noop()
#endif

#ifndef PDK_ALLOC_SIZE
#   define PDK_ALLOC_SIZE(x)
#endif

#ifndef PDK_REQUIRED_RESULT
#   define PDK_REQUIRED_RESULT
#endif

#ifndef PDK_DECL_DEPRECATED
#   define PDK_DECL_DEPRECATED
#endif

#ifndef PDK_DECL_VARIABLE_DEPRECATED
#   define PDK_DECL_VARIABLE_DEPRECATED PDK_DECL_DEPRECATED
#endif

#ifndef PDK_DECL_DEPRECATED_X
#   define PDK_DECL_DEPRECATED_X(text) PDK_DECL_DEPRECATED
#endif

#ifndef PDK_DECL_EXPORT
#   define PDK_DECL_EXPORT
#endif

#ifndef PDK_DECL_IMPORT
#   define PDK_DECL_IMPORT
#endif

#ifndef PDK_DECL_HIDDEN
#   define PDK_DECL_HIDDEN
#endif

#ifndef PDK_DECL_UNUSED
#   define PDK_DECL_UNUSED
#endif

#ifndef PDK_DECL_UNUSED_MEMBER
#   define PDK_DECL_UNUSED_MEMBER
#endif

#ifndef PDK_FUNC_INFO
#   if defined(PDK_OS_SOLARIS) || defined(PDK_CC_XLC)
#      define PDK_FUNC_INFO __FILE__ "(line number unavailable)"
#   else
#      define PDK_FUNC_INFO __FILE__ ":" PDK_STRINGIFY(__LINE__)
#   endif
#endif

#ifndef PDK_DECL_CF_RETURNS_RETAINED
#   define PDK_DECL_CF_RETURNS_RETAINED
#endif

#ifndef PDK_DECL_NS_RETURNS_AUTORELEASED
#   define PDK_DECL_NS_RETURNS_AUTORELEASED
#endif

#ifndef PDK_DECL_PURE_FUNCTION
#   define PDK_DECL_PURE_FUNCTION
#endif

#ifndef PDK_DECL_CONST_FUNCTION
#   define PDK_DECL_CONST_FUNCTION PDK_DECL_PURE_FUNCTION
#endif

#ifndef PDK_MAKE_UNCHECKED_ARRAY_ITERATOR
#   define PDK_MAKE_UNCHECKED_ARRAY_ITERATOR(x) (x)
#endif

#ifndef PDK_MAKE_CHECKED_ARRAY_ITERATOR
#   define PDK_MAKE_CHECKED_ARRAY_ITERATOR(x, N) (x)
#endif

/*
 * SG10's SD-6 feature detection and some useful extensions from Clang and GCC
 * https://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations
 * http://clang.llvm.org/docs/LanguageExtensions.html#feature-checking-macros
 */
#ifdef __has_builtin
#   define PDK_HAS_BUILTIN(x) __has_builtin(x)
#else
#   define PDK_HAS_BUILTIN(x) 0
#endif

#ifdef __has_attribute
#   define PDK_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
#   define PDK_HAS_ATTRIBUTE(x) 0
#endif

#ifdef __has_cpp_attribute
#   define PDK_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#   define PDK_HAS_CPP_ATTRIBUTE(x) 0
#endif

#ifdef __has_include
#   define PDK_HAS_INCLUDE(x) __has_include(x)
#else
#   define PDK_HAS_INCLUDE(x) 0
#endif

#ifdef __has_include_next
#   define PDK_HAS_INCLUDE_NEXT(x) __has_include_next(x)
#else
#   define PDK_HAS_INCLUDE_NEXT(x) 0
#endif

/*
 * Warning/diagnostic handling
 */
#define PDK_DO_PRAGMA(text) _Pragma(#text)
#if defined(PDK_CC_INTEL) && defined(PDK_CC_MSVC)
/* icl.exe: Intel compiler on Windows */
#   undef PDK_DO_PRAGMA                           /* not needed */
#   define PDK_WARNING_PUSH                       __pragma(warning(push))
#   define PDK_WARNING_POP                        __pragma(warning(pop))
#   define PDK_WARNING_DISABLE_MSVC(number)
#   define PDK_WARNING_DISABLE_INTEL(number)      __pragma(warning(disable: number))
#   define PDK_WARNING_DISABLE_CLANG(text)
#   define PDK_WARNING_DISABLE_GCC(text)
#   define PDK_WARNING_DISABLE_DEPRECATED         PDK_WARNING_DISABLE_INTEL(1478 1786)
#elif defined(PDK_CC_INTEL)
/* icc: Intel compiler on Linux or OS X */
#   define PDK_WARNING_PUSH                       PDK_DO_PRAGMA(warning(push))
#   define PDK_WARNING_POP                        PDK_DO_PRAGMA(warning(pop))
#   define PDK_WARNING_DISABLE_INTEL(number)      PDK_DO_PRAGMA(warning(disable: number))
#   define PDK_WARNING_DISABLE_MSVC(number)
#   define PDK_WARNING_DISABLE_CLANG(text)
#   define PDK_WARNING_DISABLE_GCC(text)
#   define PDK_WARNING_DISABLE_DEPRECATED         PDK_WARNING_DISABLE_INTEL(1478 1786)
#elif defined(PDK_CC_MSVC) && _MSC_VER >= 1500 && !defined(PDK_CC_CLANG)
#   undef PDK_DO_PRAGMA                           /* not needed */
#   define PDK_WARNING_PUSH                       __pragma(warning(push))
#   define PDK_WARNING_POP                        __pragma(warning(pop))
#   define PDK_WARNING_DISABLE_MSVC(number)       __pragma(warning(disable: number))
#   define PDK_WARNING_DISABLE_INTEL(number)
#   define PDK_WARNING_DISABLE_CLANG(text)
#   define PDK_WARNING_DISABLE_GCC(text)
#   define PDK_WARNING_DISABLE_DEPRECATED         PDK_WARNING_DISABLE_MSVC(4996)
#elif defined(PDK_CC_CLANG)
#   define PDK_WARNING_PUSH                       PDK_DO_PRAGMA(clang diagnostic push)
#   define PDK_WARNING_POP                        PDK_DO_PRAGMA(clang diagnostic pop)
#   define PDK_WARNING_DISABLE_CLANG(text)        PDK_DO_PRAGMA(clang diagnostic ignored text)
#   define PDK_WARNING_DISABLE_GCC(text)
#   define PDK_WARNING_DISABLE_INTEL(number)
#   define PDK_WARNING_DISABLE_MSVC(number)
#   define PDK_WARNING_DISABLE_DEPRECATED         PDK_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
#elif defined(PDK_CC_GNU) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 406)
#   define PDK_WARNING_PUSH                       PDK_DO_PRAGMA(GCC diagnostic push)
#   define PDK_WARNING_POP                        PDK_DO_PRAGMA(GCC diagnostic pop)
#   define PDK_WARNING_DISABLE_GCC(text)          PDK_DO_PRAGMA(GCC diagnostic ignored text)
#   define PDK_WARNING_DISABLE_CLANG(text)
#   define PDK_WARNING_DISABLE_INTEL(number)
#   define PDK_WARNING_DISABLE_MSVC(number)
#   define PDK_WARNING_DISABLE_DEPRECATED         PDK_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
#else       // All other compilers, GCC < 4.6 and MSVC < 2008
#   define PDK_WARNING_DISABLE_GCC(text)
#   define PDK_WARNING_PUSH
#   define PDK_WARNING_POP
#   define PDK_WARNING_DISABLE_INTEL(number)
#   define PDK_WARNING_DISABLE_MSVC(number)
#   define PDK_WARNING_DISABLE_CLANG(text)
#   define PDK_WARNING_DISABLE_GCC(text)
#   define PDK_WARNING_DISABLE_DEPRECATED
#endif

#define PDK_UNREACHABLE() \
    do {\
        PDK_ASSERT_X(false, "PDK_UNREACHABLE()", "PDK_UNREACHABLE was reached");\
        PDK_UNREACHABLE_IMPL();\
    } while (0)

#define PDK_ASSUME(Expr) \
    do {\
        const bool valueOfExpression = Expr;\
        PDK_ASSERT_X(valueOfExpression, "PDK_ASSUME()", "Assumption in PDK_ASSUME(\"" #Expr "\") was not correct");\
        PDK_ASSUME_IMPL(valueOfExpression);\
    } while (0)

#if PDK_HAS_CPP_ATTRIBUTE(fallthrough)
#   define PDK_FALLTHROUGH() [[fallthrough]]
#elif defined(__cplusplus)
/* Clang can not parse namespaced attributes in C mode, but defines __has_cpp_attribute */
#   if PDK_HAS_CPP_ATTRIBUTE(clang::fallthrough)
#      define PDK_FALLTHROUGH() [[clang::fallthrough]]
#   elif PDK_HAS_CPP_ATTRIBUTE(gnu::fallthrough)
#      define PDK_FALLTHROUGH() [[gnu::fallthrough]]
#   endif
#endif

#ifndef PDK_FALLTHROUGH
#   if defined(PDK_CC_GNU) && PDK_CC_GNU >= 700
#      define PDK_FALLTHROUGH() __attribute__((fallthrough))
#   else
#      define PDK_FALLTHROUGH() (void)0
#   endif
#endif

/*
    Sanitize compiler feature availability
*/
#if !defined(PDK_PROCESSOR_X86)
#   undef PDK_COMPILER_SUPPORTS_SSE2
#   undef PDK_COMPILER_SUPPORTS_SSE3
#   undef PDK_COMPILER_SUPPORTS_SSSE3
#   undef PDK_COMPILER_SUPPORTS_SSE4_1
#   undef PDK_COMPILER_SUPPORTS_SSE4_2
#   undef PDK_COMPILER_SUPPORTS_AVX
#   undef PDK_COMPILER_SUPPORTS_AVX2
#endif

#if !defined(PDK_PROCESSOR_ARM)
#   undef PDK_COMPILER_SUPPORTS_NEON
#endif

#if !defined(PDK_PROCESSOR_MIPS)
#   undef PDK_COMPILER_SUPPORTS_MIPS_DSP
#   undef PDK_COMPILER_SUPPORTS_MIPS_DSPR2
#endif

#endif // PDK_GLOBAL_COMPILERDETECTION_H
