#include "gtest/gtest.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/time/Time.h"
#include "pdk/kernel/DeadlineTimer.h"
#include "pdktest/PdkTest.h"
#include <chrono>
#include <list>

using pdk::lang::String;
using pdk::time::Time;
using pdk::kernel::DeadlineTimer;

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
