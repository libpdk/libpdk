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
#include "pdk/kernel/signal/Deconstruct.h"
#include <memory>

namespace Signals = pdk::kernel::signal;

namespace
{
class A
{
public:
   template<typename T> friend
   void adl_postconstruct(const std::shared_ptr<T> &sp, A *p)
   {
      ASSERT_TRUE(!p->_postconstructed);
      p->_postconstructed = true;
   }
   template<typename T> friend
   void adl_postconstruct(const std::shared_ptr<T> &sp, A *p, int val)
   {
      p->value = val;
      ASSERT_TRUE(!p->_postconstructed);
      p->_postconstructed = true;
   }
   friend void adl_predestruct(A *p)
   {
      p->_predestructed = true;
   }
   ~A()
   {
      doTest();
   }
   void doTest()
   {
      ASSERT_TRUE(_postconstructed);
      ASSERT_TRUE(_predestructed);
   }
   int value;
private:
   friend class Signals::DeconstructAccess;
   A(int value_in = 0):
      value(value_in),
      _postconstructed(false),
      _predestructed(false)
   {}
   bool _postconstructed;
   bool _predestructed;
};
}

TEST(DeconstructTest, testDeconstruct)
{
   {
      std::shared_ptr<A> a = Signals::deconstruct<A>(1);
      ASSERT_EQ(a->value, 1);
   }
   {// deconstruct const type
      std::shared_ptr<const A> a = Signals::deconstruct<const A>(3);
      ASSERT_EQ(a->value, 3);
   }
   {// passing arguments to postconstructor
      std::shared_ptr<A> a = Signals::deconstruct<A>().postConstruct(2);
      ASSERT_EQ(a->value, 2);
   }
}
