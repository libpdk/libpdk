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
// Created by softboy on 2018/04/20.

#ifndef PDK_M_BASE_OS_PROCESS_PROCESS_H
#define PDK_M_BASE_OS_PROCESS_PROCESS_H

#include "pdk/base/io/IoDevice.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/lang/String.h"
#include "pdk/utils/SharedData.h"

#include <functional>

namespace pdk {
namespace os {
namespace process {

using pdk::utils::SharedDataPointer;
using pdk::lang::String;
using pdk::ds::ByteArray;
using pdk::ds::StringList;
using pdk::kernel::Object;
using pdk::io::IoDevice;

// forward declare class with namespace
namespace internal {
class ProcessPrivate;
class ProcessEnvironmentPrivate;
} // internal

} // process
} // os
} // pdk

#if !defined(PDK_OS_WIN)
using PDK_PID = pdk::pint64;
#else

using PDK_PID = struct _PROCESS_INFORMATION *;
using PDK_SECURITY_ATTRIBUTES = struct _SECURITY_ATTRIBUTES;
using PDK_STARTUPINFO = struct _STARTUPINFOW;

#endif

namespace pdk {
namespace os {
namespace process {

using internal::ProcessPrivate;
using internal::ProcessEnvironmentPrivate;

class PDK_CORE_EXPORT ProcessEnvironment
{
public:
   ProcessEnvironment();
   ProcessEnvironment(const ProcessEnvironment &other);
   ~ProcessEnvironment();
   ProcessEnvironment &operator=(ProcessEnvironment && other) noexcept
   {
      swap(other);
      return *this;
   }
   
   ProcessEnvironment &operator=(const ProcessEnvironment &other);
   
   void swap(ProcessEnvironment &other) noexcept
   {
      std::swap(m_implPtr, other.m_implPtr);
   }
   
   bool operator==(const ProcessEnvironment &other) const;
   inline bool operator!=(const ProcessEnvironment &other) const
   {
      return !(*this == other);
   }
   
   bool isEmpty() const;
   void clear();
   
   bool contains(const String &name) const;
   void insert(const String &name, const String &value);
   void remove(const String &name);
   String getValue(const String &name, const String &defaultValue = String()) const;
   
   StringList toStringList() const;
   
   StringList getKeys() const;
   
   void insert(const ProcessEnvironment &e);
   
   static ProcessEnvironment getSystemEnvironment();
   
private:
   friend class ProcessPrivate;
   friend class ProcessEnvironmentPrivate;
   SharedDataPointer<ProcessEnvironmentPrivate> m_implPtr;
};

class PDK_CORE_EXPORT Process : public IoDevice
{
public:
   enum class ProcessError
   {
      FailedToStart, //### file not found, resource error
      Crashed,
      Timedout,
      ReadError,
      WriteError,
      UnknownError
   };
   
   enum class ProcessState
   {
      NotRunning,
      Starting,
      Running
   };
   
   enum class ProcessChannel
   {
      StandardOutput,
      StandardError
   };
   
   enum class ProcessChannelMode
   {
      SeparateChannels,
      MergedChannels,
      ForwardedChannels,
      ForwardedOutputChannel,
      ForwardedErrorChannel
   };
   
   enum class InputChannelMode
   {
      ManagedInputChannel,
      ForwardedInputChannel
   };
   
   enum class ExitStatus
   {
      NormalExit,
      CrashExit
   };
   
   using StartedHandlerType = void();
   using FinishedHandlerType = void(int exitCode, Process::ExitStatus exitStatus);
   using ErrorOccurredHandlerType = void(Process::ProcessError error);
   using StateChangedHandlerType = void(Process::ProcessState state);
   using ReadyReadStandardOutputHandlerType = void();
   using ReadyReadStandardErrorHandlerType = void();
   
   PDK_DEFINE_SIGNAL_ENUMS(Started, Finished, ErrorOccurred, StateChanged, 
                           ReadyReadStandardOutput, ReadyReadStandardError);
   
   PDK_DEFINE_SIGNAL_EMITTER(Started)
   PDK_DEFINE_SIGNAL_EMITTER(Finished)
   PDK_DEFINE_SIGNAL_EMITTER(ErrorOccurred)
   PDK_DEFINE_SIGNAL_EMITTER(StateChanged)
   PDK_DEFINE_SIGNAL_EMITTER(ReadyReadStandardOutput)
   PDK_DEFINE_SIGNAL_EMITTER(ReadyReadStandardError)
   
   PDK_DEFINE_SIGNAL_BINDER(Started)
   PDK_DEFINE_SIGNAL_BINDER(Finished)
   PDK_DEFINE_SIGNAL_BINDER(ErrorOccurred)
   PDK_DEFINE_SIGNAL_BINDER(StateChanged)
   PDK_DEFINE_SIGNAL_BINDER(ReadyReadStandardOutput)
   PDK_DEFINE_SIGNAL_BINDER(ReadyReadStandardError)
   
