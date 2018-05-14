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
// Created by softboy on 2018/05/10.

#include "pdk/base/io/Debug.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/kernel/StringUtils.h"

#include "pdk/global/PlatformDefs.h"
#include "pdk/base/os/process/Process.h"
#include "pdk/base/os/process/internal/ProcessPrivate.h"
#include "pdk/base/io/fs/StandardPaths.h"
#include "pdk/kernel/internal/CoreUnixPrivate.h"

#ifdef PDK_OS_MAC
#include "pdk/kernel/internal/CoreMacPrivate.h"
#endif

#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/Dir.h"
#include <list>
#include <mutex>
#include "pdk/base/os/thread/Semaphore.h"
#include "pdk/kernel/SocketNotifier.h"
#include "pdk/kernel/ElapsedTimer.h"
#include "pdk/base/os/thread/Thread.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

#if defined(PDK_PROCESS_DEBUG)
#include <ctype.h>
#endif

#include "forkfd/forkfd.h"

namespace pdk {
namespace os {
namespace process {

using pdk::ds::ByteArray;
using internal::ProcessPrivate;
using pdk::io::fs::File;
using pdk::io::fs::FileInfo;
using pdk::io::fs::Dir;
using pdk::io::fs::StandardPaths;
using pdk::lang::Latin1Character;
using pdk::lang::Latin1String;
using pdk::kernel::ElapsedTimer;

#if defined(PDK_PROCESS_DEBUG)
/*
    Returns a human readable representation of the first \a len
    characters in \a data.
*/
ByteArray pretty_debug(const char *data, int len, int maxSize)
{
   if (!data) return "(null)";
   ByteArray out;
   for (int i = 0; i < len; ++i) {
      char c = data[i];
      if (isprint(c)) {
         out += c;
      } else switch (c) {
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: {
         const char buf[] =  {
            '\\',
            pdk::to_oct(uchar(c) / 64),
            pdk::to_oct(uchar(c) % 64 / 8),
            pdk::to_oct(uchar(c) % 8),
            0
         };
         out += buf;
      }
      }
   }
   
   if (len < maxSize) {
      out += "...";
   }
   return out;
}
#endif

namespace {

struct ProcessPoller
{
   ProcessPoller(const ProcessPrivate &proc);
   
   int poll(int timeout);
   
   pollfd &stdinPipe()
   {
      return m_pfds[0];
   }
   
   pollfd &stdoutPipe()
   {
      return m_pfds[1];
   }
   
   pollfd &stderrPipe()
   {
      return m_pfds[2];
   }
   
   pollfd &forkfd()
   {
      return m_pfds[3];
   }
   
   pollfd &childStartedPipe()
   {
      return m_pfds[4];
   }
   
