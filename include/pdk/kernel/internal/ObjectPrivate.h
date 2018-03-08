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
// Created by softboy on 2018/01/23.

#ifndef PDK_KERNEL_INTERNAL_OBJECT_PRIVATE_H
#define PDK_KERNEL_INTERNAL_OBJECT_PRIVATE_H

#include "pdk/kernel/Object.h"
#include "pdk/kernel/Pointer.h"
#include "pdk/kernel/signal/Signal.h"
#include <vector>
#include <list>
#include <variant>
#include <string>

namespace pdk {

// forward declare with namespace
namespace os {
namespace thread {
namespace internal {
class ThreadData;
} // internal
} // thread
} // os

namespace kernel {
namespace internal {

using pdk::kernel::signal::Signal;
using pdk::os::thread::internal::ThreadData;

PDK_CORE_EXPORT void delete_in_event_handler(Object *);

class PDK_CORE_EXPORT ObjectPrivate : public ObjectData
{
   PDK_DECLARE_PUBLIC(Object);
   
public:
   ObjectPrivate(int version = PDK_VERSION);
   virtual ~ObjectPrivate();
   
public:
   struct ExtraData
   {
      ExtraData() {}
      std::vector<int> m_runningTimers;
      std::list<Pointer<Object>> m_eventFilters;
      String m_objectName;
   };
   
   static ObjectPrivate *get(Object *o)
   {
      return o->getImplPtr();
   }
   
public:
   union {
      Object *m_currentChildBeingDeleted;
   };
   
   ExtraData *m_extraData;
   ThreadData *m_threadData;
};

} // internal
} // kernel
} // pdk

#endif // PDK_KERNEL_INTERNAL_OBJECT_PRIVATE_H