   explicit Process(Object *parent = nullptr);
   virtual ~Process();
   
   void start(const String &program, const StringList &arguments, OpenModes mode = OpenMode::ReadWrite);
#if !defined(PDK_NO_PROCESS_COMBINED_ARGUMENT_START)
   void start(const String &command, OpenModes mode = OpenMode::ReadWrite);
#endif
   void start(OpenModes mode = OpenMode::ReadWrite);
   bool startDetached(pdk::pint64 *pid = nullptr);
   bool open(OpenModes mode = OpenMode::ReadWrite) override;
   
   String getProgram() const;
   void setProgram(const String &program);
   
   StringList getArguments() const;
   void setArguments(const StringList & arguments);
   
   ProcessChannelMode getReadChannelMode() const;
   void setReadChannelMode(ProcessChannelMode mode);
   ProcessChannelMode getProcessChannelMode() const;
   void setProcessChannelMode(ProcessChannelMode mode);
   InputChannelMode getInputChannelMode() const;
   void setInputChannelMode(InputChannelMode mode);
   
   ProcessChannel getReadChannel() const;
   void setReadChannel(ProcessChannel channel);
   
   void closeReadChannel(ProcessChannel channel);
   void closeWriteChannel();
   
   void setStandardInputFile(const String &fileName);
   void setStandardOutputFile(const String &fileName, OpenModes mode = OpenMode::Truncate);
   void setStandardErrorFile(const String &fileName, OpenModes mode = OpenMode::Truncate);
   void setStandardOutputProcess(Process *destination);
   
#if defined(PDK_OS_WIN)
   String getNativeArguments() const;
   void setNativeArguments(const String &arguments);
   struct CreateProcessArguments
   {
      const wchar_t *m_applicationName;
      wchar_t *m_arguments;
      PDK_SECURITY_ATTRIBUTES *m_processAttributes;
      PDK_SECURITY_ATTRIBUTES *m_threadAttributes;
      bool m_inheritHandles;
      unsigned long m_flags;
      void *m_environment;
      const wchar_t *m_currentDirectory;
      PDK_STARTUPINFO *m_startupInfo;
      PDK_PID m_processInformation;
   };
   using CreateProcessArgumentModifier = std::function<void(CreateProcessArguments *)>;
   CreateProcessArgumentModifier createProcessArgumentsModifier() const;
   void setCreateProcessArgumentsModifier(CreateProcessArgumentModifier modifier);
#endif // PDK_OS_WIN
   
   String getWorkingDirectory() const;
   void setWorkingDirectory(const String &dir);
   
   void setProcessEnvironment(const ProcessEnvironment &environment);
   ProcessEnvironment getProcessEnvironment() const;
   
   Process::ProcessError getError() const;
   Process::ProcessState getState() const;
   
   // #### PDK_PID is a pointer on Windows and a value on Unix
   pdk::pint64 getProcessId() const;
   
   bool waitForStarted(int msecs = 30000);
   bool waitForReadyRead(int msecs = 30000) override;
   bool waitForBytesWritten(int msecs = 30000) override;
   bool waitForFinished(int msecs = 30000);
   
   ByteArray readAllStandardOutput();
   ByteArray readAllStandardError();
   
   int getExitCode() const;
   Process::ExitStatus getExitStatus() const;
   
   // IoDevice
   pdk::pint64 getBytesAvailable() const override; // ### remove trivial override
   pdk::pint64 getBytesToWrite() const override;
   bool isSequential() const override;
   bool canReadLine() const override; // ### remove trivial override
   void close() override;
   bool atEnd() const override; // ### remove trivial override
   
   static int execute(const String &program, const StringList &arguments);
   static int execute(const String &command);
   
   static bool startDetached(const String &program, const StringList &arguments,
                             const String &workingDirectory
                             , pdk::pint64 *pid = nullptr);
   static bool startDetached(const String &program, const StringList &arguments); // ### merge overloads
   static bool startDetached(const String &command);
   
   static StringList getSystemEnvironment();
   
   static String getNullDevice();
   
public:
   void terminate();
   void kill();
   
   void readyReadStandardOutput();
   void readyReadStandardError();
   
protected:
   void setProcessState(ProcessState state);
   
   virtual void setupChildProcess();
   
   // IoDevice
   pdk::pint64 readData(char *data, pdk::pint64 maxlength) override;
   pdk::pint64 writeData(const char *data, pdk::pint64 length) override;
   void processDiedPrivateSlot(int socket);
   void startupNotificationPrivateSlot(int socket);
   void canWritePrivateSlot(int socket);
   void canReadStandardOutputPrivateSlot(int socket);
   void canReadStandardErrorPrivateSlot(int socket);
private:
   PDK_DECLARE_PRIVATE(Process);
   PDK_DISABLE_COPY(Process);
};

} // process
} // os
} // pdk

#endif // PDK_M_BASE_OS_PROCESS_PROCESS_H
