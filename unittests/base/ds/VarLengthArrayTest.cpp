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
// Created by zzu_softboy on 2017/12/27.

#include "gtest/gtest.h"
#include <list>
#include <utility>
#include <tuple>
#include <vector>
#include <algorithm>
#include <string>

#include "pdk/base/ds/VarLengthArray.h"

using pdk::ds::VarLengthArray;

namespace
{

int fooCtor = 0;
int fooDtor = 0;

struct Foo
{
   int *m_ptr;
   Foo()
   {
      m_ptr = new int;
      ++fooCtor;
   }
   
   Foo(const Foo &/*other*/)
   {
      m_ptr = new int;
      ++fooCtor;
   }
   
   ~Foo()
   {
      delete m_ptr;
      ++fooDtor;
   }
};

}

TEST(VarLengthArrayTest, testAppend)
{
   VarLengthArray<std::string> v(0);
   v.append(std::string("a"));
//   char buf[256];
//   std::string *p = new (buf) std::string("abc");
//   p->~basic_string();
}
