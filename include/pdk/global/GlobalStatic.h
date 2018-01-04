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
// Created by softboy on 2018/01/04.

#ifndef PDK_GLOBAL_GLOBAL_STATIC_H
#define PDK_GLOBAL_GLOBAL_STATIC_H

#include "pdk/global/Global.h"
#include "pdk/base/os/thread/Atomic.h"
#include <mutex>

namespace pdk {
namespace globalstatic {

enum GuardValues
{
   Destroyed = -2,
   Initialized = -1,
   Uninitialized = 0,
   Initializing = 1
};

#if defined(PDK_COMPILER_THREADSAFE_STATICS)
// some compilers support thread-safe statics
// The IA-64 C++ ABI requires this, so we know that all GCC versions since 3.4
// support it. C++11 also requires this behavior.
// Clang and Intel CC masquerade as GCC when compiling on Linux.
//
// Apple's libc++abi however uses a global lock for initializing local statics,
// which will block other threads also trying to initialize a local static
// until the constructor returns ...
// We better avoid these kind of problems by using our own locked implementation.

#  if defined(PDK_OS_UNIX) && defined(PDK_CC_INTEL)
// Work around Intel issue ID 6000058488:
// local statics inside an inline function inside an anonymous namespace are global
// symbols (this affects the IA-64 C++ ABI, so OS X and Linux only)
#     define PDK_GLOBAL_STATIC_INTERNAL_DECORATION PDK_DECL_HIDDEN
#  else
#     define PDK_GLOBAL_STATIC_INTERNAL_DECORATION PDK_DECL_HIDDEN inline
#  endif

#define PDK_GLOBAL_STATIC_INTERNAL(ARGS)                          \
    PDK_GLOBAL_STATIC_INTERNAL_DECORATION Type *inner_function()   \
    {                                                           \
        struct HolderBase {                                     \
            ~HolderBase() noexcept                        \
            { if (guard.load() == pdk::globalstatic::Initialized)  \
                  guard.store(pdk::globalstatic::Destroyed); }     \
        };                                                      \
        static struct Holder : public HolderBase {              \
            Type value;                                         \
            Holder()                                            \
                noexcept(noexcept(Type ARGS))       \
                : value ARGS                                    \
            { guard.store(pdk::globalstatic::Initialized); }       \
        } holder;                                               \
        return &holder.value;                                   \
    }
#else
#define PDK_GLOBAL_STATIC_INTERNAL(ARGS)                                  \
    PDK_DECL_HIDDEN inline Type *inner_function()                          \
    {                                                                   \
       static Type *data;                                                 \
       static std::mutex mutex;                                       \
       int x = guard.loadAcquire();                                    \
       if (PDK_UNLIKELY(x >= pdk::globalstatic::Uninitialized)) {           \
          std::lock_guard<std::mutex> locker(mutex);                              \
          if (guard.load() == pdk::globalstatic::Uninitialized) {        \
             data = new Type ARGS;                                      \
             static struct Cleanup {                                 \
                ~Cleanup() {                                        \
                   delete data;                                       \
                   guard.store(pdk::globalstatic::Destroyed);         \
                }                                                   \
             } cleanup;                                              \
             guard.storeRelease(pdk::globalstatic::Initialized);        \
          }                                                           \
       }                                                               \
       return data;                                                       \
    }
#endif

} // globalstatic


template <typename T, T *(&inner_function)(), pdk::os::thread::BasicAtomicInt &guard>
struct GlobalStatic
{
   using Type = T;
   
   bool isDestroyed() const
   {
      return guard.load() <= globalstatic::Destroyed;
   }
   
   bool exists() const
   {
      return guard.load() == globalstatic::Initialized;
   }
   
   operator Type *()
   { 
      if (isDestroyed()) {
         return 0;
      } 
      return inner_function();
   }
   
   Type *operator()()
   {
      if (isDestroyed()) {
         return 0;
      }
      return inner_function();
   }
   
   Type *operator->()
   {
      PDK_ASSERT_X(!isDestroyed(), "PDK_GLOBAL_STATIC", "The global static was used after being destroyed");
      return inner_function();
   }
   Type &operator*()
   {
      PDK_ASSERT_X(!isDestroyed(), "PDK_GLOBAL_STATIC", "The global static was used after being destroyed");
      return *inner_function();
   }
};

#define PDK_GLOBAL_STATIC_WITH_ARGS(TYPE, NAME, ARGS)                         \
   namespace { namespace PDK_GS_ ## NAME {                                  \
      typedef TYPE Type;                                                  \
      pdk::os::thread::BasicAtomicInt guard = PDK_BASIC_ATOMIC_INITIALIZER(pdk::globalstatic::Uninitialized); \
      PDK_GLOBAL_STATIC_INTERNAL(ARGS)                                      \
   } }                                                                     \
   static pdk::GlobalStatic<TYPE,                                              \
   PDK_GS_ ## NAME::inner_function,                     \
   PDK_GS_ ## NAME::guard> NAME

#define PDK_GLOBAL_STATIC(TYPE, NAME)                                         \
   PDK_GLOBAL_STATIC_WITH_ARGS(TYPE, NAME, ())

} // pdk

#endif // PDK_GLOBAL_GLOBAL_STATIC_H
