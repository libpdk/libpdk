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
#include "pdk/utils/ScopedPointer.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdktest/PdkTest.h"
#include "pdk/base/time/Time.h"
#include "pdk/stdext/utility/Algorithms.h"

#include <vector>

#define PDKTEST_DIR_SEP "/"
#define APP_FILENAME(name) Latin1String(PDKTEST_PROCESS_APPS_DIR PDKTEST_DIR_SEP PDK_STRINGIFY(name)) 

using pdk::os::process::Process;
using pdk::io::fs::TemporaryDir;
using pdk::io::fs::FileInfo;
using pdk::io::fs::Dir;
using pdk::io::fs::File;
using pdk::io::IoDevice;
using pdk::text::RegularExpression;
using pdk::lang::Latin1String;
using pdk::lang::String;
using pdk::ds::StringList;
using pdk::ds::ByteArray;
using pdk::utils::ScopedPointer;
using pdk::kernel::Object;
using pdk::os::thread::Thread;
using pdk::time::Time;
using pdk::lang::Latin1Character;
using pdk::kernel::EventLoop;
using pdk::os::process::ProcessEnvironment;

using ProcessFinishedSignal1 = void (Process::*)(int);
using ProcessFinishedSignal2 = void (Process::*)(int, Process::ExitStatus);
using ProcessErrorSignal = void (Process::*)(Process::ProcessError);

int sg_argc;
char **sg_argv;

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
   {}
   
public:
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

TEST_F(ProcessTest, testSimpleStart)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   ScopedPointer<Process> process(new Process);
   process->connectReadyReadSignal([](IoDevice::SignalType, Object *sender){
      Process *process = dynamic_cast<Process *>(sender);
      ASSERT_TRUE(process);
      int lines = 0;
      while (process->canReadLine()) {
         ++lines;
         process->readLine();
      }
   }, process.getData());
   std::list<Process::ProcessState> stateChangedData;
   process->connectStateChangedSignal([&stateChangedData](Process::ProcessState state){
      stateChangedData.push_back(state);
   }, PDK_RETRIEVE_APP_INSTANCE());
   process->start(APP_FILENAME(ProcessNormalApp));

   if(process->getState() != Process::ProcessState::Starting) {
      ASSERT_EQ(process->getState(), Process::ProcessState::Running);
   }

   ASSERT_TRUE(process->waitForStarted(5000)) << pdk_printable(process->getErrorString());
   ASSERT_EQ(process->getState(), Process::ProcessState::Running);
   PDK_TRY_COMPARE(process->getState(), Process::ProcessState::NotRunning);
   process.reset();
   ASSERT_EQ(stateChangedData.size(), 3u);
   auto iter = stateChangedData.begin();
   ASSERT_EQ(*iter++, Process::ProcessState::Starting);
   ASSERT_EQ(*iter++, Process::ProcessState::Running);
   ASSERT_EQ(*iter++, Process::ProcessState::NotRunning);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testStartWithOpen)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   ASSERT_EQ(process.open(IoDevice::OpenMode::ReadOnly), false);
   process.setProgram(APP_FILENAME(ProcessNormalApp));
   ASSERT_EQ(process.getProgram(), APP_FILENAME(ProcessNormalApp));
   process.setArguments(StringList() << Latin1String("arg1") << Latin1String("arg2"));
   ASSERT_EQ(process.getArguments().size(), 2u);
   ASSERT_TRUE(process.open(IoDevice::OpenMode::ReadOnly));
   ASSERT_EQ(process.getOpenMode(), IoDevice::OpenMode::ReadOnly);
   ASSERT_TRUE(process.waitForFinished(5000));
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testStartWithOldOpen)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   class OverriddenOpen : public Process
   {
   public:
      virtual bool open(OpenModes mode) override
      { return IoDevice::open(mode); }
   };
   OverriddenOpen process;
   process.start(APP_FILENAME(ProcessNormalApp));
   ASSERT_TRUE(process.waitForStarted(5000));
   ASSERT_TRUE(process.waitForFinished(5000));
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testExecute)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   ASSERT_EQ(Process::execute(APP_FILENAME(ProcessNormalApp),
                              StringList() << Latin1String("arg1") << Latin1String("arg2")), 0);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testStartDetached)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   ASSERT_TRUE(Process::startDetached(APP_FILENAME(ProcessNormalApp),
                                      StringList() << Latin1String("arg1") << Latin1String("arg2")));
   ASSERT_EQ(Process::startDetached(Latin1String("nonexistingexe")), false);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testCrashTest)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   ScopedPointer<Process> process(new Process);

   std::list<Process::ProcessState> stateChangedData;
   process->connectStateChangedSignal([&stateChangedData](Process::ProcessState state){
      stateChangedData.push_back(state);
   }, PDK_RETRIEVE_APP_INSTANCE());

   process->start(APP_FILENAME(ProcessCrashApp));

   ASSERT_TRUE(process->waitForStarted(5000));

   std::list<Process::ProcessError> errorData;
   process->connectErrorOccurredSignal([&errorData](Process::ProcessError error){
      errorData.push_back(error);
   }, PDK_RETRIEVE_APP_INSTANCE());

   std::list<Process::ExitStatus> exitStatusData;
   process->connectFinishedSignal([&exitStatusData](int exitCode, Process::ExitStatus status){
      exitStatusData.push_back(status);
   }, PDK_RETRIEVE_APP_INSTANCE());

   ASSERT_TRUE(process->waitForFinished(30000));
   ASSERT_EQ(errorData.size(), 1u);
   ASSERT_EQ(*errorData.begin(), Process::ProcessError::Crashed);
   ASSERT_EQ(exitStatusData.size(), 1u);
   ASSERT_EQ(*exitStatusData.begin(), Process::ExitStatus::CrashExit);
   process.reset();

   ASSERT_EQ(stateChangedData.size(), 3u);
   auto iter = stateChangedData.begin();
   ASSERT_EQ(*iter++, Process::ProcessState::Starting);
   ASSERT_EQ(*iter++, Process::ProcessState::Running);
   ASSERT_EQ(*iter++, Process::ProcessState::NotRunning);

   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testCrashTest2)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.start(APP_FILENAME(ProcessCrashApp));
   ASSERT_TRUE(process.waitForStarted(5000));

   std::list<Process::ProcessError> errorData;
   process.connectErrorOccurredSignal([&errorData](Process::ProcessError error){
      errorData.push_back(error);
   }, PDK_RETRIEVE_APP_INSTANCE());

   std::list<Process::ExitStatus> exitStatusData;
   process.connectFinishedSignal([&exitStatusData](int exitCode, Process::ExitStatus status){
      exitStatusData.push_back(status);
   }, PDK_RETRIEVE_APP_INSTANCE());

   process.connectFinishedSignal([](int exitCode, Process::ExitStatus status){
      pdktest::TestEventLoop::instance().exitLoop();
   }, PDK_RETRIEVE_APP_INSTANCE());

   pdktest::TestEventLoop::instance().enterLoop(30);
   if (pdktest::TestEventLoop::instance().getTimeout()) {
      FAIL() << "Failed to detect crash : operation timed out";
   }

   ASSERT_EQ(errorData.size(), 1u);
   ASSERT_EQ(*errorData.begin(), Process::ProcessError::Crashed);
   ASSERT_EQ(exitStatusData.size(), 1u);
   ASSERT_EQ(*exitStatusData.begin(), Process::ExitStatus::CrashExit);

   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::CrashExit);

   PDKTEST_END_APP_CONTEXT();
}

namespace {

void init_echotest_data(std::list<ByteArray> &data)
{
   data.push_back(ByteArray("H"));
   data.push_back(ByteArray("He"));
   data.push_back(ByteArray("Hel"));
   data.push_back(ByteArray("Hell"));
   data.push_back(ByteArray("Hello"));

   data.push_back(ByteArray(100, '@'));
   data.push_back(ByteArray(1000, '@'));
   data.push_back(ByteArray(10000, '@'));
}

} // anonymous namespace

