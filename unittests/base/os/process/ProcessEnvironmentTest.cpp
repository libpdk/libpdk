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
// Created by softboy on 2018/05/17.

#include "gtest/gtest.h"
#include "pdktest/PdkTest.h"
#include "pdk/kernel/Object.h"
#include "pdk/base/os/process/Process.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/ds/ByteArray.h"

using pdk::os::process::ProcessEnvironment;
using pdk::lang::Latin1String;
using pdk::lang::String;
using pdk::ds::StringList;
using pdk::ds::ByteArray;

TEST(ProcessEnvironmentTest, testOperatorEq)
{
   ProcessEnvironment env1;
   ASSERT_EQ(env1, env1);
   env1.clear();
   ASSERT_EQ(env1, env1);
   env1 = ProcessEnvironment();
   ProcessEnvironment env2;
   ASSERT_EQ(env1, env2);
   env1.clear();
   ASSERT_EQ(env1, env2);
   env2.clear();
   ASSERT_EQ(env1, env2);
   env1.insert(Latin1String("FOO"), Latin1String("bar"));
   ASSERT_TRUE(env1 != env2);
   env2.insert(Latin1String("FOO"), Latin1String("bar"));
   ASSERT_TRUE(env1 == env2);
   env2.insert(Latin1String("FOO"), Latin1String("baz"));
   ASSERT_TRUE(env1 != env2);
}

TEST(ProcessEnvironmentTest, testClearAndIsEmpty)
{
   ProcessEnvironment env;
   env.insert(Latin1String("FOO"), Latin1String("bar"));
   ASSERT_TRUE(!env.isEmpty());
   env.clear();
   ASSERT_TRUE(env.isEmpty());
}

TEST(ProcessEnvironmentTest, testInsert)
{
   ProcessEnvironment env;
   env.insert(Latin1String("FOO"), Latin1String("bar"));
   ASSERT_TRUE(!env.isEmpty());
   ASSERT_TRUE(env.contains(Latin1String("FOO")));
   ASSERT_EQ(env.getValue(Latin1String("FOO")), String(Latin1String("bar")));
   
   env.remove(Latin1String("FOO"));
   ASSERT_TRUE(!env.contains(Latin1String("FOO")));
   ASSERT_TRUE(env.getValue(Latin1String("FOO")).isNull());
   
   env.clear();
   ASSERT_TRUE(!env.contains(Latin1String("FOO")));
}

TEST(ProcessEnvironmentTest, testEmptyNull)
{
   ProcessEnvironment env;
   env.insert(Latin1String("FOO"), Latin1String(""));
   ASSERT_TRUE(env.contains(Latin1String("FOO")));
   ASSERT_TRUE(env.getValue(Latin1String("FOO")).isEmpty());
   ASSERT_TRUE(!env.getValue(Latin1String("FOO")).isNull());
   
   env.insert(Latin1String("FOO"), String());
   ASSERT_TRUE(env.contains(Latin1String("FOO")));
   ASSERT_TRUE(env.getValue(Latin1String("FOO")).isEmpty());
   // don't test if it's NULL, since we shall not make a guarantee
   
   env.remove(Latin1String("FOO"));
   ASSERT_TRUE(!env.contains(Latin1String("FOO")));
}

TEST(ProcessEnvironmentTest, testToStringList)
{
   ProcessEnvironment env;
   ASSERT_TRUE(env.isEmpty());
   ASSERT_TRUE(env.toStringList().empty());
   
   env.insert(Latin1String("FOO"), Latin1String("bar"));
   StringList result = env.toStringList();
   ASSERT_TRUE(!result.empty());
   ASSERT_EQ(result.size(), 1u);
   ASSERT_EQ(result.at(0), String(Latin1String("FOO=bar")));
   
   env.clear();
   env.insert(Latin1String("BAZ"), Latin1String(""));
   result = env.toStringList();
   ASSERT_EQ(result.at(0), String(Latin1String("BAZ=")));
   
   env.insert(Latin1String("FOO"), Latin1String("bar"));
   env.insert(Latin1String("A"), Latin1String("bc"));
   env.insert(Latin1String("HELLO"), Latin1String("World"));
   result = env.toStringList();
   ASSERT_EQ(result.size(), 4u);
   
   // order is not specified, so use contains()
   ASSERT_TRUE(result.contains(Latin1String("FOO=bar")));
   ASSERT_TRUE(result.contains(Latin1String("BAZ=")));
   ASSERT_TRUE(result.contains(Latin1String("A=bc")));
   ASSERT_TRUE(result.contains(Latin1String("HELLO=World")));
}

