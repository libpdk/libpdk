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
// Created by softboy on 2018/03/26.

#ifndef PDK_TESTLIB_TEST_SYSTEM_H
#define PDK_TESTLIB_TEST_SYSTEM_H

#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/DeadlineTimer.h"
#include "TestGlobal.h"
#include "Utils.h"

namespace pdktest {

using pdk::kernel::CoreApplication;
using pdk::kernel::DeadlineTimer;
using pdk::kernel::EventLoop;
using pdk::kernel::Event;

namespace {

template <typename Functor>
PDK_REQUIRED_RESULT bool waitfor(Functor predicate, int timeout = 5000)
{
   // We should not spin the event loop in case the predicate is already true,
   // otherwise we might send new events that invalidate the predicate.
   if (predicate()) {
      return true;
   }
   // qWait() is expected to spin the event loop, even when called with a small
   // timeout like 1ms, so we we can't use a simple while-loop here based on
   // the deadline timer not having timed out. Use do-while instead.
   
   int remaining = timeout;
   DeadlineTimer deadline(remaining, pdk::TimerType::PreciseTimer);
   
   do {
      CoreApplication::processEvents(EventLoop::AllEvents, remaining);
      CoreApplication::sendPostedEvents(nullptr, Event::Type::DeferredDelete);
      
      remaining = deadline.getRemainingTime();
      if (remaining > 0) {
         pdktest::sleep(std::min(10, remaining));
         remaining = deadline.getRemainingTime();
      }
      if (predicate()) {
         return true;
      }         
      remaining = deadline.getRemainingTime();
   } while (remaining > 0);
   
   return predicate(); // Last chance
}

PDK_DECL_UNUSED void wait(int ms)
{
   // Ideally this method would be implemented in terms of qWaitFor, with
   // a predicate that always returns false, but due to a compiler bug in
   // GCC 6 we can't do that.
   PDK_ASSERT(CoreApplication::getInstance());
   DeadlineTimer timer(ms, pdk::TimerType::PreciseTimer);
   int remaining = ms;
   do {
      CoreApplication::processEvents(EventLoop::AllEvents, remaining);
      CoreApplication::sendPostedEvents(nullptr, Event::Type::DeferredDelete);
      remaining = timer.getRemainingTime();
      if (remaining <= 0) {
         break;
      }
      pdktest::sleep(std::min(10, remaining));
      remaining = timer.getRemainingTime();
   } while (remaining > 0);
}

} // anonymous namespace

} // pdktest

#endif // PDK_TESTLIB_TEST_SYSTEM_H
