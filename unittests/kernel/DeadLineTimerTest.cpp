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
#include "pdk/base/lang/String.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/time/Time.h"
#include "pdk/kernel/DeadlineTimer.h"
#include "pdk/utils/Locale.h"
#include "pdktest/PdkTest.h"
#include <chrono>
#include <list>

using pdk::lang::String;
using pdk::time::Time;
using pdk::kernel::DeadlineTimer;
using pdk::ds::ByteArray;
using pdk::utils::Locale;

static const int minResolution = 400; // the minimum resolution for the tests
static std::list<pdk::TimerType> sg_timerTypes
{
   pdk::TimerType::CoarseTimer,
         pdk::TimerType::PreciseTimer
};

TEST(DeadlineTimerTest, testBasics)
{
   DeadlineTimer deadline;
   ASSERT_EQ(deadline.getTimerType(), pdk::TimerType::CoarseTimer);
   for (const pdk::TimerType timerType: sg_timerTypes) {
      deadline = DeadlineTimer(timerType);
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline, DeadlineTimer(timerType));
      ASSERT_TRUE(!(deadline != DeadlineTimer(timerType)));
      ASSERT_TRUE(!(deadline < DeadlineTimer()));
      ASSERT_TRUE(deadline <= DeadlineTimer());
      ASSERT_TRUE(deadline >= DeadlineTimer());
      ASSERT_TRUE(!(deadline > DeadlineTimer()));
      ASSERT_TRUE(!(deadline < deadline));
      ASSERT_TRUE(deadline <= deadline);
      ASSERT_TRUE(deadline >= deadline);
      ASSERT_TRUE(!(deadline > deadline));
      
      // should have expired, but we may be running too early after boot
      PDK_TRY_VERIFY_WITH_TIMEOUT(deadline.hasExpired(), 100);
      
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(0));
      ASSERT_EQ(deadline.getDeadlineNSecs(), pdk::pint64(0));
      ASSERT_EQ(deadline.getDeadline(), pdk::pint64(0));
      ASSERT_EQ(deadline.getDeadlineNSecs(), pdk::pint64(0));
      
      deadline.setRemainingTime(0, timerType);
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(0));
      ASSERT_EQ(deadline.getRemainingTimeNSecs(), pdk::pint64(0));
      ASSERT_TRUE(deadline.getDeadline() != 0);
      ASSERT_TRUE(deadline.getDeadline() != std::numeric_limits<pdk::pint64>::max());
      ASSERT_TRUE(deadline.getDeadlineNSecs() != 0);
      ASSERT_TRUE(deadline.getDeadlineNSecs() != std::numeric_limits<pdk::pint64>::max());
      
      deadline.setPreciseRemainingTime(0, 0, timerType);
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(0));
      ASSERT_EQ(deadline.getRemainingTimeNSecs(), pdk::pint64(0));
      ASSERT_TRUE(deadline.getDeadline() != 0);
      ASSERT_TRUE(deadline.getDeadline() != std::numeric_limits<pdk::pint64>::max());
      ASSERT_TRUE(deadline.getDeadlineNSecs() != 0);
      ASSERT_TRUE(deadline.getDeadlineNSecs() != std::numeric_limits<pdk::pint64>::max());
      
      deadline.setDeadline(0, timerType);
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(0));
      ASSERT_EQ(deadline.getDeadlineNSecs(), pdk::pint64(0));
      ASSERT_EQ(deadline.getDeadline(), pdk::pint64(0));
      ASSERT_EQ(deadline.getDeadlineNSecs(), pdk::pint64(0));
      
      deadline.setPreciseDeadline(0, 0, timerType);
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(0));
      ASSERT_EQ(deadline.getDeadlineNSecs(), pdk::pint64(0));
      ASSERT_EQ(deadline.getDeadline(), pdk::pint64(0));
      ASSERT_EQ(deadline.getDeadlineNSecs(), pdk::pint64(0));
   }
}

