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

#ifndef PDK_KERNEL_SOCKET_NOTIFIER_H
#define PDK_KERNEL_SOCKET_NOTIFIER_H

#include "pdk/kernel/Object.h"

namespace pdk {
namespace kernel {

// forward declare with namespace
namespace internal {
class SocketNotifierPrivate;
} // internal

using internal::SocketNotifierPrivate;
class PDK_CORE_EXPORT SocketNotifier : public Object
{
   PDK_DECLARE_PRIVATE(SocketNotifier);
public:
   enum class Type
   {
      Read,
      Write,
      Exception
   };
   
   SocketNotifier(pdk::intptr socket, Type, Object *parent = nullptr);
   ~SocketNotifier();
   
   pdk::intptr getSocket() const;
   Type getType() const;
   
   bool isEnabled() const;
   void setEnabled(bool);
   
protected:
   bool event(Event *) override;
   
private:
   PDK_DISABLE_COPY(SocketNotifier);
};

} // kernel
} // pdk

#endif // PDK_KERNEL_SOCKET_NOTIFIER_H
