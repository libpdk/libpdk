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

#ifndef PDK_KERNEL_OBJECT_PRIVATE_H
#define PDK_KERNEL_OBJECT_PRIVATE_H

#include "pdk/kernel/Object.h"
#include "pdk/kernel/Pointer.h"
#include <vector>
#include <list>
#include <variant>
#include <string>

namespace pdk {
namespace kernel {
namespace internal {

class PDK_CORE_EXPORT ObjectPrivate : public ObjectData
{
   PDK_DECLARE_PUBLIC(Object);
public:
   struct ExtraData
   {
      std::vector<int> m_runningTimers;
      std::list<Pointer<Object>> m_eventFilters;
      std::string m_objectName;
   };
};

} // internal
} // kernel
} // pdk

#endif // PDK_KERNEL_OBJECT_PRIVATE_H
