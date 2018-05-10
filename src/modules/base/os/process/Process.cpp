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

#include "pdk/base/io/Debug.h"
#include "pdk/base/io/fs/Dir.h"
#if defined(PDK_OS_WIN)
#include "pdk/kernel/Timer.h"
#endif

#include "pdk/base/os/process/Process.h"
#include "pdk/base/os/process/internal/ProcessPrivate.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/ds/internal/RingBufferPrivate.h"
#include "pdk/kernel/ElapsedTimer.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/Timer.h"
#include "pdk/global/Logging.h"

#if PDK_HAS_INCLUDE(<paths.h>)
#include <paths.h>
#endif

#ifdef PDK_OS_WIN
// @TODO include WinEventNotifier.h
#else
#include "pdk/kernel/internal/CoreUnixPrivate.h"
#endif

namespace pdk {
namespace os {
namespace process {

using pdk::ds::ByteArray;
using pdk::ds::StringList;
using pdk::lang::Latin1Character;
using pdk::ds::internal::RingBuffer;
using pdk::io::fs::Dir;
using pdk::io::IoDevice;
using pdk::kernel::ElapsedTimer;

#if defined(PDK_PROCESS_DEBUG)

#include "pdk/base/lang/String.h"
#include <ctype.h>

/*
 * Returns a human readable representation of the first len
 * characters in data.
 **/
namespace {

ByteArray pretty_debug(const char *data, int len, int maxSize)
{
   if (!data) {
      return "(null)";
   }
   ByteArray out;
   for (int i = 0; i < len && i < maxSize; ++i) {
      char c = data[i];
      if (isprint(c)) {
         out += c;
      } else {
         case '\n': out += "\\n"; break;
         case '\r': out += "\\r"; break;
         case '\t': out += "\\t"; break;
         default:
            char buf[5];
            pdk::snprintf(buf, sizeof(buf), "\\%3o", c);
            buf[4] = '\0';
            out += ByteArray(buf);
      }
   }
   if (len < maxSize) {
      out += "...";
   }
   return out;
}

} // anonymous namespace

#endif

namespace internal
{

StringList ProcessEnvironmentPrivate::toList() const
{
   StringList result;
   for (auto iter = m_vars.cbegin(), end = m_vars.cend(); iter != end; ++iter) {
      result << nameToString(iter->first) + Latin1Character('=') + valueToString(iter->second);
   }
   return result;
}

ProcessEnvironment ProcessEnvironmentPrivate::fromList(const StringList &list)
{
   ProcessEnvironment env;
   StringList::const_iterator iter = list.cbegin();
   StringList::const_iterator end = list.cend();
   for ( ; iter != end; ++iter) {
      int pos = iter->indexOf(Latin1Character('='), 1);
      if (pos < 1) {
         continue;
      }
      String value = iter->substring(pos + 1);
      String name = *iter;
      name.truncate(pos);
      env.insert(name, value);
   }
   return env;
}

StringList ProcessEnvironmentPrivate::getKeys() const
{
   StringList result;
   auto iter = m_vars.cbegin();
   const auto end = m_vars.cend();
   for ( ; iter != end; ++iter) {
      result << nameToString(iter->first);
   }
   return result;
}

void ProcessEnvironmentPrivate::insert(const ProcessEnvironmentPrivate &other)
{
   auto iter = other.m_vars.cbegin();
   const auto end = other.m_vars.cend();
   for ( ; iter != end; ++iter) {
      m_vars[iter->first] = iter->second;
   }
#ifdef PDK_OS_UNIX
   auto niter = other.m_nameMap.cbegin();
   const auto nend = other.m_nameMap.cend();
   for ( ; niter != nend; ++niter) {
      m_nameMap[niter->first] = niter->second;
   }
#endif
}

} // internal

ProcessEnvironment::ProcessEnvironment()
   : m_implPtr(nullptr)
{}

ProcessEnvironment::~ProcessEnvironment()
{}

ProcessEnvironment::ProcessEnvironment(const ProcessEnvironment &other)
   : m_implPtr(other.m_implPtr)
{}

ProcessEnvironment &ProcessEnvironment::operator =(const ProcessEnvironment &other)
{
   m_implPtr = other.m_implPtr;
   return *this;
}

bool ProcessEnvironment::operator==(const ProcessEnvironment &other) const
{
   if (m_implPtr == other.m_implPtr)
      return true;
   if (m_implPtr) {
      if (other.m_implPtr) {
         std::scoped_lock locker(m_implPtr->m_mutex, other.m_implPtr->m_mutex);
         return m_implPtr->m_vars == other.m_implPtr->m_vars;
      } else {
         return isEmpty();
      }
   } else {
      return other.isEmpty();
   }
}

bool ProcessEnvironment::isEmpty() const
{
   // Needs no locking, as no hash nodes are accessed
   return m_implPtr ? m_implPtr->m_vars.empty() : true;
}

void ProcessEnvironment::clear()
{
   if (m_implPtr) {
      m_implPtr->m_vars.clear();
   }
   // Unix: Don't clear m_implPtr->m_nameMap, as the environment is likely to be
   // re-populated with the same keys again.
}

bool ProcessEnvironment::contains(const String &name) const
{
   if (!m_implPtr) {
      return false;
   }
   
   std::lock_guard locker(m_implPtr->m_mutex);
   return m_implPtr->m_vars.find(m_implPtr->prepareName(name)) != m_implPtr->m_vars.end();
}

void ProcessEnvironment::insert(const String &name, const String &value)
{
   // our re-impl of detach() detaches from null
   m_implPtr.detach(); // detach before prepareName()
   m_implPtr->m_vars[m_implPtr->prepareName(name)] = m_implPtr->prepareValue(value);
}

void ProcessEnvironment::remove(const String &name)
{
   if (m_implPtr) {
      m_implPtr.detach(); // detach before prepareName()
      m_implPtr->m_vars.erase(m_implPtr->prepareName(name));
   }
}

String ProcessEnvironment::getValue(const String &name, const String &defaultValue) const
{
   if (!m_implPtr) {
      return defaultValue;
   }   
   std::lock_guard locker(m_implPtr->m_mutex);
   const auto iter = m_implPtr->m_vars.find(m_implPtr->prepareName(name));
   if (iter == m_implPtr->m_vars.cend()) {
      return defaultValue;
   }
   return m_implPtr->valueToString(iter->second);
}

StringList ProcessEnvironment::toStringList() const
{
   if (!m_implPtr) {
      return StringList();
   }
   std::lock_guard locker(m_implPtr->m_mutex);
   return m_implPtr->toList();
}

StringList ProcessEnvironment::getKeys() const
{
   if (!m_implPtr) {
      return StringList();
   }
   std::lock_guard locker(m_implPtr->m_mutex);
   return m_implPtr->getKeys();
}

void ProcessEnvironment::insert(const ProcessEnvironment &env)
{
   if (!env.m_implPtr) {
      return;
   }
   // our re-impl of detach() detaches from null
   std::lock_guard locker(env.m_implPtr->m_mutex);
   m_implPtr->insert(*env.m_implPtr);
}

namespace internal {

void ProcessPrivate::Channel::clear()
{
   switch (m_type) {
   case PipeSource:
      PDK_ASSERT(m_process);
      m_process->m_stdinChannel.m_type = Normal;
      m_process->m_stdinChannel.m_process = nullptr;
      break;
   case PipeSink:
      PDK_ASSERT(m_process);
      m_process->m_stdoutChannel.m_type = Normal;
      m_process->m_stdoutChannel.m_process = nullptr;
      break;
   }
   
   m_type = Normal;
   m_file.clear();
   m_process = nullptr;
}

ProcessPrivate::ProcessPrivate()
{
   m_readBufferChunkSize = PDK_RING_BUFFER_CHUNK_SIZE;
   m_writeBufferChunkSize = PDK_RING_BUFFER_CHUNK_SIZE;
   m_processChannelMode = Process::ProcessChannelMode::SeparateChannels;
   m_inputChannelMode = Process::InputChannelMode::ManagedInputChannel;
   m_processError = Process::ProcessError::UnknownError;
   m_processState = Process::ProcessState::NotRunning;
   m_pid = 0;
   m_sequenceNumber = 0;
   m_exitCode = 0;
   m_exitStatus = Process::ExitStatus::NormalExit;
   m_startupSocketNotifier = nullptr;
   m_deathNotifier = nullptr;
   m_childStartedPipe[0] = INVALID_PDK_PIPE;
   m_childStartedPipe[1] = INVALID_PDK_PIPE;
   m_forkfd = -1;
   m_crashed = false;
   m_dying = false;
   m_emittedReadyRead = false;
   m_emittedBytesWritten = false;
#ifdef PDK_OS_WIN
   m_stdinWriteTrigger = 0;
   m_processFinishedNotifier = nullptr;
#endif // PDK_OS_WIN
}

ProcessPrivate::~ProcessPrivate()
{
   if (m_stdinChannel.m_process) {
      m_stdinChannel.m_process->m_stdoutChannel.clear();
   }
   if (m_stdoutChannel.m_process) {
      m_stdoutChannel.m_process->m_stdoutChannel.clear();
   }
}

void ProcessPrivate::cleanup()
{
   getApiPtr()->setProcessState(Process::ProcessState::NotRunning);
#ifdef PDK_OS_WIN
   if (m_pid) {
      CloseHandle(m_pid->hThread);
      CloseHandle(m_pid->hProcess);
      delete m_pid;
      m_pid = nullptr;
   }
   if (m_stdinWriteTrigger) {
      delete m_stdinWriteTrigger;
      m_stdinWriteTrigger = nullptr;
   }
   if (m_processFinishedNotifier) {
      delete m_processFinishedNotifier;
      m_processFinishedNotifier = nullptr;
   }
   
#endif
   m_pid = 0;
   m_sequenceNumber = 0;
   m_dying = false;
   
   if (m_stdoutChannel.m_notifier) {
      delete m_stdoutChannel.m_notifier;
      m_stdoutChannel.m_notifier = nullptr;
   }
   if (m_stderrChannel.m_notifier) {
      delete m_stderrChannel.m_notifier;
      m_stderrChannel.m_notifier = nullptr;
   }
   if (m_stdinChannel.m_notifier) {
      delete m_stdinChannel.m_notifier;
      m_stdinChannel.m_notifier = nullptr;
   }
   if (m_startupSocketNotifier) {
      delete m_startupSocketNotifier;
      m_startupSocketNotifier = nullptr;
   }
   if (m_deathNotifier) {
      delete m_deathNotifier;
      m_deathNotifier = nullptr;
   }
   closeChannel(&m_stdoutChannel);
   closeChannel(&m_stderrChannel);
   closeChannel(&m_stdinChannel);
   destroyPipe(m_childStartedPipe);
#ifdef PDK_OS_UNIX
   if (m_forkfd != -1) {
      pdk::kernel::safe_close(m_forkfd);
   }
   m_forkfd = -1;
#endif
}

void ProcessPrivate::setError(Process::ProcessError error, const String &description)
{
   m_processError = error;
   if (description.isEmpty()) {
      switch (error) {
      case Process::ProcessError::FailedToStart:
         m_errorString = Process::tr("Process failed to start");
         break;
      case Process::ProcessError::Crashed:
         m_errorString = Process::tr("Process crashed");
         break;
      case Process::ProcessError::Timedout:
         m_errorString = Process::tr("Process operation timed out");
         break;
      case Process::ProcessError::ReadError:
         m_errorString = Process::tr("Error reading from process");
         break;
      case Process::ProcessError::WriteError:
         m_errorString = Process::tr("Error writing to process");
         break;
      case Process::ProcessError::UnknownError:
         m_errorString.clear();
         break;
      }
   } else {
      m_errorString = description;
   }
}

void ProcessPrivate::setErrorAndEmit(Process::ProcessError error, const String &description)
{
   PDK_Q(Process);
   PDK_ASSERT(error != Process::ProcessError::UnknownError);
   setError(error, description);
   // @TODO emit signal
}

bool ProcessPrivate::tryReadFromChannel(Channel *channel)
{
   PDK_Q(Process);
   if (channel->m_pipe[0] == INVALID_PDK_PIPE) {
      return false;
   }
   pdk::pint64 available = bytesAvailableInChannel(channel);
   if (available == 0) {
      available = 1;      // always try to read at least one byte
   }
   Process::ProcessChannel channelIdx = (channel == &m_stdoutChannel
                                         ? Process::ProcessChannel::StandardOutput
                                         : Process::ProcessChannel::StandardError);
   PDK_ASSERT(m_readBuffers.size() > size_t(channelIdx));
   RingBuffer &readBuffer = m_readBuffers[int(channelIdx)];
   char *ptr = readBuffer.reserve(available);
   pdk::pint64 readBytes = readFromChannel(channel, ptr, available);
   if (readBytes <= 0) {
      readBuffer.chop(available);
   }
   if (readBytes == -2) {
      // EWOULDBLOCK
      return false;
   }
   if (readBytes == -1) {
      setErrorAndEmit(Process::ProcessError::ReadError);
#if defined PDK_PROCESS_DEBUG
      debug_stream("ProcessPrivate::tryReadFromChannel(%d), failed to read from the process",
                   int(channel - &m_stdinChannel));
#endif
      return false;
   }
   if (readBytes == 0) {
      // EOF
      if (channel->m_notifier) {
         channel->m_notifier->setEnabled(false);
      }  
      closeChannel(channel);
#if defined PDK_PROCESS_DEBUG
      debug_stream("ProcessPrivate::tryReadFromChannel(%d), 0 bytes available",
                   int(channel - &m_stdinChannel));
#endif
      return false;
   }
#if defined Process_DEBUG
   debug_stream("ProcessPrivate::tryReadFromChannel(%d), read %d bytes from the process' output",
                int(channel - &m_stdinChannel), int(readBytes));
#endif
   
   if (channel->m_closed) {
      readBuffer.chop(readBytes);
      return false;
   }
   
   readBuffer.chop(available - readBytes);
   
   bool didRead = false;
   if (m_currentReadChannel == pdk::as_integer<Process::ProcessChannel>(channelIdx)) {
      didRead = true;
      if (!m_emittedReadyRead) {
         m_emittedReadyRead = true;
         // emit readyRead();
         m_emittedReadyRead = false;
      }
   }
   // emit channelReadyRead(int(channelIdx));
   if (channelIdx == Process::ProcessChannel::StandardOutput) {
      //emit readyReadStandardOutput(Process::PrivateSignal());
   } else {
      // emit readyReadStandardError(Process::PrivateSignal());
   }
   return didRead;
}

bool ProcessPrivate::canReadStandardOutputPrivateSlot()
{
   return tryReadFromChannel(&m_stdoutChannel);
}

bool ProcessPrivate::canReadStandardErrorPrivateSlot()
{
   return tryReadFromChannel(&m_stdoutChannel);
}

bool ProcessPrivate::canWritePrivateSlot()
{
   if (m_stdinChannel.m_notifier) {
      m_stdinChannel.m_notifier->setEnabled(false);
   }
   if (m_writeBuffer.isEmpty()) {
#if defined PDK_PROCESS_DEBUG
      debug_stream("ProcessPrivate::canWrite(), not writing anything (empty write buffer).");
#endif
      return false;
   }
   
   const bool writeSucceeded = writeToStdin();
   
   if (m_stdinChannel.m_notifier && !m_writeBuffer.isEmpty()) {
      m_stdinChannel.m_notifier->setEnabled(true);
   }
   if (m_writeBuffer.isEmpty() && m_stdinChannel.m_closed) {
      closeWriteChannel();
   }
   return writeSucceeded;
}

bool ProcessPrivate::processDiedPrivateSlot()
{
#if defined PDK_PROCESS_DEBUG
   debug_stream("ProcessPrivate::processDiedPrivateSlot()");
#endif
#ifdef PDK_OS_UNIX
   if (!waitForDeadChild()) {
      return false;
   }
   
#endif
#ifdef PDK_OS_WIN
   if (processFinishedNotifier) {
      processFinishedNotifier->setEnabled(false);
   }
   drainOutputPipes();
#endif
   
   // the process may have died before it got a chance to report that it was
   // either running or stopped, so we will call _PDK_startupNotification() and
   // give it a chance to emit started() or errorOccurred(FailedToStart).
   if (m_processState == Process::ProcessState::Starting) {
      if (!startupNotificationPrivateSlot()) {
         return true;
      }
   }
   
   if (m_dying) {
      // at this point we know the process is dead. prevent
      // reentering this slot recursively by calling waitForFinished()
      // or opening a dialog inside slots connected to the readyRead
      // signals emitted below.
      return true;
   }
   m_dying = true;
   // in case there is data in the pipe line and this slot by chance
   // got called before the read notifications, call these two slots
   // so the data is made available before the process dies.
   canReadStandardOutputPrivateSlot();
   canReadStandardErrorPrivateSlot();
   findExitCode();
   if (m_crashed) {
      m_exitStatus = Process::ExitStatus::CrashExit;
      setErrorAndEmit(Process::ProcessError::Crashed);
   }
   bool wasRunning = (m_processState == Process::ProcessState::Running);
   cleanup();
   if (wasRunning) {
      // we received EOF now:
      // emit readChannelFinished();
      // in the future:
      // emit standardOutputClosed();
      // emit standardErrorClosed();
      
      // emit finished(exitCode);
      // emit finished(exitCode, exitStatus);
   }
#if defined PDK_PROCESS_DEBUG
   debug_stream("ProcessPrivate::processDiedPrivateSlot() process is dead");
#endif
   return true;
}

bool ProcessPrivate::startupNotificationPrivateSlot()
{
   PDK_Q(Process);
#if defined PDK_PROCESS_DEBUG
   debug_stream("ProcessPrivate::startupNotification()");
#endif
   
   if (m_startupSocketNotifier) {
      m_startupSocketNotifier->setEnabled(false);
   }
   
   String errorMessage;
   if (processStarted(&errorMessage)) {
      apiPtr->setProcessState(Process::ProcessState::Running);
      // emit started(Process::PrivateSignal());
      return true;
   }
   
   apiPtr->setProcessState(Process::ProcessState::NotRunning);
   setErrorAndEmit(Process::ProcessError::FailedToStart, errorMessage);
#ifdef PDK_OS_UNIX
   // make sure the process manager removes this entry
   waitForDeadChild();
   findExitCode();
#endif
   cleanup();
   return false;
}

void ProcessPrivate::closeWriteChannel()
{
#if defined PDK_PROCESS_DEBUG
   debug_stream("ProcessPrivate::closeWriteChannel()");
#endif
   if (m_stdinChannel.m_notifier) {
      delete m_stdinChannel.m_notifier;
      m_stdinChannel.m_notifier = 0;
   }
#ifdef PDK_OS_WIN
   // ### Find a better fix, feeding the process little by little
   // instead.
   flushPipeWriter();
#endif
   closeChannel(&m_stdinChannel);
}

} // internal

Process::Process(Object *parent)
   : IoDevice(*new ProcessPrivate, parent)
{
#if defined PDK_PROCESS_DEBUG
   debug_stream("Process::Process(%p)", parent);
#endif
}

Process::~Process()
{
   PDK_D(Process);
   if (implPtr->m_processState != ProcessState::NotRunning) {
      warning_stream().nospace()
            << "Process: Destroyed while process (" << Dir::toNativeSeparators(getProgram()) << ") is still running.";
      kill();
      waitForFinished();
   }
#ifdef PDK_OS_UNIX
   // make sure the process manager removes this entry
   implPtr->findExitCode();
#endif
   implPtr->cleanup();
}

Process::ProcessChannelMode Process::getReadChannelMode() const
{
   return getProcessChannelMode();
}

void Process::setReadChannelMode(ProcessChannelMode mode)
{
   setProcessChannelMode(mode);
}

Process::ProcessChannelMode Process::getProcessChannelMode() const
{
   PDK_D(const Process);
   return implPtr->m_processChannelMode;
}

void Process::setProcessChannelMode(ProcessChannelMode mode)
{
   PDK_D(Process);
   implPtr->m_processChannelMode = mode;
}

Process::InputChannelMode Process::getInputChannelMode() const
{
   PDK_D(const Process);
   return implPtr->m_inputChannelMode;
}

void Process::setInputChannelMode(InputChannelMode mode)
{
   PDK_D(Process);
   implPtr->m_inputChannelMode = mode;
}

Process::ProcessChannel Process::getReadChannel() const
{
   PDK_D(const Process);
   return ProcessChannel(implPtr->m_currentReadChannel);
}

void Process::setReadChannel(ProcessChannel channel)
{
   IoDevice::setCurrentReadChannel(int(channel));
}

void Process::closeReadChannel(ProcessChannel channel)
{
   PDK_D(Process);
   if (channel == ProcessChannel::StandardOutput) {
      implPtr->m_stdoutChannel.m_closed = true;
   } else {
      implPtr->m_stderrChannel.m_closed = true;
   }
}

void Process::closeWriteChannel()
{
   PDK_D(Process);
   implPtr->m_stdinChannel.m_closed = true; // closing
   if (implPtr->m_writeBuffer.isEmpty()) {
      implPtr->closeWriteChannel();
   }
}

void Process::setStandardInputFile(const String &fileName)
{
   PDK_D(Process);
   implPtr->m_stdinChannel = fileName;
}

void Process::setStandardOutputFile(const String &fileName, OpenModes mode)
{
   PDK_ASSERT(mode == OpenMode::Append || mode == OpenMode::Truncate);
   PDK_D(Process);
   
   implPtr->m_stdoutChannel = fileName;
   implPtr->m_stdoutChannel.m_append = mode == OpenMode::Append;
}

void Process::setStandardErrorFile(const String &fileName, OpenModes mode)
{
   PDK_ASSERT(mode == OpenMode::Append || mode == OpenMode::Truncate);
   PDK_D(Process);
   
   implPtr->m_stderrChannel = fileName;
   implPtr->m_stderrChannel.m_append = mode == OpenMode::Append;
}

void Process::setStandardOutputProcess(Process *destination)
{
   ProcessPrivate *dfrom = getImplPtr();
   ProcessPrivate *dto = destination->getImplPtr();
   dfrom->m_stdoutChannel.pipeTo(dto);
   dto->m_stdinChannel.pipeFrom(dfrom);
}

#if defined(PDK_OS_WIN)

String Process::nativeArguments() const
{
   PDK_D(const Process);
   return implPtr->m_nativeArguments;
}

void Process::setNativeArguments(const String &arguments)
{
   PDK_D(Process);
   implPtr->m_nativeArguments = arguments;
}

Process::CreateProcessArgumentModifier Process::createProcessArgumentsModifier() const
{
   PDK_D(const Process);
   return implPtr->m_modifyCreateProcessArgs;
}

void Process::setCreateProcessArgumentsModifier(CreateProcessArgumentModifier modifier)
{
   PDK_D(Process);
   implPtr->m_modifyCreateProcessArgs = modifier;
}

#endif

String Process::getWorkingDirectory() const
{
   PDK_D(const Process);
   return implPtr->m_workingDirectory;
}

void Process::setWorkingDirectory(const String &dir)
{
   PDK_D(Process);
   implPtr->m_workingDirectory = dir;
}


pdk::pint64 Process::getProcessId() const
{
   PDK_D(const Process);
#ifdef PDK_OS_WIN
   return implPtr->m_pid ? implPtr->m_pid->dwProcessId : 0;
#else
   return implPtr->m_pid;
#endif
}

bool Process::canReadLine() const
{
   return IoDevice::canReadLine();
}

void Process::close()
{
   PDK_D(Process);
   // emit aboutToClose();
   while (waitForBytesWritten(-1))
      ;
   kill();
   waitForFinished(-1);
   implPtr->setWriteChannelCount(0);
   IoDevice::close();
}

bool Process::atEnd() const
{
   return IoDevice::atEnd();
}

bool Process::isSequential() const
{
   return true;
}

pdk::pint64 Process::getBytesAvailable() const
{
   return IoDevice::getBytesAvailable();
}

pdk::pint64 Process::getBytesToWrite() const
{
   pdk::pint64 size = IoDevice::getBytesToWrite();
#ifdef PDK_OS_WIN
   size += getImplPtr()->getPipeWriterBytesToWrite();
#endif
   return size;
}

Process::ProcessError Process::getError() const
{
   PDK_D(const Process);
   return implPtr->m_processError;
}

Process::ProcessState Process::getState() const
{
   PDK_D(const Process);
   return implPtr->m_processState;
}

void Process::setProcessEnvironment(const ProcessEnvironment &environment)
{
   PDK_D(Process);
   implPtr->m_environment = environment;
}

ProcessEnvironment Process::getProcessEnvironment() const
{
   PDK_D(const Process);
   return implPtr->m_environment;
}

bool Process::waitForStarted(int msecs)
{
   PDK_D(Process);
   if (implPtr->m_processState == Process::ProcessState::Starting) {
      return implPtr->waitForStarted(msecs);
   }
   return implPtr->m_processState == Process::ProcessState::Running;
}

bool Process::waitForReadyRead(int msecs)
{
   PDK_D(Process);
   
   if (implPtr->m_processState == Process::ProcessState::NotRunning) {
      return false;
   }
   
   if (implPtr->m_currentReadChannel == pdk::as_integer<ProcessChannel>(ProcessChannel::StandardOutput) && 
       implPtr->m_stdoutChannel.m_closed) {
      return false;
   }
   
   if (implPtr->m_currentReadChannel == pdk::as_integer<ProcessChannel>(ProcessChannel::StandardError) &&
       implPtr->m_stderrChannel.m_closed) {
      return false;
   }
   
   return implPtr->waitForReadyRead(msecs);
}

bool Process::waitForBytesWritten(int msecs)
{
   PDK_D(Process);
   if (implPtr->m_processState == Process::ProcessState::NotRunning)
      return false;
   if (implPtr->m_processState == Process::ProcessState::Starting) {
      ElapsedTimer stopWatch;
      stopWatch.start();
      bool started = waitForStarted(msecs);
      if (!started) {
         return false;
      }
      msecs = pdk::io::internal::subtract_from_timeout(msecs, stopWatch.getElapsed());
   }
   
   return implPtr->waitForBytesWritten(msecs);
}

bool Process::waitForFinished(int msecs)
{
   PDK_D(Process);
   if (implPtr->m_processState == Process::ProcessState::NotRunning)
      return false;
   if (implPtr->m_processState == Process::ProcessState::Starting) {
      ElapsedTimer stopWatch;
      stopWatch.start();
      bool started = waitForStarted(msecs);
      if (!started) {
         return false;
      }
      
      msecs = pdk::io::internal::subtract_from_timeout(msecs, stopWatch.getElapsed());
   }
   return implPtr->waitForFinished(msecs);
}

void Process::setProcessState(ProcessState state)
{
   PDK_D(Process);
   if (implPtr->m_processState == state) {
      return;
   }
   implPtr->m_processState = state;
   // emit stateChanged(state, PrivateSignal());
}

void Process::setupChildProcess()
{
}

pdk::pint64 Process::readData(char *data, pdk::pint64 maxlen)
{
   PDK_D(Process);
   PDK_UNUSED(data);
   if (!maxlen) {
      return 0;
   }
   if (implPtr->m_processState == Process::ProcessState::NotRunning) {
      return -1;              // EOF
   }
   return 0;
}

pdk::pint64 Process::writeData(const char *data, pdk::pint64 len)
{
   PDK_D(Process);
   
   if (implPtr->m_stdinChannel.m_closed) {
#if defined PDK_PROCESS_DEBUG
      debug_stream("Process::writeData(%p \"%s\", %lld) == 0 (write channel closing)",
                   data, pretty_debug(data, len, 16).getConstRawData(), len);
#endif
      return 0;
   }
   
#if defined(PDK_OS_WIN)
   if (!implPtr->m_stdinWriteTrigger) {
      implPtr->m_stdinWriteTrigger = new Timer;
      implPtr->m_stdinWriteTrigger->setSingleShot(true);
      // @ TODO connect signal and slot
   }
#endif
   
   implPtr->m_writeBuffer.append(data, len);
#ifdef PDK_OS_WIN
   if (!implPtr->m_stdinWriteTrigger->isActive())
      implPtr->m_stdinWriteTrigger->start();
#else
   if (implPtr->m_stdinChannel.m_notifier)
      implPtr->m_stdinChannel.m_notifier->setEnabled(true);
#endif
#if defined PDK_PROCESS_DEBUG
   debug_stream("Process::writeData(%p \"%s\", %lld) == %lld (written to buffer)",
                data, pretty_debug(data, len, 16).getConstRawData(), len, len);
#endif
   return len;
}

ByteArray Process::readAllStandardOutput()
{
   ProcessChannel tmp = getReadChannel();
   setReadChannel(ProcessChannel::StandardOutput);
   ByteArray data = readAll();
   setReadChannel(tmp);
   return data;
}


ByteArray Process::readAllStandardError()
{
   ProcessChannel tmp = getReadChannel();
   setReadChannel(ProcessChannel::StandardError);
   ByteArray data = readAll();
   setReadChannel(tmp);
   return data;
}

void Process::start(const String &program, const StringList &arguments, OpenModes mode)
{
   PDK_D(Process);
   if (implPtr->m_processState != ProcessState::NotRunning) {
      warning_stream("Process::start: Process is already running");
      return;
   }
   if (program.isEmpty()) {
      implPtr->setErrorAndEmit(Process::ProcessError::FailedToStart, tr("No program defined"));
      return;
   }
   
   implPtr->m_program = program;
   implPtr->m_arguments = arguments;
   
   implPtr->start(mode);
}

void Process::start(OpenModes mode)
{
   PDK_D(Process);
   if (implPtr->m_processState != ProcessState::NotRunning) {
      warning_stream("Process::start: Process is already running");
      return;
   }
   if (implPtr->m_program.isEmpty()) {
      implPtr->setErrorAndEmit(Process::ProcessError::FailedToStart, tr("No program defined"));
      return;
   }
   
   implPtr->start(mode);
}

bool Process::startDetached(pdk::pint64 *pid)
{
   PDK_D(Process);
   if (implPtr->m_processState != ProcessState::NotRunning) {
      warning_stream("Process::startDetached: Process is already running");
      return false;
   }
   if (implPtr->m_program.isEmpty()) {
      implPtr->setErrorAndEmit(ProcessError::FailedToStart, tr("No program defined"));
      return false;
   }
   return implPtr->startDetached(pid);
}

bool Process::open(OpenModes mode)
{
   PDK_D(Process);
   if (implPtr->m_processState != ProcessState::NotRunning) {
      warning_stream("Process::start: Process is already running");
      return false;
   }
   if (implPtr->m_program.isEmpty()) {
      warning_stream("Process::start: program not set");
      return false;
   }
   implPtr->start(mode);
   return true;
}

namespace internal {

void ProcessPrivate::start(IoDevice::OpenModes mode)
{
   PDK_Q(Process);
#if defined PDK_PROCESS_DEBUG
   debug_stream() << "Process::start(" << m_program << ',' << m_arguments << ',' << m_mode << ')';
#endif
   if (m_stdinChannel.m_type != ProcessPrivate::Channel::Normal)
      mode &= ~pdk::as_integer<IoDevice::OpenMode>(IoDevice::OpenMode::WriteOnly);     // not open for writing
   if (m_stdoutChannel.m_type != ProcessPrivate::Channel::Normal &&
       (m_stderrChannel.m_type != ProcessPrivate::Channel::Normal ||
        m_processChannelMode == Process::ProcessChannelMode::MergedChannels))
      mode &= ~pdk::as_integer<IoDevice::OpenMode>(IoDevice::OpenMode::ReadOnly);      // not open for reading
   if (mode == 0) {
      mode = IoDevice::OpenMode::Unbuffered;
   }
   if ((mode & IoDevice::OpenMode::ReadOnly) == 0) {
      if (m_stdoutChannel.m_type == ProcessPrivate::Channel::Normal) {
         apiPtr->setStandardOutputFile(apiPtr->getNullDevice());
      }
      if (m_stderrChannel.m_type == ProcessPrivate::Channel::Normal
          && m_processChannelMode != Process::ProcessChannelMode::MergedChannels) {
         apiPtr->setStandardErrorFile(apiPtr->getNullDevice());
      }
   }
   apiPtr->IoDevice::open(mode);
   
   if (apiPtr->isReadable() && 
       m_processChannelMode != Process::ProcessChannelMode::MergedChannels) {
      setReadChannelCount(2);
   }
   
   
   m_stdinChannel.m_closed = false;
   m_stdoutChannel.m_closed = false;
   m_stderrChannel.m_closed = false;
   
   m_exitCode = 0;
   m_exitStatus = Process::ExitStatus::NormalExit;
   m_processError = Process::ProcessError::UnknownError;
   m_errorString.clear();
   startProcess();
}

} // internal

namespace {

static StringList parse_combined_arg_string(const String &program)
{
   StringList args;
   String tmp;
   int quoteCount = 0;
   bool inQuote = false;
   
   // handle quoting. tokens can be surrounded by double quotes
   // "hello world". three consecutive double quotes represent
   // the quote character itself.
   for (int i = 0; i < program.size(); ++i) {
      if (program.at(i) == Latin1Character('"')) {
         ++quoteCount;
         if (quoteCount == 3) {
            // third consecutive quote
            quoteCount = 0;
            tmp += program.at(i);
         }
         continue;
      }
      if (quoteCount) {
         if (quoteCount == 1) {
            inQuote = !inQuote;
         }
         quoteCount = 0;
      }
      if (!inQuote && program.at(i).isSpace()) {
         if (!tmp.isEmpty()) {
            args += tmp;
            tmp.clear();
         }
      } else {
         tmp += program.at(i);
      }
   }
   if (!tmp.isEmpty()) {
      args += tmp;
   }
   return args;
}

} // anonymous namespace

#if !defined(PDK_NO_PROCESS_COMBINED_ARGUMENT_START)
void Process::start(const String &command, OpenModes mode)
{
   StringList args = parse_combined_arg_string(command);
   if (args.empty()) {
      PDK_D(Process);
      implPtr->setErrorAndEmit(ProcessError::FailedToStart, tr("No program defined"));
      return;
   }
   const String prog = args.takeFirst();
   start(prog, args, mode);
}
#endif

String Process::getProgram() const
{
   PDK_D(const Process);
   return implPtr->m_program;
}

StringList Process::getArguments() const
{
   PDK_D(const Process);
   return implPtr->m_arguments;
}

void Process::setArguments(const StringList &arguments)
{
   PDK_D(Process);
   if (implPtr->m_processState != ProcessState::NotRunning) {
      warning_stream("Process::setProgram: Process is already running");
      return;
   }
   implPtr->m_arguments = arguments;
}

void Process::terminate()
{
   PDK_D(Process);
   implPtr->terminateProcess();
}

void Process::kill()
{
   PDK_D(Process);
   implPtr->killProcess();
}

int Process::getExitCode() const
{
   PDK_D(const Process);
   return implPtr->m_exitCode;
}

Process::ExitStatus Process::getExitStatus() const
{
   PDK_D(const Process);
   return implPtr->m_exitStatus;
}

int Process::execute(const String &program, const StringList &arguments)
{
   Process process;
   process.setReadChannelMode(ProcessChannelMode::ForwardedChannels);
   process.start(program, arguments);
   if (!process.waitForFinished(-1) || process.getError() == ProcessError::FailedToStart)
      return -2;
   return process.getExitStatus() == Process::ExitStatus::NormalExit ? process.getExitCode() : -1;
}

int Process::execute(const String &command)
{
   Process process;
   process.setReadChannelMode(ProcessChannelMode::ForwardedChannels);
   process.start(command);
   if (!process.waitForFinished(-1) || process.getError() == ProcessError::FailedToStart) {
      return -2;
   }
   return process.getExitStatus() == ExitStatus::NormalExit ? process.getExitCode() : -1;
}

bool Process::startDetached(const String &program,
                            const StringList &arguments,
                            const String &workingDirectory,
                            pdk::pint64 *pid)
{
   Process process;
   process.setProgram(program);
   process.setArguments(arguments);
   process.setWorkingDirectory(workingDirectory);
   return process.startDetached(pid);
}

bool Process::startDetached(const String &program,
                            const StringList &arguments)
{
   Process process;
   process.setProgram(program);
   process.setArguments(arguments);
   return process.startDetached();
}

bool Process::startDetached(const String &command)
{
   StringList args = parse_combined_arg_string(command);
   if (args.empty()) {
      return false;
   }
   Process process;
   process.setProgram(args.takeFirst());
   process.setArguments(args);
   return process.startDetached();
}

} // process
} // os
} // pdk

#if defined(PDK_OS_MACX)
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
#elif defined(PDK_PLATFORM_UIKIT)
static char *sg_empty_environ[] = { 0 };
#define environ sg_empty_environ
#elif !defined(PDK_OS_WIN)
extern char **environ;
#endif

namespace pdk {
namespace os {
namespace process {

StringList Process::getSystemEnvironment()
{
   StringList tmp;
   char *entry = 0;
   int count = 0;
   while ((entry = environ[count++])) {
      tmp << String::fromLocal8Bit(entry);
   }
   return tmp;
}

String Process::getNullDevice()
{
#ifdef PDK_OS_WIN
   return StringLiteral("\\\\.\\NUL");
#elif defined(_PATH_DEVNULL)
   return StringLiteral(_PATH_DEVNULL);
#else
   return StringLiteral("/dev/null");
#endif
}

} // process
} // os
} // pdk
