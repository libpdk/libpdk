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
// Created by softboy on 2018/01/03.

#ifndef PDK_KERNEL_OBJECT_H
#define PDK_KERNEL_OBJECT_H

#include "pdk/global/Global.h"
#include <list>

namespace pdk {
namespace kernel {

class Object;
using ObjectList = std::list<Object *>;

class PDK_CORE_EXPORT ObjectData
{
public:
   virtual ~ObjectData();
   Object *m_apiPtr;
   Object *m_parent;
   ObjectList m_children;
   uint wasDeleted : 1;
   uint isDeletingChildren : 1;
   uint sendChildEvents : 1;
   uint receiveChildEvents : 1;
   uint unused : 28;
   int postedEvents;
};

class Object
{
   
   
};

} // kernel
} // pdk

#endif // PDK_KERNEL_OBJECT_H