TEST_F(ProcessTest, testEcho)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   std::list<ByteArray> data;
   init_echotest_data(data);
   for (ByteArray &input : data) {
      Process process;
      process.connectReadyReadSignal([](){
         pdktest::TestEventLoop::instance().exitLoop();
      });
      process.start(APP_FILENAME(ProcessEchoApp));
      ASSERT_TRUE(process.waitForStarted(5000));
      process.write(input);
      Time stopWatch;
      stopWatch.start();
      do {
         ASSERT_TRUE(process.isOpen());
         pdktest::TestEventLoop::instance().enterLoop(2);
      } while (stopWatch.elapsed() < 60000 && process.getBytesAvailable() < input.size());
      if (stopWatch.elapsed() >= 60000) {
         FAIL() << "Timed out";
      }
      ByteArray message = process.readAll();
      ASSERT_EQ(message.size(), input.size());
      char *c1 = message.getRawData();
      char *c2 = input.getRawData();
      while (*c1 && *c2) {
         if (*c1 != *c2) {
            ASSERT_EQ(*c1, *c2);
         }
         ++c1;
         ++c2;
      }
      ASSERT_EQ(*c1, *c2);
      process.write("", 1);
      ASSERT_TRUE(process.waitForFinished(5000));
      ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
      ASSERT_EQ(process.getExitCode(), 0);
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testEcho2)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.connectReadyReadSignal([](){
      pdktest::TestEventLoop::instance().exitLoop();
   });
   process.start(APP_FILENAME(ProcessEcho2App));
   ASSERT_TRUE(process.waitForStarted(5000));
   ASSERT_TRUE(!process.waitForReadyRead(250));
   ASSERT_EQ(process.getError(), Process::ProcessError::Timedout);
   process.write("Hello");

   int channelReadyReadCount = 0;
   process.connectChannelReadyReadSignal([&channelReadyReadCount](int count) {
      ++channelReadyReadCount;
   });

   int ReadyReadStandardOutputCount = 0;
   process.connectReadyReadStandardOutputSignal([&ReadyReadStandardOutputCount]() {
      ++ReadyReadStandardOutputCount;
   });

   int ReadyReadStandardErrorCount = 0;
   process.connectReadyReadStandardErrorSignal([&ReadyReadStandardErrorCount]() {
      ++ReadyReadStandardErrorCount;
   });

   Time stopWatch;
   stopWatch.start();
   while(true) {
      pdktest::TestEventLoop::instance().enterLoop(1);
      if (stopWatch.elapsed() >= 30000) {
         FAIL() << "Timed out";
      }

      process.setReadChannel(Process::ProcessChannel::StandardOutput);
      pdk::pint64 baso = process.getBytesAvailable();

      process.setReadChannel(Process::ProcessChannel::StandardError);
      pdk::pint64 base = process.getBytesAvailable();
      if (baso == 5 && base == 5) {
         break;
      }
   }
   ASSERT_TRUE(channelReadyReadCount > 0);
   ASSERT_TRUE(ReadyReadStandardOutputCount > 0);
   ASSERT_TRUE(ReadyReadStandardErrorCount > 0);

   ASSERT_EQ(process.readAllStandardOutput(), ByteArray("Hello"));
   ASSERT_EQ(process.readAllStandardError(), ByteArray("Hello"));

   process.write("", 1);

   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   PDKTEST_END_APP_CONTEXT();
}

// @TODO test setNamedPipeHandleState
// @TODO test batFiles

namespace {

using ExitStatusDataType = std::list<std::tuple<StringList, std::vector<Process::ExitStatus>>>;
void init_exit_status_data(ExitStatusDataType &data)
{
   {
      StringList strList;
      strList << APP_FILENAME(ProcessNormalApp);
      std::vector<Process::ExitStatus> exitStatusData;
      exitStatusData.push_back(Process::ExitStatus::NormalExit);
      data.push_back(std::make_tuple(strList, exitStatusData));
   }

   {
      StringList strList;
      strList << APP_FILENAME(ProcessCrashApp);
      std::vector<Process::ExitStatus> exitStatusData;
      exitStatusData.push_back(Process::ExitStatus::CrashExit);
      data.push_back(std::make_tuple(strList, exitStatusData));
   }

   {
      StringList strList;
      strList << APP_FILENAME(ProcessNormalApp)
              << APP_FILENAME(ProcessCrashApp);
      std::vector<Process::ExitStatus> exitStatusData;
      exitStatusData.push_back(Process::ExitStatus::NormalExit);
      exitStatusData.push_back(Process::ExitStatus::CrashExit);
      data.push_back(std::make_tuple(strList, exitStatusData));
   }

   {
      StringList strList;
      strList << APP_FILENAME(ProcessCrashApp)
              << APP_FILENAME(ProcessNormalApp);
      std::vector<Process::ExitStatus> exitStatusData;
      exitStatusData.push_back(Process::ExitStatus::CrashExit);
      exitStatusData.push_back(Process::ExitStatus::NormalExit);
      data.push_back(std::make_tuple(strList, exitStatusData));
   }
}

} // anonymous namespace

TEST_F(ProcessTest, testExitStatus)
{
   ExitStatusDataType data;
   init_exit_status_data(data);
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   for (auto &item : data) {
      StringList &processList = std::get<0>(item);
      std::vector<Process::ExitStatus> exitStatus = std::get<1>(item);
      ASSERT_EQ(exitStatus.size(), processList.size());
      for (size_t i = 0; i < processList.size(); ++i) {
         process.start(processList.at(i));
         ASSERT_TRUE(process.waitForStarted(5000));
         ASSERT_TRUE(process.waitForFinished(30000));

         ASSERT_EQ(process.getExitStatus(), exitStatus.at(i));
      }
   }

   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testLoopBack)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.start(APP_FILENAME(ProcessEchoApp));
   for (int i = 0; i < 100; ++i) {
      process.write("Hello");
      do {
         ASSERT_TRUE(process.waitForReadyRead(5000));
      } while (process.getBytesAvailable() < 5);
      ASSERT_EQ(process.readAll(), ByteArray("Hello"));
   }
   process.write("", 1);
   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);

   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testReadTimeoutAndThenCrash)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.start(APP_FILENAME(ProcessEchoApp));
   if (process.getState() != Process::ProcessState::Starting) {
      ASSERT_EQ(process.getState(), Process::ProcessState::Running);
   }

   std::list<Process::ProcessError> errorData;
   process.connectErrorOccurredSignal([&errorData](Process::ProcessError error){
      errorData.push_back(error);
   }, PDK_RETRIEVE_APP_INSTANCE());

   ASSERT_TRUE(process.waitForStarted(5000));
   ASSERT_EQ(process.getState(), Process::ProcessState::Running);

   ASSERT_TRUE(!process.waitForReadyRead(5000));
   ASSERT_EQ(process.getError(), Process::ProcessError::Timedout);

   process.kill();

   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getState(), Process::ProcessState::NotRunning);

   ASSERT_EQ(errorData.size(), 1u);
   ASSERT_EQ(*errorData.begin(), Process::ProcessError::Crashed);

   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testWaitForFinished)
{
   PDKTEST_BEGIN_APP_CONTEXT();

   Process process;
   process.start(APP_FILENAME(ProcessOutputApp));

   ASSERT_TRUE(process.waitForFinished());
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);

   String output = Latin1String(process.readAll());
   ASSERT_EQ(output.count(Latin1String("\n")), 10 * 1024);

   process.start(Latin1String("notexitloop"));

   ASSERT_TRUE(!process.waitForFinished());
   ASSERT_EQ(process.getError(), Process::ProcessError::FailedToStart);

   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testDeadWhileReading)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.start(APP_FILENAME(ProcessDeadWhileReadingApp));
   String output;
   ASSERT_TRUE(process.waitForStarted(5000));
   while (process.waitForReadyRead(5000)) {
      output += Latin1String(process.readAll());
   }
   ASSERT_EQ(output.count(Latin1String("\n")), 10 * 1024);
   process.waitForFinished();
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testRestartProcessDeadlock)
{
   PDKTEST_BEGIN_APP_CONTEXT();

   Process process;
   auto conn = process.connectFinishedSignal([](int exitCode, Process::ExitStatus status, Process::SignalType signal, Object *sender){
      Process *process = dynamic_cast<Process *>(sender);
      ASSERT_TRUE(process);
      process->start(APP_FILENAME(ProcessEchoApp));
   }, PDK_RETRIEVE_APP_INSTANCE());

   process.start(APP_FILENAME(ProcessEchoApp));
   ASSERT_EQ(process.write("", 1), pdk::plonglong(1));
   ASSERT_TRUE(process.waitForFinished(5000));
   process.disconnectFinishedSignal(conn);

   ASSERT_EQ(process.write("", 1), pdk::plonglong(1));
   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testCloseWriteChannel)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   ByteArray testData("Data to read");
   Process more;
   more.start(APP_FILENAME(ProcessEOFApp));

   ASSERT_TRUE(more.waitForStarted(5000));
   ASSERT_TRUE(!more.waitForReadyRead(250));
   ASSERT_EQ(more.getError(), Process::ProcessError::Timedout);

   ASSERT_EQ(more.write(testData), pdk::pint64(testData.size()));

   ASSERT_TRUE(!more.waitForReadyRead(250));
   ASSERT_EQ(more.getError(), Process::ProcessError::Timedout);

   more.closeWriteChannel();

   while (more.getBytesAvailable() < testData.size()) {
      ASSERT_TRUE(more.waitForReadyRead(5000));
   }

   ASSERT_EQ(more.readAll(), testData);

   if (more.getState() == Process::ProcessState::Running) {
      ASSERT_TRUE(more.waitForFinished(5000));
   }

   ASSERT_EQ(more.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(more.getExitCode(), 0);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testCloseReadChannel)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   for (int i = 0; i < 10; ++i) {
      Process::ProcessChannel channel1 = Process::ProcessChannel::StandardOutput;
      Process::ProcessChannel channel2 = Process::ProcessChannel::StandardError;
      Process process;
      process.start(APP_FILENAME(ProcessEcho2App));
      ASSERT_TRUE(process.waitForStarted(5000));
      process.closeReadChannel(i & 1 ? channel2 : channel1);
      process.setReadChannel(i & 1 ? channel2 : channel1);
      process.write("Data");
      ASSERT_TRUE(!process.waitForReadyRead(5000));
      ASSERT_TRUE(process.readAll().isEmpty());
      process.setReadChannel(i & 1 ? channel1 : channel2);

      while (process.getBytesAvailable() < 4 && process.waitForReadyRead(1000))
      {}

      ASSERT_EQ(process.readAll(), ByteArray("Data"));

      process.write("", 1);
      ASSERT_TRUE(process.waitForFinished(5000));
      ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
      ASSERT_EQ(process.getExitCode(), 0);
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testOpenModes)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   ASSERT_TRUE(!process.isOpen());
   ASSERT_EQ(process.getOpenMode(), Process::OpenMode::NotOpen);
   process.start(APP_FILENAME(ProcessEcho3App));
   ASSERT_TRUE(process.waitForStarted(5000));
   ASSERT_TRUE(process.isOpen());
   ASSERT_EQ(process.getOpenMode(), Process::OpenMode::ReadWrite);
   ASSERT_TRUE(process.isReadable());
   ASSERT_TRUE(process.isWritable());
   process.write("Data");
   process.closeWriteChannel();
   ASSERT_TRUE(process.isWritable());
   ASSERT_EQ(process.getOpenMode(), Process::OpenMode::ReadWrite);
   while (process.getBytesAvailable() < 4 && process.waitForReadyRead(5000))
   {}
   ASSERT_EQ(process.readAll(), ByteArray("Data"));

   process.closeReadChannel(Process::ProcessChannel::StandardOutput);

   ASSERT_EQ(process.getOpenMode(), Process::OpenMode::ReadWrite);
   ASSERT_TRUE(process.isReadable());

   process.closeReadChannel(Process::ProcessChannel::StandardError);

   ASSERT_EQ(process.getOpenMode(), Process::OpenMode::ReadWrite);
   ASSERT_TRUE(process.isReadable());

   process.close();
   ASSERT_TRUE(!process.isOpen());
   ASSERT_TRUE(!process.isReadable());
   ASSERT_TRUE(!process.isWritable());
   ASSERT_EQ(process.getState(), Process::ProcessState::NotRunning);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testEmitReadyReadOnlyWhenNewDataArrives)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   int readyReadCount = 0;
   auto conn = process.connectReadyReadSignal([&readyReadCount]() {
      ++readyReadCount;
      pdktest::TestEventLoop::instance().exitLoop();
   });
   process.start(APP_FILENAME(ProcessEchoApp));
   ASSERT_EQ(readyReadCount, 0);
   process.write("A");
   pdktest::TestEventLoop::instance().enterLoop(5);
   if (pdktest::TestEventLoop::instance().getTimeout()) {
      FAIL() << "Operation timed out";
   }
   ASSERT_EQ(readyReadCount, 1);

   pdktest::TestEventLoop::instance().enterLoop(1);
   ASSERT_TRUE(pdktest::TestEventLoop::instance().getTimeout());
   ASSERT_TRUE(!process.waitForReadyRead(250));

   process.disconnectReadyReadSignal(conn);

   process.write("B");
   ASSERT_TRUE(process.waitForReadyRead(5000));

   process.write("", 1);
   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testHardExit)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.start(APP_FILENAME(ProcessEchoApp));
   ASSERT_TRUE(process.waitForStarted()) << pdk_printable(process.getErrorString());
   process.kill();
   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getState(), Process::ProcessState::NotRunning);
   ASSERT_EQ(process.getError(), Process::ProcessError::Crashed);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testSoftExit)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   ASSERT_EQ(process.getProcessId(), 0);
   process.start(APP_FILENAME(SoftExitApp));
   ASSERT_TRUE(process.waitForStarted(10000));
   ASSERT_TRUE(process.waitForReadyRead(10000));
   ASSERT_TRUE(process.getProcessId() > 0);

   process.terminate();

   ASSERT_TRUE(process.waitForFinished(10000));
   ASSERT_EQ(process.getState(), Process::ProcessState::NotRunning);
   ASSERT_EQ(process.getError(), Process::ProcessError::UnknownError);
   PDKTEST_END_APP_CONTEXT();
}

