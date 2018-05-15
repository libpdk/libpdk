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
// Created by softboy on 2018/05/15.

#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/os/process/Process.h"
#include "pdk/base/lang/String.h"
#include <cstdlib>

using pdk::kernel::CoreApplication;
using pdk::os::process::Process;
using pdk::lang::Latin1String;

#define PDKTEST_DIR_SEP "/"
#define APP_FILENAME(name) Latin1String(PDKTEST_PROCESS_APPS_DIR PDKTEST_DIR_SEP PDK_STRINGIFY(name)) 

int main(int argc, char **argv)
{
   CoreApplication app(argc, argv);
   
   if (argc < 3) {
      return 13;
   }

   Process process;
   
   Process::ProcessChannelMode mode = (Process::ProcessChannelMode)atoi(argv[1]);
   process.setProcessChannelMode(mode);
   if (process.getProcessChannelMode() != mode) {
      return 1;
   }
   Process::InputChannelMode inmode = (Process::InputChannelMode)atoi(argv[2]);
   process.setInputChannelMode(inmode);
   if (process.getInputChannelMode() != inmode) {
      return 11;      
   }

   process.start(APP_FILENAME(ProcessEcho2App));
   
   if (!process.waitForStarted(5000)) {
      return 2;
   }
   
   if (inmode == Process::InputChannelMode::ManagedInputChannel && process.write("forwarded") != 9) {
      return 3;
   }
   
   process.closeWriteChannel();
   if (!process.waitForFinished(5000)) {
      return 4;
   }
   
   if ((mode == Process::ProcessChannelMode::ForwardedOutputChannel || mode == Process::ProcessChannelMode::ForwardedChannels)
       && !process.readAllStandardOutput().isEmpty()) {
      return 5;
   }
      
   if ((mode == Process::ProcessChannelMode::ForwardedErrorChannel || mode == Process::ProcessChannelMode::ForwardedChannels)
       && !process.readAllStandardError().isEmpty()) {
      return 6;
   }
      
   return 0;
}
