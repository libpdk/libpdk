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
#include "pdk/base/ds/ByteArray.h"
#include <map>
#include <mutex>

//#define PDK_SHARED_POINTER_BACKTRACE_SUPPORT

#ifdef PDK_SHARED_POINTER_BACKTRACE_SUPPORT
#  if defined(__GLIBC__) && (__GLIBC__ >= 2) && !defined(__UCLIBC__) && !defined(PDK_LINUXBASE)
#    define BACKTRACE_SUPPORTED
#  elif defined(PDK_OS_MAC)
#    define BACKTRACE_SUPPORTED
#  endif
#endif

using pdk::ds::ByteArray;

#if defined(BACKTRACE_SUPPORTED)
#    include <sys/types.h>
#    include <execinfo.h>
#    include <stdio.h>
#    include <unistd.h>
#    include <sys/wait.h>

namespace pdk {

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

static void print_backtrace(ByteArray stacktrace)
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

} // pdk

#endif // BACKTRACE_SUPPORTED

namespace {

struct Data
{
   const volatile void *m_pointer;
#ifdef BACKTRACE_SUPPORTED
   ByteArray m_backtrace;
#endif
   class KnownPointers
   {
   public:
      std::mutex m_mutex;
      std::map<const void *, Data> m_dPointers;
      std::map<const volatile void *, const void *> m_dataPointers;
   };
};

} // anonymous namespace
