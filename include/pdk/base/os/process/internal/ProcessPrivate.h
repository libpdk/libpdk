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

#ifndef PDK_M_BASE_OS_PROCESS_INTERNAL_PROCESS_PRIVATE_H
#define PDK_M_BASE_OS_PROCESS_INTERNAL_PROCESS_PRIVATE_H

#include "pdk/base/os/process/Process.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/utils/SharedData.h"
#include "pdk/base/io/internal/IoDevicePrivate.h"
#include "pdk/kernel/SocketNotifier.h"
#include <map>
#include <unordered_map>
#include <mutex>

#ifdef pdk_OS_WIN
#include "pdk/global/Windows.h"
using PDK_PIPE = HANDLE;
#define INVALID_PDK_PIPE INVALID_HANDLE_VALUE
#else
using PDK_PIPE = int;
#define INVALID_PDK_PIPE -1
#endif

// forward declare with namespace
namespace pdk {
namespace kernel {
class Timer;
} // kernel
} // pdk

namespace pdk {
namespace os {
namespace process {
namespace internal {

using pdk::utils::SharedData;
using pdk::kernel::Timer;
using pdk::lang::String;

#ifdef PDK_OS_WIN
#else
using ProcEnvKey = ByteArray;

class ProcEnvValue
{
public:
   ProcEnvValue()
   {}
   
   ProcEnvValue(const ProcEnvValue &other)
   {
      *this = other;
   }
   
   explicit ProcEnvValue(const String &value)
      : m_stringValue(value)
   {}
   
   explicit ProcEnvValue(const ByteArray &value)
      : m_byteValue(value)
   {}
   
   bool operator==(const ProcEnvValue &other) const
   {
      return m_byteValue.isEmpty() && other.m_byteValue.isEmpty()
            ? m_stringValue == other.m_stringValue
            : getBytes() == other.getBytes();
   }
   
   ByteArray getBytes() const
   {
      if (m_byteValue.isEmpty() && !m_stringValue.isEmpty()) {
         m_byteValue = m_stringValue.toLocal8Bit();
      }
      return m_byteValue;
   }
   
   String getString() const
   {
      if (m_stringValue.isEmpty() && !m_byteValue.isEmpty()) {
         m_stringValue = String::fromLocal8Bit(m_byteValue);
      }
      return m_stringValue;
   }
   
   mutable ByteArray m_byteValue;
   mutable String m_stringValue;
};

#endif

class ProcessEnvironmentPrivate: public SharedData
{
public:
   using Key = ProcEnvKey;
   using Value = ProcEnvValue;
#ifdef PDK_OS_WIN
   inline Key prepareName(const String &name) const
   {
      return Key(name);
   }
   
   inline String nameToString(const Key &name) const
   {
      return name;
   }
   
   inline Value prepareValue(const String &value) const
   {
      return value;
   }
   
   inline String valueToString(const Value &value) const
   {
      return value;
   }
   
   struct OrderedMutexLocker
   {
      OrderedMutexLocker(const ProcessEnvironmentPrivate *,
                         const ProcessEnvironmentPrivate *)
      {}
   };
#else
   inline Key prepareName(const String &name) const
   {
      Key &ent = m_nameMap[name];
      if (ent.isEmpty()) {
         ent = name.toLocal8Bit();
      }
      return ent;
   }
   
   inline String nameToString(const Key &name) const
   {
      const String sname = String::fromLocal8Bit(name);
      m_nameMap[sname] = name;
      return sname;
   }
   
   inline Value prepareValue(const String &value) const
   {
      return Value(value);
   }
   
   inline String valueToString(const Value &value) const
   {
      return value.getString();
   }
   
   ProcessEnvironmentPrivate() = default;
   
   ProcessEnvironmentPrivate(const ProcessEnvironmentPrivate &other)
   {
      // This being locked ensures that the functions that only assign
      // d pointers don't need explicit locking.
      // We don't need to lock our own mutex, as this object is new and
      // consequently not shared. For the same reason, non-const methods
      // do not need a lock, as they detach objects (however, we need to
      // ensure that they really detach before using prepareName()).
      // is really need this
      std::lock_guard locker(other.m_mutex);
      m_vars = other.m_vars;
      m_nameMap = other.m_nameMap;
   }
#endif
   
   using Map = std::map<Key, Value>;
   Map m_vars;
   
#ifdef PDK_OS_UNIX
   using NameHash = std::unordered_map<String, Key>;
   mutable NameHash m_nameMap;
   
   mutable std::mutex m_mutex;
#endif
   
   static ProcessEnvironment fromList(const StringList &list);
   StringList toList() const;
   StringList getKeys() const;
   void insert(const ProcessEnvironmentPrivate &other);
};

} // internal
} // process
} // os
} // pdk

namespace pdk {
namespace utils {

using pdk::os::process::internal::ProcessEnvironmentPrivate;

template<>
inline void SharedDataPointer<ProcessEnvironmentPrivate>::detach()
{
   if (m_implPtr && m_implPtr->m_ref.load() == 1) {
      return;
   }   
   ProcessEnvironmentPrivate *x = (m_implPtr ? new ProcessEnvironmentPrivate(*m_implPtr)
                                             : new ProcessEnvironmentPrivate);
   x->m_ref.ref();
   if (m_implPtr && !m_implPtr->m_ref.deref()) {
      delete m_implPtr;
   }
   m_implPtr = x;
}

} // utils
} // pdk

