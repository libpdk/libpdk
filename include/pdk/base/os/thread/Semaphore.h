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
// Created by softboy on 2018/01/08.

#ifndef PDK_M_BASE_OS_THREAD_SEMAPHORE_H
#define PDK_M_BASE_OS_THREAD_SEMAPHORE_H

#include "pdk/global/Global.h"

namespace pdk {
namespace os {
namespace thread {

// forward declare class with namespace
namespace internal
{
class SemaphorePrivate;
} // internal

class PDK_CORE_EXPORT Semaphore
{
public:
   explicit Semaphore(int num = 0);
   ~Semaphore();
   
   void acquire(int num = 1);
   bool tryAcquire(int num = 1);
   bool tryAcquire(int num, int timeout);
   void release(int num = 1);
   int available() const;
private:
   PDK_DISABLE_COPY(Semaphore);
   internal::SemaphorePrivate *m_implPtr;
};

} // thread
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_SEMAPHORE_H

