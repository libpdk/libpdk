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
// Created by softboy on 2018/01/21.

#include "gtest/gtest.h"
#include "pdk/kernel/signal/Signal.h"
#include <optional>
#include <functional>
#include <iostream>
#include <typeinfo>

namespace {

template<typename T>
struct MaxOrDefault {
   typedef T ResultType;
   using result_type = ResultType;
   template<typename InputIterator>
   typename InputIterator::ValueType
   operator()(InputIterator first, InputIterator last) const
   {
      std::optional<T> max;
      for (; first != last; ++first) {
         try
         {
            if(!max) max = *first;
            else max = (*first > max.value())? *first : max;
         }
         catch(const std::bad_weak_ptr &)
         {}
      }
      if(max) return max.value();
      return T();
   }
};

struct MakeInt {
   MakeInt(int n, int cn) : N(n), CN(cn) {}
   
   int operator()()
   { 
      return N;
   }
   
   int operator()() const
   {
      return CN;
   }
   
   int N;
   int CN;
};

template<int N>
struct MakeIncreasingInt {
   MakeIncreasingInt() : n(N)
   {}
   
   int operator()() const
   {
      return n++;
   }
   
   mutable int n;
};

}

namespace Signals = pdk::kernel::signal;

TEST(SignalTest, testZeroArgs)
{
   MakeInt i42(42, 41);
   MakeInt i2(2, 1);
   MakeInt i72(72, 71);
   MakeInt i63(63, 63);
   MakeInt i62(62, 61);
   {
      Signals::Signal<int (), MaxOrDefault<int>> signal0;
      std::cout << "sizeof(signal) = " << sizeof(signal0) << std::endl;
      Signals::Connection conn2 = signal0.connect(i2);
      Signals::Connection conn72 = signal0.connect(72, i72);
      Signals::Connection conn62 = signal0.connect(60, i62);
      Signals::Connection conn42 = signal0.connect(42, i42);
      ASSERT_EQ(signal0(), 72);
      signal0.disconnect(72);
      ASSERT_EQ(signal0(), 62);
      // Double-disconnect should be safe
      signal0.disconnect(72);
      ASSERT_EQ(signal0(), 62);
      // Triple-disconect should be safe
      ASSERT_EQ(signal0(), 62);
      // Also connect 63 in the same group as 62
      signal0.connect(60, i63);
      ASSERT_EQ(signal0(), 63);
      // Disconnect all of the 60's
      signal0.disconnect(60);
      ASSERT_EQ(signal0(), 42);
      conn42.disconnect();
      ASSERT_EQ(signal0(), 2);
      conn2.disconnect();
      ASSERT_EQ(signal0(), 0);
   }
   {
      Signals::Signal<int (), MaxOrDefault<int>> signal0;
      Signals::Connection conn2 = signal0.connect(i2);
      Signals::Connection conn72 = signal0.connect(i72);
      Signals::Connection conn62 = signal0.connect(i62);
      Signals::Connection conn42 = signal0.connect(i42);
      const Signals::Signal<int (), MaxOrDefault<int>> &csiganl0 = signal0;
      ASSERT_EQ(csiganl0(), 72);
   }
   {
      MakeIncreasingInt<7> i7;
      MakeIncreasingInt<10> i10;
      Signals::Signal<int (), MaxOrDefault<int>> signal0;
      Signals::Connection conn7 = signal0.connect(i7);
      Signals::Connection conn10 = signal0.connect(i10);
      ASSERT_EQ(signal0(), 10);
      ASSERT_EQ(signal0(), 11);
      PDK_UNUSED(conn7);
      PDK_UNUSED(conn10);
   }
}

TEST(SignalTest, testOneArgs)
{
   Signals::Signal<int (int value), MaxOrDefault<int>> signal0;
   signal0.connect(std::negate<int>());
   signal0.connect(std::bind(std::multiplies<int>(), 2, std::placeholders::_1));
   ASSERT_EQ(signal0(1), 2);
   ASSERT_EQ(signal0(-1), 1);
   signal0.connect([](int value){
      return 100 + value;
   });
   ASSERT_EQ(signal0(2), 102);
}

TEST(SignalTest, testSignalSignalConnect)
{
   using SignalType = Signals::Signal<int (int value), MaxOrDefault<int>>;
   SignalType signal1;
   signal1.connect(std::negate<int>());
   ASSERT_EQ(signal1(3), -3);
   {
      SignalType signal2;
      signal1.connect(signal2);
      signal2.connect(std::bind(std::multiplies<int>(), 2, std::placeholders::_1));
      signal2.connect(std::bind(std::multiplies<int>(), -3, std::placeholders::_1));
      ASSERT_EQ(signal2(-3), 9);
      ASSERT_EQ(signal1(3), 6);
   }
   ASSERT_EQ(signal1(3), -3);
}

