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
  signal.connect(&my_slot);
}

}

TEST(RegressionTest, testSlotConnect)
{
   SignalType signal;
   //signal.connect(SignalType::SlotType(&my_connecting_slot, std::ref(sig)).track(sig));
}


