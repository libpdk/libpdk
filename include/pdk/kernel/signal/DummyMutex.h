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
// Created by softboy on 2018/01/18.

// A model of the Lockable concept from Boost.Thread which
// does nothing.  It can be passed as the Mutex template parameter
// for a signal, if the user wishes to disable thread-safety
// (presumably for performance reasons).

// Copyright Frank Mori Hess 2008.
// Distributed under the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/signals2 for library home page.

#ifndef PDK_KERNEL_SIGNAL_DUMMY_MUTEX_H
#define PDK_KERNEL_SIGNAL_DUMMY_MUTEX_H

namespace pdk {
namespace kernel {
namespace signal {

class DummyMutex
{
public:
   void lock() = default;
   bool try_lock()
   {
      return true;
   }
   void unlock() = default;
};

} // pdk
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_DUMMY_MUTEX_H
