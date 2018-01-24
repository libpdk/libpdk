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
// Created by softboy on 2018/01/24.

#include "gtest/gtest.h"
#include "pdk/kernel/signal/Signal.h"
#include <iostream>
#include <sstream>
#include <string>
#include <array>

namespace Signals = pdk::kernel::signal;

namespace {

static std::array<Signals::Connection, 4> connections;
static std::ostringstream testOutput;

struct TestSlot {
   explicit TestSlot(int v = 0) : value(v)
   {}
   void operator()() const {
      testOutput << value;
   }
   int value;
};

}

TEST(SharedConnectionBlockTest, testSharedBlockConnection)
{
   Signals::Signal<void ()> signal0;
   for (unsigned i = 0; i < connections.size(); ++i) {
      connections.at(i) = signal0.connect(TestSlot(i));
   }
   {
      // Blocking 2
      Signals::SharedConnectionBlock block(connections.at(2));
      ASSERT_TRUE(block.blocking());
      testOutput.str("");
      signal0();
      ASSERT_EQ(testOutput.str(), "013");
   }
   // Unblocking 2
   testOutput.str("");
   signal0();
   ASSERT_EQ(testOutput.str(), "0123");
   
   {
      // Blocking 1 through const connection
      const Signals::Connection conn = connections.at(1);
      Signals::SharedConnectionBlock block(conn);
      testOutput.str("");
      signal0();
      std::cout << testOutput.str() << std::endl;
      ASSERT_EQ(testOutput.str(), "023");
      // Unblocking 1
      block.unblock();
      ASSERT_EQ(block.blocking(), false);
      testOutput.str("");
      signal0();
      ASSERT_EQ(testOutput.str(), "0123");
   }
   
   {
      // initially unblocked
      Signals::SharedConnectionBlock block(connections.at(3), false);
      ASSERT_EQ(block.blocking(), false);
      testOutput.str("");
      signal0();
      ASSERT_EQ(testOutput.str(), "0123");
      // block
      block.block();
      testOutput.str("");
      signal0();
      ASSERT_EQ(testOutput.str(), "012");
   }
   {
      // test default constructed block
      Signals::SharedConnectionBlock block;
      ASSERT_EQ(block.blocking(), true);
      block.unblock();
      ASSERT_EQ(block.blocking(), false);
      block.block();
      ASSERT_EQ(block.blocking(), true);
      // test assignment
      {
         block.unblock();
         Signals::SharedConnectionBlock block2(connections.at(0));
         ASSERT_TRUE(block.connection() != block2.connection());
         ASSERT_TRUE(block.blocking() != block2.blocking());
         block = block2;
         ASSERT_TRUE(block.connection() == block2.connection());
         ASSERT_TRUE(block.blocking() == block2.blocking());
      }
      testOutput.str("");
      signal0();
      ASSERT_TRUE(testOutput.str() == "123");
   }
}