namespace pdk {
namespace os {
namespace process {
namespace internal {

using pdk::kernel::SocketNotifier;
using pdk::io::internal::IoDevicePrivate;

class ProcessPrivate : public IoDevicePrivate
{
public:
   PDK_DECLARE_PUBLIC(Process);
   
   struct Channel
   {
      enum ProcessChannelType
      {
         Normal = 0,
         PipeSource = 1,
         PipeSink = 2,
         Redirect = 3
         // if you add "= 4" here, increase the number of bits below
      };
      
      Channel() 
         : m_process(nullptr), 
           m_notifier(nullptr), 
           m_type(Normal), 
           m_closed(false), 
           m_append(false)
      {
         m_pipe[0] = INVALID_PDK_PIPE;
         m_pipe[1] = INVALID_PDK_PIPE;
#ifdef PDK_OS_WIN
         reader = 0;
#endif
      }
      
      void clear();
      
      Channel &operator=(const String &fileName)
      {
         clear();
         m_file = fileName;
         m_type = fileName.isEmpty() ? Normal : Redirect;
         return *this;
      }
      
      void pipeTo(ProcessPrivate *other)
      {
         clear();
         m_process = other;
         m_type = PipeSource;
      }
      
      void pipeFrom(ProcessPrivate *other)
      {
         clear();
         m_process = other;
         m_type = PipeSink;
      }
      
      String m_file;
      ProcessPrivate *m_process;
      SocketNotifier *m_notifier;
#ifdef PDK_OS_WIN
      union {
         WindowsPipeReader *m_reader;
         WindowsPipeWriter *m_writer;
      };
#endif
      PDK_PIPE m_pipe[2];
      
      unsigned m_type : 2;
      bool m_closed : 1;
      bool m_append : 1;
   };
   
   ProcessPrivate();
   virtual ~ProcessPrivate();
   
   // private slots
   bool canReadStandardOutputPrivateSlot();
   bool canReadStandardErrorPrivateSlot();
   bool canWritePrivateSlot();
   bool startupNotificationPrivateSlot();
   bool processDiedPrivateSlot();
   
   Process::ProcessChannelMode m_processChannelMode;
   Process::InputChannelMode m_inputChannelMode;
   Process::ProcessError m_processError;
   Process::ProcessState m_processState;
   String m_workingDirectory;
   PDK_PID m_pid;
   int m_sequenceNumber;
   
   bool m_dying;
   bool m_emittedReadyRead;
   bool m_emittedBytesWritten;
   
   Channel m_stdinChannel;
   Channel m_stdoutChannel;
   Channel m_stderrChannel;
   bool openChannel(Channel &channel);
   void closeChannel(Channel *channel);
   void closeWriteChannel();
   bool tryReadFromChannel(Channel *channel); // obviously, only stdout and stderr
   
   String m_program;
   StringList m_arguments;
#if defined(PDK_OS_WIN)
   String nativeArguments;
   Process::CreateProcessArgumentModifier modifyCreateProcessArgs;
#endif
   ProcessEnvironment m_environment;
   
   PDK_PIPE m_childStartedPipe[2];
   void destroyPipe(PDK_PIPE pipe[2]);
   
   SocketNotifier *m_startupSocketNotifier;
   SocketNotifier *m_deathNotifier;
   
   int m_forkfd;
   
#ifdef PDK_OS_WIN
   Timer *stdinWriteTrigger;
   WinEventNotifier *processFinishedNotifier;
#endif
   
   void start(IoDevice::OpenModes mode);
   void startProcess();
#if defined(PDK_OS_UNIX)
   void execChild(const char *workingDirectory, char **argv, char **envp);
#endif
   bool processStarted(String *errorMessage = nullptr);
   void terminateProcess();
   void killProcess();
   void findExitCode();
#ifdef PDK_OS_UNIX
   bool waitForDeadChild();
#endif
#ifdef PDK_OS_WIN
   bool callCreateProcess(Process::CreateProcessArguments *cpargs);
   bool drainOutputPipes();
   void flushPipeWriter();
   pdk::pint64 pipeWriterBytesToWrite() const;
#endif
   
   bool startDetached(pdk::pint64 *pPid);
   
   int m_exitCode;
   Process::ExitStatus m_exitStatus;
   bool m_crashed;
   
   bool waitForStarted(int msecs = 30000);
   bool waitForReadyRead(int msecs = 30000);
   bool waitForBytesWritten(int msecs = 30000);
   bool waitForFinished(int msecs = 30000);
   
   pdk::pint64 bytesAvailableInChannel(const Channel *channel) const;
   pdk::pint64 readFromChannel(const Channel *channel, char *data, pdk::pint64 maxLength);
   bool writeToStdin();
   
   void cleanup();
   void setError(Process::ProcessError error, const String &description = String());
   void setErrorAndEmit(Process::ProcessError error, const String &description = String());
};

} // internal
} // process
} // os
} // pdk

PDK_DECLARE_TYPEINFO(pdk::os::process::internal::ProcEnvValue, PDK_MOVABLE_TYPE);

#endif // PDK_M_BASE_OS_PROCESS_INTERNAL_PROCESS_PRIVATE_H
