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
// Created by softboy on 2018/03/02.

#ifndef PDK_M_BASE_OS_THREAD_RUNNABLE_H
#define PDK_M_BASE_OS_THREAD_RUNNABLE_H

#include "pdk/global/Global.h"

namespace pdk {
namespace os {
namespace thread {

class PDK_CORE_EXPORT Runnable
{
public:
   virtual void run() = 0;
   
   Runnable()
      : m_ref(0)
   {}
   
   virtual ~Runnable();
   bool autoDelete() const
   {
      return m_ref != -1;
   }
   
   void setAutoDelete(bool autoDelete)
   {
      ref = autoDelete ? 0 : -1;
   }
private:
   int m_ref;
   friend class ThreadPool;
   friend class ThreadPoolPrivate;
   friend class ThreadPoolThread;
   PDK_DISABLE_COPY(Runnable);
};

} // thread
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_RUNNABLE_H
