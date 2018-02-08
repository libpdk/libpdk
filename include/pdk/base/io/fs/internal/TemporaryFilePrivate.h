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

#ifndef PDK_M_BASE_IO_FS_INTERNAL_TEMPORARY_FILE_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_TEMPORARY_FILE_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/io/fs/internal/FileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FilePrivate.h"
#include "pdk/base/io/fs/TemporaryFile.h"

#if defined(PDK_OS_LINUX) && PDK_CONFIG(linkat)
#  include <fcntl.h>
#  ifdef O_TMPFILE
// some early libc support had the wrong values for O_TMPFILE
// (see https://bugzilla.gnome.org/show_bug.cgi?id=769453#c18)
#    if (O_TMPFILE & O_DIRECTORY) == O_DIRECTORY
#      define LINUX_UNNAMED_TMPFILE
#    endif
#  endif
#endif

namespace pdk {
namespace io {
namespace fs {
namespace internal {

struct TemporaryFileName
{
   FileSystemEntry::NativePath m_path;
   pdk::sizetype m_pos;
   pdk::sizetype m_length;
   
   TemporaryFileName(const String &templateName);
   FileSystemEntry::NativePath generateNext();
};

#ifndef PDK_NO_TEMPORARYFILE

class TemporaryFilePrivate : public FilePrivate
{
   PDK_DECLARE_PUBLIC(TemporaryFile);
   
public:
   TemporaryFilePrivate();
   explicit TemporaryFilePrivate(const String &templateNameIn);
   ~TemporaryFilePrivate();
   
   AbstractFileEngine *getEngine() const override;
   void resetFileEngine() const;
   void materializeUnnamedFile();
   
   bool m_autoRemove = true;
   String m_templateName = getDefaultTemplateName();
   
   static String getDefaultTemplateName();
   
   friend class LockFilePrivate;
};

class TemporaryFileEngine : public FileEngine
{
   PDK_DECLARE_PRIVATE(FileEngine);
public:
   TemporaryFileEngine(const String *templateName)
      : m_templateName(*templateName)
   {}
   
   void initialize(const String &file, pdk::puint32 mode, bool nameIsTemplate = true)
   {
      PDK_D(FileEngine);
      PDK_ASSERT(!isReallyOpen());
      m_fileMode = mode;
      m_filePathIsTemplate = m_filePathWasTemplate = nameIsTemplate;
      
      if (m_filePathIsTemplate) {
         implPtr->m_fileEntry.clear();
      } else {
         implPtr->m_fileEntry = FileSystemEntry(file);
         FileEngine::setFileName(file);
      }
   }
   ~TemporaryFileEngine();
   
   bool isReallyOpen() const;
   void setFileName(const String &file) override;
   
   bool open(IoDevice::OpenModes flags) override;
   bool remove() override;
   bool rename(const String &newName) override;
   bool renameOverwrite(const String &newName) override;
   bool close() override;
   String getFileName(FileName file) const override;
   
   enum MaterializationMode { Overwrite, DontOverwrite, NameIsTemplate };
   bool materializeUnnamedFile(const String &newName, MaterializationMode mode);
   bool isUnnamedFile() const override final;
   
   const String &m_templateName;
   pdk::puint32 m_fileMode;
   bool m_filePathIsTemplate;
   bool m_filePathWasTemplate;
   bool m_unnamedFile = false;
};

#endif // PDK_NO_TEMPORARYFILE

} // internal
} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_INTERNAL_TEMPORARY_FILE_PRIVATE_H