namespace {

class SoftExitProcess : public Process
{
public:
   bool waitedForFinished;

   SoftExitProcess(int n) : waitedForFinished(false), n(n), killing(false)
   {

      this->connectFinishedSignal(this, &SoftExitProcess::finishedSlot);

      switch (n) {
      case 0:
         setReadChannelMode(Process::ProcessChannelMode::MergedChannels);
         this->connectReadyReadSignal(this, &SoftExitProcess::terminateSlot);
         break;
      case 1:
         this->connectReadyReadStandardOutputSignal(this, &SoftExitProcess::terminateSlot);
         break;
      case 2:
         this->connectReadyReadStandardErrorSignal(this, &SoftExitProcess::terminateSlot);
         break;
      case 3:
         this->connectStartedSignal(this, &SoftExitProcess::terminateSlot);
         break;
      case 4:
         setReadChannelMode(Process::ProcessChannelMode::MergedChannels);
         this->connectChannelReadyReadSignal(this, &SoftExitProcess::channelReadyReadSlot);
         break;
      default:
         this->connectStateChangedSignal(this, &SoftExitProcess::stateChangedSlot);
         break;
      }
   }

   void writeAfterStart(const char *buf, int count)
   {
      dataToWrite = ByteArray(buf, count);
   }

   void start(const String &program)
   {
      Process::start(program);
      writePendingData();
   }

public:
   void terminateSlot()
   {
      writePendingData(); // In cases 3 and 5 we haven't written the data yet.
      if (killing || (n == 5 && getState() != ProcessState::Running)) {
         // Don't try to kill the process before it is running - that can
         // be hazardous, as the actual child process might not be running
         // yet. Also, don't kill it "recursively".
         return;
      }
      killing = true;
      readAll();
      terminate();
      if ((waitedForFinished = waitForFinished(5000)) == false) {
         kill();
         if (getState() != ProcessState::NotRunning) {
            waitedForFinished = waitForFinished(5000);
         }  
      }
   }

   void finishedSlot(int, Process::ExitStatus)
   {
      waitedForFinished = true;
   }

   void channelReadyReadSlot(int)
   {
      terminateSlot();
   }

   void stateChangedSlot()
   {
      terminateSlot();
   }
private:
   void writePendingData()
   {
      if (!dataToWrite.isEmpty()) {
         write(dataToWrite);
         dataToWrite.clear();
      }
   }

private:
   int n;
   bool killing;
   ByteArray dataToWrite;
};

void init_soft_exit_in_slots_data(std::list<std::tuple<String, int>> &data)
{
   ByteArray dataTagPrefix("console app ");
   for (int i = 0; i < 6; ++i) {
      data.push_back(std::make_tuple(APP_FILENAME(ProcessEcho2App), i));
   }
}

using ForwardChannelDataType = std::list<std::tuple<Process::ProcessChannelMode, Process::InputChannelMode, ByteArray, ByteArray>>;
void init_forwarded_channels_data(ForwardChannelDataType &data)
{
   data.push_back(std::make_tuple(Process::ProcessChannelMode::SeparateChannels,
                                  Process::InputChannelMode::ManagedInputChannel,
                                  ByteArray(), ByteArray()));

   data.push_back(std::make_tuple(Process::ProcessChannelMode::ForwardedChannels,
                                  Process::InputChannelMode::ManagedInputChannel,
                                  ByteArray("forwarded"), ByteArray("forwarded")));

   data.push_back(std::make_tuple(Process::ProcessChannelMode::ForwardedOutputChannel,
                                  Process::InputChannelMode::ManagedInputChannel,
                                  ByteArray("forwarded"), ByteArray()));

   data.push_back(std::make_tuple(Process::ProcessChannelMode::ForwardedErrorChannel,
                                  Process::InputChannelMode::ManagedInputChannel,
                                  ByteArray(), ByteArray("forwarded")));

   data.push_back(std::make_tuple(Process::ProcessChannelMode::ForwardedErrorChannel,
                                  Process::InputChannelMode::ForwardedInputChannel,
                                  ByteArray(), ByteArray("input")));
}

} // anonymous namespace

TEST_F(ProcessTest, testSoftExitInSlots)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   std::list<std::tuple<String, int>> data;
   init_soft_exit_in_slots_data(data);
   for (auto item : data) {
      String &appName = std::get<0>(item);
      int signalToConnect = std::get<1>(item);

      SoftExitProcess process(signalToConnect);
      process.writeAfterStart("OLEBOLE", 8); // include the \0
      process.start(appName);
      PDK_TRY_VERIFY_WITH_TIMEOUT(process.waitedForFinished, 60000);
      ASSERT_EQ(process.getState(), Process::ProcessState::NotRunning);
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testMergedChannels)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.setReadChannelMode(Process::ProcessChannelMode::MergedChannels);
   ASSERT_EQ(process.getReadChannelMode(), Process::ProcessChannelMode::MergedChannels);
   process.start(APP_FILENAME(ProcessEcho2App));
   ASSERT_TRUE(process.waitForStarted(5000));
   for (int i = 0; i < 100; ++i) {
      ASSERT_EQ(process.write("abc"), pdk::plonglong(3));
      while (process.getBytesAvailable() < 6) {
         ASSERT_TRUE(process.waitForReadyRead(5000));
      }
      ASSERT_EQ(process.readAll(), ByteArray("aabbcc"));
   }
   process.closeWriteChannel();
   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testForwardedChannels)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   ForwardChannelDataType data;
   init_forwarded_channels_data(data);
   for (auto item : data) {
      Process::ProcessChannelMode mode = std::get<0>(item);
      Process::InputChannelMode inmode = std::get<1>(item);
      ByteArray &outData = std::get<2>(item);
      ByteArray &errorData = std::get<3>(item);
      Process process;
      process.start(APP_FILENAME(ForwardingApp), StringList() 
                    << String::number(pdk::as_integer<Process::ProcessChannelMode>(mode)) 
                    << String::number(pdk::as_integer<Process::InputChannelMode>(inmode)));
      ASSERT_TRUE(process.waitForStarted(5000));
      ASSERT_EQ(process.write("input"), 5);
      process.closeWriteChannel();
      ASSERT_TRUE(process.waitForFinished(5000));
      ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
      ASSERT_EQ(process.getExitCode(), 0);
      const char *err;
      switch (process.getExitCode()) {
      case 0: err = "ok"; break;
      case 1: err = "processChannelMode is wrong"; break;
      case 11: err = "inputChannelMode is wrong"; break;
      case 2: err = "failed to start"; break;
      case 3: err = "failed to write"; break;
      case 4: err = "did not finish"; break;
      case 5: err = "unexpected stdout"; break;
      case 6: err = "unexpected stderr"; break;
      case 13: err = "parameter error"; break;
      default: err = "unknown exit code"; break;
      }
      ASSERT_TRUE(!process.getExitCode()) << err;
      ASSERT_EQ(process.readAllStandardOutput(), outData);
      ASSERT_EQ(process.readAllStandardError(), errorData);
   }
   PDKTEST_END_APP_CONTEXT();
}

