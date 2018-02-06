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
// Created by softboy on 2018/02/06.


#ifndef PDK_M_BASE_IO_FS_INTERNAL_FILE_SYSTEM_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_FILE_SYSTEM_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/ds/ByteArray.h"

#if defined(PDK_OS_WIN)
#define PDK_FILESYSTEMENTRY_NATIVE_PATH_IS_UTF16
#endif

namespace pdk {


// forward declare class with namespace
namespace ds {
class ByteArray;
} // ds

// forward declare class with namespace
namespace lang {
class String;
} // lang

namespace io {
namespace fs {
namespace internal {

using pdk::ds::ByteArray;
using pdk::lang::String;

class FileSystemEntry
{
public:
   
#ifndef PDK_FILESYSTEMENTRY_NATIVE_PATH_IS_UTF16
   using NativePath = ByteArray;
#else
   using NativePath = String;
#endif
   struct FromNativePath{};
   struct FromInternalPath{};
   
   FileSystemEntry();
   explicit FileSystemEntry(const String &filePath);
   
   FileSystemEntry(const String &filePath, FromInternalPath dummy);
   FileSystemEntry(const NativePath &nativeFilePath, FromNativePath dummy);
   FileSystemEntry(const String &filePath, const NativePath &nativeFilePath);
   
   String getFilePath() const;
   String getFileName() const;
   String getPath() const;
   NativePath getNativeFilePath() const;
   String getBaseName() const;
   String getCompleteBaseName() const;
   String getSuffix() const;
   String getCompleteSuffix() const;
   bool isAbsolute() const;
   bool isRelative() const;
   bool isClean() const;
   
#if defined(PDK_OS_WIN)
   bool isDriveRoot() const;
   static bool isDriveRootPath(const String &path);
#endif
   bool isRoot() const;
   bool isEmpty() const
   {
      return m_filePath.isEmpty() && m_nativeFilePath.isEmpty();
   }
   void clear()
   {
      *this = FileSystemEntry();
   }
   
   PDK_CORE_EXPORT static bool isRootPath(const String &path);
   
private:
   // creates the String version out of the bytearray version
   void resolveFilePath() const;
   // creates the bytearray version out of the String version
   void resolveNativeFilePath() const;
   // resolves the separator
   void findLastSeparator() const;
   // resolves the dots and the separator
   void findFileNameSeparators() const;
   
   mutable String m_filePath; // always has slashes as separator
   mutable NativePath m_nativeFilePath; // native encoding and separators
   
   mutable pdk::pint16 m_lastSeparator; // index in m_filePath of last separator
   mutable pdk::pint16 m_firstDotInFileName; // index after m_filePath for first dot (.)
   mutable pdk::pint16 m_lastDotInFileName; // index after m_firstDotInFileName for last dot (.)
};

} // internal
} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_INTERNAL_FILE_SYSTEM_PRIVATE_H
