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

#ifndef PDK_KERNEL_ABSTRACT_EVENT_DISPATCHER_PRIVATE_H
#define PDK_KERNEL_ABSTRACT_EVENT_DISPATCHER_PRIVATE_H

#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include <list>

namespace pdk {
namespace kernel {
namespace internal {

PDK_CORE_EXPORT uint global_posted_events_count();

class PDK_CORE_EXPORT AbstractEventDispatcherPrivate : public ObjectPrivate
{
   PDK_DECLARE_PUBLIC(AbstractEventDispatcher);
public:
   inline AbstractEventDispatcherPrivate()
   {}
   std::list<AbstractNativeEventFilter *> m_eventFilters;
   static int allocateTimerId();
   static void releaseTimerId(int id);
};

} // internal
} // kernel
} // pdk

#endif // PDK_KERNEL_ABSTRACT_EVENT_DISPATCHER_PRIVATE_H