namespace {

template<typename ResultType>
ResultType disconnecting_slot(const Signals::Connection &conn, int)
{
   conn.disconnect();
   return ResultType();
}

template<>
void disconnecting_slot<void>(const Signals::Connection &conn, int)
{
   conn.disconnect();
   return;
}

template<typename ResultType>
void test_extended_slot()
{
   {
      using SignalType = Signals::Signal<ResultType (int)>;
      using SlotType = typename SignalType::ExtendedSlotType;
      SignalType signal0;
      // attempting to work around msvc 7.1 bug by explicitly assigning to a function pointer
      ResultType (*fp)(const Signals::Connection &conn, int) = &disconnecting_slot<ResultType>;
      SlotType myslot(fp);
      signal0.connectExtended(myslot);
      ASSERT_EQ(signal0.getNumSlots(), 1ul);
      signal0(0);
      ASSERT_EQ(signal0.getNumSlots(), 0ul);
   }
   {
      // test 0 arg signal
      using SignalType = Signals::Signal<ResultType ()>;
      using SlotType = typename SignalType::ExtendedSlotType;
      SignalType signal0;
      
      // attempting to work around msvc 7.1 bug by explicitly assigning to a function pointer
      ResultType (*fp)(const Signals::Connection &conn, int) = &disconnecting_slot<ResultType>;
      SlotType myslot(fp, std::placeholders::_1, 0);
      signal0.connectExtended(myslot);
      ASSERT_EQ(signal0.getNumSlots(), 1ul);
      signal0();
      ASSERT_EQ(signal0.getNumSlots(), 0ul);
   }
   // test disconnection by slot
   {
      using SignalType = Signals::Signal<ResultType (int)>;
      using SlotType = typename SignalType::ExtendedSlotType;
      SignalType signal0;
      // attempting to work around msvc 7.1 bug by explicitly assigning to a function pointer
      ResultType (*fp)(const Signals::Connection &conn, int) = &disconnecting_slot<ResultType>;
      SlotType myslot(fp);
      signal0.connectExtended(myslot);
      ASSERT_EQ(signal0.getNumSlots(), 1ul);
      signal0.disconnect(fp);
      ASSERT_EQ(signal0.getNumSlots(), 0ul);
   }
}

void increment_arg(int &value)
{
   ++value;
}

class DummyCombiner
{
public:
   typedef int ResultType;
   
   DummyCombiner(ResultType retValue)
      : m_retValue(retValue)
   {}
   template<typename SlotIterator>
   ResultType operator()(SlotIterator, SlotIterator)
   {
      return m_retValue;
   }
private:
   ResultType m_retValue;
};

}

TEST(SignalTest, testExtendedSlot)
{
   test_extended_slot<int>();
   test_extended_slot<void>();
}

TEST(SignalTest, testReferenceArgs)
{
   using SignalType = Signals::Signal<void (int &)>;
   SignalType signal1;
   signal1.connect(&increment_arg);
   int value = 0;
   signal1(value);
   ASSERT_EQ(value, 1);
}

TEST(SignalTest, testTypedefsEtc)
{
   using SignalType = Signals::Signal<int (double, long)>;
   using SlotType = typename SignalType::SlotType;
   ASSERT_EQ(typeid(SignalType::SlotResultType), typeid(int));
   ASSERT_EQ(typeid(SignalType::ResultType), typeid(std::optional<int>));
   ASSERT_EQ(typeid(SignalType::Arg<0>::type), typeid(double));
   ASSERT_EQ(typeid(SignalType::Arg<1>::type), typeid(long));
   ASSERT_EQ(typeid(SignalType::SignatureType), typeid(int (double, long)));
   ASSERT_EQ(SignalType::arity, 2);
   
   ASSERT_EQ(typeid(SignalType::SlotResultType), typeid(SlotType::ResultType));
   ASSERT_EQ(typeid(SignalType::Arg<0>::type), typeid(SlotType::Arg<0>::type));
   ASSERT_EQ(typeid(SignalType::Arg<1>::type), typeid(SlotType::Arg<1>::type));
   ASSERT_EQ(typeid(SignalType::SignatureType), typeid(SlotType::SignatureType));
   ASSERT_EQ(SlotType::arity, 2);
   
   using UnarySignalType = Signals::Signal<void (short)>;
   ASSERT_EQ(typeid(UnarySignalType::SlotResultType), typeid(void));
   ASSERT_EQ(typeid(UnarySignalType::ArgType), typeid(short));
   ASSERT_EQ(typeid(UnarySignalType::SlotType::ArgType), typeid(short));
}

TEST(SignalTest, testSetCombiner)
{
   using SignalType = Signals::Signal<int (), DummyCombiner>;
   SignalType signal(DummyCombiner(0));
   ASSERT_EQ(signal(), 0);
   ASSERT_EQ(signal.getCombiner()(0,0), 0);
   signal.setCombiner(DummyCombiner(1));
   ASSERT_EQ(signal(), 1);
   ASSERT_EQ(signal.getCombiner()(0,0), 1);
}

TEST(SignalTest, testSwap)
{
   using SignalType = Signals::Signal<int (), DummyCombiner>;
   SignalType signal1(DummyCombiner(1));
   ASSERT_EQ(signal1(), 1);
   SignalType signal2(DummyCombiner(2));
   ASSERT_EQ(signal2(), 2);
   
   signal1.swap(signal2);
   ASSERT_EQ(signal1(), 2);
   ASSERT_EQ(signal2(), 1);
}

TEST(SignalTest, testMove)
{
   using SignalType = Signals::Signal<int (), DummyCombiner>;
   SignalType signal1(DummyCombiner(1));
   ASSERT_EQ(signal1(), 1);
   SignalType signal2(DummyCombiner(2));
   ASSERT_EQ(signal2(), 2);
   
   signal1 = std::move(signal2);
   ASSERT_EQ(signal1(), 2);
   
   SignalType signal3(std::move(signal1));
   ASSERT_EQ(signal3(), 2);
}
