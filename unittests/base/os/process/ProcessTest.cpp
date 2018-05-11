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
#include "pdk/base/ds/StringList.h"
#include "pdk/base/ds/ByteArray.h"

using pdk::os::process::Process;
using pdk::io::fs::TemporaryDir;
using pdk::lang::Latin1String;
using pdk::lang::String;
using pdk::io::fs::FileInfo;
using pdk::io::fs::Dir;
using pdk::ds::StringList;
using pdk::ds::ByteArray;
using pdk::io::IoDevice;

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
   // ProcessChannelMode Process::readChannelMode()
   // void Process::setReadChannelMode(ProcessChannelMode)
   obj1.setReadChannelMode(Process::ProcessChannelMode::SeparateChannels);
   ASSERT_EQ(Process::ProcessChannelMode::SeparateChannels, obj1.getReadChannelMode());
   obj1.setReadChannelMode(Process::ProcessChannelMode::MergedChannels);
   ASSERT_EQ(Process::ProcessChannelMode::MergedChannels, obj1.getReadChannelMode());
   obj1.setReadChannelMode(Process::ProcessChannelMode::ForwardedChannels);
   ASSERT_EQ(Process::ProcessChannelMode::ForwardedChannels, obj1.getReadChannelMode());
   
   // ProcessChannel Process::readChannel()
   // void Process::setReadChannel(ProcessChannel)
   obj1.setReadChannel(Process::ProcessChannel::StandardOutput);
   ASSERT_EQ(Process::ProcessChannel::StandardOutput, obj1.getReadChannel());
   obj1.setReadChannel(Process::ProcessChannel::StandardError);
   ASSERT_EQ(Process::ProcessChannel::StandardError, obj1.getReadChannel());
}

TEST_F(ProcessTest, testConstructing)
{
   Process process;
   ASSERT_EQ(process.getReadChannel(), Process::ProcessChannel::StandardOutput);
   ASSERT_EQ(process.getWorkingDirectory(), String());
   ASSERT_EQ(process.getProcessEnvironment().toStringList(), StringList());
   ASSERT_EQ(process.getError(), Process::ProcessError::UnknownError);
   ASSERT_EQ(process.getState(), Process::ProcessState::NotRunning);
   ASSERT_EQ(process.getProcessId(), PDK_PID(0));
   ASSERT_EQ(process.readAllStandardOutput(), ByteArray());
   ASSERT_EQ(process.readAllStandardError(), ByteArray());
   ASSERT_EQ(process.canReadLine(), false);
   
   // IoDevice
   ASSERT_EQ(process.getOpenMode(), IoDevice::OpenMode::NotOpen);
   ASSERT_TRUE(!process.isOpen());
   ASSERT_TRUE(!process.isReadable());
   ASSERT_TRUE(!process.isWritable());
   ASSERT_TRUE(process.isSequential());
   ASSERT_EQ(process.getPosition(), pdk::plonglong(0));
   ASSERT_EQ(process.getSize(), pdk::plonglong(0));
   ASSERT_TRUE(process.atEnd());
   ASSERT_EQ(process.getBytesAvailable(), pdk::plonglong(0));
   ASSERT_EQ(process.getBytesToWrite(), pdk::plonglong(0));
   ASSERT_TRUE(!process.getErrorString().isEmpty());
   
   char c;
   ASSERT_EQ(process.read(&c, 1), pdk::plonglong(-1));
   ASSERT_EQ(process.write(&c, 1), pdk::plonglong(-1));
}