// @TODO testEnd
TEST_F(ProcessTest, testEnd)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.start(APP_FILENAME(ProcessEchoApp));
   process.write("abcdefgh\n");
   while (process.getBytesAvailable() < 8) {
      ASSERT_TRUE(process.waitForReadyRead(5000));
   }

   PDKTEST_END_APP_CONTEXT();
}

namespace {

void init_invalid_program_string_data(std::list<String> &data)
{
   data.push_back(String());
   data.push_back(String(Latin1String("")));
   data.push_back(String(Latin1String("   ")));
}

class TestThread : public Thread
{
public:
   inline int getCode()
   {
      return m_exitCode;
   }

protected:
   inline void run()
   {
      m_exitCode = 90210;

      Process process;
      process.connectFinishedSignal(this, &TestThread::catchExitCode, pdk::ConnectionType::DirectConnection);

      process.start(APP_FILENAME(ProcessEchoApp));

      ASSERT_EQ(process.write("abc\0", 4), pdk::pint64(4));
      m_exitCode = exec();
   }

protected:
   inline void catchExitCode(int exitCode)
   {
      m_exitCode = exitCode;
      exit(exitCode);
   }

private:
   int m_exitCode;
};

} // anonymous namespace

TEST_F(ProcessTest, testProcessInAThread)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   for (int i = 0; i < 10; ++i) {
      TestThread thread;
      thread.start();
      ASSERT_TRUE(thread.wait(10000));
      ASSERT_EQ(thread.getCode(), 0);
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testProcessesInMultipleThreads)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   for (int i = 0; i < 10; ++i) {
      // run from 1 to 10 threads, but run at least some tests
      // with more threads than the ideal
      int threadCount = i;
      if (i > 7) {
         threadCount = std::max(threadCount, Thread::getIdealThreadCount() + 2);
      }
      std::vector<TestThread *> threads(threadCount);
      for (int j = 0; j < threadCount; ++j) {
         threads[j] = new TestThread;
      }
      for (int j = 0; j < threadCount; ++j) {
         threads[j]->start();
      }
      for (int j = 0; j < threadCount; ++j) {
         ASSERT_TRUE(threads[j]->wait(10000));
      }
      for (int j = 0; j < threadCount; ++j) {
         ASSERT_EQ(threads[j]->getCode(), 0);
      }
      pdk::stdext::delete_all(threads);
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testWaitForFinishedWithTimeout)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.start(APP_FILENAME(ProcessEchoApp));
   ASSERT_TRUE(process.waitForStarted(5000));
   ASSERT_TRUE(!process.waitForFinished(1));

   process.write("", 1);
   ASSERT_TRUE(process.waitForFinished());
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testWaitForReadyReadInAReadyReadSlot)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   int readyReadCount = 0;
   auto conn = process.connectReadyReadSignal([&readyReadCount](IoDevice::SignalType signalType, Object *sender) {
      ++readyReadCount;
      Process *process = dynamic_cast<Process *>(sender);
      ASSERT_TRUE(process);
      ProcessTest::m_bytesAvailable = process->getBytesAvailable();
      process->write("bar", 4);
      ASSERT_TRUE(process->waitForReadyRead(5000));
      pdktest::TestEventLoop::instance().exitLoop();
   }, PDK_RETRIEVE_APP_INSTANCE());

   process.connectFinishedSignal([](){
      pdktest::TestEventLoop::instance().exitLoop();
   }, PDK_RETRIEVE_APP_INSTANCE());

   m_bytesAvailable = 0;

   process.start(APP_FILENAME(ProcessEchoApp));
   ASSERT_TRUE(process.waitForStarted(5000));

   process.write("foo");
   pdktest::TestEventLoop::instance().enterLoop(30);
   ASSERT_TRUE(!pdktest::TestEventLoop::instance().getTimeout());

   ASSERT_EQ(readyReadCount, 1);
   process.disconnectReadyReadSignal(conn);
   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   ASSERT_TRUE(process.getBytesAvailable() > m_bytesAvailable);

   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testWaitForBytesWrittenInABytesWrittenSlot)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   int byteWrittenCount = 0;
   m_bytesAvailable = 0;
   auto conn = process.connectBytesWrittenSignal([&byteWrittenCount](IoDevice::SignalType signalType, Object *sender) {
      ++byteWrittenCount;
      Process *process = dynamic_cast<Process *>(sender);
      ASSERT_TRUE(process);
      process->write("b");
      ASSERT_TRUE(process->waitForBytesWritten(5000));
      pdktest::TestEventLoop::instance().exitLoop();
   });
   process.start(APP_FILENAME(ProcessEchoApp));
   ASSERT_TRUE(process.waitForStarted(5000));

   process.write("f");
   pdktest::TestEventLoop::instance().enterLoop(30);
   ASSERT_TRUE(!pdktest::TestEventLoop::instance().getTimeout());

   ASSERT_EQ(byteWrittenCount, 1);
   process.disconnectBytesWrittenSignal(conn);
   process.write("", 1);
   process.disconnectBytesWrittenSignal(conn);
   ASSERT_TRUE(process.waitForFinished());
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);

   PDKTEST_END_APP_CONTEXT();
}

namespace {

void init_space_args_test_data(std::list<std::tuple<StringList, String>> &data)
{
   data.push_back(std::make_tuple(StringList() << String::fromLatin1("arg1") << String::fromLatin1("arg2"),
                                  String::fromLatin1("arg1 arg2")));

   data.push_back(std::make_tuple(StringList() << String::fromLatin1("\"arg1\"") << String::fromLatin1("ar \"g2"),
                                  String::fromLatin1("\"\"\"\"arg1\"\"\"\" \"ar \"\"\"g2\"")));

   data.push_back(std::make_tuple(StringList() << String::fromLatin1("ar g1") << String::fromLatin1("a rg 2"),
                                  String::fromLatin1("\"ar g1\" \"a rg 2\"")));

   data.push_back(std::make_tuple(StringList() << String::fromLatin1("-lar g1") << String::fromLatin1("-l\"ar g2\""),
                                  String::fromLatin1("\"-lar g1\" \"-l\"\"\"ar g2\"\"\"\"")));

   data.push_back(std::make_tuple(StringList() << String::fromLatin1("ar\"g1"),
                                  String::fromLatin1("ar\"\"\"\"g1")));

   data.push_back(std::make_tuple(StringList() << String::fromLatin1("ar\\g1"),
                                  String::fromLatin1("ar\\g1")));

   data.push_back(std::make_tuple(StringList() << String::fromLatin1("ar\\g\"1"),
                                  String::fromLatin1("ar\\g\"\"\"\"1")));

   data.push_back(std::make_tuple(StringList() << String::fromLatin1("arg\\\"1"),
                                  String::fromLatin1("arg\\\"\"\"1")));

   data.push_back(std::make_tuple(StringList() << String::fromLatin1("\"\"\"\""),
                                  String::fromLatin1("\"\"\"\"\"\"\"\"\"\"\"\"")));

   data.push_back(std::make_tuple(StringList() << String::fromLatin1("\"\"\"\"") << String::fromLatin1("\"\" \"\""),
                                  String::fromLatin1("\"\"\"\"\"\"\"\"\"\"\"\" \"\"\"\"\"\"\" \"\"\"\"\"\"\"")));

   data.push_back(std::make_tuple(StringList() << String::fromLatin1("\"\"  \"\""),
                                  String::fromLatin1("\"\"\"\"\"\"\" \"\" \"\"\"\"\"\"\"")));

   data.push_back(std::make_tuple(StringList() << String::fromLatin1("\"\"  \"\""),
                                  String::fromLatin1(" \"\"\"\"\"\"\" \"\" \"\"\"\"\"\"\"   ")));
}

ByteArray start_fail_message(const String &program, const Process &process)
{
   ByteArray result = "Process '";
   result += program.toLocal8Bit();
   result += "' failed to start: ";
   result += process.getErrorString().toLocal8Bit();
   return result;
}

} // anonymous namespace

