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
// Created by zzu_softboy on 2018/04/09.

#include "gtest/gtest.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/io/fs/DirIterator.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"

#if defined(PDK_OS_VXWORKS)
#define PDK_NO_SYMLINKS
#endif

using pdk::lang::String;
using pdk::io::fs::Dir;

namespace {

//bool create_directory(const String &dirName)
//{
//   if (currentDir.mkdir(dirName)) {
//      createdDirectories.prepend(dirName);
//      return true;
//   }
//   return false;
//}

} // anonymous namespace