   constexpr const static size_t n_pfds = 5;
   pollfd m_pfds[n_pfds];
};

ProcessPoller::ProcessPoller(const ProcessPrivate &proc)
{
   for (size_t i = 0; i < n_pfds; i++) {
      m_pfds[i] = pdk::kernel::make_pollfd(-1, POLLIN);
   }
   stdoutPipe().fd = proc.m_stdoutChannel.m_pipe[0];
   stderrPipe().fd = proc.m_stderrChannel.m_pipe[0];
   
   if (!proc.m_writeBuffer.isEmpty()) {
      stdinPipe().fd = proc.m_stdinChannel.m_pipe[1];
      stdinPipe().events = POLLOUT;
   }
   
   forkfd().fd = proc.m_forkfd;
   
   if (proc.m_processState == Process::ProcessState::Starting) {
      childStartedPipe().fd = proc.m_childStartedPipe[0];
   }
}

int ProcessPoller::poll(int timeout)
{
   const nfds_t nfds = (childStartedPipe().fd == -1) ? 4 : 5;
   return pdk::kernel::poll_msecs(m_pfds, nfds, timeout);
}

bool pollfd_check(const pollfd &pfd, short revents)
{
   return pfd.fd >= 0 && (pfd.revents & (revents | POLLHUP | POLLERR | POLLNVAL)) != 0;
}

int create_pipe(int *pipe)
{
   if (pipe[0] != -1) {
      pdk::kernel::safe_close(pipe[0]);
   }
   
   if (pipe[1] != -1) {
      pdk::kernel::safe_close(pipe[1]);
   }
   
   int pipe_ret = pdk::kernel::safe_pipe(pipe);
   if (pipe_ret != 0) {
      warning_stream("ProcessPrivate::createPipe: Cannot create pipe %p: %s",
                     (void *)pipe, pdk_printable(pdk::error_string(errno)));
   }
   return pipe_ret;
}

} // anonymous namespace

namespace internal {

void ProcessPrivate::destroyPipe(int *pipe)
{
   if (pipe[1] != -1) {
      pdk::kernel::safe_close(pipe[1]);
      pipe[1] = -1;
   }
   if (pipe[0] != -1) {
      pdk::kernel::safe_close(pipe[0]);
      pipe[0] = -1;
   }
}

void ProcessPrivate::closeChannel(Channel *channel)
{
   destroyPipe(channel->m_pipe);
}

bool ProcessPrivate::openChannel(Channel &channel)
{
   PDK_Q(Process);
   
   if (&channel == &m_stderrChannel && m_processChannelMode == Process::ProcessChannelMode::MergedChannels) {
      channel.m_pipe[0] = -1;
      channel.m_pipe[1] = -1;
      return true;
   }
   
   if (channel.m_type == Channel::Normal) {
      // we're piping this channel to our own process
      if (create_pipe(channel.m_pipe) != 0)
         return false;
      
      // create the socket notifiers
      if (m_threadData->hasEventDispatcher()) {
         if (&channel == &m_stdinChannel) {
            channel.m_notifier = new SocketNotifier(channel.m_pipe[1],
                  SocketNotifier::Type::Write, apiPtr);
            channel.m_notifier->setEnabled(false);
            channel.m_notifier->connectActivatedSignal(apiPtr, &Process::canWritePrivateSlot);
            
         } else {
            channel.m_notifier = new SocketNotifier(channel.m_pipe[0],
                  SocketNotifier::Type::Read, apiPtr);
            if (&channel == &m_stdoutChannel) {
               channel.m_notifier->connectActivatedSignal(apiPtr, &Process::canReadStandardOutputPrivateSlot);
            } else {
               channel.m_notifier->connectActivatedSignal(apiPtr, &Process::canReadStandardErrorPrivateSlot);
            }
         }
      }
      
      return true;
   } else if (channel.m_type == Channel::Redirect) {
      // we're redirecting the channel to/from a file
      ByteArray fname = File::encodeName(channel.m_file);
      
      if (&channel == &m_stdinChannel) {
         // try to open in read-only mode
         channel.m_pipe[1] = -1;
         if ( (channel.m_pipe[0] = pdk::kernel::safe_open(fname, O_RDONLY)) != -1)
            return true;    // success
         setErrorAndEmit(Process::ProcessError::FailedToStart,
                         Process::tr("Could not open input redirection for reading"));
      } else {
         int mode = O_WRONLY | O_CREAT;
         if (channel.m_append) {
            mode |= O_APPEND;
         } else {
            mode |= O_TRUNC;
         }
         channel.m_pipe[0] = -1;
         if ( (channel.m_pipe[1] = pdk::kernel::safe_open(fname, mode, 0666)) != -1) {
            return true; // success
         }
         setErrorAndEmit(Process::ProcessError::FailedToStart,
                         Process::tr("Could not open input redirection for reading"));
      }
      cleanup();
      return false;
   } else {
      PDK_ASSERT_X(channel.m_process, "Process::start", "Internal error");
      
      Channel *source;
      Channel *sink;
      
      if (channel.m_type == Channel::PipeSource) {
         // we are the source
         source = &channel;
         sink = &channel.m_process->m_stdinChannel;
         
         PDK_ASSERT(source == &m_stdoutChannel);
         PDK_ASSERT(sink->m_process == this && sink->m_type == Channel::PipeSink);
      } else {
         // we are the sink;
         source = &channel.m_process->m_stdoutChannel;
         sink = &channel;
         
         PDK_ASSERT(sink == &m_stdinChannel);
         PDK_ASSERT(source->m_process == this && source->m_type == Channel::PipeSource);
      }
      
      if (source->m_pipe[1] != INVALID_PDK_PIPE || sink->m_pipe[0] != INVALID_PDK_PIPE) {
         // already created, do nothing
         return true;
      } else {
         PDK_ASSERT(source->m_pipe[0] == INVALID_PDK_PIPE && source->m_pipe[1] == INVALID_PDK_PIPE);
         PDK_ASSERT(sink->m_pipe[0] == INVALID_PDK_PIPE && sink->m_pipe[1] == INVALID_PDK_PIPE);
         
         PDK_PIPE pipe[2] = { -1, -1 };
         if (create_pipe(pipe) != 0) {
            return false;
         }
         
         sink->m_pipe[0] = pipe[0];
         source->m_pipe[1] = pipe[1];
         return true;
      }
   }
}

namespace {

char **dup_environment(const ProcessEnvironmentPrivate::Map &environment, int *envc)
{
   *envc = 0;
   if (environment.empty()) {
      return nullptr;
   }
   char **envp = new char *[environment.size() + 2];
   envp[environment.size()] = 0;
   envp[environment.size() + 1] = 0;
   
   auto iter = environment.cbegin();
   const auto end = environment.cend();
   for ( ; iter != end; ++iter) {
      ByteArray key = iter->first;
      ByteArray value = iter->second.getBytes();
      key.reserve(key.length() + 1 + value.length());
      key.append('=');
      key.append(value);
      envp[(*envc)++] = ::strdup(key.getConstRawData());
   }
   
   return envp;
}

} // anonymous namespace

void ProcessPrivate::startProcess()
{
   PDK_Q(Process);
   
#if defined (PDK_PROCESS_DEBUG)
   debug_stream("ProcessPrivate::startProcess()");
#endif
   
   // Initialize pipes
   if (!openChannel(m_stdinChannel) ||
       !openChannel(m_stdoutChannel) ||
       !openChannel(m_stderrChannel) ||
       create_pipe(m_childStartedPipe) != 0) {
      setErrorAndEmit(Process::ProcessError::FailedToStart, pdk::error_string(errno));
      cleanup();
      return;
   }
   
   if (m_threadData->hasEventDispatcher()) {
      m_startupSocketNotifier = new SocketNotifier(m_childStartedPipe[0],
            SocketNotifier::Type::Read, apiPtr);
      m_startupSocketNotifier->connectActivatedSignal(apiPtr, &Process::startupNotificationPrivateSlot);
   }
   
   // Start the process (platform dependent)
   apiPtr->setProcessState(Process::ProcessState::Starting);
   
   // Create argument list with right number of elements, and set the final
   // one to 0.
   char **argv = new char *[m_arguments.size() + 2];
   argv[m_arguments.size() + 1] = 0;
   
   // Encode the program name.
   ByteArray encodedProgramName = File::encodeName(m_program);
#ifdef PDK_OS_MAC
   // allow invoking of .app bundles on the Mac.
   FileInfo fileInfo(m_program);
   if (encodedProgramName.endsWith(".app") && fileInfo.isDir()) {
      pdk::kernel::CFType<CFURLRef> url = CFURLCreateWithFileSystemPath(0,
                                                                        pdk::kernel::CFString(fileInfo.getAbsoluteFilePath()),
                                                                        kCFURLPOSIXPathStyle, true);
      {
         // CFBundle is not reentrant, since CFBundleCreate might return a reference
         // to a cached bundle object. Protect the bundle calls with a mutex lock.
         static std::mutex cfbundleMutex;
         std::lock_guard lock(cfbundleMutex);
         pdk::kernel::CFType<CFBundleRef> bundle = CFBundleCreate(0, url);
         // 'executableURL' can be either relative or absolute ...
         pdk::kernel::CFType<CFURLRef> executableURL = CFBundleCopyExecutableURL(bundle);
         // not to depend on caching - make sure it's always absolute.
         url = CFURLCopyAbsoluteURL(executableURL);
      }
      if (url) {
         const pdk::kernel::CFString str = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
         encodedProgramName += (Dir::getSeparator() + Dir(m_program).getRelativeFilePath(String::fromCFString(str))).toUtf8();
      }
   }
#endif
   
   // Add the program name to the argument list.
   argv[0] = nullptr;
   if (!m_program.contains(Latin1Character('/'))) {
      const String &exeFilePath = StandardPaths::findExecutable(m_program);
      if (!exeFilePath.isEmpty()) {
         const ByteArray &tmp = File::encodeName(exeFilePath);
         argv[0] = ::strdup(tmp.getConstRawData());
      }
   }
   if (!argv[0]) {
      argv[0] = ::strdup(encodedProgramName.getConstRawData());
   }
   
   // Add every argument to the list
   for (size_t i = 0; i < m_arguments.size(); ++i) {
      argv[i + 1] = ::strdup(File::encodeName(m_arguments.at(i)).getConstRawData());
   }
   // Duplicate the environment.
   int envc = 0;
   char **envp = 0;
   if (m_environment.m_implPtr.constData()) {
      std::lock_guard locker(m_environment.m_implPtr->m_mutex);
      envp = dup_environment(m_environment.m_implPtr.constData()->m_vars, &envc);
   }
   
   // Encode the working directory if it's non-empty, otherwise just pass 0.
   const char *workingDirPtr = 0;
   ByteArray encodedWorkingDirectory;
   if (!m_workingDirectory.isEmpty()) {
      encodedWorkingDirectory = File::encodeName(m_workingDirectory);
      workingDirPtr = encodedWorkingDirectory.getConstRawData();
   }
   
   // Start the process manager, and fork off the child process.
   pid_t childPid;
   m_forkfd = ::forkfd(FFD_CLOEXEC, &childPid);
   int lastForkErrno = errno;
   if (m_forkfd != FFD_CHILD_PROCESS) {
      // Parent process.
      // Clean up duplicated memory.
      for (size_t i = 0; i <= m_arguments.size(); ++i) {
         free(argv[i]);
      }
      for (int i = 0; i < envc; ++i) {
         free(envp[i]);
      }  
      delete [] argv;
      delete [] envp;
   }
   
   // On QNX, if spawnChild failed, childPid will be -1 but forkfd is still 0.
   // This is intentional because we only want to handle failure to fork()
   // here, which is a rare occurrence. Handling of the failure to start is
   // done elsewhere.
   if (m_forkfd == -1) {
      // Cleanup, report error and return
#if defined (PDK_PROCESS_DEBUG)
      debug_stream("fork failed: %s", qPrintable(qt_error_string(lastForkErrno)));
#endif
      apiPtr->setProcessState(Process::ProcessState::NotRunning);
      setErrorAndEmit(Process::ProcessError::FailedToStart,
                      Process::tr("Resource error (fork failure): %1").arg(pdk::error_string(lastForkErrno)));
      cleanup();
      return;
   }
   
   // Start the child.
   if (m_forkfd == FFD_CHILD_PROCESS) {
      execChild(workingDirPtr, argv, envp);
      ::_exit(-1);
   }
   
   m_pid = PDK_PID(childPid);
   
   // parent
   // close the ends we don't use and make all pipes non-blocking
   pdk::kernel::safe_close(m_childStartedPipe[1]);
   m_childStartedPipe[1] = -1;
   
   if (m_stdinChannel.m_pipe[0] != -1) {
      pdk::kernel::safe_close(m_stdinChannel.m_pipe[0]);
      m_stdinChannel.m_pipe[0] = -1;
   }
   
   if (m_stdinChannel.m_pipe[1] != -1) {
      ::fcntl(m_stdinChannel.m_pipe[1], F_SETFL, ::fcntl(m_stdinChannel.m_pipe[1], F_GETFL) | O_NONBLOCK);
   }
   
   if (m_stdoutChannel.m_pipe[1] != -1) {
      pdk::kernel::safe_close(m_stdoutChannel.m_pipe[1]);
      m_stdoutChannel.m_pipe[1] = -1;
   }
   
   if (m_stdoutChannel.m_pipe[0] != -1) {
      ::fcntl(m_stdoutChannel.m_pipe[0], F_SETFL, ::fcntl(m_stdoutChannel.m_pipe[0], F_GETFL) | O_NONBLOCK);
   }
   
   if (m_stderrChannel.m_pipe[1] != -1) {
      pdk::kernel::safe_close(m_stderrChannel.m_pipe[1]);
      m_stderrChannel.m_pipe[1] = -1;
   }
   if (m_stderrChannel.m_pipe[0] != -1)
      ::fcntl(m_stderrChannel.m_pipe[0], F_SETFL, ::fcntl(m_stderrChannel.m_pipe[0], F_GETFL) | O_NONBLOCK);
   
   if (m_threadData->m_eventDispatcher) {
      m_deathNotifier = new SocketNotifier(m_forkfd, SocketNotifier::Type::Read, apiPtr);
      m_deathNotifier->connectActivatedSignal(apiPtr, &Process::processDiedPrivateSlot);
   }
}

struct ChildError
{
   int m_code;
   char m_function[8];
};

void ProcessPrivate::execChild(const char *workingDir, char **argv, char **envp)
{
   ::signal(SIGPIPE, SIG_DFL);         // reset the signal that we ignored
   
   PDK_Q(Process);
   ChildError error = { 0, {} };       // force zeroing of function[8]
   
   // copy the stdin socket if asked to (without closing on exec)
   if (m_inputChannelMode != Process::InputChannelMode::ForwardedInputChannel) {
      pdk::kernel::safe_dup2(m_stdinChannel.m_pipe[0], STDIN_FILENO, 0);
   }
   // copy the stdout and stderr if asked to
   if (m_processChannelMode != Process::ProcessChannelMode::ForwardedChannels) {
      if (m_processChannelMode != Process::ProcessChannelMode::ForwardedOutputChannel) {
         pdk::kernel::safe_dup2(m_stdoutChannel.m_pipe[1], STDOUT_FILENO, 0);
      }
      // merge stdout and stderr if asked to
      if (m_processChannelMode == Process::ProcessChannelMode::MergedChannels) {
         pdk::kernel::safe_dup2(STDOUT_FILENO, STDERR_FILENO, 0);
      } else if (m_processChannelMode != Process::ProcessChannelMode::ForwardedErrorChannel) {
         pdk::kernel::safe_dup2(m_stderrChannel.m_pipe[1], STDERR_FILENO, 0);
      }
   }
   
   // make sure this fd is closed if execv() succeeds
   pdk::kernel::safe_close(m_childStartedPipe[0]);
   
   // enter the working directory
   if (workingDir && PDK_CHDIR(workingDir) == -1) {
      // failed, stop the process
      strcpy(error.m_function, "chdir");
      goto report_errno;
   }
   
   // this is a virtual call, and it base behavior is to do nothing.
   apiPtr->setupChildProcess();
   
   // execute the process
   if (!envp) {
      pdk::kernel::safe_execv(argv[0], argv);
      strcpy(error.m_function, "execvp");
   } else {
#if defined (PDK_PROCESS_DEBUG)
      fprintf(stderr, "ProcessPrivate::execChild() starting %s\n", argv[0]);
#endif
      pdk::kernel::safe_execve(argv[0], argv, envp);
      strcpy(error.m_function, "execve");
   }
   
   // notify failure
   // don't use strerror or any other routines that may allocate memory, since
   // some buggy libc versions can deadlock on locked mutexes.
report_errno:
   error.m_code = errno;
   pdk::kernel::safe_write(m_childStartedPipe[1], &error, sizeof(error));
   m_childStartedPipe[1] = -1;
}

bool ProcessPrivate::processStarted(String *errorMessage)
{
   ChildError buf;
   int ret = pdk::kernel::safe_read(m_childStartedPipe[0], &buf, sizeof(buf));
   
   if (m_startupSocketNotifier) {
      m_startupSocketNotifier->setEnabled(false);
      m_startupSocketNotifier->deleteLater();
      m_startupSocketNotifier = nullptr;
   }
   pdk::kernel::safe_close(m_childStartedPipe[0]);
   m_childStartedPipe[0] = -1;
   
#if defined (PDK_PROCESS_DEBUG)
   debug_stream("ProcessPrivate::processStarted() == %s", i <= 0 ? "true" : "false");
#endif
   
   // did we read an error message?
   if (ret > 0 && errorMessage) {
      *errorMessage = Latin1String(buf.m_function) + Latin1String(": ") + pdk::error_string(buf.m_code);
   }
   return ret <= 0;
}

pdk::pint64 ProcessPrivate::bytesAvailableInChannel(const Channel *channel) const
{
   PDK_ASSERT(channel->m_pipe[0] != INVALID_PDK_PIPE);
   int nbytes = 0;
   pdk::pint64 available = 0;
   if (::ioctl(channel->m_pipe[0], FIONREAD, (char *) &nbytes) >= 0) {
      available = (pdk::pint64) nbytes;
   }
#if defined (PDK_PROCESS_DEBUG)
   debug_stream("ProcessPrivate::bytesAvailableInChannel(%d) == %lld", int(channel - &m_stdinChannel), available);
#endif
   return available;
}

pdk::pint64 ProcessPrivate::readFromChannel(const Channel *channel, char *data, pdk::pint64 maxlen)
{
   PDK_ASSERT(channel->m_pipe[0] != INVALID_PDK_PIPE);
   pdk::pint64 bytesRead = pdk::kernel::safe_read(channel->m_pipe[0], data, maxlen);
#if defined PDK_PROCESS_DEBUG
   int save_errno = errno;
   debug_stream("ProcessPrivate::readFromChannel(%d, %p \"%s\", %lld) == %lld",
                int(channel - &stdinChannel),
                data, pretty_debug(data, bytesRead, 16).getConstRawData(), maxlen, bytesRead);
   errno = save_errno;
#endif
   if (bytesRead == -1 && errno == EWOULDBLOCK) {
      return -2;
   }
   return bytesRead;
}

bool ProcessPrivate::writeToStdin()
{
   const char *data = m_writeBuffer.readPointer();
   const pdk::pint64 bytesToWrite = m_writeBuffer.nextDataBlockSize();
   
   pdk::pint64 written = pdk::kernel::safe_write_nosignal(m_stdinChannel.m_pipe[1], data, bytesToWrite);
#if defined PDK_PROCESS_DEBUG
   debug_stream("ProcessPrivate::writeToStdin(), write(%p \"%s\", %lld) == %lld",
                data, pretty_debug(data, bytesToWrite, 16).getConstRawData(), bytesToWrite, written);
   if (written == -1)
      debug_stream("ProcessPrivate::writeToStdin(), failed to write (%s)", pdk_printable(qt_error_string(errno)));
#endif
   if (written == -1) {
      // If the O_NONBLOCK flag is set and If some data can be written without blocking
      // the process, write() will transfer what it can and return the number of bytes written.
      // Otherwise, it will return -1 and set errno to EAGAIN
      if (errno == EAGAIN)
         return true;
      
      closeChannel(&m_stdinChannel);
      setErrorAndEmit(Process::ProcessError::WriteError);
      return false;
   }
   m_writeBuffer.free(written);
   if (!m_emittedBytesWritten && written != 0) {
      m_emittedBytesWritten = true;
      getApiPtr()->emitBytesWrittenSignal(written);
      m_emittedBytesWritten = false;
   }
   return true;
}

void ProcessPrivate::terminateProcess()
{
#if defined (PDK_PROCESS_DEBUG)
   debug_stream("ProcessPrivate::terminateProcess()");
#endif
   if (m_pid) {
      ::kill(pid_t(m_pid), SIGTERM);
   }  
}

void ProcessPrivate::killProcess()
{
#if defined (PDK_PROCESS_DEBUG)
   debug_stream("ProcessPrivate::killProcess()");
#endif
   if (m_pid) {
      ::kill(pid_t(m_pid), SIGKILL);
   }
}

bool ProcessPrivate::waitForStarted(int msecs)
{
#if defined (PDK_PROCESS_DEBUG)
   debug_stream("ProcessPrivate::waitForStarted(%d) waiting for child to start (fd = %d)", msecs,
                m_childStartedPipe[0]);
#endif
   
   pollfd pfd = pdk::kernel::make_pollfd(m_childStartedPipe[0], POLLIN);
   
   if (pdk::kernel::poll_msecs(&pfd, 1, msecs) == 0) {
      setError(Process::ProcessError::Timedout);
#if defined (PDK_PROCESS_DEBUG)
      debug_stream("ProcessPrivate::waitForStarted(%d) == false (timed out)", msecs);
#endif
      return false;
   }
   
   bool startedEmitted = startupNotificationPrivateSlot();
#if defined (PDK_PROCESS_DEBUG)
   debug_stream("ProcessPrivate::waitForStarted() == %s", startedEmitted ? "true" : "false");
#endif
   return startedEmitted;
}

bool ProcessPrivate::waitForReadyRead(int msecs)
{
#if defined (PDK_PROCESS_DEBUG)
   debug_stream("ProcessPrivate::waitForReadyRead(%d)", msecs);
#endif
   
   ElapsedTimer stopWatch;
   stopWatch.start();
   
   while(true) {
      ProcessPoller poller(*this);
      
      int timeout = pdk::io::internal::subtract_from_timeout(msecs, stopWatch.getElapsed());
      int ret = poller.poll(timeout);
      
      if (ret < 0) {
         break;
      }
      if (ret == 0) {
         setError(Process::ProcessError::Timedout);
         return false;
      }
      
      if (pollfd_check(poller.childStartedPipe(), POLLIN)) {
         if (!startupNotificationPrivateSlot()) {
            return false;
         }
      }
      
      bool readyReadEmitted = false;
      if (pollfd_check(poller.stdoutPipe(), POLLIN)) {
         bool canRead = canReadStandardOutputPrivateSlot();
         if (m_currentReadChannel == pdk::as_integer<Process::ProcessChannel>(Process::ProcessChannel::StandardOutput) &&
             canRead) {
            readyReadEmitted = true;
         }
         
      }
      if (pollfd_check(poller.stderrPipe(), POLLIN)) {
         bool canRead = canReadStandardErrorPrivateSlot();
         if (m_currentReadChannel == pdk::as_integer<Process::ProcessChannel>(Process::ProcessChannel::StandardError) && 
             canRead) {
            readyReadEmitted = true;
         } 
      }
      if (readyReadEmitted) {
         return true;
      }
      if (pollfd_check(poller.stdinPipe(), POLLOUT)) {
         canWritePrivateSlot();
      }
      // Signals triggered by I/O may have stopped this process:
      if (m_processState == Process::ProcessState::NotRunning) {
         return false;
      }
      if (pollfd_check(poller.forkfd(), POLLIN)) {
         if (processDiedPrivateSlot()) {
            return false;
         }
      }
   }
   return false;
}

bool ProcessPrivate::waitForBytesWritten(int msecs)
{
#if defined (PDK_PROCESS_DEBUG)
   debug_stream("ProcessPrivate::waitForBytesWritten(%d)", msecs);
#endif
   
   ElapsedTimer stopWatch;
   stopWatch.start();
   
   while (!m_writeBuffer.isEmpty()) {
      ProcessPoller poller(*this);
      
      int timeout = pdk::io::internal::subtract_from_timeout(msecs, stopWatch.getElapsed());
      int ret = poller.poll(timeout);
      
      if (ret < 0) {
         break;
      }
      
      if (ret == 0) {
         setError(Process::ProcessError::Timedout);
         return false;
      }
      
      if (pollfd_check(poller.childStartedPipe(), POLLIN)) {
         if (!startupNotificationPrivateSlot()) {
            return false;
         }  
      }
      
      if (pollfd_check(poller.stdinPipe(), POLLOUT)) {
         return canWritePrivateSlot();
      }
      
      if (pollfd_check(poller.stdoutPipe(), POLLIN)) {
         canReadStandardOutputPrivateSlot();
      }
      
      if (pollfd_check(poller.stderrPipe(), POLLIN)) {
         canReadStandardErrorPrivateSlot();
      }
      
      // Signals triggered by I/O may have stopped this process:
      if (m_processState == Process::ProcessState::NotRunning) {
         return false;
      }
      if (pollfd_check(poller.forkfd(), POLLIN)) {
         if (processDiedPrivateSlot()) {
            return false;
         }
      }
   }
   
   return false;
}

bool ProcessPrivate::waitForFinished(int msecs)
{
#if defined (PDK_PROCESS_DEBUG)
   debug_stream("ProcessPrivate::waitForFinished(%d)", msecs);
#endif
   
   ElapsedTimer stopWatch;
   stopWatch.start();
   
   while (true) {
      ProcessPoller poller(*this);
      int timeout = pdk::io::internal::subtract_from_timeout(msecs, stopWatch.getElapsed());
      int ret = poller.poll(timeout);
      if (ret < 0) {
         break;
      }
      if (ret == 0) {
         setError(Process::ProcessError::Timedout);
         return false;
      }
      
      if (pollfd_check(poller.childStartedPipe(), POLLIN)) {
         if (!startupNotificationPrivateSlot()) {
            return false;
         }  
      }
      if (pollfd_check(poller.stdinPipe(), POLLOUT)) {
         canWritePrivateSlot();
      }
      
      if (pollfd_check(poller.stdoutPipe(), POLLIN)) {
         canReadStandardOutputPrivateSlot();
      }
      
      if (pollfd_check(poller.stderrPipe(), POLLIN)) {
         canReadStandardErrorPrivateSlot();
      }
      // Signals triggered by I/O may have stopped this process:
      if (m_processState == Process::ProcessState::NotRunning) {
         return true;
      }
      if (pollfd_check(poller.forkfd(), POLLIN)) {
         if (processDiedPrivateSlot()) {
            return true;
         } 
      }
   }
   return false;
}

void ProcessPrivate::findExitCode()
{
}

bool ProcessPrivate::waitForDeadChild()
{
   if (m_forkfd == -1) {
      return true; // child has already exited
   }
   // read the process information from our fd
   forkfd_info info;
   int ret;
   PDK_EINTR_LOOP(ret, forkfd_wait(m_forkfd, &info, nullptr));
   
   m_exitCode = info.status;
   m_crashed = info.code != CLD_EXITED;
   
   delete m_deathNotifier;
   m_deathNotifier = nullptr;
   
   PDK_EINTR_LOOP(ret, forkfd_close(m_forkfd));
   m_forkfd = -1; // Child is dead, don't try to kill it anymore
   
#if defined PDK_PROCESS_DEBUG
   debug_stream() << "ProcessPrivate::waitForDeadChild() dead with exitCode"
                  << m_exitCode << ", crashed?" << m_crashed;
#endif
   return true;
}

bool ProcessPrivate::startDetached(pdk::pint64 *pid)
{
   ByteArray encodedWorkingDirectory = File::encodeName(m_workingDirectory);
   
   // To catch the startup of the child
   int startedPipe[2];
   if (pdk::kernel::safe_pipe(startedPipe) != 0) {
      return false;
   }
   // To communicate the pid of the child
   int pidPipe[2];
   if (pdk::kernel::safe_pipe(pidPipe) != 0) {
      pdk::kernel::safe_close(startedPipe[0]);
      pdk::kernel::safe_close(startedPipe[1]);
      return false;
   }
   
   if ((m_stdinChannel.m_type == Channel::Redirect && !openChannel(m_stdinChannel))
       || (m_stdoutChannel.m_type == Channel::Redirect && !openChannel(m_stdoutChannel))
       || (m_stderrChannel.m_type == Channel::Redirect && !openChannel(m_stderrChannel))) {
      closeChannel(&m_stdinChannel);
      closeChannel(&m_stdoutChannel);
      closeChannel(&m_stderrChannel);
      pdk::kernel::safe_close(startedPipe[0]);
      pdk::kernel::safe_close(startedPipe[1]);
      return false;
   }
   
   pid_t childPid = fork();
   if (childPid == 0) {
      struct sigaction noaction;
      memset(&noaction, 0, sizeof(noaction));
      noaction.sa_handler = SIG_IGN;
      ::sigaction(SIGPIPE, &noaction, 0);
      
      ::setsid();
      
      pdk::kernel::safe_close(startedPipe[0]);
      pdk::kernel::safe_close(pidPipe[0]);
      
      pid_t doubleForkPid = fork();
      if (doubleForkPid == 0) {
         pdk::kernel::safe_close(pidPipe[1]);
         
         // copy the stdin socket if asked to (without closing on exec)
         if (m_inputChannelMode != Process::InputChannelMode::ForwardedInputChannel) {
            pdk::kernel::safe_dup2(m_stdinChannel.m_pipe[0], STDIN_FILENO, 0);
         }
         // copy the stdout and stderr if asked to
         if (m_processChannelMode != Process::ProcessChannelMode::ForwardedChannels) {
            if (m_processChannelMode != Process::ProcessChannelMode::ForwardedOutputChannel) {
               pdk::kernel::safe_dup2(m_stdoutChannel.m_pipe[1], STDOUT_FILENO, 0);
            }
            if (m_processChannelMode != Process::ProcessChannelMode::ForwardedErrorChannel) {
               pdk::kernel::safe_dup2(m_stderrChannel.m_pipe[1], STDERR_FILENO, 0);
            }
            
         }
         
         if (!encodedWorkingDirectory.isEmpty()) {
            if (PDK_CHDIR(encodedWorkingDirectory.getConstRawData()) == -1) {
               warning_stream("ProcessPrivate::startDetached: failed to chdir to %s", encodedWorkingDirectory.getConstRawData());
            }
         }
         
         char **argv = new char *[m_arguments.size() + 2];
         for (size_t i = 0; i < m_arguments.size(); ++i) {
            argv[i + 1] = ::strdup(File::encodeName(m_arguments.at(i)).getConstRawData());
         }
         
         argv[m_arguments.size() + 1] = 0;
         
         // Duplicate the environment.
         int envc = 0;
         char **envp = nullptr;
         if (m_environment.m_implPtr.constData()) {
            std::lock_guard locker(m_environment.m_implPtr->m_mutex);
            envp = dup_environment(m_environment.m_implPtr.constData()->m_vars, &envc);
         }
         
         ByteArray tmp;
         if (!m_program.contains(Latin1Character('/'))) {
            const String &exeFilePath = StandardPaths::findExecutable(m_program);
            if (!exeFilePath.isEmpty()) {
               tmp = File::encodeName(exeFilePath);
            }
         }
         if (tmp.isEmpty()) {
            tmp = File::encodeName(m_program);
         }
         
         argv[0] = tmp.getRawData();
         
         if (envp) {
            pdk::kernel::safe_execve(argv[0], argv, envp);
         } else {
            pdk::kernel::safe_execv(argv[0], argv);
         }
         struct sigaction noaction;
         memset(&noaction, 0, sizeof(noaction));
         noaction.sa_handler = SIG_IGN;
         ::sigaction(SIGPIPE, &noaction, 0);
         
         // '\1' means execv failed
         char c = '\1';
         pdk::kernel::safe_write(startedPipe[1], &c, 1);
         pdk::kernel::safe_close(startedPipe[1]);
         ::_exit(1);
      } else if (doubleForkPid == -1) {
         struct sigaction noaction;
         memset(&noaction, 0, sizeof(noaction));
         noaction.sa_handler = SIG_IGN;
         ::sigaction(SIGPIPE, &noaction, 0);
         
         // '\2' means internal error
         char c = '\2';
         pdk::kernel::safe_write(startedPipe[1], &c, 1);
      }
      
      pdk::kernel::safe_close(startedPipe[1]);
      pdk::kernel::safe_write(pidPipe[1], (const char *)&doubleForkPid, sizeof(pid_t));
      if (PDK_CHDIR("/") == -1) {
         warning_stream("ProcessPrivate::startDetached: failed to chdir to /");
      }
      ::_exit(1);
   }
   
   closeChannel(&m_stdinChannel);
   closeChannel(&m_stdoutChannel);
   closeChannel(&m_stderrChannel);
   pdk::kernel::safe_close(startedPipe[1]);
   pdk::kernel::safe_close(pidPipe[1]);
   
   if (childPid == -1) {
      pdk::kernel::safe_close(startedPipe[0]);
      pdk::kernel::safe_close(pidPipe[0]);
      return false;
   }
   
   char reply = '\0';
   int startResult = pdk::kernel::safe_read(startedPipe[0], &reply, 1);
   int result;
   pdk::kernel::safe_close(startedPipe[0]);
   pdk::kernel::safe_waitpid(childPid, &result, 0);
   bool success = (startResult != -1 && reply == '\0');
   if (success && pid) {
      pid_t actualPid = 0;
      if (pdk::kernel::safe_read(pidPipe[0], (char *)&actualPid, sizeof(pid_t)) == sizeof(pid_t)) {
         *pid = actualPid;
      } else {
         *pid = 0;
      }
   }
   pdk::kernel::safe_close(pidPipe[0]);
   return success;
}

} // internal

} // process
} // os
} // pdk
