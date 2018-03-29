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
// Created by softboy on 2018/03/29.

#include "gtest/gtest.h"
#include "pdk/kernel/ElapsedTimer.h"
#include "pdk/base/time/Time.h"
#include "pdk/base/io/Debug.h"
#include "pdk/global/Logging.h"
#include "pdktest/PdkTest.h"
#include <iostream>

using pdk::kernel::ElapsedTimer;
using pdk::time::Time;
using pdk::io::Debug;

static const int minResolution = 100; // the minimum resolution for the tests
Debug operator<<(Debug s, const ElapsedTimer &t)
{
   s.nospace() << "(" << t.getMsecsSinceReference() << ")";
   return s.space();
}

TEST(ElapsedTimerTest, testStatics)
{
   //   debug_stream() << "Clock type is" << pdk::as_integer<ElapsedTimer::ClockType>(ElapsedTimer::getClockType());
   //   debug_stream() << "Said clock is" << (ElapsedTimer::isMonotonic() ? "monotonic" : "not monotonic");
   //   ElapsedTimer t;
   //   t.start();
   //   debug_stream() << "Current time is" << t.getMsecsSinceReference();
   
   std::cout << "Clock type is" << pdk::as_integer<ElapsedTimer::ClockType>(ElapsedTimer::getClockType()) << std::endl;
   std::cout << "Said clock is" << (ElapsedTimer::isMonotonic() ? "monotonic" : "not monotonic") << std::endl;
   ElapsedTimer t;
   t.start();
   std::cout << "Current time is " << t.getMsecsSinceReference() << std::endl;
}

TEST(ElapsedTimerTest, testBasics)
{
   ElapsedTimer t1;
   t1.start();
   
   ASSERT_TRUE(t1.getMsecsSinceReference() != 0);
   
   ASSERT_EQ(t1, t1);
   ASSERT_TRUE(!(t1 != t1));
   ASSERT_TRUE(!(t1 < t1));
   ASSERT_EQ(t1.msecsTo(t1), pdk::pint64(0));
   ASSERT_EQ(t1.secsTo(t1), pdk::pint64(0));
   
   pdk::puint64 value1 = t1.getMsecsSinceReference();
   //std::cout << "value1:" << value1 << "t1:" << t1 << std::endl;
   pdk::pint64 nsecs = t1.getNsecsElapsed();
   pdk::pint64 elapsed = t1.restart();
   ASSERT_TRUE(elapsed < minResolution);
   ASSERT_TRUE(nsecs / 1000000 < minResolution);
   
   pdk::puint64 value2 = t1.getMsecsSinceReference();
   //   std::cout << "value2:" << value2 << "t1:" << t1
   //            << "elapsed:" << elapsed << "nsecs:" << nsecs << std::endl;
   // in theory, elapsed == value2 - value1
   
   // However, since ElapsedTimer keeps internally the full resolution,
   // we have here a rounding error due to integer division
   ASSERT_TRUE(std::abs(elapsed - pdk::pint64(value2 - value1)) <= 1);
}

TEST(ElapsedTimerTest, testElapsed)
{
   ElapsedTimer t1;
   t1.start();
   
   pdktest::sleep(2*minResolution);
   ElapsedTimer t2;
   t2.start();
   
   ASSERT_TRUE(t1 != t2);
   ASSERT_TRUE(!(t1 == t2));
   ASSERT_TRUE(t1 < t2);
   ASSERT_TRUE(t1.msecsTo(t2) > 0);
   
   ASSERT_TRUE(t1.getNsecsElapsed() > 0);
   ASSERT_TRUE(t1.getElapsed() > 0);
   // the number of elapsed nanoseconds and milliseconds should match
   ASSERT_TRUE(t1.getNsecsElapsed() - t1.getElapsed() * 1000000 < 1000000);
   ASSERT_TRUE(t1.hasExpired(minResolution));
   ASSERT_TRUE(!t1.hasExpired(8*minResolution));
   ASSERT_TRUE(!t2.hasExpired(minResolution));
   
   ASSERT_TRUE(!t1.hasExpired(-1));
   ASSERT_TRUE(!t2.hasExpired(-1));
   
   pdk::pint64 elapsed = t1.restart();
   ASSERT_TRUE(elapsed > minResolution);
   ASSERT_TRUE(elapsed < 3*minResolution);
   pdk::pint64 diff = t2.msecsTo(t1);
   ASSERT_TRUE(diff < minResolution);
}
