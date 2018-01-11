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
// Created by softboy on 2017/01/11.

#include <sstream>
#include "gtest/gtest.h"
#include "pdk/stdext/optional/Optional.h"
#include "pdk/stdext/optional/OptionalIo.h"
#include "pdk/stdext/None.h"

using pdk::stdext::optional::Optional;

template <typename Opt>
void test2(Opt o, Opt buff)
{
   std::stringstream s ;
   const int markv = 123;
   int mark = 0;
   s << o << " " << markv;
   s >> buff >> mark;
   ASSERT_TRUE(buff == o);
   ASSERT_TRUE(mark == markv);
}

template <typename T>
void test(T v, T w)
{
   test2(pdk::stdext::make_optional(v), Optional<T>());
   test2(pdk::stdext::make_optional(v), pdk::stdext::make_optional(w));
   test2(Optional<T>(), Optional<T>());
   test2(Optional<T>(), pdk::stdext::make_optional(w));
}

template <class T>
void subtest_tag_none_reversibility_with_optional(Optional<T> ov)
{
   std::stringstream s;
   s << pdk::stdext::none;
   s >> ov;
   ASSERT_TRUE(!ov);
}

template <class T>
void subtest_tag_none_equivalence_with_optional()
{
   std::stringstream s, r;
   Optional<T> ov;
   s << pdk::stdext::none;
   r << ov;
   ASSERT_EQ(s.str(), r.str());
}

template <class T>
void test_tag_none(T v)
{
   subtest_tag_none_reversibility_with_optional(Optional<T>(v));
   subtest_tag_none_reversibility_with_optional(Optional<T>());
   subtest_tag_none_equivalence_with_optional<T>();
}

TEST(OptionalIoTest, test1)
{
   test(1,2);
   test(std::string("hello"), std::string("buffer"));
}

