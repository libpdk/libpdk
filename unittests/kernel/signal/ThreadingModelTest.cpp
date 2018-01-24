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

#include "gtest/gtest.h"
#include <memory>
#include <mutex>
#include "pdk/kernel/signal/Signal.h"


namespace Signals = pdk::kernel::signal;

namespace {

// combiner that returns the number of slots invoked
struct SlotCounter {
   typedef unsigned ResultType;
   template<typename InputIterator>
   unsigned operator()(InputIterator first, InputIterator last) const
   {
      unsigned m_count = 0;
      for (; first != last; ++first)
      {
         try
         {
            *first;
            ++m_count;
         }
         catch(const std::bad_weak_ptr &)
         {}
      }
      return m_count;
   }
};

void myslot()
{
}

template<typename SignalType>
void simple_test()
{
   SignalType sig;
   auto conn = sig.connect(typename SignalType::SlotType(&myslot));
   ASSERT_TRUE(sig() == 1);
   sig.disconnect(conn);
   ASSERT_TRUE(sig() == 0);
}

class RecursionCheckingDummyMutex
{
   int m_recursionCount;
public:
   RecursionCheckingDummyMutex(): m_recursionCount(0)
   {}
   void lock() 
   { 
      ASSERT_TRUE(m_recursionCount == 0);
      ++m_recursionCount;
   }
   bool try_lock() 
   { 
      lock(); 
      return true;
   }
   void unlock() 
   { 
      --m_recursionCount;
      ASSERT_TRUE(m_recursionCount == 0);
   }
};

}

TEST(ThreadingModelTest, testThreadingModel)
{
   typedef Signals::Signal<void (), SlotCounter, int, std::less<int>, std::function<void ()>,
         std::function<void (const Signals::Connection &)>, RecursionCheckingDummyMutex> sig0_rc_type;
   simple_test<sig0_rc_type>();
   typedef Signals::Signal<void (), SlotCounter, int, std::less<int>, std::function<void ()>,
         std::function<void (const Signals::Connection &)>, std::mutex> sig0_mt_type;
   simple_test<sig0_mt_type>();
   typedef Signals::Signal<void (), SlotCounter, int, std::less<int>, std::function<void ()>,
         std::function<void (const Signals::Connection &)>, Signals::DummyMutex> sig0_st_type;
   simple_test<sig0_st_type>();
}