TEST(ProcessEnvironmentTest, testKeys)
{
   ProcessEnvironment env;
   ASSERT_TRUE(env.isEmpty());
   ASSERT_TRUE(env.getKeys().empty());
   
   env.insert(Latin1String("FOO"), Latin1String("bar"));
   StringList result = env.getKeys();
   ASSERT_EQ(result.size(), 1u);
   ASSERT_EQ(result.at(0), String(Latin1String("FOO")));
   
   env.clear();
   env.insert(Latin1String("BAZ"), Latin1String(""));
   result = env.getKeys();
   ASSERT_EQ(result.at(0), String(Latin1String("BAZ")));
   
   env.insert(Latin1String("FOO"), Latin1String("bar"));
   env.insert(Latin1String("A"), Latin1String("bc"));
   env.insert(Latin1String("HELLO"), Latin1String("World"));
   result = env.getKeys();
   ASSERT_EQ(result.size(), 4u);
   
   // order is not specified, so use contains()
   ASSERT_TRUE(result.contains(Latin1String("FOO")));
   ASSERT_TRUE(result.contains(Latin1String("BAZ")));
   ASSERT_TRUE(result.contains(Latin1String("A")));
   ASSERT_TRUE(result.contains(Latin1String("HELLO")));
}

TEST(ProcessEnvironmentTest, testInsertEnv)
{
   ProcessEnvironment env;
   env.insert(Latin1String("FOO"), Latin1String("bar"));
   env.insert(Latin1String("A"), Latin1String("bc"));
   env.insert(Latin1String("Hello"), Latin1String("World"));
   
   ProcessEnvironment env2;
   env2.insert(Latin1String("FOO2"), Latin1String("bar2"));
   env2.insert(Latin1String("A2"), Latin1String("bc2"));
   env2.insert(Latin1String("Hello"), Latin1String("Another World"));
   
   env.insert(env2);
   StringList keys = env.getKeys();
   ASSERT_EQ(keys.size(), 5u);
   
   ASSERT_EQ(env.getValue(Latin1String("FOO")), String(Latin1String("bar")));
   ASSERT_EQ(env.getValue(Latin1String("A")), String(Latin1String("bc")));
   ASSERT_EQ(env.getValue(Latin1String("Hello")), String(Latin1String("Another World")));
   ASSERT_EQ(env.getValue(Latin1String("FOO2")), String(Latin1String("bar2")));
   ASSERT_EQ(env.getValue(Latin1String("A2")), String(Latin1String("bc2")));
   
   ProcessEnvironment env3;
   env3.insert(Latin1String("FOO2"), Latin1String("bar2"));
   env3.insert(Latin1String("A2"), Latin1String("bc2"));
   env3.insert(Latin1String("Hello"), Latin1String("Another World"));
   
   env3.insert(env3); // mustn't deadlock
   
   ASSERT_EQ(env3, env2);
}

