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

#include "gtest/gtest.h"
#include "pdk/stdext/optional/Optional.h"

using pdk::stdext::optional::Optional;

class NonConstructible
{
private:
   NonConstructible();
   NonConstructible(NonConstructible const&);
   NonConstructible(NonConstructible &&);
};

TEST(MinimumRequirementTest, testNonConstructible)
{
   Optional<NonConstructible> o;
   ASSERT_TRUE(!o);
   ASSERT_TRUE(o == pdk::stdext::none);
   ASSERT_THROW(o.value(), pdk::stdext::optional::BadAccess);
}

class Guard
{
public:
   explicit Guard(int) {}
private:
   Guard();
   Guard(Guard const&);
   Guard(Guard &&); 
};

TEST(MinimumRequirementTest, testGuard)
{
   Optional<Guard> o;
   o.emplace(1);
   ASSERT_TRUE(o);
   ASSERT_TRUE(o != pdk::stdext::none);
}

TEST(MinimumRequirementTest, testNonAssignable)
{
   Optional<const std::string> o;
   o.emplace("cat");
   ASSERT_TRUE(o);
   ASSERT_TRUE(o != pdk::stdext::none);
   ASSERT_EQ(*o, std::string("cat"));
}
