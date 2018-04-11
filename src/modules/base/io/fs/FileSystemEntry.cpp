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
// Created by softboy on 2018/02/07.

#include "pdk/base/io/fs/internal/FileSystemEntryPrivate.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/internal/FileEnginePrivate.h"
#include "pdk/base/lang/String.h"
#ifdef PDK_OS_WIN
#include "pdk/base/lang/StringBuilder.h"
#endif

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::lang::Character;

namespace {
#ifdef PDK_OS_WIN
bool is_unc_root(const String &server)
{
   String localPath = Dir::toNativeSeparators(server);
   if (!localPath.startsWith(Latin1String("\\\\"))) {
      return false;
   }
   int idx = localPath.indexOf(Latin1Character('\\'), 2);
   if (idx == -1 || idx + 1 == localPath.length()) {
      return true;
   }
   return localPath.rightRef(localPath.length() - idx - 1).trimmed().isEmpty();
}

inline String fix_if_relative_unc_path(const String &path)
{
   String currentPath = Dir::getCurrentPath();
   if (currentPath.startsWith(Latin1String("//"))) {
      return currentPath % Character(Latin1Character('/')) % path;
   }
   return path;
}
#endif // PDK_OS_WIN
} // anonymous namespace

FileSystemEntry::FileSystemEntry()
   : m_lastSeparator(-1),
     m_firstDotInFileName(-1),
     m_lastDotInFileName(-1)
{
}

FileSystemEntry::FileSystemEntry(const String &filePath)
   : m_filePath(Dir::fromNativeSeparators(filePath)),
     m_lastSeparator(-2),
     m_firstDotInFileName(-2),
     m_lastDotInFileName(0)
{
}

FileSystemEntry::FileSystemEntry(const String &filePath, FromInternalPath /* dummy */)
   : m_filePath(filePath),
     m_lastSeparator(-2),
     m_firstDotInFileName(-2),
     m_lastDotInFileName(0)
{
}

FileSystemEntry::FileSystemEntry(const NativePath &nativeFilePath, FromNativePath /* dummy */)
   : m_nativeFilePath(nativeFilePath),
     m_lastSeparator(-2),
     m_firstDotInFileName(-2),
     m_lastDotInFileName(0)
{
}

FileSystemEntry::FileSystemEntry(const String &filePath, const NativePath &nativeFilePath)
   : m_filePath(Dir::fromNativeSeparators(filePath)),
     m_nativeFilePath(nativeFilePath),
     m_lastSeparator(-2),
     m_firstDotInFileName(-2),
     m_lastDotInFileName(0)
{
}

String FileSystemEntry::getFilePath() const
{
   resolveFilePath();
   return m_filePath;
}

FileSystemEntry::NativePath FileSystemEntry::getNativeFilePath() const
{
   resolveNativeFilePath();
   return m_nativeFilePath;
}

void FileSystemEntry::resolveFilePath() const
{
   if (m_filePath.isEmpty() && !m_nativeFilePath.isEmpty()) {
#if defined(PDK_FILESYSTEMENTRY_NATIVE_PATH_IS_UTF16)
      m_filePath = Dir::fromNativeSeparators(m_nativeFilePath);
#  ifdef PDK_OS_WIN
      if (m_filePath.startsWith(Latin1String("//?/UNC/"))) {
         m_filePath = m_filePath.remove(2,6);
      }
      if (m_filePath.startsWith(Latin1String("//?/"))) {
         m_filePath = m_filePath.remove(0,4);
      }
#  endif
#else
      m_filePath = Dir::fromNativeSeparators(File::decodeName(m_nativeFilePath));
#endif
   }
}

