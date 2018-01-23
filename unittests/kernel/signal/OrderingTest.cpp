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
// Created by softboy on 2018/01/22.

#include "gtest/gtest.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <functional>
#include "pdk/kernel/signal/Signal.h"

namespace {

std::vector<int> valuesOutput;
bool ungrouped1 = false;
bool ungrouped2 = false;
bool ungrouped3 = false;

struct EmitInt {
   EmitInt(int v) : value(v) {}
   
   void operator()() const
   {
      ASSERT_TRUE(value == 42 || (!ungrouped1 && !ungrouped2 && !ungrouped3));
      valuesOutput.push_back(value);
      std::cout << value << ' ';
   }
   
private:
   int value;
};

struct WriteUngrouped1 {
   void operator()() const
   {
      ASSERT_TRUE(!ungrouped1);
      ungrouped1 = true;
      std::cout << "(Ungrouped #1)" << ' ';
   }
};

struct WriteUngrouped2 {
   void operator()() const
   {
      ASSERT_TRUE(!ungrouped2);
      ungrouped2 = true;
      std::cout << "(Ungrouped #2)" << ' ';
   }
};

struct WriteUngrouped3 {
   void operator()() const
   {
      ASSERT_TRUE(!ungrouped3);
      ungrouped3 = true;
      std::cout << "(Ungrouped #3)" << ' ';
   }
};

int return_argument(int x)
{
   return x;
}

}

namespace Signals = pdk::kernel::signal;

TEST(SignalOrderTest, testGroupCompare)
{
   Signals::Signal
         <
         int (),
         Signals::LastValue<int>,
         int,
         std::greater< int >
         > sig;
   sig.connect(1, std::bind(&return_argument, 1));
   sig.connect(2, std::bind(&return_argument, 2));
   ASSERT_EQ(sig(), 1);
}

TEST(SignalOrderTest, testOrder)
{
   std::srand(std::time(0));
   std::vector<int> sortedValues;
   Signals::Signal<void ()> signal;
   signal.connect(WriteUngrouped1());
   for (int i = 0; i < 100; ++i) {
      int v = std::rand() % 100;
      sortedValues.push_back(v);
      signal.connect(v, EmitInt(v));
      
      if (i == 50) {
         signal.connect(WriteUngrouped2());
      }
   }
   signal.connect(WriteUngrouped3());
   std::sort(sortedValues.begin(), sortedValues.end());
   // 17 at beginning, 42 at end
   sortedValues.insert(sortedValues.begin(), 17);
   signal.connect(EmitInt(17), Signals::ConnectPosition::AtFront);
   sortedValues.push_back(42);
   signal.connect(EmitInt(42));
   signal();
   std::cout << std::endl;
   ASSERT_TRUE(valuesOutput == sortedValues);
   ASSERT_TRUE(ungrouped1);
   ASSERT_TRUE(ungrouped2);
   ASSERT_TRUE(ungrouped3);
}
