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
#include "pdk/kernel/signal/Signal.h"

namespace Signals = pdk::kernel::signal;

using SignalType = Signals::Signal<void ()>;

namespace {

// combiner that returns the number of slots invoked
struct SlotCounter {
   typedef unsigned ResultType;
   template<typename InputIterator>
   unsigned operator()(InputIterator first, InputIterator last) const
   {
      unsigned count = 0;
      for (; first != last; ++first)
      {
         try
         {
            *first;
            ++count;
         }
         catch(const std::bad_weak_ptr &)
         {}
      }
      return count;
   }
};

void my_slot()
{
}

void my_connecting_slot(SignalType &signal)
{
   auto conn = signal.connect(&my_slot);
   ASSERT_EQ(signal.getNumSlots(), 2ul);
   signal.disconnect(conn);
}

}

TEST(RegressionTest, testSlotConnect)
{
   SignalType signal;
   signal.connect(SignalType::SlotType(&my_connecting_slot, std::ref(signal)).track(signal));
   signal();
   ASSERT_EQ(signal.getNumSlots(), 1ul);
   signal();
   ASSERT_EQ(signal.getNumSlots(), 1ul);
}

TEST(RegressionTest, testScopedConnection)
{
   using SignalType = Signals::Signal<void (), SlotCounter>;
   SignalType signal;
   {
      Signals::ScopedConnection conn(signal.connect(&my_slot));
      ASSERT_EQ(signal(), 1ul);
      conn = signal.connect(&my_slot);
      ASSERT_EQ(signal(), 1ul);
   }
   ASSERT_EQ(signal(), 0ul);
}

namespace {

// testsignal that returns a reference type

struct RefReturner
{
  static int m_i;
  
  int& refReturnSlot()
  {
    return m_i;
  }
};

int RefReturner::m_i = 0;

}

TEST(RegressionTest, testReferenceReturn)
{
   using SignalType = Signals::Signal<int &()>;
   SignalType signal;
   RefReturner refCounter;
   signal.connect(std::bind(&RefReturner::refReturnSlot, &refCounter));
   int& r = *signal();
   ASSERT_EQ(RefReturner::m_i, 0);
   r = 1;
   ASSERT_EQ(RefReturner::m_i, 1);
}




