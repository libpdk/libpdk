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
// Created by softboy on 2018/01/20.

// Boost.Signals2 library

// Copyright Frank Mori Hess 2007-2008.
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef PDK_KERNEL_SIGNAL_SHARED_CONNECTION_BLOCK_H
#define PDK_KERNEL_SIGNAL_SHARED_CONNECTION_BLOCK_H

#include "pdk/kernel/signal/Connection.h"
#include <memory>

namespace pdk {
namespace kernel {
namespace signal {

class SharedConnectionBlock
{
public:
   SharedConnectionBlock(const Connection &conn = Connection(),
                         bool initiallyBlocked = true)
      : m_weakConnectionBody(conn.m_weakConnectionBody)
   {
      if(initiallyBlocked) {
         block();
      }
   }
   
   void block()
   {
      if(blocking()) {
         return;
      }
      std::shared_ptr<internal::ConnectionBodyBase> connectionBody(m_weakConnectionBody.lock());
      if(!connectionBody)
      {
         // Make _blocker non-empty so the blocking() method still returns the correct value
         // after the connection has expired.
         m_blocker.reset(static_cast<int *>(new int(0)));
         return;
      }
      m_blocker = connectionBody->getBlocker();
   }
   
   void unblock()
   {
      m_blocker.reset();
   }
   
   bool blocking() const
   {
      return static_cast<bool>(m_blocker);
   }
   
   Connection connection() const
   {
      return Connection(m_weakConnectionBody);
   }
   
private:
   std::weak_ptr<internal::ConnectionBodyBase> m_weakConnectionBody;
   std::shared_ptr<void> m_blocker;
};

} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_SHARED_CONNECTION_BLOCK_H