TEST(DeadlineTimerTest, testForeverness)
{
   for (const pdk::TimerType timerType: sg_timerTypes) {
      DeadlineTimer deadline = DeadlineTimer::ForeverConstant::Forever;
      ASSERT_EQ(deadline.getTimerType(), pdk::TimerType::CoarseTimer);
      ASSERT_TRUE(deadline.isForever());
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(-1));
      ASSERT_EQ(deadline.getRemainingTimeNSecs(), pdk::pint64(-1));
      ASSERT_EQ(deadline.getDeadline(), std::numeric_limits<pdk::pint64>::max());
      ASSERT_EQ(deadline.getDeadlineNSecs(), std::numeric_limits<pdk::pint64>::max());
      
      deadline = DeadlineTimer(-1, timerType);
      ASSERT_TRUE(deadline.isForever());
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(-1));
      ASSERT_EQ(deadline.getRemainingTimeNSecs(), pdk::pint64(-1));
      ASSERT_EQ(deadline.getDeadline(), std::numeric_limits<pdk::pint64>::max());
      ASSERT_EQ(deadline.getDeadlineNSecs(), std::numeric_limits<pdk::pint64>::max());
      
      deadline.setRemainingTime(-1, timerType);
      ASSERT_TRUE(deadline.isForever());
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(-1));
      ASSERT_EQ(deadline.getRemainingTimeNSecs(), pdk::pint64(-1));
      ASSERT_EQ(deadline.getDeadline(), std::numeric_limits<pdk::pint64>::max());
      ASSERT_EQ(deadline.getDeadlineNSecs(), std::numeric_limits<pdk::pint64>::max());
      
      deadline.setPreciseRemainingTime(-1, 0, timerType);
      ASSERT_TRUE(deadline.isForever());
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(-1));
      ASSERT_EQ(deadline.getRemainingTimeNSecs(), pdk::pint64(-1));
      ASSERT_EQ(deadline.getDeadline(), std::numeric_limits<pdk::pint64>::max());
      ASSERT_EQ(deadline.getDeadlineNSecs(), std::numeric_limits<pdk::pint64>::max());
      
      deadline.setPreciseRemainingTime(-1, -1, timerType);
      ASSERT_TRUE(deadline.isForever());
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(-1));
      ASSERT_EQ(deadline.getRemainingTimeNSecs(), pdk::pint64(-1));
      ASSERT_EQ(deadline.getDeadline(), std::numeric_limits<pdk::pint64>::max());
      ASSERT_EQ(deadline.getDeadlineNSecs(), std::numeric_limits<pdk::pint64>::max());
      
      deadline.setDeadline(std::numeric_limits<pdk::pint64>::max(), timerType);
      ASSERT_TRUE(deadline.isForever());
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(-1));
      ASSERT_EQ(deadline.getRemainingTimeNSecs(), pdk::pint64(-1));
      ASSERT_EQ(deadline.getDeadline(), std::numeric_limits<pdk::pint64>::max());
      ASSERT_EQ(deadline.getDeadlineNSecs(), std::numeric_limits<pdk::pint64>::max());
      
      deadline.setPreciseDeadline(std::numeric_limits<pdk::pint64>::max(), 0, timerType);
      ASSERT_TRUE(deadline.isForever());
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(-1));
      ASSERT_EQ(deadline.getRemainingTimeNSecs(), pdk::pint64(-1));
      ASSERT_EQ(deadline.getDeadline(), std::numeric_limits<pdk::pint64>::max());
      ASSERT_EQ(deadline.getDeadlineNSecs(), std::numeric_limits<pdk::pint64>::max());
      
      ASSERT_EQ(deadline, deadline);
      ASSERT_TRUE(!(deadline < deadline));
      ASSERT_TRUE(deadline <= deadline);
      ASSERT_TRUE(deadline >= deadline);
      ASSERT_TRUE(!(deadline > deadline));
      
      // adding to forever must still be forever
      DeadlineTimer deadline2 = deadline + 1;
      ASSERT_TRUE(deadline2.isForever());
      ASSERT_TRUE(!deadline2.hasExpired());
      ASSERT_EQ(deadline2.getRemainingTime(), pdk::pint64(-1));
      ASSERT_EQ(deadline2.getRemainingTimeNSecs(), pdk::pint64(-1));
      ASSERT_EQ(deadline2.getDeadline(), std::numeric_limits<pdk::pint64>::max());
      ASSERT_EQ(deadline2.getDeadlineNSecs(), std::numeric_limits<pdk::pint64>::max());
      ASSERT_EQ(deadline2.getTimerType(), deadline.getTimerType());
      
      ASSERT_EQ(deadline2 - deadline, pdk::pint64(0));
      ASSERT_EQ(deadline2, deadline);
      ASSERT_TRUE(!(deadline2 < deadline));
      ASSERT_TRUE(deadline2 <= deadline);
      ASSERT_TRUE(deadline2 >= deadline);
      ASSERT_TRUE(!(deadline2 > deadline));
      
      // subtracting from forever is *also* forever
      deadline2 = deadline - 1;
      ASSERT_TRUE(deadline2.isForever());
      ASSERT_TRUE(!deadline2.hasExpired());
      ASSERT_EQ(deadline2.getRemainingTime(), pdk::pint64(-1));
      ASSERT_EQ(deadline2.getRemainingTimeNSecs(), pdk::pint64(-1));
      ASSERT_EQ(deadline2.getDeadline(), std::numeric_limits<pdk::pint64>::max());
      ASSERT_EQ(deadline2.getDeadlineNSecs(), std::numeric_limits<pdk::pint64>::max());
      ASSERT_EQ(deadline2.getTimerType(), deadline.getTimerType());
      
      ASSERT_EQ(deadline2 - deadline, pdk::pint64(0));
      ASSERT_EQ(deadline2, deadline);
      ASSERT_TRUE(!(deadline2 < deadline));
      ASSERT_TRUE(deadline2 <= deadline);
      ASSERT_TRUE(deadline2 >= deadline);
      ASSERT_TRUE(!(deadline2 > deadline));
      
      // compare and order against a default-constructed object
      DeadlineTimer expired;
      ASSERT_TRUE(!(deadline == expired));
      ASSERT_TRUE(deadline != expired);
      ASSERT_TRUE(!(deadline < expired));
      ASSERT_TRUE(!(deadline <= expired));
      ASSERT_TRUE(deadline >= expired);
      ASSERT_TRUE(deadline > expired);
   }
}

