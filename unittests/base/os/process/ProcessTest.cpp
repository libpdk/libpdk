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
// Created by softboy on 2018/05/11.

#include <cstdio>
#include "gtest/gtest.h"
#include "pdk/base/os/process/Process.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/TemporaryDir.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/io/fs/TemporaryFile.h"
#include "pdk/base/text/RegularExpression.h"
#include "pdk/base/io/Debug.h"

using pdk::os::process::Process;
using pdk::io::fs::TemporaryDir;
using pdk::lang::Latin1String;
using pdk::lang::String;
using pdk::io::fs::FileInfo;
using pdk::io::fs::Dir;

using ProcessFinishedSignal1 = void (Process::*)(int);
using ProcessFinishedSignal2 = void (Process::*)(int, Process::ExitStatus);
using ProcessErrorSignal = void (Process::*)(Process::ProcessError);


class ProcessTest : public ::testing::Test
{
public:
   static void SetUpTestCase()
   {
      ASSERT_TRUE(m_temporaryDir.isValid()) << pdk_printable(m_temporaryDir.getErrorString());
      // chdir to our testdata path and execute helper apps relative to that.
      String testdataDir = FileInfo(Latin1String(PDKTEST_CURRENT_TEST_DIR)).getAbsolutePath();
      ASSERT_TRUE(Dir::setCurrent(testdataDir)) << pdk_printable(Latin1String("Could not chdir to ") + testdataDir);
   }
   
   static void TearDownTestCase()
   {
      
   }
private:
   static pdk::pint64 m_bytesAvailable;
   static TemporaryDir m_temporaryDir;
};

TemporaryDir ProcessTest::m_temporaryDir;
pdk::pint64 ProcessTest::m_bytesAvailable = 0;

TEST_F(ProcessTest, testGetSetCheck)
{
   Process obj1;
   obj1.setReadChannelMode(Process::ProcessChannelMode::SeparateChannels);
   ASSERT_EQ(Process::ProcessChannelMode::SeparateChannels, obj1.getReadChannelMode());
}
