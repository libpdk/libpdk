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
// Copyright Timmo Stange 2007.
// Copyright Douglas Gregor 2001-2004. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef PDK_KERNEL_SIGNAL_INTERNAL_TRACKED_OBJECTS_VISITOR_H
#define PDK_KERNEL_SIGNAL_INTERNAL_TRACKED_OBJECTS_VISITOR_H

namespace pdk {
namespace kernel {
namespace signal {
namespace internal {

// Visitor to collect tracked objects from a bound function.
class TrackedObjectsVisitor
{
public:
   TrackedObjectsVisitor(slot_base *slot) : slot_(slot)
   {}
   template<typename T>
   void operator()(const T& t) const
   {
      visitReferenceWrapper(t, mpl::bool_<is_reference_wrapper<T>::value>());
   }
   
private:
   template<typename T>
   void visitReferenceWrapper(const reference_wrapper<T> &t, const mpl::bool_<true> &) const
   {
      visitPointer(t.get_pointer(), mpl::bool_<true>());
   }
   
   template<typename T>
   void visitReferenceWrapper(const T &t, const mpl::bool_<false> &) const
   {
      visitPointer(t, mpl::bool_<is_pointer<T>::value>());
   }
   
   template<typename T>
   void visitPointer(const T &t, const mpl::bool_<true> &) const
   {
      m_visit_not_function_pointer(t, mpl::bool_<!is_function<typename remove_pointer<T>::type>::value>());
   }
   
   template<typename T>
   void visitPointer(const T &t, const mpl::bool_<false> &) const
   {
      visitPointer(boost::addressof(t), mpl::bool_<true>());
   }
   
   template<typename T>
   void visitNotFunctionPointer(const T *t, const mpl::bool_<true> &) const
   {
      visitSignal(t, mpl::bool_<is_signal<T>::value>());
   }
   
   template<typename T>
   void visitNotFunctionPointer(const T &, const mpl::bool_<false> &) const
   {}
   
   template<typename T>
   void visitSignal(const T *signal, const mpl::bool_<true> &) const
   {
      if(signal)
         slot_->track_signal(*signal);
   }
   
   template<typename T>
   void visitSignal(const T &t, const mpl::bool_<false> &) const
   {
      addIfTrackable(t);
   }
   
   void addIfTrackable(const trackable *trackable) const
   {
      if(trackable)
         slot_->_tracked_objects.push_back(trackable->get_weak_ptr());
   }
   
   void addIfTrackable(const void *) const 
   {}
   
   mutable slot_base * slot_;
};


} // internal
} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_INTERNAL_TRACKED_OBJECTS_VISITOR_H