TEST(DeadlineTimerTest, testCurrent)
{
   for (const pdk::TimerType timerType: sg_timerTypes) {
      auto deadline = DeadlineTimer::getCurrent(timerType);
      ASSERT_TRUE(deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(0));
      ASSERT_EQ(deadline.getRemainingTimeNSecs(), pdk::pint64(0));
      ASSERT_TRUE(deadline.getDeadline() != 0);
      ASSERT_TRUE(deadline.getDeadline() != std::numeric_limits<pdk::pint64>::max());
      ASSERT_TRUE(deadline.getDeadlineNSecs() != 0);
      ASSERT_TRUE(deadline.getDeadlineNSecs() != std::numeric_limits<pdk::pint64>::max());
      
      // subtracting from current should be "more expired"
      DeadlineTimer earlierDeadline = deadline - 1;
      ASSERT_TRUE(earlierDeadline.hasExpired());
      ASSERT_TRUE(!earlierDeadline.isForever());
      ASSERT_EQ(earlierDeadline.getTimerType(), timerType);
      ASSERT_EQ(earlierDeadline.getRemainingTime(), pdk::pint64(0));
      ASSERT_EQ(earlierDeadline.getRemainingTimeNSecs(), pdk::pint64(0));
      ASSERT_TRUE(earlierDeadline.getDeadline() != 0);
      ASSERT_TRUE(earlierDeadline.getDeadline() != std::numeric_limits<pdk::pint64>::max());
      ASSERT_TRUE(earlierDeadline.getDeadlineNSecs() != 0);
      ASSERT_TRUE(earlierDeadline.getDeadlineNSecs() != std::numeric_limits<pdk::pint64>::max());
      ASSERT_EQ(earlierDeadline.getDeadline(), deadline.getDeadline() - 1);
      ASSERT_EQ(earlierDeadline.getDeadlineNSecs(), deadline.getDeadlineNSecs() - 1000*1000);
      
      ASSERT_EQ(earlierDeadline - deadline, pdk::pint64(-1));
      ASSERT_TRUE(earlierDeadline != deadline);
      ASSERT_TRUE(earlierDeadline < deadline);
      ASSERT_TRUE(earlierDeadline <= deadline);
      ASSERT_TRUE(!(earlierDeadline >= deadline));
      ASSERT_TRUE(!(earlierDeadline > deadline));
   }
}

