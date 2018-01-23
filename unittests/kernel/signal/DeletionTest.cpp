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
#include "pdk/kernel/signal/Signal.h"

namespace Signals = pdk::kernel::signal;

namespace {
static Signals::Connection connections[5];
static std::string testOutput;

struct RemoveConnection {
   explicit RemoveConnection(int v = 0, int i = -1) 
      : m_value(v), m_idx(i) {}
   
   void operator()() const {
      if (m_idx >= 0)
         connections[m_idx].disconnect();
      
      //return value;
      std::cout << m_value << " ";
      
      testOutput += static_cast<char>(m_value + '0');
   }
   
   int m_value;
   int m_idx;
};

bool operator==(const RemoveConnection &lhs, const RemoveConnection &rhs)
{
   return lhs.m_value == rhs.m_value && lhs.m_idx == rhs.m_idx;
}

}

TEST(SignalDeleteTest, testRemoveSelf)
{
   Signals::Signal<void()> signal;
   connections[0] = signal.connect(RemoveConnection(0));
   connections[1] = signal.connect(RemoveConnection(1));
   connections[2] = signal.connect(RemoveConnection(2, 2));
   connections[3] = signal.connect(RemoveConnection(3));
   std::cout << "Deleting 2" << std::endl;
   testOutput = "";
   signal(); 
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "0123");
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "013");
   signal.disconnectAllSlots();
   ASSERT_TRUE(signal.empty());
   
   connections[0] = signal.connect(RemoveConnection(0));
   connections[1] = signal.connect(RemoveConnection(1));
   connections[2] = signal.connect(RemoveConnection(2));
   connections[3] = signal.connect(RemoveConnection(3, 3));
   std::cout << "Deleting 3" << std::endl;
   testOutput = "";
   signal(); 
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "0123");
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "012");
   signal.disconnectAllSlots();
   ASSERT_TRUE(signal.empty());
   
   connections[0] = signal.connect(RemoveConnection(0, 0));
   connections[1] = signal.connect(RemoveConnection(1));
   connections[2] = signal.connect(RemoveConnection(2));
   connections[3] = signal.connect(RemoveConnection(3));
   std::cout << "Deleting 0" << std::endl;
   testOutput = "";
   signal(); 
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "0123");
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "123");
   signal.disconnectAllSlots();
   ASSERT_TRUE(signal.empty());
   
   connections[0] = signal.connect(RemoveConnection(0, 0));
   connections[1] = signal.connect(RemoveConnection(1, 1));
   connections[2] = signal.connect(RemoveConnection(2, 2));
   connections[3] = signal.connect(RemoveConnection(3, 3));
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "0123");
   
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "");
}

TEST(SignalDeleteTest, testRemovePrior)
{
   Signals::Signal<void()> signal;
   connections[0] = signal.connect(RemoveConnection(0));
   connections[1] = signal.connect(RemoveConnection(1, 0));
   connections[2] = signal.connect(RemoveConnection(2));
   connections[3] = signal.connect(RemoveConnection(3));
   std::cout << "1 removes 0" << std::endl;
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "0123");
   
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "123");
   signal.disconnectAllSlots();
   ASSERT_TRUE(signal.empty());
   
   connections[0] = signal.connect(RemoveConnection(0));
   connections[1] = signal.connect(RemoveConnection(1));
   connections[2] = signal.connect(RemoveConnection(2));
   connections[3] = signal.connect(RemoveConnection(3, 2));
   std::cout << "3 removes 2" << std::endl;
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "0123");
   
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "013");
   signal.disconnectAllSlots();
   ASSERT_TRUE(signal.empty());
   
}

TEST(SignalDeleteTest, testRemoveAfter)
{
   Signals::Signal<void()> signal;
   connections[0] = signal.connect(RemoveConnection(0, 1));
   connections[1] = signal.connect(RemoveConnection(1));
   connections[2] = signal.connect(RemoveConnection(2));
   connections[3] = signal.connect(RemoveConnection(3));
   std::cout << "0 removes 1" << std::endl;
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "023");
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "023");
   signal.disconnectAllSlots();
   
   connections[0] = signal.connect(RemoveConnection(0));
   connections[1] = signal.connect(RemoveConnection(1, 3));
   connections[2] = signal.connect(RemoveConnection(2));
   connections[3] = signal.connect(RemoveConnection(3));
   std::cout << "0 removes 3" << std::endl;
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "012");
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "012");
   signal.disconnectAllSlots();
}

TEST(SignalDeleteTest, testBloodBath)
{
   Signals::Signal<void()> signal;
   connections[0] = signal.connect(RemoveConnection(0, 1));
   connections[1] = signal.connect(RemoveConnection(1, 1));
   connections[2] = signal.connect(RemoveConnection(2, 0));
   connections[3] = signal.connect(RemoveConnection(3, 2));
   std::cout << "0 removes 1, 2 removes 0, 3 removes 2" << std::endl;
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "023");
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "3");
}

TEST(SignalDeleteTest, testDisconnectEqual)
{
   Signals::Signal<void()> signal;
   connections[0] = signal.connect(RemoveConnection(0));
   connections[1] = signal.connect(RemoveConnection(1));
   connections[2] = signal.connect(RemoveConnection(2));
   connections[3] = signal.connect(RemoveConnection(3));
   std::cout << "Deleting 2" << std::endl;
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "0123");
   signal.disconnect(connections[2]);
   testOutput = "";
   signal();
   std::cout << std::endl;
   ASSERT_EQ(testOutput, "013");
}

struct SignalDeletionTester 
{ 
public:
   SignalDeletionTester() {
      m_bHasRun = false;
      m_sig = new Signals::Signal<void(void)>();
      m_connection0 = m_sig->connect(0, std::bind(&SignalDeletionTester::a, this)); 
      m_connection1 = m_sig->connect(1, std::bind(&SignalDeletionTester::b, this));
   }
   
   ~SignalDeletionTester()
   {
      if(m_sig != 0)
         delete m_sig;
   }
   
   void a() 
   {
      if(m_sig != 0)
         delete m_sig;
      m_sig = 0;
   }
   
   void b() 
   {
      m_bHasRun = true;
   } 
   
   Signals::Signal<void(void)> *m_sig;
   bool m_bHasRun;
   Signals::Connection m_connection0;
   Signals::Connection m_connection1;
}; 

TEST(SignalDeleteTest, testSignalDeletion)
{
   SignalDeletionTester tester;
   (*tester.m_sig)();
   ASSERT_EQ(tester.m_bHasRun, true);
   ASSERT_EQ(tester.m_connection0.connected(), false);
   ASSERT_EQ(tester.m_connection1.connected(), false);
}
