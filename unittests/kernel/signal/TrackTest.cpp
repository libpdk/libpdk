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

#include <memory>
#include <optional>
#include "pdk/kernel/signal/Signal.h"
#include "gtest/gtest.h"

namespace Signals = pdk::kernel::signal;

namespace {

struct Swallow {
   typedef int ResultType;
   template<typename T> ResultType operator()(const T*, int i) { return i; }
};

template<typename T>
struct MaxOrDefault {
   typedef T ResultType;
   
   template<typename InputIterator>
   T operator()(InputIterator first, InputIterator last) const
   {
      std::optional<T> max;
      for(; first != last; ++first) {
         T value = *first;
         if(!max)
         {
            max = value;
         }else if(value > *max)
         {
            max = value;
         }
      }
      if(max) return *max;
      else return T();
   }
};

static int myfunc(int i, double z)
{
   return i;
}

}

TEST(TrackTest, testSlotTracks)
{
   using SignalType = Signals::Signal<int (int), MaxOrDefault<int>>;
   SignalType signal1;
   Signals::Connection connection;
   ASSERT_EQ(signal1(5), 0);
   {
      std::shared_ptr<int> shorty(new int());
      signal1.connect(SignalType::SlotType(Swallow(), shorty.get(), std::placeholders::_1).track(shorty));
      ASSERT_EQ(signal1(5), 5);
   }
   ASSERT_EQ(signal1(5), 0);
   
   {
      std::shared_ptr<int> shorty(new int(1));
      // doesn't work on gcc 3.3.5, it says: error: type specifier omitted for parameter `shorty'
      // does work on gcc 4.1.2
      //    sig_type::slot_type slot(swallow(), shorty.get(), _1);
      Swallow myswallow;
      SignalType::SlotType slot(myswallow, shorty.get(), std::placeholders::_1);
      
      slot.track(shorty);
      shorty.reset();
      signal1.connect(slot);
      ASSERT_EQ(signal1(5), 0);
   }
   // Test binding of a slot to another slot
   {
      std::shared_ptr<int> shorty(new int(2));
      Signals::Slot<int (double)> other_slot(&myfunc, std::cref(*shorty.get()), std::placeholders::_1);
      other_slot.track(shorty);
      connection = signal1.connect(SignalType::SlotType(other_slot, 0.5).track(other_slot));
      ASSERT_EQ(signal1(3), 2);
   }
   ASSERT_EQ(connection.connected(), false);
   ASSERT_EQ(signal1(3), 0);
   // Test binding of a signal as a slot
   {
      SignalType signal2;
      signal1.connect(signal2);
      signal2.connect(SignalType::SlotType(&myfunc, std::placeholders::_1, 0.7));
      ASSERT_EQ(signal1(4), 4);
   }
   ASSERT_EQ(signal1(4), 0);
   
   // Test tracking of null but not empty shared_ptr
   ASSERT_EQ(signal1(2), 0);
   {
      std::shared_ptr<int> shorty((int*)(0));
      signal1.connect(SignalType::SlotType(Swallow(), shorty.get(), std::placeholders::_1).track(shorty));
      ASSERT_EQ(signal1(2), 2);
   }
   ASSERT_EQ(signal1(2), 0);
}