TEST(DeadlineTimerTest, testDeadlines)
{
   for (const pdk::TimerType timerType: sg_timerTypes) {
      DeadlineTimer deadline(4 * minResolution, timerType);
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_TRUE(deadline.getRemainingTime() > (3 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTime() <= (4 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTimeNSecs() > (3000000 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTimeNSecs() <= (4000000 * minResolution));
      ASSERT_TRUE(deadline.getDeadline() != 0);
      ASSERT_TRUE(deadline.getDeadline() != std::numeric_limits<pdk::pint64>::max());
      ASSERT_TRUE(deadline.getDeadlineNSecs() != 0);
      ASSERT_TRUE(deadline.getDeadlineNSecs() != std::numeric_limits<pdk::pint64>::max());
      
      deadline.setRemainingTime(4 * minResolution, timerType);
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_TRUE(deadline.getRemainingTime() > (3 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTime() <= (4 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTimeNSecs() > (3000000 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTimeNSecs() <= (4000000 * minResolution));
      ASSERT_TRUE(deadline.getDeadline() != 0);
      ASSERT_TRUE(deadline.getDeadline() != std::numeric_limits<pdk::pint64>::max());
      ASSERT_TRUE(deadline.getDeadlineNSecs() != 0);
      ASSERT_TRUE(deadline.getDeadlineNSecs() != std::numeric_limits<pdk::pint64>::max());
      
      deadline.setPreciseRemainingTime(0, 4000000 * minResolution, timerType);
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_TRUE(deadline.getRemainingTime() > (3 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTime() <= (4 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTimeNSecs() > (3000000 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTimeNSecs() <= (4000000 * minResolution));
      ASSERT_TRUE(deadline.getDeadline() != 0);
      ASSERT_TRUE(deadline.getDeadline() != std::numeric_limits<pdk::pint64>::max());
      ASSERT_TRUE(deadline.getDeadlineNSecs() != 0);
      ASSERT_TRUE(deadline.getDeadlineNSecs() != std::numeric_limits<pdk::pint64>::max());
      
      deadline.setPreciseRemainingTime(1, 0, timerType); // 1 sec
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_TRUE(deadline.getRemainingTime() > (1000 - minResolution));
      ASSERT_TRUE(deadline.getRemainingTime() <= 1000);
      ASSERT_TRUE(deadline.getRemainingTimeNSecs() > (1000 - minResolution)*1000*1000);
      ASSERT_TRUE(deadline.getRemainingTimeNSecs() <= (1000*1000*1000));
      ASSERT_TRUE(deadline.getDeadline() != 0);
      ASSERT_TRUE(deadline.getDeadline() != std::numeric_limits<pdk::pint64>::max());
      ASSERT_TRUE(deadline.getDeadlineNSecs() != 0);
      ASSERT_TRUE(deadline.getDeadlineNSecs() != std::numeric_limits<pdk::pint64>::max());
      
      // adding to a future deadline must still be further in the future
      DeadlineTimer laterDeadline = deadline + 1;
      ASSERT_TRUE(!laterDeadline.hasExpired());
      ASSERT_TRUE(!laterDeadline.isForever());
      ASSERT_EQ(laterDeadline.getTimerType(), timerType);
      ASSERT_TRUE(laterDeadline.getRemainingTime() > (1000 - minResolution));
      ASSERT_TRUE(laterDeadline.getRemainingTime() <= 1001);
      ASSERT_TRUE(laterDeadline.getRemainingTimeNSecs() > (1001 - minResolution)*1000*1000);
      ASSERT_TRUE(laterDeadline.getRemainingTimeNSecs() <= (1001*1000*1000));
      ASSERT_TRUE(laterDeadline.getDeadline() != 0);
      ASSERT_TRUE(laterDeadline.getDeadline() != std::numeric_limits<pdk::pint64>::max());
      ASSERT_TRUE(laterDeadline.getDeadlineNSecs() != 0);
      ASSERT_TRUE(laterDeadline.getDeadlineNSecs() != std::numeric_limits<pdk::pint64>::max());
      ASSERT_EQ(laterDeadline.getDeadline(), deadline.getDeadline() + 1);
      ASSERT_EQ(laterDeadline.getDeadlineNSecs(), deadline.getDeadlineNSecs() + 1000*1000);
      
      ASSERT_EQ(laterDeadline - deadline, pdk::pint64(1));
      ASSERT_TRUE(laterDeadline != deadline);
      ASSERT_TRUE(!(laterDeadline < deadline));
      ASSERT_TRUE(!(laterDeadline <= deadline));
      ASSERT_TRUE(laterDeadline >= deadline);
      ASSERT_TRUE(laterDeadline > deadline);
      
      // compare and order against a default-constructed object
      DeadlineTimer expired;
      ASSERT_TRUE(!(deadline == expired));
      ASSERT_TRUE(deadline != expired);
      ASSERT_TRUE(!(deadline < expired));
      ASSERT_TRUE(!(deadline <= expired));
      ASSERT_TRUE(deadline >= expired);
      ASSERT_TRUE(deadline > expired);
      
      // compare and order against a forever deadline
      DeadlineTimer forever_(DeadlineTimer::ForeverConstant::Forever);
      ASSERT_TRUE(!(deadline == forever_));
      ASSERT_TRUE(deadline != forever_);
      ASSERT_TRUE(deadline < forever_);
      ASSERT_TRUE(deadline <= forever_);
      ASSERT_TRUE(!(deadline >= forever_));
      ASSERT_TRUE(!(deadline > forever_));
   }
}

TEST(DeadlineTimerTest, testSetDeadline)
{
   for (const pdk::TimerType timerType: sg_timerTypes) {
      auto now = DeadlineTimer::getCurrent(timerType);
      DeadlineTimer deadline;
      
      deadline.setDeadline(now.getDeadline(), timerType);
      ASSERT_TRUE(deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(0));
      ASSERT_EQ(deadline.getRemainingTimeNSecs(), pdk::pint64(0));
      ASSERT_EQ(deadline.getDeadline(), now.getDeadline());
      // don't check deadlineNSecs!
      
      deadline.setPreciseDeadline(now.getDeadlineNSecs() / (1000 * 1000 * 1000),
                                  now.getDeadlineNSecs() % (1000 * 1000 * 1000), timerType);
      ASSERT_TRUE(deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(0));
      ASSERT_EQ(deadline.getRemainingTimeNSecs(), pdk::pint64(0));
      ASSERT_EQ(deadline.getDeadline(), now.getDeadline());
      ASSERT_EQ(deadline.getDeadlineNSecs(), now.getDeadlineNSecs());
      
      now = DeadlineTimer::getCurrent(timerType);
      deadline.setDeadline(now.getDeadline() + 4 * minResolution, timerType);
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_TRUE(deadline.getRemainingTime() > (3 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTime() <= (4 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTimeNSecs() > (3000000 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTimeNSecs() <= (4000000 * minResolution));
      ASSERT_EQ(deadline.getDeadline(), now.getDeadline() + 4 * minResolution);  // yes, it's exact
      // don't check deadlineNSecs!
      
      now = DeadlineTimer::getCurrent(timerType);
      pdk::pint64 nsec = now.getDeadlineNSecs() + 4000000 * minResolution;
      deadline.setPreciseDeadline(nsec / (1000 * 1000 * 1000),
                                  nsec % (1000 * 1000 * 1000), timerType);
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_TRUE(deadline.getRemainingTime() > (3 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTime() <= (4 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTimeNSecs() > (3000000 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTimeNSecs() <= (4000000 * minResolution));
      ASSERT_EQ(deadline.getDeadline(), nsec / (1000 * 1000));
      ASSERT_EQ(deadline.getDeadlineNSecs(), nsec);
   }
}

TEST(DeadlineTimerTest, testExpire)
{
   for (const pdk::TimerType timerType: sg_timerTypes) {
      DeadlineTimer deadline(minResolution, timerType);
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      pdk::pint64 previousDeadline = deadline.getDeadlineNSecs();
      pdktest::sleep(2 * minResolution);
      ASSERT_EQ(deadline.getRemainingTime(), pdk::pint64(0));
      ASSERT_EQ(deadline.getRemainingTimeNSecs(), pdk::pint64(0));
      ASSERT_TRUE(deadline.getDeadline() != 0);
      ASSERT_TRUE(deadline.getDeadline() != std::numeric_limits<pdk::pint64>::max());
      ASSERT_TRUE(deadline.getDeadlineNSecs() != 0);
      ASSERT_TRUE(deadline.getDeadlineNSecs() != std::numeric_limits<pdk::pint64>::max());
      ASSERT_EQ(deadline.getDeadlineNSecs(), previousDeadline);
   }
}

TEST(DeadlineTimerTest, testStdchrono)
{
   using namespace std::chrono;
   for (const pdk::TimerType timerType: sg_timerTypes) {
      
      // create some forevers
      DeadlineTimer deadline = milliseconds::max();
      ASSERT_TRUE(deadline.isForever());
      deadline = milliseconds::max();
      ASSERT_TRUE(deadline.isForever());
      deadline.setRemainingTime(milliseconds::max(), timerType);
      ASSERT_TRUE(deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      deadline = nanoseconds::max();
      ASSERT_TRUE(deadline.isForever());
      deadline.setRemainingTime(nanoseconds::max(), timerType);
      ASSERT_TRUE(deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      deadline = hours::max();
      ASSERT_TRUE(deadline.isForever());
      deadline.setRemainingTime(hours::max(), timerType);
      ASSERT_TRUE(deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      
      deadline = time_point<system_clock>::max();
      ASSERT_TRUE(deadline.isForever());
      deadline.setDeadline(time_point<system_clock>::max(), timerType);
      ASSERT_TRUE(deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      deadline = time_point<steady_clock>::max();
      ASSERT_TRUE(deadline.isForever());
      deadline.setDeadline(time_point<steady_clock>::max(), timerType);
      ASSERT_TRUE(deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      
      ASSERT_TRUE(deadline == time_point<steady_clock>::max());
      ASSERT_TRUE(deadline == time_point<system_clock>::max());
      ASSERT_EQ(deadline.getRemainingTimeAsDuration(), nanoseconds::max());
      
      // make it expired
      deadline = time_point<system_clock>();
      ASSERT_TRUE(deadline.hasExpired());
      deadline.setDeadline(time_point<system_clock>(), timerType);
      ASSERT_TRUE(deadline.hasExpired());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      deadline = time_point<steady_clock>();
      ASSERT_TRUE(deadline.hasExpired());
      deadline.setDeadline(time_point<steady_clock>(), timerType);
      ASSERT_TRUE(deadline.hasExpired());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      
      ASSERT_EQ(deadline.getRemainingTimeAsDuration(), nanoseconds::zero());
      
      auto steady_before = steady_clock::now();
      auto system_before = system_clock::now();
      
      pdktest::sleep(minResolution);
      auto now = DeadlineTimer::getCurrent(timerType);
      pdktest::sleep(minResolution);
      
      auto steady_after = steady_clock::now();
      auto system_after = system_clock::now();
      
      {
//         auto diff = duration_cast<milliseconds>(steady_after - now.deadline<steady_clock>());
//         ASSERT_TRUE(diff.count() > minResolution/2, ByteArray::number(pdk::pint64(diff.count())));
//         ASSERT_TRUE(diff.count() < 3*minResolution/2, ByteArray::number(pdk::pint64(diff.count())));
//         DeadlineTimer dt_after(steady_after, timerType);
//         ASSERT_TRUE(now < dt_after,
//                  ("now = " + Locale().toString(now.getDeadlineNSecs()) +
//                   "; after = " + Locale().toString(dt_after.getDeadlineNSecs())).toLatin1());
         
//         diff = duration_cast<milliseconds>(now.deadline<steady_clock>() - steady_before);
//         ASSERT_TRUE(diff.count() > minResolution/2, ByteArray::number(pdk::pint64(diff.count())));
//         ASSERT_TRUE(diff.count() < 3*minResolution/2, ByteArray::number(pdk::pint64(diff.count())));
//         DeadlineTimer dt_before(steady_before, timerType);
//         ASSERT_TRUE(now > dt_before,
//                  ("now = " + Locale().toString(now.getDeadlineNSecs()) +
//                   "; before = " + Locale().toString(dt_before.getDeadlineNSecs())).toLatin1());
         auto diff = duration_cast<milliseconds>(steady_after - now.getDeadline<steady_clock>());
         ASSERT_TRUE(diff.count() > minResolution/2);
         ASSERT_TRUE(diff.count() < 3*minResolution/2);
         DeadlineTimer dt_after(steady_after, timerType);
         ASSERT_TRUE(now < dt_after);
         
         diff = duration_cast<milliseconds>(now.getDeadline<steady_clock>() - steady_before);
         ASSERT_TRUE(diff.count() > minResolution/2);
         ASSERT_TRUE(diff.count() < 3*minResolution/2);
         DeadlineTimer dt_before(steady_before, timerType);
         ASSERT_TRUE(now > dt_before);
      }
      {
//         auto diff = duration_cast<milliseconds>(system_after - now.deadline<system_clock>());
//         ASSERT_TRUE(diff.count() > minResolution/2, ByteArray::number(pdk::pint64(diff.count())));
//         ASSERT_TRUE(diff.count() < 3*minResolution/2, ByteArray::number(pdk::pint64(diff.count())));
//         DeadlineTimer dt_after(system_after, timerType);
//         ASSERT_TRUE(now < dt_after,
//                  ("now = " + Locale().toString(now.getDeadlineNSecs()) +
//                   "; after = " + Locale().toString(dt_after.getDeadlineNSecs())).toLatin1());
         
//         diff = duration_cast<milliseconds>(now.deadline<system_clock>() - system_before);
//         ASSERT_TRUE(diff.count() > minResolution/2, ByteArray::number(pdk::pint64(diff.count())));
//         ASSERT_TRUE(diff.count() < 3*minResolution/2, ByteArray::number(pdk::pint64(diff.count())));
//         DeadlineTimer dt_before(system_before, timerType);
//         ASSERT_TRUE(now > dt_before,
//                  ("now = " + Locale().toString(now.getDeadlineNSecs()) +
//                   "; before = " + Locale().toString(dt_before.getDeadlineNSecs())).toLatin1());
         auto diff = duration_cast<milliseconds>(system_after - now.getDeadline<system_clock>());
         ASSERT_TRUE(diff.count() > minResolution/2);
         ASSERT_TRUE(diff.count() < 3*minResolution/2);
         DeadlineTimer dt_after(system_after, timerType);
         ASSERT_TRUE(now < dt_after);
         
         diff = duration_cast<milliseconds>(now.getDeadline<system_clock>() - system_before);
         ASSERT_TRUE(diff.count() > minResolution/2);
         ASSERT_TRUE(diff.count() < 3*minResolution/2);
         DeadlineTimer dt_before(system_before, timerType);
         ASSERT_TRUE(now > dt_before);
         
      }
      
      // make it regular
      now = DeadlineTimer::getCurrent(timerType);
      deadline.setRemainingTime(milliseconds(4 * minResolution), timerType);
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_TRUE(deadline.getRemainingTimeAsDuration() > milliseconds(3 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTimeAsDuration() < milliseconds(5 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTimeAsDuration() > nanoseconds(3000000 * minResolution));
      ASSERT_TRUE(deadline.getRemainingTimeAsDuration() < nanoseconds(5000000 * minResolution));
      ASSERT_TRUE(deadline.getDeadline<steady_clock>() > (steady_clock::now() + milliseconds(3 * minResolution)));
      ASSERT_TRUE(deadline.getDeadline<steady_clock>() < (steady_clock::now() + milliseconds(5 * minResolution)));
      ASSERT_TRUE(deadline.getDeadline<system_clock>() > (system_clock::now() + milliseconds(3 * minResolution)));
      ASSERT_TRUE(deadline.getDeadline<system_clock>() < (system_clock::now() + milliseconds(5 * minResolution)));
      if (timerType == pdk::TimerType::CoarseTimer) {
         ASSERT_TRUE(deadline > (now + milliseconds(3 * minResolution)));
         ASSERT_TRUE(deadline < (now + milliseconds(5 * minResolution)));
         ASSERT_TRUE(deadline > (now + nanoseconds(3000000 * minResolution)));
         ASSERT_TRUE(deadline < (now + nanoseconds(5000000 * minResolution)));
         ASSERT_TRUE(deadline > milliseconds(3 * minResolution));
         ASSERT_TRUE(deadline < milliseconds(5 * minResolution));
         ASSERT_TRUE(deadline > nanoseconds(3000000 * minResolution));
         ASSERT_TRUE(deadline < nanoseconds(5000000 * minResolution));
         ASSERT_TRUE(deadline >= steady_clock::now());
         ASSERT_TRUE(deadline >= system_clock::now());
      }
      
      now = DeadlineTimer::getCurrent(timerType);
      deadline = DeadlineTimer(seconds(1), timerType);
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_TRUE(deadline.getRemainingTimeAsDuration() > (seconds(1) - milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getRemainingTimeAsDuration() <= seconds(1));
      ASSERT_TRUE(deadline.getDeadline<steady_clock>() > (steady_clock::now() + seconds(1) - milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getDeadline<steady_clock>() <= (steady_clock::now() + seconds(1) + milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getDeadline<system_clock>() > (system_clock::now() + seconds(1) - milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getDeadline<system_clock>() <= (system_clock::now() + seconds(1) + milliseconds(minResolution)));
      if (timerType == pdk::TimerType::CoarseTimer) {
         ASSERT_TRUE(deadline > (seconds(1) - milliseconds(minResolution)));
         ASSERT_TRUE(deadline <= seconds(1));
      }
      
      now = DeadlineTimer::getCurrent(timerType);
      deadline.setRemainingTime(hours(1), timerType);
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_TRUE(deadline.getRemainingTimeAsDuration() > (hours(1) - milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getRemainingTimeAsDuration() <= hours(1));
      ASSERT_TRUE(deadline.getDeadline<steady_clock>() > (steady_clock::now() + hours(1) - milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getDeadline<steady_clock>() <= (steady_clock::now() + hours(1) + milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getDeadline<system_clock>() > (system_clock::now() + hours(1) - milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getDeadline<system_clock>() <= (system_clock::now() + hours(1) + milliseconds(minResolution)));
      
      now = DeadlineTimer::getCurrent(timerType);
      deadline.setDeadline(system_clock::now() + seconds(1), timerType);
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_TRUE(deadline.getRemainingTimeAsDuration() > (seconds(1) - milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getRemainingTimeAsDuration() <= seconds(1));
      ASSERT_TRUE(deadline.getDeadline<steady_clock>() > (steady_clock::now() + seconds(1) - milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getDeadline<steady_clock>() <= (steady_clock::now() + seconds(1) + milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getDeadline<system_clock>() > (system_clock::now() + seconds(1) - milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getDeadline<system_clock>() <= (system_clock::now() + seconds(1) + milliseconds(minResolution)));
      
      now = DeadlineTimer::getCurrent(timerType);
      deadline.setDeadline(steady_clock::now() + seconds(1), timerType);
      ASSERT_TRUE(!deadline.hasExpired());
      ASSERT_TRUE(!deadline.isForever());
      ASSERT_EQ(deadline.getTimerType(), timerType);
      ASSERT_TRUE(deadline.getRemainingTimeAsDuration() > (seconds(1) - milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getRemainingTimeAsDuration() <= seconds(1));
      ASSERT_TRUE(deadline.getDeadline<steady_clock>() > (steady_clock::now() + seconds(1) - milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getDeadline<steady_clock>() <= (steady_clock::now() + seconds(1) + milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getDeadline<system_clock>() > (system_clock::now() + seconds(1) - milliseconds(minResolution)));
      ASSERT_TRUE(deadline.getDeadline<system_clock>() <= (system_clock::now() + seconds(1) + milliseconds(minResolution)));
   }
}
