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
// Created by softboy on 2017/12/18.

#include "gtest/gtest.h"
#include <list>
#include <utility>
#include <tuple>
#include <cstdlib>
#include <cmath>

#include "pdk/kernel/StringUtils.h"
#include "pdk/base/ds/ByteArray.h"

using pdk::ds::ByteArray;

TEST(StringUtilsTest, testStrLen)
{
   const char *src = "Something about ... \0 a string.";
   ASSERT_EQ(pdk::strlen(static_cast<char *>(0)), static_cast<uint>(0));
   ASSERT_EQ(pdk::strlen(src), static_cast<uint>(20));
}

TEST(StringUtilsTest, testStrNLen)
{
   const char *src = "Something about ... \0 a string.";
   ASSERT_EQ(pdk::strnlen(static_cast<char *>(0), 1), static_cast<uint>(0));
   ASSERT_EQ(pdk::strnlen(src, 31), static_cast<uint>(20));
   ASSERT_EQ(pdk::strnlen(src, 19), static_cast<uint>(19));
   ASSERT_EQ(pdk::strnlen(src, 21), static_cast<uint>(20));
   ASSERT_EQ(pdk::strnlen(src, 20), static_cast<uint>(20));
}

TEST(StringUtilsTest, testStrCopy)
{
   const char *src = "Something about ... \0 a string.";
   const char *expected = "Something about ... ";
   char dest[128];
   ASSERT_STREQ(pdk::strcopy(0, 0), static_cast<char *>(0));
   ASSERT_STREQ(pdk::strcopy(dest, 0), static_cast<char *>(0));
   ASSERT_STREQ(pdk::strcopy(dest, src), static_cast<char *>(dest));
   ASSERT_STREQ(static_cast<char *>(dest), const_cast<char *>(expected));
}

TEST(StringUtilsTest, testStrNCopy)
{
   ByteArray src(1024, 'a');
   ByteArray dest(1024, 'b');
   ASSERT_STREQ(pdk::strncopy(0, src.getRawData(), 0), static_cast<char *>(0));
   ASSERT_STREQ(pdk::strncopy(0, src.getRawData(), 10), static_cast<char *>(0));
   
   ASSERT_STREQ(pdk::strncopy(dest.getRawData(), 0, 0), static_cast<char *>(0));
   ASSERT_STREQ(pdk::strncopy(dest.getRawData(), 0, 10), static_cast<char *>(0));
   
   ASSERT_STREQ(pdk::strncopy(dest.getRawData(), src.getRawData(), 0), dest.getRawData());
   ASSERT_EQ(*dest.getRawData(), 'b');
   
   ASSERT_STREQ(pdk::strncopy(dest.getRawData(), src.getRawData(), src.size()), dest.getRawData());
   ASSERT_EQ(*dest.getRawData(), 'a');
   src = ByteArray("Tumdelidum\0foo");
}

namespace 
{
std::string to_lower_string(const std::string &str)
{
   std::string ret;
   for(auto c : str) {
      ret.push_back(std::tolower(c));
   }
   return ret;
}

std::string to_upper_string(const std::string &str)
{
   std::string ret;
   for(auto c : str) {
      ret.push_back(std::toupper(c));
   }
   return ret;
}

}

TEST(StringUtilsTest, testStriCmp)
{
   using DataType = std::list<std::tuple<std::string, std::string>>;
   DataType data;
   data.push_back(std::make_tuple("abcEdb", "abcEdb"));
   data.push_back(std::make_tuple("abcEdb", "ABCeDB"));
   data.push_back(std::make_tuple("ABCEDB", "abcedb"));
   data.push_back(std::make_tuple("abcdef", "abcdefg"));
   data.push_back(std::make_tuple("abcdeF", "abcdef"));
   data.push_back(std::make_tuple("abcdef", "abcdeF"));
   data.push_back(std::make_tuple("abcdefg", "abcdef"));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      std::string str1 = std::get<0>(item);
      std::string str2 = std::get<1>(item);
      int expected = std::strcmp(to_upper_string(str1).c_str(), to_upper_string(str2).c_str());
      if (expected != 0) {
         expected = (expected < 0 ? -1 : 1);
      }
      int actual = pdk::stricmp(str1.c_str(), str2.c_str());
      if (actual != 0) {
         actual = (actual < 0 ? -1 : 1);
      }
      ASSERT_EQ(expected, actual);
      ++begin;
   }
}
