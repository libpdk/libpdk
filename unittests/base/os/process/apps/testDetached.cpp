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


#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/Dir.h"

#include <cstdio>

#if defined(PDK_OS_UNIX)
#include <sys/types.h>
#include <unistd.h>
#elif defined(PDK_OS_WIN)
#include <windows.h>
#endif

using pdk::io::fs::File;
using pdk::io::fs::Dir;
using pdk::ds::ByteArray;
using pdk::ds::StringList;
using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::kernel::CoreApplication;
using pdk::io::IoDevice;

namespace {

void write_stuff(File &file)
{
   file.write(Dir::getCurrentPath().toUtf8());
   file.putChar('\n');
#if defined(PDK_OS_UNIX)
   file.write(ByteArray::number(pdk::puint64(getpid())));
#elif defined(PDK_OS_UNIX)
   file.write(ByteArray::number(pdk::puint64(GetCurrentProcessId())));
#endif
   file.putChar('\n');
   file.write(pdk::get_env("ProcessTest"));
   file.putChar('\n');
}

struct Args
{
   int m_exitCode = 0;
   ByteArray m_errorMessage;
   String m_fileName;
   FILE *m_channel = nullptr;
   ByteArray m_channelName;
};

Args parse_arguments(const StringList &args)
{
   Args result;
   if (args.size() < 2) {
      result.m_exitCode = 128;
      result.m_errorMessage = "Usage: DetachedApp [--out-channel={stdout|stderr}] filename.txt\n";
      return result;
   }
   for (const String &arg : args) {
      if (arg.startsWith(Latin1String("--"))) {
         if (!arg.startsWith(Latin1String("--out-channel="))) {
            result.m_exitCode = 2;
            result.m_errorMessage = ByteArray("Unknown argument ") + arg.toLocal8Bit();
            return result;
         }
         result.m_channelName = arg.substring(14).toLocal8Bit();
         if (result.m_channelName == ByteArray("stdout")) {
            result.m_channel = stdout;
         } else if (result.m_channelName == ByteArray("stderr")) {
            result.m_channel = stderr;
         } else {
            result.m_exitCode = 3;
            result.m_errorMessage = ByteArray("Unknown channel ") + result.m_channelName;
            return result;
         }
      } else {
         result.m_fileName = arg;
      }
   }
   return result;
}

} // anonymous namespace

int main(int argc, char **argv)
{
   CoreApplication app(argc, argv);
   
   const Args args = parse_arguments(app.getArguments());
   if (args.m_exitCode) {
      fprintf(stderr, "testDetached: %s\n", args.m_errorMessage.getConstRawData());
      return args.m_exitCode;
   }
   
   if (args.m_channel) {
      File channel;
      if (!channel.open(args.m_channel, IoDevice::OpenModes(IoDevice::OpenMode::WriteOnly) | IoDevice::OpenMode::Text)) {
         fprintf(stderr, "Cannot open channel %s for writing: %s\n",
                 pdk_printable(Latin1String(args.m_channelName)), pdk_printable(channel.getErrorString()));
         return 4;
      }
      write_stuff(channel);
   }
   
   File file(args.m_fileName);
   if (!file.open(IoDevice::OpenModes(IoDevice::OpenMode::WriteOnly) | IoDevice::OpenMode::Truncate | IoDevice::OpenMode::Text)) {
      fprintf(stderr, "Cannot open %s for writing: %s\n",
              pdk_printable(file.getFileName()), pdk_printable(file.getErrorString()));
      return 1;
   }
   
   write_stuff(file);
   file.close();
   return 0;
}
