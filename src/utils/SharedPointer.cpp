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

#include "pdk/utils/SharedPointer.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/global/GlobalStatic.h"
#include "pdk/global/Logging.h"
#include <unordered_map>
#include <mutex>

#ifdef PDK_SHARED_POINTER_BACKTRACE_SUPPORT
#  if defined(__GLIBC__) && (__GLIBC__ >= 2) && !defined(__UCLIBC__) && !defined(PDK_LINUXBASE)
#    define BACKTRACE_SUPPORTED
#  elif defined(PDK_OS_MAC)
#    define BACKTRACE_SUPPORTED
#  endif
#endif

namespace pdk {
namespace utils {
namespace internal {
namespace sharedptr {

using pdk::ds::ByteArray;
using pdk::kernel::Object;
using pdk::kernel::internal::ObjectPrivate;

void ExternalRefCountData::setObjectShared(const kernel::Object *, bool)
{}

void ExternalRefCountData::checkObjectShared(const kernel::Object *)
{}

ExternalRefCountData *ExternalRefCountData::getAndRef(const kernel::Object *object)
{
   PDK_ASSERT(object);
   ObjectPrivate *implPtr = ObjectPrivate::get(const_cast<Object *>(object));
   PDK_ASSERT_X(!implPtr->m_wasDeleted, "WeakPointer", "Detected WeakPointer creation in a Object being deleted");
   
   ExternalRefCountData *that = implPtr->m_sharedRefcount.load();
   if (that) {
      that->m_weakRef.ref();
      return that;
   }
   
   // we can create the refcount data because it doesn't exist
   ExternalRefCountData *newRefCountData = new ExternalRefCountData(pdk::Uninitialized);
   newRefCountData->m_strongRef.store(-1);
   newRefCountData->m_weakRef.store(2);  // the QWeakPointer that called us plus the QObject itself
   if (!implPtr->m_sharedRefcount.testAndSetRelease(0, newRefCountData)) {
      delete newRefCountData;
      newRefCountData = implPtr->m_sharedRefcount.loadAcquire();
      newRefCountData->m_weakRef.ref();
   }
   return newRefCountData;
}

#if defined(BACKTRACE_SUPPORTED)
#    include <sys/types.h>
#    include <execinfo.h>
#    include <stdio.h>
#    include <unistd.h>
#    include <sys/wait.h>

namespace {

inline ByteArray save_backtrace() __attribute__((always_inline));
inline ByteArray save_backtrace()
{
   static const int maxFrames = 32;
   
   ByteArray stacktrace;
   stacktrace.resize(sizeof(void*) * maxFrames);
   int stack_size = backtrace((void**)stacktrace.getRawData(), maxFrames);
   stacktrace.resize(sizeof(void*) * stack_size);
   return stacktrace;
}

void print_backtrace(ByteArray stacktrace)
{
   void *const *stack = (void *const *)stacktrace.getConstRawData();
   int stackSize = stacktrace.size() / sizeof(void*);
   char **stackSymbols = backtrace_symbols(stack, stackSize);
   
   int filter[2];
   pid_t child = -1;
   if (pipe(filter) != -1) {
      child = fork();
   }
   if (child == 0) {
      // child process
      dup2(fileno(stderr), fileno(stdout));
      dup2(filter[0], fileno(stdin));
      close(filter[0]);
      close(filter[1]);
      execlp("c++filt", "c++filt", "-n", NULL);
      
      // execlp failed
      execl("/bin/cat", "/bin/cat", NULL);
      _exit(127);
   }
   
   // parent process
   close(filter[0]);
   FILE *output;
   if (child == -1) {
      // failed forking
      close(filter[1]);
      output = stderr;
   } else {
      output = fdopen(filter[1], "w");
   }
   
   fprintf(stderr, "Backtrace of the first creation (most recent frame first):\n");
   for (int i = 0; i < stackSize; ++i) {
      if (strlen(stackSymbols[i])) {
         fprintf(output, "#%-2d %s\n", i, stackSymbols[i]);
      } else {
         fprintf(output, "#%-2d %p\n", i, stack[i]);
      }
   }
   if (child != -1) {
      fclose(output);
      waitpid(child, 0, 0);
   }
}

} // anonymous namespace

#endif // BACKTRACE_SUPPORTED

namespace {

struct Data
{
   const volatile void *m_pointer;
#ifdef BACKTRACE_SUPPORTED
   ByteArray m_backtrace;
#endif
};

class KnownPointers
{
public:
   std::mutex m_mutex;
   std::unordered_map<const void *, Data> m_dPointers;
   std::unordered_map<const volatile void *, const void *> m_dataPointers;
};

} // anonymous namespace

PDK_GLOBAL_STATIC(KnownPointers, sg_knownPointers);

PDK_UNITTEST_EXPORT void internal_safety_check_clean_check();

void internal_safety_check_add(const void *implPtr, const volatile void *ptr)
{
   KnownPointers *const kp = sg_knownPointers();
   if (!kp) {
      return;
   }
   std::lock_guard<std::mutex> locker(kp->m_mutex);
   // debug_stream("Adding d=%p value=%p", implPtr, ptr);
   const void *otherImplPtr = kp->m_dataPointers.find(ptr) != kp->m_dataPointers.end() 
         ? kp->m_dataPointers.at(ptr) : nullptr;
   if (PDK_UNLIKELY(otherImplPtr)) {
#  ifdef BACKTRACE_SUPPORTED
      print_backtrace(sg_knownPointers()->m_dPointers.at(otherImplPtr).m_backtrace);
#  endif
      fatal_stream("SharedPointer: internal self-check failed: pointer %p was already tracked "
                   "by another SharedPointer object %p", ptr, otherImplPtr);
   }
   
   Data data;
   data.m_pointer = ptr;
#  ifdef BACKTRACE_SUPPORTED
   data.m_backtrace = save_backtrace();
#  endif
   
   kp->m_dPointers[implPtr] = data;
   kp->m_dataPointers[ptr] = implPtr;
   PDK_ASSERT(kp->m_dPointers.size() == kp->m_dataPointers.size());
}

void internal_safety_check_remove(const void *implPtr)
{
   KnownPointers *const kp = sg_knownPointers();
   if (!kp) {
      return;
   }
   std::lock_guard<std::mutex> locker(kp->m_mutex);
   auto iter = kp->m_dPointers.find(implPtr);
   if (PDK_UNLIKELY(iter == kp->m_dPointers.end())) {
      fatal_stream("SharedPointer: internal self-check inconsistency: pointer %p was not tracked. "
                   "To use PDK_SHAREDPOINTER_TRACK_POINTERS, you have to enable it throughout "
                   "in your code.", implPtr);
   }
   const auto iter2 = kp->m_dataPointers.find(iter->first);
   PDK_ASSERT(iter2 != kp->m_dataPointers.end());
   
   //debug_stream("Removing d=%p value=%p", d_ptr, it->pointer);
   
   // remove entries
   kp->m_dataPointers.erase(iter2);
   kp->m_dPointers.erase(iter);
   PDK_ASSERT(kp->m_dPointers.size() == kp->m_dataPointers.size());
}

void internal_safety_check_clean_check()
{
   KnownPointers *const kp = sg_knownPointers();
   PDK_ASSERT_X(kp, "internal_safety_check_self_check()", "Called after global statics deletion!");
   
   if (PDK_UNLIKELY(kp->m_dPointers.size() != kp->m_dataPointers.size())) {
      fatal_stream("Internal consistency error: the number of pointers is not equal!");
   }
   
   if (PDK_UNLIKELY(!kp->m_dPointers.empty())) {
      fatal_stream("Pointer cleaning failed: %d entries remaining", (int)kp->m_dPointers.size());
   }
}

} // sharedptr
} // internal
} // utils
} // pdk
