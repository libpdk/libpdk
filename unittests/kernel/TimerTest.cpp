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
// Created by softboy on 2018/03/28.

#include "gtest/gtest.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/global/Global.h"
#include "pdktest/PdkTest.h"
#include "pdk/kernel/Timer.h"
#include "pdk/base/os/thread/Thread.h"

#if defined PDK_OS_UNIX
#include <unistd.h>
#endif

using pdk::kernel::Object;
using pdk::kernel::Timer;
using pdk::kernel::CoreApplication;

class TimerHelper : public Object
{
public:
   TimerHelper() 
      : m_count(0),
        m_remainingTime(-1)
   {
   }
   
   int m_count;
   int m_remainingTime;
   
public:
   void timeout();
   void fetchRemainingTime(Timer::SignalType signal, Object *sender);
};

void TimerHelper::timeout()
{
   ++m_count;
}

void TimerHelper::fetchRemainingTime(Timer::SignalType signal, Object *sender)
{
   Timer *timer = static_cast<Timer *>(sender);
   m_remainingTime = timer->getRemainingTime();
}

TEST(TimerTest, testZeroTimer)
{
   TimerHelper helper;
   Timer timer;
   timer.setInterval(0);
   timer.start();
   timer.connectTimeoutSignal(&helper, &TimerHelper::timeout);
   CoreApplication::processEvents();
   ASSERT_EQ(helper.m_count, 1);
}

TEST(TimerTest, testSingleShotTimeout)
{
   TimerHelper helper;
   Timer timer;
   timer.setSingleShot(true);
   timer.connectTimeoutSignal(&helper, &TimerHelper::timeout);
   timer.start(100);
   pdktest::wait(500);
   ASSERT_EQ(helper.m_count, 1);
   pdktest::wait(500);
   ASSERT_EQ(helper.m_count, 1);
}

#define TIMEOUT_TIMEOUT 200

TEST(TimerTest, testTimeout)
{
   TimerHelper helper;
   Timer timer;
   timer.connectTimeoutSignal(&helper, &TimerHelper::timeout);
   timer.start(100);
   ASSERT_EQ(helper.m_count, 0);
   PDK_TRY_VERIFY_WITH_TIMEOUT(helper.m_count > 0, TIMEOUT_TIMEOUT);
   int oldCount = helper.m_count;
   PDK_TRY_VERIFY_WITH_TIMEOUT(helper.m_count > oldCount, TIMEOUT_TIMEOUT);
}
