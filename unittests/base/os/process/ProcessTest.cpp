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
using pdk::io::IoDevice;
using pdk::lang::Latin1String;
using pdk::lang::String;
using pdk::ds::StringList;
using pdk::ds::ByteArray;
using pdk::utils::ScopedPointer;
using pdk::kernel::Object;
using pdk::os::thread::Thread;
using pdk::time::Time;

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
