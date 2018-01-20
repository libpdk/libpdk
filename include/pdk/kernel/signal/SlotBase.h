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

typedef std::variant<std::weak_ptr<trackable_pointee>, std::weak_ptr<void>, detail::foreign_void_weak_ptr > void_weak_ptr_variant;
typedef std::variant<std::shared_ptr<void>, detail::foreign_void_shared_ptr > void_shared_ptr_variant;

class lock_weak_ptr_visitor
{
public:
   typedef void_shared_ptr_variant result_type;
   template<typename WeakPtr>
   result_type operator()(const WeakPtr &wp) const
   {
      return wp.lock();
   }
   // overload to prevent incrementing use count of shared_ptr associated
   // with signals2::trackable objects
   result_type operator()(const weak_ptr<trackable_pointee> &) const
   {
      return boost::shared_ptr<void>();
   }
};

class expired_weak_ptr_visitor
{
public:
   typedef bool result_type;
   template<typename WeakPtr>
   bool operator()(const WeakPtr &wp) const
   {
      return wp.expired();
   }
};

} // internal

class slot_base
{
public:
   typedef std::vector<detail::void_weak_ptr_variant> tracked_container_type;
   typedef std::vector<detail::void_shared_ptr_variant> locked_container_type;
   
   const tracked_container_type& tracked_objects() const {return _tracked_objects;}
   locked_container_type lock() const
   {
      locked_container_type locked_objects;
      tracked_container_type::const_iterator it;
      for(it = tracked_objects().begin(); it != tracked_objects().end(); ++it)
      {
         locked_objects.push_back(apply_visitor(detail::lock_weak_ptr_visitor(), *it));
         if(apply_visitor(detail::expired_weak_ptr_visitor(), *it))
         {
            boost::throw_exception(expired_slot());
         }
      }
      return locked_objects;
   }
   bool expired() const
   {
      tracked_container_type::const_iterator it;
      for(it = tracked_objects().begin(); it != tracked_objects().end(); ++it)
      {
         if(apply_visitor(detail::expired_weak_ptr_visitor(), *it)) return true;
      }
      return false;
   }
protected:
   friend class detail::tracked_objects_visitor;
   
   void track_signal(const signal_base &signal)
   {
      _tracked_objects.push_back(signal.lock_pimpl());
   }
   
   tracked_container_type _tracked_objects;
};

} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_SLOT_BASE_H