void FileSystemEntry::resolveNativeFilePath() const
{
   if (!m_filePath.isEmpty() && m_nativeFilePath.isEmpty()) {
#ifdef PDK_OS_WIN
      String filePath = m_filePath;
      if (isRelative()) {
         filePath = fix_if_relative_unc_path(m_filePath);
      }
      m_nativeFilePath = FileEnginePrivate::longFileName(Dir::toNativeSeparators(filePath));
#elif defined(PDK_FILESYSTEMENTRY_NATIVE_PATH_IS_UTF16)
      m_nativeFilePath = Dir::toNativeSeparators(m_filePath);
#else
      m_nativeFilePath = File::encodeName(Dir::toNativeSeparators(m_filePath));
#endif
   }
}

String FileSystemEntry::getFileName() const
{
   findLastSeparator();
#if defined(PDK_OS_WIN)
   if (m_lastSeparator == -1 && m_filePath.length() >= 2 && m_filePath.at(1) == Latin1Character(':')) {
      return m_filePath.substring(2);
   }
#endif
   return m_filePath.substring(m_lastSeparator + 1);
}

String FileSystemEntry::getPath() const
{
   findLastSeparator();
   if (m_lastSeparator == -1) {
#if defined(PDK_OS_WIN)
      if (m_filePath.length() >= 2 && m_filePath.at(1) == Latin1Character(':')) {
         return m_filePath.left(2);
      }
#endif
      return String(Latin1Character('.'));
   }
   if (m_lastSeparator == 0) {
      return String(Latin1Character('/'));
   }
#if defined(PDK_OS_WIN)
   if (m_lastSeparator == 2 && m_filePath.at(1) == Latin1Character(':')) {
      return m_filePath.left(m_lastSeparator + 1);
   }
#endif
   return m_filePath.left(m_lastSeparator);
}

String FileSystemEntry::getBaseName() const
{
   findFileNameSeparators();
   int length = -1;
   if (m_firstDotInFileName >= 0) {
      length = m_firstDotInFileName;
      if (m_lastSeparator != -1) {// avoid off by one
         length--;
      }
   }
#if defined(PDK_OS_WIN)
   if (m_lastSeparator == -1 && m_filePath.length() >= 2 && m_filePath.at(1) == Latin1Character(':')) {
      return m_filePath.substring(2, length - 2);
   }
#endif
   return m_filePath.substring(m_lastSeparator + 1, length);
}

String FileSystemEntry::getCompleteBaseName() const
{
   findFileNameSeparators();
   int length = -1;
   if (m_firstDotInFileName >= 0) {
      length = m_firstDotInFileName + m_lastDotInFileName;
      if (m_lastSeparator != -1) {// avoid off by one
         length--;
      }
   }
#if defined(PDK_OS_WIN)
   if (m_lastSeparator == -1 && m_filePath.length() >= 2 && m_filePath.at(1) == Latin1Character(':')) {
      return m_filePath.substring(2, length - 2);
   }
#endif
   return m_filePath.substring(m_lastSeparator + 1, length);
}

String FileSystemEntry::getSuffix() const
{
   findFileNameSeparators();   
   if (m_lastDotInFileName == -1) {
      return String();
   }
   return m_filePath.substring(std::max((pdk::pint16)0, m_lastSeparator) + m_firstDotInFileName + m_lastDotInFileName + 1);
}

String FileSystemEntry::getCompleteSuffix() const
{
   findFileNameSeparators();
   if (m_firstDotInFileName == -1) {
      return String();
   }
   return m_filePath.substring(std::max((pdk::pint16)0, m_lastSeparator) + m_firstDotInFileName + 1);
}

#if defined(PDK_OS_WIN)
bool FileSystemEntry::isRelative() const
{
   resolveFilePath();
   return (m_filePath.isEmpty()
           || (m_filePath.at(0).unicode() != '/'
         && !(m_filePath.length() >= 2 && m_filePath.at(1).unicode() == ':')));
}

bool FileSystemEntry::isAbsolute() const
{
   resolveFilePath();
   return ((m_filePath.length() >= 3
            && m_filePath.at(0).isLetter()
            && m_filePath.at(1).unicode() == ':'
            && m_filePath.at(2).unicode() == '/')
           || (m_filePath.length() >= 2
               && m_filePath.at(0) == Latin1Character('/')
               && m_filePath.at(1) == Latin1Character('/')));
}
#else
bool FileSystemEntry::isRelative() const
{
   return !isAbsolute();
}

