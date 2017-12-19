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
// Created by zzu_softboy on 2017/12/19.


#include "gtest/gtest.h"
#include <list>
#include <utility>
#include <tuple>
#include <vector>
#include <algorithm>

#include "pdk/base/ds/internal/ByteArrayMatcher.h"
#include "pdk/base/ds/ByteArray.h"

using pdk::ds::internal::ByteArrayMatcher;
using pdk::ds::ByteArray;

TEST(ByteArrayMatcherTest, testInterface)
{
   static ByteArrayMatcher matcher1;
   const char needle[] = "abc123";
   ByteArray haystack(500, 'a');
   haystack.insert(6, "123");
   haystack.insert(31, "abc");
   haystack.insert(42, "abc123");
   haystack.insert(84, "abc123");
   matcher1 = ByteArrayMatcher(ByteArray(needle));
   ByteArrayMatcher matcher2;
   matcher2.setPattern(ByteArray(needle));
   ByteArrayMatcher matcher3 = ByteArrayMatcher(ByteArray(needle));
   ByteArrayMatcher matcher4(needle, sizeof(needle) - 1);
   ByteArrayMatcher matcher5(matcher2);
   ByteArrayMatcher matcher6;
   matcher6 = matcher3;
   
   ASSERT_EQ(matcher1.findIndex(haystack), 42);
   ASSERT_EQ(matcher2.findIndex(haystack), 42);
   ASSERT_EQ(matcher3.findIndex(haystack), 42);
   ASSERT_EQ(matcher4.findIndex(haystack), 42);
   ASSERT_EQ(matcher5.findIndex(haystack), 42);
   ASSERT_EQ(matcher6.findIndex(haystack), 42);
   
   ASSERT_EQ(matcher1.findIndex(haystack.getConstRawData(), haystack.size()), 42);
   
   ASSERT_EQ(matcher1.findIndex(haystack, 43), 84);
   ASSERT_EQ(matcher1.findIndex(haystack.getConstRawData(), haystack.size(), 43), 84);
   ASSERT_EQ(matcher1.findIndex(haystack, 85), -1);
   ASSERT_EQ(matcher1.findIndex(haystack.getConstRawData(), haystack.size(), 85), -1);
   
   ByteArrayMatcher matcher7(ByteArray("123"));
   ASSERT_EQ(matcher7.findIndex(haystack), 6);
   
   matcher7 = ByteArrayMatcher(ByteArray("abc"));
   ASSERT_EQ(matcher7.findIndex(haystack), 31);
   
   matcher7.setPattern(matcher4.getPattern());
   ASSERT_EQ(matcher7.findIndex(haystack), 42);
}

TEST(ByteArrayMatcherTest, testFindIndex)
{
   static ByteArrayMatcher matcher;
   const char pdata[] = { 0x0, 0x0, 0x1 };
   ByteArray pattern(pdata, sizeof(pdata));
   
   ByteArray haystack(8, '\0');
   haystack[7] = 0x1;
   
   matcher = ByteArrayMatcher(pattern);
   ASSERT_EQ(matcher.findIndex(haystack, 0), 5);
   ASSERT_EQ(matcher.findIndex(haystack, 1), 5);
   ASSERT_EQ(matcher.findIndex(haystack, 2), 5);
   
   matcher.setPattern(pattern);
   ASSERT_EQ(matcher.findIndex(haystack, 0), 5);
   ASSERT_EQ(matcher.findIndex(haystack, 1), 5);
   ASSERT_EQ(matcher.findIndex(haystack, 2), 5);
}
