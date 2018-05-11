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

#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/ds/ByteArray.h"
#include <cstdio>

using pdk::kernel::CoreApplication;
using pdk::io::fs::File;
using pdk::ds::ByteArray;
using pdk::io::IoDevice;
using pdk::lang::Latin1String;

int main(int argc, char **argv)
{
   CoreApplication app(argc, argv);
   File file;
   file.open(stdin, IoDevice::OpenMode::ReadOnly);
   ByteArray input;
   char buf[1024];
   pdk::pint64 len;
   while ((len = file.read(buf, 1024)) > 0) {
      input.append(buf, len);
   }
   file.close();
   File file2(Latin1String("fileWriterProcess.txt"));
   if (!file2.open(IoDevice::OpenMode::WriteOnly | IoDevice::OpenMode::Truncate)) {
      fprintf(stderr, "Cannot open %s for writing: %s\n",
              pdk_printable(file2.getFileName()), pdk_printable(file2.getErrorString()));
      return 1;
   }
   file2.write(input);
   file2.close();
   return 0;
}
