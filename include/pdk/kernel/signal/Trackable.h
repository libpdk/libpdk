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

// Copyright Frank Mori Hess 2007,2009.
// Copyright Timmo Stange 2007.
// Copyright Douglas Gregor 2001-2004. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Compatibility class to ease porting from the original
// Boost.Signals library.  However,
// boost::signals2::trackable is NOT thread-safe.

// For more information, see http://www.boost.org

#ifndef PDK_KERNEL_SIGNAL_TRACKABLE_H
#define PDK_KERNEL_SIGNAL_TRACKABLE_H

namespace pdk {
namespace kernel {
namespace signal {
namespace internal {

class TrackedObjectsVisitor;
// trackable_pointee is used to identify the tracked shared_ptr 
// originating from the signal::trackable class.  These tracked
// shared_ptr are special in that we shouldn't bother to
// increment their use count during signal invocation, since
// they don't actually control the lifetime of the
// signal::trackable object they are associated with.
class TrackablePointee
{};
} // internal

class Trackable {
protected:
   Trackable()
      : m_trackedPtr(static_cast<internal::TrackablePointee*>(0))
   {}
   
   Trackable(const Trackable &)
      : m_trackedPtr(static_cast<internal::TrackablePointee*>(0)) 
   {}
   
   Trackable& operator=(const Trackable &)
   {
      return *this;
   }
   
   ~Trackable() {}
private:
   friend class internal::TrackedObjectsVisitor;
   
   std::weak_ptr<internal::TrackablePointee> getWeakPtr() const
   {
      return m_trackedPtr;
   }
   
   std::shared_ptr<internal::TrackablePointee> m_trackedPtr;
};

} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_TRACKABLE_H