TEST_F(ProcessTest, testSpaceArgsTest)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   std::list<std::tuple<StringList, String>> data;
   init_space_args_test_data(data);
   for (auto item : data) {
      StringList &args = std::get<0>(item);
      String &stringArgs = std::get<1>(item);
      StringList programs;
      programs << String(APP_FILENAME(ProcessSpacesArgsApp))
               << String(APP_FILENAME(one space))
               << String(APP_FILENAME(two space s));
      Process process;
      for (size_t i = 0; i < programs.size(); ++i) {
         String program = programs.at(i);
         process.start(program, args);

         ByteArray errorMessage;
         bool started = process.waitForStarted();
         if (!started) {
            errorMessage = start_fail_message(program, process);
         }

         ASSERT_TRUE(started) << errorMessage.getConstRawData();
         ASSERT_TRUE(process.waitForFinished());
         ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
         ASSERT_EQ(process.getExitCode(), 0);

         StringList actual = String::fromLatin1(process.readAll()).split(Latin1String("|"));
         ASSERT_TRUE(!actual.empty());
         // not interested in the program name, it might be different.
         actual.pop_front();
         ASSERT_EQ(actual, args);
         if (program.contains(Latin1Character(' '))) {
            program = Latin1Character('"') + program + Latin1Character('"');
         }
         if (!stringArgs.isEmpty()) {
            program += Latin1Character(' ') + stringArgs;
         }
         errorMessage.clear();
         process.start(program);
         started = process.waitForStarted(5000);
         if (!started) {
            errorMessage = start_fail_message(program, process);
         }
         ASSERT_TRUE(started) <<  errorMessage.getConstRawData();
         ASSERT_TRUE(process.waitForFinished(5000));

         actual = String::fromLatin1(process.readAll()).split(Latin1String("|"));
         ASSERT_TRUE(!actual.empty());
         // not interested in the program name, it might be different.
         actual.pop_front();

         ASSERT_EQ(actual, args);
      }
   }

   PDKTEST_END_APP_CONTEXT();
}

// Windows platform
// @todo Process::nativeArguments
// @todo Process::createProcessArgumentsModifier

TEST_F(ProcessTest, testExitCode)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   for (int i = 0; i < 255; ++i) {
      Process process;
      process.start(String(APP_FILENAME(ExitCodesApp)) + Latin1String(" ") + String::number(i));
      ASSERT_TRUE(process.waitForFinished(5000));
      ASSERT_EQ(process.getExitCode(), i);
      ASSERT_EQ(process.getError(), Process::ProcessError::UnknownError);
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testFailToStart)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;

   std::vector<Process::ProcessState> stateChangedData;
   process.connectStateChangedSignal([&stateChangedData](Process::ProcessState state){
      stateChangedData.push_back(state);
   }, PDK_RETRIEVE_APP_INSTANCE());

   int errorOccuredCount = 0;
   process.connectErrorOccurredSignal([&errorOccuredCount](Process::ProcessError error){
      ++errorOccuredCount;
   }, PDK_RETRIEVE_APP_INSTANCE());

   int finishedCount = 0;
   process.connectFinishedSignal([&finishedCount](int exitCode, Process::ExitStatus exitStatus){
      ++finishedCount;
   }, PDK_RETRIEVE_APP_INSTANCE());

   // OS X and HP-UX have a really low default process limit (~100), so spawning
   // to many processes here will cause test failures later on.
#if defined PDK_OS_HPUX
   const int attempts = 15;
#elif defined PDK_OS_MAC
   const int attempts = 15;
#else
   const int attempts = 50;
#endif
   for (int j = 0; j < 8; ++j) {
      for (int i = 0; i < attempts; ++i) {
         ASSERT_EQ(errorOccuredCount, j * attempts + i);
         process.start(Latin1String("/blurp"));

         switch (j) {
         case 0:
         case 1:
            ASSERT_TRUE(!process.waitForStarted());
            break;
         case 2:
         case 3:
            ASSERT_TRUE(!process.waitForFinished());
            break;
         case 4:
         case 5:
            ASSERT_TRUE(!process.waitForReadyRead());
            break;
         case 6:
         case 7:
         default:
            ASSERT_TRUE(!process.waitForBytesWritten());
            break;
         }

         ASSERT_EQ(process.getError(), Process::ProcessError::FailedToStart);
         ASSERT_EQ(errorOccuredCount, j * attempts + i + 1);
         ASSERT_EQ(finishedCount, 0);

         int it = j * attempts + i + 1;

         ASSERT_EQ(stateChangedData.size(), (size_t)(it * 2));
         ASSERT_EQ(stateChangedData.at(it * 2 - 2), Process::ProcessState::Starting);
         ASSERT_EQ(stateChangedData.at(it * 2 - 1), Process::ProcessState::NotRunning);
      }
   }

   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testFailToStartWithWait)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   int errorOccuredCount = 0;
   process.connectErrorOccurredSignal([&errorOccuredCount](Process::ProcessError error){
      ++errorOccuredCount;
   }, PDK_RETRIEVE_APP_INSTANCE());

   int finishedCount = 0;
   process.connectFinishedSignal([&finishedCount](int exitCode, Process::ExitStatus exitStatus){
      ++finishedCount;
   }, PDK_RETRIEVE_APP_INSTANCE());

   for (int i = 0; i < 50; ++i) {
      process.start(Latin1String("/blurp"), StringList() << Latin1String("-v") << Latin1String("-debug"));
      process.waitForStarted();

      ASSERT_EQ(process.getError(), Process::ProcessError::FailedToStart);
      ASSERT_EQ(errorOccuredCount, i + 1);
      ASSERT_EQ(finishedCount, 0);
   }

   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testFailToStartWithEventLoop)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   EventLoop loop;
   int errorOccuredCount = 0;
   process.connectErrorOccurredSignal([&errorOccuredCount](Process::ProcessError error){
      ++errorOccuredCount;
   }, PDK_RETRIEVE_APP_INSTANCE());

   int finishedCount = 0;
   process.connectFinishedSignal([&finishedCount](int exitCode, Process::ExitStatus exitStatus){
      ++finishedCount;
   }, PDK_RETRIEVE_APP_INSTANCE());

   process.connectErrorOccurredSignal([&loop](Process::ProcessError error){
      loop.exit();
   }, PDK_RETRIEVE_APP_INSTANCE(), pdk::ConnectionType::QueuedConnection);

   for (int i = 0; i < 50; ++i) {
      process.start(Latin1String("/blurp"), StringList() << Latin1String("-v") << Latin1String("-debug"));
      loop.exec();
      ASSERT_EQ(process.getError(), Process::ProcessError::FailedToStart);
      ASSERT_EQ(errorOccuredCount, i + 1);
      ASSERT_EQ(finishedCount, 0);
   }
   PDKTEST_END_APP_CONTEXT();
}

namespace {

void init_fail_to_start_empty_args_data(std::list<int> &data)
{
   data.push_back(0);
   data.push_back(1);
   data.push_back(2);
}

} // anonymous namespace

TEST_F(ProcessTest, testFailToStartEmptyArgs)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   std::list<int> data;
   init_fail_to_start_empty_args_data(data);
   for (int startOverload: data) {
      Process process;
      std::list<Process::ProcessError> errorData;
      process.connectErrorOccurredSignal([&errorData](Process::ProcessError error){
         errorData.push_back(error);
      }, PDK_RETRIEVE_APP_INSTANCE());
      switch (startOverload) {
      case 0:
         process.start(String(), StringList(), IoDevice::OpenMode::ReadWrite);
         break;
      case 1:
         process.start(String(), IoDevice::OpenMode::ReadWrite);
         break;
      case 2:
         process.start(IoDevice::OpenMode::ReadWrite);
         break;
      default:
         FAIL() << "Unhandled Process::start overload.";
      };

      ASSERT_TRUE(!process.waitForStarted());
      ASSERT_EQ(errorData.size(), 1u);
      ASSERT_EQ(process.getError(), Process::ProcessError::FailedToStart);
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testRemoveFileWhileProcessIsRunning)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   File file(m_temporaryDir.getPath() + Latin1String("/removeFile.txt"));
   ASSERT_TRUE(file.open(File::OpenMode::WriteOnly));
   Process process;
   process.start(APP_FILENAME(ProcessEchoApp));
   ASSERT_TRUE(process.waitForStarted(5000));

   ASSERT_TRUE(file.remove());

   process.write("", 1);
   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   PDKTEST_END_APP_CONTEXT();
}

namespace {

void init_set_environment_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("ProcessTest"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("ProcessTest"), Latin1String("value")));
   // @TODO add Windows platform testcases
   data.push_back(std::make_tuple(Latin1String("PATH"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("PATH"), Latin1String("value")));
}

} // anonymous namespace

TEST_F(ProcessTest, testProcessEnvironment)
{
   ASSERT_TRUE(pdk::get_env("ProcessTest").isEmpty());
   ASSERT_TRUE(!pdk::get_env("PATH").isEmpty());

   std::list<std::tuple<String, String>> data;
   init_set_environment_data(data);
   for (auto &item : data) {
      String &name = std::get<0>(item);
      String &value = std::get<1>(item);
      String executable = APP_FILENAME(ProcessEnvironmentApp);
      {
         Process process;
         ProcessEnvironment environment = ProcessEnvironment::getSystemEnvironment();
         if (value.isNull()) {
            environment.remove(name);
         } else {
            environment.insert(name, value);
         }
         process.setProcessEnvironment(environment);
         process.start(executable, StringList() << name);

         ASSERT_TRUE(process.waitForFinished());
         if (value.isNull()) {
            ASSERT_EQ(process.getExitCode(), 1);
         } else if (!value.isEmpty()) {
            ASSERT_EQ(process.getExitCode(), 0);
         }
         ASSERT_EQ(process.readAll(), value.toLocal8Bit());
      }
   }
}

TEST_F(ProcessTest, testEnvironmentIsSorted)
{
   ProcessEnvironment env;
   env.insert(Latin1String("a"), Latin1String("foo_a"));
   env.insert(Latin1String("B"), Latin1String("foo_B"));
   env.insert(Latin1String("c"), Latin1String("foo_c"));
   env.insert(Latin1String("D"), Latin1String("foo_D"));
   env.insert(Latin1String("e"), Latin1String("foo_e"));
   env.insert(Latin1String("F"), Latin1String("foo_F"));
   env.insert(Latin1String("Path"), Latin1String("foo_Path"));
   env.insert(Latin1String("SystemRoot"), Latin1String("foo_SystemRoot"));
   const StringList envlist = env.toStringList();
   const StringList expected = { Latin1String("B=foo_B"),
                                 Latin1String("D=foo_D"),
                                 Latin1String("F=foo_F"),
                                 Latin1String("Path=foo_Path"),
                                 Latin1String("SystemRoot=foo_SystemRoot"),
                                 Latin1String("a=foo_a"),
                                 Latin1String("c=foo_c"),
                                 Latin1String("e=foo_e") };
   ASSERT_EQ(envlist, expected);
}