TEST(ProcessEnvironmentTest, testCaseSensitivity)
{
   ProcessEnvironment env;
   env.insert(Latin1String("foo"), Latin1String("bar"));
   
#ifdef PDK_OS_WIN
   // Windows is case-insensitive, but case-preserving
   ASSERT_TRUE(env.contains(Latin1String("foo")));
   ASSERT_TRUE(env.contains(Latin1String("FOO")));
   ASSERT_TRUE(env.contains(Latin1String("FoO")));
   
   ASSERT_EQ(env.value(Latin1String("foo")), String(Latin1String("bar")));
   ASSERT_EQ(env.value(Latin1String("FOO")), String(Latin1String("bar")));
   ASSERT_EQ(env.value(Latin1String("FoO")), String(Latin1String("bar")));
   
   // Per Windows, this overwrites the value, but keeps the name's original capitalization
   env.insert(Latin1String("Foo"), Latin1String("Bar"));
   
   StringList list = env.toStringList();
   ASSERT_EQ(list.length(), 1);
   ASSERT_EQ(list.at(0), String(Latin1String("foo=Bar")));
#else
   // otherwise, it's case sensitive
   ASSERT_TRUE(env.contains(Latin1String("foo")));
   ASSERT_TRUE(!env.contains(Latin1String("FOO")));
   
   env.insert(Latin1String("FOO"), Latin1String("baz"));
   ASSERT_TRUE(env.contains(Latin1String("FOO")));
   ASSERT_EQ(env.getValue(Latin1String("FOO")), String(Latin1String("baz")));
   ASSERT_EQ(env.getValue(Latin1String("foo")), String(Latin1String("bar")));
   
   StringList list = env.toStringList();
   ASSERT_EQ(list.size(), 2u);
   ASSERT_TRUE(list.contains(Latin1String("foo=bar")));
   ASSERT_TRUE(list.contains(Latin1String("FOO=baz")));
#endif
}

TEST(ProcessEnvironmentTest, testSystemEnvironment)
{
   static const char envname[] = "THIS_ENVIRONMENT_VARIABLE_HOPEFULLY_DOESNT_EXIST";
   ByteArray path = pdk::get_env("PATH");
   ByteArray nonexistant = pdk::get_env(envname);
   ProcessEnvironment system = ProcessEnvironment::getSystemEnvironment();
   
   ASSERT_TRUE(nonexistant.isNull());
   
   // all other system have environments
   if (path.isEmpty()) {
      FAIL() << "Could not find the PATH environment variable -- please correct the test environment";
   }
   ASSERT_TRUE(system.contains(Latin1String("PATH")));
   ASSERT_EQ(system.getValue(Latin1String("PATH")), String::fromLocal8Bit(path));
   
   ASSERT_TRUE(!system.contains(Latin1String(envname)));
   
#ifdef PDK_OS_WIN
   // check case-insensitive too
   ASSERT_TRUE(system.contains(Latin1String("path")));
   ASSERT_EQ(system.getValue(Latin1String("path")), String::fromLocal8Bit(path));
   
   ASSERT_TRUE(!system.contains(String(envname).toLower()));
#endif
}

TEST(ProcessEnvironmentTest, testPutEnv)
{
   static const char envname[] = "WE_RE_SETTING_THIS_ENVIRONMENT_VARIABLE";
   static bool testRan = false;
   
   if (testRan) {
      FAIL() << "You cannot run this test more than once, since we modify the environment";
   }
   testRan = true;
   
   ByteArray valBefore = pdk::get_env(envname);
   if (!valBefore.isNull()) {
      FAIL() << "The environment variable we set in the environment is already set! -- please correct the test environment";
   }      
   ProcessEnvironment eBefore = ProcessEnvironment::getSystemEnvironment();
   pdk::put_env(envname, "Hello, World");
   ByteArray valAfter = pdk::get_env(envname);
   ASSERT_EQ(valAfter, ByteArray("Hello, World"));
   
   ProcessEnvironment eAfter = ProcessEnvironment::getSystemEnvironment();
   
   ASSERT_TRUE(!eBefore.contains(Latin1String(envname)));
   ASSERT_TRUE(eAfter.contains(Latin1String(envname)));
   ASSERT_EQ(eAfter.getValue(Latin1String(envname)), String(Latin1String("Hello, World")));
   
# ifdef PDK_OS_WIN
   // check case-insensitive too
   String lower = envname;
   lower = lower.toLower();
   ASSERT_TRUE(!eBefore.contains(lower));
   ASSERT_TRUE(eAfter.contains(lower));
   ASSERT_EQ(eAfter.value(lower), String(Latin1String("Hello, World")));
# endif
}