bool FileSystemEntry::isAbsolute() const
{
   resolveFilePath();
   return (!m_filePath.isEmpty() && (m_filePath.at(0).unicode() == '/'));
}
#endif

#if defined(PDK_OS_WIN)
bool FileSystemEntry::isDriveRoot() const
{
   resolveFilePath();
   return FileSystemEntry::isDriveRootPath(m_filePath);
}

bool FileSystemEntry::isDriveRootPath(const String &path)
{
   return (path.length() == 3
           && path.at(0).isLetter() && path.at(1) == Latin1Character(':')
           && path.at(2) == Latin1Character('/'));
}
#endif // PDK_OS_WIN

bool FileSystemEntry::isRootPath(const String &path)
{
   if (path == Latin1String("/")
    #if defined(PDK_OS_WIN)
       || isDriveRootPath(path)
       || isUncRoot(path)
    #endif
       )
      return true;
   
   return false;
}

bool FileSystemEntry::isRoot() const
{
   resolveFilePath();
   return isRootPath(m_filePath);
}

// private methods

void FileSystemEntry::findLastSeparator() const
{
   if (m_lastSeparator == -2) {
      resolveFilePath();
      m_lastSeparator = m_filePath.lastIndexOf(Latin1Character('/'));
   }
}

void FileSystemEntry::findFileNameSeparators() const
{
   if (m_firstDotInFileName == -2) {
      resolveFilePath();
      int firstDotInFileName = -1;
      int lastDotInFileName = -1;
      int lastSeparator = m_lastSeparator;
      
      int stop;
      if (lastSeparator < 0) {
         lastSeparator = -1;
         stop = 0;
      } else {
         stop = lastSeparator;
      }
      
      int i = m_filePath.size() - 1;
      for (; i >= stop; --i) {
         if (m_filePath.at(i).unicode() == '.') {
            firstDotInFileName = lastDotInFileName = i;
            break;
         } else if (m_filePath.at(i).unicode() == '/') {
            lastSeparator = i;
            break;
         }
      }
      
      if (lastSeparator != i) {
         for (--i; i >= stop; --i) {
            if (m_filePath.at(i).unicode() == '.') {
               firstDotInFileName = i;
            } else if (m_filePath.at(i).unicode() == '/') {
               lastSeparator = i;
               break;
            }
         }
      }
      m_lastSeparator = lastSeparator;
      m_firstDotInFileName = firstDotInFileName == -1 ? -1 : firstDotInFileName - std::max(0, lastSeparator);
      if (lastDotInFileName == -1) {
         m_lastDotInFileName = -1;
      } else if (firstDotInFileName == lastDotInFileName) {
         m_lastDotInFileName = 0;
      } else {
         m_lastDotInFileName = lastDotInFileName - firstDotInFileName;
      }
   }
}

bool FileSystemEntry::isClean() const
{
   resolveFilePath();
   int dots = 0;
   bool dotok = true; // checking for ".." or "." starts to relative paths
   bool slashok = true;
   for (String::const_iterator iter = m_filePath.constBegin(); iter != m_filePath.constEnd(); ++iter) {
      if (*iter == Latin1Character('/')) {
         if (dots == 1 || dots == 2) {
            return false; // path contains "./" or "../"
         }
         if (!slashok) {
            return false; // path contains "//"
         }
         dots = 0;
         dotok = true;
         slashok = false;
      } else if (dotok) {
         slashok = true;
         if (*iter == Latin1Character('.')) {
            dots++;
            if (dots > 2) {
               dotok = false;
            } 
         } else {
            //path element contains a character other than '.', it's clean
            dots = 0;
            dotok = false;
         }
      }
   }
   return (dots != 1 && dots != 2); // clean if path doesn't end in . or ..
}

} // internal
} // fs
} // io
} // pdk