TEST_F(ProcessTest, testSystemEnvironment)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   ASSERT_TRUE(!Process::getSystemEnvironment().empty());
   ASSERT_TRUE(!ProcessEnvironment::getSystemEnvironment().isEmpty());

   ASSERT_TRUE(ProcessEnvironment::getSystemEnvironment().contains(Latin1String("PATH")));
   ASSERT_TRUE(!Process::getSystemEnvironment().filter(RegularExpression(Latin1String("^PATH="), RegularExpression::PatternOption::CaseInsensitiveOption)).empty());
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testSpaceInName)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.start(APP_FILENAME(test Space In Name), StringList());
   ASSERT_TRUE(process.waitForStarted());
   process.write("", 1);
   ASSERT_TRUE(process.waitForFinished());
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   PDKTEST_END_APP_CONTEXT();
}

// @TODO testLockupsInStartDetached

TEST_F(ProcessTest, testAtEnd2)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.start(APP_FILENAME(ProcessEchoApp));
   process.write("Foo\nBar\nBaz\nBodukon\nHadukan\nTorwukan\nend\n");
   process.putChar('\0');
   ASSERT_TRUE(process.waitForFinished());
   std::list<ByteArray> lines;
   while (!process.atEnd()) {
      lines.push_back(process.readLine());
   }
   ASSERT_EQ(lines.size(), 7u);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testWaitForReadyReadForNonexistantProcess)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   std::vector<Process::ProcessError> errorData;
   process.connectErrorOccurredSignal([&errorData](Process::ProcessError error){
      errorData.push_back(error);
   }, PDK_RETRIEVE_APP_INSTANCE());

   int finishedCount = 0;
   process.connectFinishedSignal([&finishedCount](int exitCode, Process::ExitStatus exitStatus){
      ++finishedCount;
   }, PDK_RETRIEVE_APP_INSTANCE());

   ASSERT_TRUE(!process.waitForReadyRead()); // used to crash
   process.start(Latin1String("doesntexist"));
   ASSERT_TRUE(!process.waitForReadyRead());
   ASSERT_EQ(errorData.at(0), Process::ProcessError::FailedToStart);
   ASSERT_EQ(errorData.size(), 1u);
   ASSERT_EQ(finishedCount, 0);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testSetStandardInputFile)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   static const char data[] = "A bunch\1of\2data\3\4\5\6\7...";
   Process process;
   File file(m_temporaryDir.getPath() + Latin1String("/data-sif"));

   ASSERT_TRUE(file.open(IoDevice::OpenMode::WriteOnly));
   file.write(data, sizeof data);
   file.close();

   process.setStandardInputFile(file.getFileName());
   process.start(APP_FILENAME(ProcessEchoApp));

   ASSERT_TRUE(process.waitForFinished());
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   ByteArray all = process.readAll();
   ASSERT_EQ(all.size(), int(sizeof data) - 1); // testProcessEcho drops the ending \0
   ASSERT_TRUE(all == data);

   Process process2;
   process2.setStandardInputFile(Process::getNullDevice());
   process2.start(APP_FILENAME(ProcessEchoApp));
   ASSERT_TRUE(process2.waitForFinished());
   all = process2.readAll();
   ASSERT_EQ(all.size(), 0);

   PDKTEST_END_APP_CONTEXT();
}

namespace {

void init_set_standard_output_file_data(std::list<std::tuple<Process::ProcessChannel, Process::ProcessChannelMode, bool>> &data)
{
   data.push_back(std::make_tuple(Process::ProcessChannel::StandardOutput,
                                  Process::ProcessChannelMode::SeparateChannels,
                                  false));

   data.push_back(std::make_tuple(Process::ProcessChannel::StandardOutput,
                                  Process::ProcessChannelMode::SeparateChannels,
                                  true));

   data.push_back(std::make_tuple(Process::ProcessChannel::StandardError,
                                  Process::ProcessChannelMode::SeparateChannels,
                                  false));

   data.push_back(std::make_tuple(Process::ProcessChannel::StandardError,
                                  Process::ProcessChannelMode::SeparateChannels,
                                  true));

   data.push_back(std::make_tuple(Process::ProcessChannel::StandardOutput,
                                  Process::ProcessChannelMode::MergedChannels,
                                  false));

   data.push_back(std::make_tuple(Process::ProcessChannel::StandardOutput,
                                  Process::ProcessChannelMode::MergedChannels,
                                  true));
}

} // anonymous namespace

TEST_F(ProcessTest, testSetStandardOutputFile)
{
   static const char data[] = "Original data. ";
   static const char testdata[] = "Test data.";
   std::list<std::tuple<Process::ProcessChannel, Process::ProcessChannelMode, bool>> dd;
   init_set_standard_output_file_data(dd);
   for (auto &item : dd) {
      Process::ProcessChannel channelToTest = std::get<0>(item);
      Process::ProcessChannelMode channelMode = std::get<1>(item);
      bool append = std::get<2>(item);
      IoDevice::OpenMode mode = append ? IoDevice::OpenMode::Append : IoDevice::OpenMode::Truncate;
      // create the destination file with data
      File file(m_temporaryDir.getPath() + Latin1String("/data-stdof-") + Latin1String("testSetStandardOutputFile"));
      ASSERT_TRUE(file.open(IoDevice::OpenMode::WriteOnly));
      file.write(data, sizeof data - 1);
      file.close();

      // run the process
      Process process;
      process.setReadChannelMode(channelMode);
      if (channelToTest == Process::ProcessChannel::StandardOutput) {
         process.setStandardOutputFile(file.getFileName(), mode);
      } else {
         process.setStandardErrorFile(file.getFileName(), mode);
      }
      process.start(APP_FILENAME(ProcessEcho2App));
      process.write(testdata, sizeof testdata);
      ASSERT_TRUE(process.waitForFinished());
      ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
      ASSERT_EQ(process.getExitCode(), 0);

      // open the file again and verify the data
      ASSERT_TRUE(file.open(IoDevice::OpenMode::ReadOnly));
      ByteArray all = file.readAll();
      file.close();

      int expectedsize = sizeof testdata - 1;
      if (mode == IoDevice::OpenMode::Append) {
         ASSERT_TRUE(all.startsWith(data));
         expectedsize += sizeof data - 1;
      }
      if (channelMode == Process::ProcessChannelMode::MergedChannels) {
         expectedsize += sizeof testdata - 1;
      } else {
         ASSERT_TRUE(all.endsWith(testdata));
      }
      ASSERT_EQ(all.size(), expectedsize);
   }
}

TEST_F(ProcessTest, testSetStandardOutputFileNullDevice)
{
   static const char testdata[] = "Test data.";
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.setStandardOutputFile(Process::getNullDevice());
   process.start(APP_FILENAME(ProcessEcho2App));
   process.write(testdata, sizeof testdata);
   ASSERT_TRUE(process.waitForFinished());
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   ASSERT_EQ(process.getBytesAvailable(), PDK_INT64_C(0));
   ASSERT_TRUE(!FileInfo(Process::getNullDevice()).isFile());
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testSetStandardOutputFileAndWaitForBytesWritten)
{
   static const char testdata[] = "Test data.";
   PDKTEST_BEGIN_APP_CONTEXT();

   File file(m_temporaryDir.getPath() + Latin1String("/data-stdofawfbw"));
   Process process;
   process.setStandardOutputFile(file.getFileName());
   process.start(APP_FILENAME(ProcessEcho2App));
   ASSERT_TRUE(process.waitForStarted()) << pdk_printable(process.getErrorString());
   process.write(testdata, sizeof testdata);
   process.waitForBytesWritten();
   ASSERT_TRUE(process.waitForFinished());
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);

   // open the file again and verify the data
   ASSERT_TRUE(file.open(IoDevice::OpenMode::ReadOnly));
   ByteArray all = file.readAll();
   file.close();

   ASSERT_EQ(all, ByteArray::fromRawData(testdata, sizeof testdata - 1));

   PDKTEST_END_APP_CONTEXT();
}

namespace {

void init_set_standard_output_process_data(std::list<std::tuple<bool, bool>> &data)
{
   data.push_back(std::make_tuple(false, false));
   data.push_back(std::make_tuple(false, true));
   data.push_back(std::make_tuple(true, false));
}

void init_detached_rocess_parameters_data(std::list<String> &data)
{
   data.push_back(String());
   data.push_back(String(Latin1String("stdout")));
   data.push_back(String(Latin1String("stderr")));
}

} // anonymous namespace

