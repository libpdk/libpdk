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

// Copyright Frank Mori Hess 2009.
//
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef PDK_KERNEL_SIGNAL_SLOT_BASE_H
#define PDK_KERNEL_SIGNAL_SLOT_BASE_H

#include "pdk/kernel/signal/ExpiredSlot.h"
#include "pdk/kernel/signal/SignalBase.h"
#include <vector>
#include <memory>
#include <variant>

namespace pdk {
namespace kernel {
namespace signal {
namespace internal {

class TrackedObjectsVisitor;
class TrackablePointee;

using VoidWeakPtrVariant = std::variant<std::weak_ptr<TrackablePointee>, std::weak_ptr<void>>;
using VoidSharedPtrVariant = std::variant<std::shared_ptr<void>>;

class LockWeakPtrVisitor
{
public:
   using ResultType = VoidSharedPtrVariant;
   using result_type = ResultType;
   
   template<typename WeakPtr>
   ResultType operator()(const WeakPtr &wp) const
   {
      return wp.lock();
   }
   // overload to prevent incrementing use count of shared_ptr associated
   // with signal::Trackable objects
   result_type operator()(const std::weak_ptr<TrackablePointee> &) const
   {
      return std::shared_ptr<void>();
   }
};

class ExpiredWeakPtrVisitor
{
public:
   using result_type = bool;
   using ResultType = result_type;
   
   template<typename WeakPtr>
   bool operator()(const WeakPtr &wp) const
   {
      return wp.expired();
   }
};

} // internal

class SlotBase
{
public:
   using TrackedContainerType = std::vector<internal::VoidWeakPtrVariant>;
   using LockedContainerType = std::vector<internal::VoidSharedPtrVariant>;
   
   const TrackedContainerType &getTrackedObjects() const
   {
      return m_trackedObjects;
   }
   
   LockedContainerType lock() const
   {
      LockedContainerType lockedObjects;
      TrackedContainerType::const_iterator it;
      for(it = getTrackedObjects().begin(); it != getTrackedObjects().end(); ++it) {
         lockedObjects.push_back(std::visit(internal::LockWeakPtrVisitor(), *it));
         if(std::visit(internal::ExpiredWeakPtrVisitor(), *it))
         {
            throw ExpiredSlot();
         }
      }
      return lockedObjects;
   }
   
   bool expired() const
   {
      TrackedContainerType::const_iterator it;
      for(it = getTrackedObjects().begin(); it != getTrackedObjects().end(); ++it)
      {
         if(std::visit(internal::ExpiredWeakPtrVisitor(), *it)) {
            return true;
         }
      }
      return false;
   }
   
protected:
   friend class internal::TrackedObjectsVisitor;
   
   void trackSignal(const SignalBase &signal)
   {
      m_trackedObjects.push_back(signal.getLockPimpl());
   }
   
   TrackedContainerType m_trackedObjects;
};

} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_SLOT_BASE_H