TEST_F(ProcessTest, testSetStandardOutputProcess)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process source;
   Process sink;

   std::list<std::tuple<bool, bool>> data;
   init_set_standard_output_process_data(data);
   for (auto &item : data) {
      bool merged = std::get<0>(item);
      bool waitForBytesWritten = std::get<1>(item);
      source.setReadChannelMode(merged ? Process::ProcessChannelMode::MergedChannels : Process::ProcessChannelMode::SeparateChannels);
      source.setStandardOutputProcess(&sink);
      source.start(APP_FILENAME(ProcessEcho2App));
      sink.start(APP_FILENAME(ProcessEcho2App));
      ByteArray data("Hello, World");
      source.write(data);
      if (waitForBytesWritten) {
         source.waitForBytesWritten();
      }
      source.closeWriteChannel();
      ASSERT_TRUE(source.waitForFinished());
      ASSERT_EQ(source.getExitStatus(), Process::ExitStatus::NormalExit);
      ASSERT_EQ(source.getExitCode(), 0);
      ASSERT_TRUE(sink.waitForFinished());
      ASSERT_EQ(sink.getExitStatus(), Process::ExitStatus::NormalExit);
      ASSERT_EQ(sink.getExitCode(), 0);
      ByteArray all = sink.readAll();

      if (!merged) {
         ASSERT_EQ(all, data);
      } else {
         ASSERT_EQ(all, ByteArray("HHeelllloo,,  WWoorrlldd"));
      }         
   }

   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testFileWriterProcess)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   const ByteArray line = ByteArrayLiteral(" -- testing testing 1 2 3\n");
   ByteArray stdinStr;
   stdinStr.reserve(5000 * (4 + line.size()) + 1);
   for (int i = 0; i < 5000; ++i) {
      stdinStr += ByteArray::number(i);
      stdinStr += line;
   }

   Time stopWatch;
   stopWatch.start();
   const String fileName = m_temporaryDir.getPath() + Latin1String("/fileWriterProcess.txt");
   const String binary = APP_FILENAME(FileWriterProcessApp);

   do {
      if (File::exists(fileName)) {
         ASSERT_TRUE(File::remove(fileName));
      }
      Process process;
      process.setWorkingDirectory(m_temporaryDir.getPath());
      process.start(binary, IoDevice::OpenMode::ReadWrite | IoDevice::OpenMode::Text);
      process.write(stdinStr);
      process.closeWriteChannel();
      while (process.getBytesToWrite()) {
         ASSERT_TRUE(stopWatch.elapsed() < 3500);
         ASSERT_TRUE(process.waitForBytesWritten(2000));
      }
      ASSERT_TRUE(process.waitForFinished());
      ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
      ASSERT_EQ(process.getExitCode(), 0);
      ASSERT_EQ(File(fileName).getSize(), pdk::pint64(stdinStr.size()));
   } while (stopWatch.elapsed() < 3000);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testDetachedProcessParameters)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   std::list<String> data;
   init_detached_rocess_parameters_data(data);
   String workingDir = Dir::getCurrentPath() + Latin1String("/process");
   Dir workingDirObject(workingDir);
   workingDir += Latin1String("/testDetached");
   if (!File::exists(workingDir)) {
      ASSERT_TRUE(workingDirObject.mkdir(Latin1String("testDetached"))) << "make working directory error";
   }
   
   ASSERT_TRUE(File::exists(workingDir));
   for (String &outChannel : data) {
      pdk::pint64 pid;
      File infoFile(m_temporaryDir.getPath() + Latin1String("/detachedinfo.txt"));
      if (infoFile.exists()) {
         ASSERT_TRUE(infoFile.remove());
      }
      File channelFile(m_temporaryDir.getPath() + Latin1String("detachedinfo2.txt"));
      if (channelFile.exists()) {
         ASSERT_TRUE(channelFile.remove());
      }
      ASSERT_TRUE(pdk::get_env("ProcessTest").isEmpty());
      ByteArray envVarValue("foobarbaz");
      ProcessEnvironment environment = ProcessEnvironment::getSystemEnvironment();
      environment.insert(StringLiteral("ProcessTest"), String::fromUtf8(envVarValue));
      
      Process process;
      process.setProgram(APP_FILENAME(DetachedApp));
#ifdef PDK_OS_WIN
      int modifierCalls = 0;
      process.setCreateProcessArgumentsModifier(
               [&modifierCalls] (QProcess::CreateProcessArguments *) { modifierCalls++; });
#endif
      StringList args(infoFile.getFileName());
      if (!outChannel.isEmpty()) {
         args << StringLiteral("--out-channel=") + outChannel;
         if (outChannel == Latin1String("stdout")) {
            process.setStandardOutputFile(channelFile.getFileName());
         } else if (outChannel == Latin1String("stderr")) {
            process.setStandardErrorFile(channelFile.getFileName());
         }
      }
      process.setArguments(args);
      process.setWorkingDirectory(workingDir);
      process.setProcessEnvironment(environment);
      ASSERT_TRUE(process.startDetached(&pid));
      
      FileInfo fi(infoFile);
      fi.setCaching(false);
      //The guard counter ensures the test does not hang if the sub process fails.
      //Instead, the test will fail when trying to open & verify the sub process output file.
      for (int guard = 0; guard < 100 && fi.getSize() == 0; guard++) {
         pdktest::sleep(100);
      }
      
      ASSERT_TRUE(infoFile.open(IoDevice::OpenMode::ReadOnly | IoDevice::OpenMode::Text));
      String actualWorkingDir = String::fromUtf8(infoFile.readLine()).trimmed();
      ByteArray processIdString = infoFile.readLine().trimmed();
      ByteArray actualEnvVarValue = infoFile.readLine().trimmed();
      ByteArray infoFileContent;
      if (!outChannel.isEmpty()) {
         infoFile.seek(0);
         infoFileContent = infoFile.readAll();
      }
      infoFile.close();
      infoFile.remove();
      
      if (!outChannel.isEmpty()) {
         ASSERT_TRUE(channelFile.open(IoDevice::OpenMode::ReadOnly | IoDevice::OpenMode::Text));
         ByteArray channelContent = channelFile.readAll();
         channelFile.close();
         channelFile.remove();
         ASSERT_EQ(channelContent, infoFileContent);
      }
      
      bool ok = false;
      pdk::pint64 actualPid = processIdString.toLongLong(&ok);
      ASSERT_TRUE(ok);
      
      ASSERT_EQ(actualWorkingDir, workingDir);
      ASSERT_EQ(actualPid, pid);
      ASSERT_EQ(actualEnvVarValue, envVarValue);
#ifdef PDK_OS_WIN
      ASSERT_EQ(modifierCalls, 1);
#endif
   }
   if (File::exists(workingDir)) {
      ASSERT_TRUE(workingDirObject.rmdir(Latin1String("testDetached"))) << "delete working directory error";
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testSwitchReadChannels)
{
   const char data[] = "ABCD";
   
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   
   process.start(APP_FILENAME(ProcessEcho2App));
   process.write(data);
   process.closeWriteChannel();
   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   
   for (int i = 0; i < 4; ++i) {
      process.setReadChannel(Process::ProcessChannel::StandardOutput);
      ASSERT_EQ(process.read(1), ByteArray(&data[i], 1));
      process.setReadChannel(Process::ProcessChannel::StandardError);
      ASSERT_EQ(process.read(1), ByteArray(&data[i], 1));
   }
   
   process.ungetChar('D');
   process.setReadChannel(Process::ProcessChannel::StandardOutput);
   process.ungetChar('D');
   process.setReadChannel(Process::ProcessChannel::StandardError);
   ASSERT_EQ(process.read(1), ByteArray("D"));
   process.setReadChannel(Process::ProcessChannel::StandardOutput);
   ASSERT_EQ(process.read(1), ByteArray("D"));
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testDiscardUnwantedOutput)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.setProgram(APP_FILENAME(ProcessEcho2App));
   process.start(IoDevice::OpenMode::WriteOnly);
   process.write("Hello, World");
   process.closeWriteChannel();
   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   
   process.setReadChannel(Process::ProcessChannel::StandardOutput);
   ASSERT_EQ(process.getBytesAvailable(), PDK_INT64_C(0));
   process.setReadChannel(Process::ProcessChannel::StandardError);
   ASSERT_EQ(process.getBytesAvailable(), PDK_INT64_C(0));
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testSetWorkingDirectory)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.setWorkingDirectory(Dir::getCurrentPath());
   
   // use absolute path because on Windows, the executable is relative to the parent's CWD
   // while on Unix with fork it's relative to the child's (with posix_spawn, it could be either).
   process.start(APP_FILENAME(SetWorkingDirectoryApp));
   
   ASSERT_TRUE(process.waitForFinished()) << process.getErrorString();
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   
   ByteArray workingDir = process.readAllStandardOutput();
   ASSERT_EQ(Dir(Dir::getCurrentPath()).getCanonicalPath(), Dir(Latin1String(workingDir.getConstRawData())).getCanonicalPath());
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testSetNonExistentWorkingDirectory)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.setWorkingDirectory(Latin1String("this/directory/should/not/exist/for/sure"));
   
   // use absolute path because on Windows, the executable is relative to the parent's CWD
   // while on Unix with fork it's relative to the child's (with posix_spawn, it could be either).
   process.start(APP_FILENAME(SetWorkingDirectoryApp));
   ASSERT_TRUE(!process.waitForFinished());
   ASSERT_EQ(process.getError(), Process::ProcessError::FailedToStart);
   
#ifdef PDK_OS_UNIX
#  ifdef PROCESS_USE_SPAWN
   FAIL() << "Process cannot detect failure to start when using posix_spawn()";
#  endif
   ASSERT_TRUE(process.getErrorString().startsWith(Latin1String("chdir:"))) << process.getErrorString();
#endif
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testStartFinishStartFinish)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   
   for (int i = 0; i < 3; ++i) {
      ASSERT_EQ(process.getState(), Process::ProcessState::NotRunning);
      
      process.start(APP_FILENAME(ProcessOutputApp));
      ASSERT_TRUE(process.waitForReadyRead(10000));
      ASSERT_EQ(String::fromLatin1(process.readLine().trimmed()),
                String(Latin1String("0 -this is a number")));
      if (process.getState() != Process::ProcessState::NotRunning) {
         ASSERT_TRUE(process.waitForFinished(10000));
         ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
         ASSERT_EQ(process.getExitCode(), 0);
      }
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testInvalidProgramString)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   std::list<String> data;
   init_invalid_program_string_data(data);
   for (const String &programString : data) {
      Process process;
      std::list<Process::ProcessError> errorData;
      process.connectErrorOccurredSignal([&errorData](Process::ProcessError error){
         errorData.push_back(error);
      }, PDK_RETRIEVE_APP_INSTANCE());

      process.start(programString);
      ASSERT_EQ(process.getError(), Process::ProcessError::FailedToStart);
      ASSERT_EQ(errorData.size(), 1u);
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testOnlyOneStartedSignal)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;

   int startedCount = 0;
   process.connectStartedSignal([&startedCount](){
      ++startedCount;
   }, PDK_RETRIEVE_APP_INSTANCE());

   int finishedCount = 0;
   process.connectFinishedSignal([&finishedCount](int exitCode, Process::ExitStatus exitStatus){
      ++finishedCount;
   }, PDK_RETRIEVE_APP_INSTANCE());

   process.start(APP_FILENAME(ProcessNormalApp));
   ASSERT_TRUE(process.waitForStarted(5000));
   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(startedCount, 1);
   ASSERT_EQ(finishedCount, 1);

   startedCount = 0;
   finishedCount = 0;

   process.start(APP_FILENAME(ProcessNormalApp));
   ASSERT_TRUE(process.waitForFinished(5000));
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   ASSERT_EQ(startedCount, 1);
   ASSERT_EQ(finishedCount, 1);

   PDKTEST_END_APP_CONTEXT();
}

namespace {

class BlockOnReadStdOut : public Object
{
public:
   BlockOnReadStdOut(Process *process)
   {
      process->connectReadyReadSignal(this, &BlockOnReadStdOut::block);
   }
   
public:
   void block()
   {
      Thread::sleep(1);
   }
};

} // anonymous namespace


TEST_F(ProcessTest, testFinishProcessBeforeReadingDone)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   BlockOnReadStdOut blocker(&process);
   EventLoop loop;
   process.connectFinishedSignal([&loop](int exitCode, Process::ExitStatus exitStatus){
      loop.exit();
   });
   process.start(APP_FILENAME(ProcessOutputApp));
   ASSERT_TRUE(process.waitForStarted());
   loop.exec();
   StringList lines = String::fromLocal8Bit(process.readAllStandardOutput()).split(
            RegularExpression(StringLiteral("[\r\n]")), String::SplitBehavior::SkipEmptyParts);
   ASSERT_TRUE(!lines.empty());
   ASSERT_EQ(lines.back(), StringLiteral("10239 -this is a number"));
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testWaitForStartedWithoutStart)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   ASSERT_TRUE(!process.waitForStarted(5000));
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(ProcessTest, testStartStopStartStop)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process process;
   process.start(APP_FILENAME(ProcessNormalApp));
   ASSERT_TRUE(process.waitForFinished());
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);

   process.start(APP_FILENAME(ExitCodesApp), StringList() << Latin1String("1"));
   ASSERT_TRUE(process.waitForFinished());
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 1);

   process.start(APP_FILENAME(ProcessNormalApp));
   ASSERT_TRUE(process.waitForFinished());
   ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
   ASSERT_EQ(process.getExitCode(), 0);

   PDKTEST_END_APP_CONTEXT();
}

namespace {

void init_start_stop_start_stop_buffers_data(std::list<std::tuple<Process::ProcessChannelMode, Process::ProcessChannelMode>> &data)
{
   data.push_back(std::make_tuple(Process::ProcessChannelMode::SeparateChannels, 
                                  Process::ProcessChannelMode::SeparateChannels));
   data.push_back(std::make_tuple(Process::ProcessChannelMode::SeparateChannels, 
                                  Process::ProcessChannelMode::MergedChannels));
   data.push_back(std::make_tuple(Process::ProcessChannelMode::MergedChannels, 
                                  Process::ProcessChannelMode::SeparateChannels));
   data.push_back(std::make_tuple(Process::ProcessChannelMode::MergedChannels, 
                                  Process::ProcessChannelMode::MergedChannels));
   data.push_back(std::make_tuple(Process::ProcessChannelMode::MergedChannels, 
                                  Process::ProcessChannelMode::ForwardedChannels));
}

} // anonymous namespace

TEST_F(ProcessTest, testStartStopStartStopBuffers)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   std::list<std::tuple<Process::ProcessChannelMode, Process::ProcessChannelMode>> data;
   init_start_stop_start_stop_buffers_data(data);
   for (const auto &item : data) {
      Process::ProcessChannelMode channelMode1 = std::get<0>(item);
      Process::ProcessChannelMode channelMode2 = std::get<1>(item);
      Process process;
      process.setProcessChannelMode(channelMode1);
      process.start(APP_FILENAME(ProcessHangApp));
      ASSERT_TRUE(process.waitForReadyRead()) << process.getErrorString().toStdString();
      if (channelMode1 == Process::ProcessChannelMode::SeparateChannels ||
          channelMode1 == Process::ProcessChannelMode::ForwardedOutputChannel) {
         process.setReadChannel(Process::ProcessChannel::StandardError);
         if (process.getBytesAvailable() == 0) {
            ASSERT_TRUE(process.waitForReadyRead());
         }
         process.setReadChannel(Process::ProcessChannel::StandardOutput);
      }
      // We want to test that the write buffer still has bytes after the child
      // exiting. We do that by writing to a child process that never reads. We
      // just have to write more data than a pipe can hold, so that even if
      // Process finds the pipe writable (during waitForFinished() or in the
      // WindowsPipeWriter thread), some data will remain. The worst case I know
      // of is Linux, which defaults to 64 kB of buffer.
      process.write(ByteArray(128 * 1024, 'a'));
      ASSERT_TRUE(process.getBytesToWrite() > 0);
      process.kill();
      ASSERT_TRUE(process.waitForFinished());

#ifndef PDK_OS_WIN
      // confirm that our buffers are still full
      // Note: this doesn't work on Windows because our buffers are drained into
      // WindowsPipeWriter before being sent to the child process.
      ASSERT_TRUE(process.getBytesToWrite() > 0);
      ASSERT_TRUE(process.getBytesAvailable() > 0); // channelMode1 is not ForwardedChannels
      if (channelMode1 == Process::ProcessChannelMode::SeparateChannels || 
          channelMode1 == Process::ProcessChannelMode::ForwardedOutputChannel) {
         process.setReadChannel(Process::ProcessChannel::StandardError);
         ASSERT_TRUE(process.getBytesAvailable() > 0);
         process.setReadChannel(Process::ProcessChannel::StandardOutput);
      }
#endif
      process.setProcessChannelMode(channelMode2);
      process.start(APP_FILENAME(ProcessEcho2App), IoDevice::OpenModes(IoDevice::OpenMode::ReadWrite) | IoDevice::OpenMode::Text);

      // the buffers should now be empty
      ASSERT_EQ(process.getBytesToWrite(), pdk::pint64(0));
      ASSERT_EQ(process.getBytesAvailable(), pdk::pint64(0));
      process.setReadChannel(Process::ProcessChannel::StandardError);
      ASSERT_EQ(process.getBytesAvailable(), pdk::pint64(0));
      process.setReadChannel(Process::ProcessChannel::StandardOutput);

      process.write("line3\n");
      process.closeWriteChannel();
      ASSERT_TRUE(process.waitForFinished());
      ASSERT_EQ(process.getExitStatus(), Process::ExitStatus::NormalExit);
      ASSERT_EQ(process.getExitCode(), 0);

      if (channelMode2 == Process::ProcessChannelMode::MergedChannels) {
         ASSERT_EQ(process.readAll(), ByteArray("lliinnee33\n\n"));
      } else if (channelMode2 != Process::ProcessChannelMode::ForwardedChannels) {
         ASSERT_EQ(process.readAllStandardOutput(), ByteArray("line3\n"));
         if (channelMode2 == Process::ProcessChannelMode::SeparateChannels) {
            ASSERT_EQ(process.readAllStandardError(), ByteArray("line3\n"));
         }
      }
   }
   PDKTEST_END_APP_CONTEXT();
}

namespace {

void init_process_events_ina_ready_read_slot_data(std::list<bool> &data)
{
   data.push_back(false);
   data.push_back(true);
}

} // anonymous namespace

TEST_F(ProcessTest, testProcessEventsInAReadyReadSlot)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   std::list<bool> data;
   init_process_events_ina_ready_read_slot_data(data);
   for (bool callWaitForReadyRead : data) {
      Process process;
      process.connectReadyReadStandardOutputSignal([](){

      }, PDK_RETRIEVE_APP_INSTANCE());
      process.start(APP_FILENAME(ProcessEchoApp));
      ASSERT_TRUE(process.waitForStarted());
      const ByteArray data(156, 'x');
      process.write(data.getConstRawData(), data.size() + 1);
      if (callWaitForReadyRead) {
         ASSERT_TRUE(process.waitForReadyRead());
      }
      if (process.getState() == Process::ProcessState::Running) {
         ASSERT_TRUE(process.waitForFinished());
      }
   }
   PDKTEST_END_APP_CONTEXT();
}

int main(int argc, char **argv)
{
   sg_argc = argc;
   sg_argv = argv;
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
