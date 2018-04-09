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

#include "pdk/base/io/fs/DirIterator.h"
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEntryPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemMetaDataPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/os/thread/ReadWriteLock.h"
#include "pdk/base/lang/StringBuilder.h"
#include "pdk/global/GlobalStatic.h"

#include <any>
#include <list>

// @TODO include resource header

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::os::thread::ReadWriteLock;
using pdk::os::thread::WriteLocker;
using pdk::os::thread::ReadLocker;
using pdk::time::DateTime;

static bool sg_fileEngineHandlersInUse = false;

// All application-wide handlers are stored in this list. The mutex must be
// acquired to ensure thread safety.
PDK_GLOBAL_STATIC_WITH_ARGS(ReadWriteLock, sg_fileEngineHandlerMutex, (ReadWriteLock::RecursionMode::Recursion));
static bool sg_abstractFileEngineHandlerListShutDown = false;

class AbstractFileEngineHandlerList : public std::list<AbstractFileEngineHandler *>
{
public:
   ~AbstractFileEngineHandlerList()
   {
      WriteLocker locker(sg_fileEngineHandlerMutex());
      sg_abstractFileEngineHandlerListShutDown = true;
   }
   
   bool removeOne(const_reference value)
   {
      auto iter = std::find(cbegin(), cend(), value);
      if (iter == cend()) {
         return false;
      }
      erase(iter);
      return true;
   }
   
   const_reference at(int pos) const
   {
      PDK_ASSERT((pos >= 0 && static_cast<size_type>(pos) < size()));
      auto iter = cbegin();
      std::advance(iter, pos);
      return *iter;
   }
};

PDK_GLOBAL_STATIC(AbstractFileEngineHandlerList, sg_fileEngineHandlers);

AbstractFileEngineHandler::AbstractFileEngineHandler()
{
   WriteLocker locker(sg_fileEngineHandlerMutex());
   sg_fileEngineHandlersInUse = true;
   sg_fileEngineHandlers()->push_front(this);
}

AbstractFileEngineHandler::~AbstractFileEngineHandler()
{
   WriteLocker locker(sg_fileEngineHandlerMutex());
   // Remove this handler from the handler list only if the list is valid.
   if (!sg_abstractFileEngineHandlerListShutDown) {
      AbstractFileEngineHandlerList *handlers = sg_fileEngineHandlers();
      handlers->removeOne(this);
      if (handlers->empty()) {
         sg_fileEngineHandlersInUse = false;
      }
   }
}

AbstractFileEngine *pdk_custom_file_engine_handler_create(const String &path)
{
   AbstractFileEngine *engine = nullptr;
   if (sg_fileEngineHandlersInUse) {
      ReadLocker locker(sg_fileEngineHandlerMutex());
      // check for registered handlers that can load the file
      AbstractFileEngineHandlerList *handlers = sg_fileEngineHandlers();
      for (AbstractFileEngineHandlerList::size_type i = 0; i < handlers->size(); ++i) {
         if ((engine = handlers->at(i)->create(path))) {
            break;
         }
      }
   }
   return engine;
}

AbstractFileEngine *AbstractFileEngine::create(const String &fileName)
{
   FileSystemEntry entry(fileName);
   FileSystemMetaData metaData;
   AbstractFileEngine *engine = FileSystemEngine::resolveEntryAndCreateLegacyEngine(entry, metaData);
   
#ifndef PDK_NO_FILEENGINE
   if (!engine) {
      // fall back to regular file engine
      return new FileEngine(entry.getFilePath());
   }
#endif
   return engine;
}

AbstractFileEngine::AbstractFileEngine() 
   : m_implPtr(new AbstractFileEnginePrivate)
{
   m_implPtr->m_apiPtr = this;
}

AbstractFileEngine::AbstractFileEngine(AbstractFileEnginePrivate &dd)
   : m_implPtr(&dd)
{
   m_implPtr->m_apiPtr = this;
}

AbstractFileEngine::~AbstractFileEngine()
{
}

bool AbstractFileEngine::open(IoDevice::OpenModes openMode)
{
   PDK_UNUSED(openMode);
   return false;
}

bool AbstractFileEngine::close()
{
   return false;
}

bool AbstractFileEngine::syncToDisk()
{
   return false;
}

bool AbstractFileEngine::flush()
{
   return false;
}

pdk::pint64 AbstractFileEngine::getSize() const
{
   return 0;
}

pdk::pint64 AbstractFileEngine::getPosition() const
{
   return 0;
}

bool AbstractFileEngine::seek(pdk::pint64 pos)
{
   PDK_UNUSED(pos);
   return false;
}

bool AbstractFileEngine::isSequential() const
{
   return false;
}

bool AbstractFileEngine::remove()
{
   return false;
}

bool AbstractFileEngine::copy(const String &newName)
{
   PDK_UNUSED(newName);
   return false;
}

bool AbstractFileEngine::rename(const String &newName)
{
   PDK_UNUSED(newName);
   return false;
}

bool AbstractFileEngine::renameOverwrite(const String &newName)
{
   PDK_UNUSED(newName);
   return false;
}

bool AbstractFileEngine::link(const String &newName)
{
   PDK_UNUSED(newName);
   return false;
}

bool AbstractFileEngine::mkdir(const String &dirName, bool createParentDirectories) const
{
   PDK_UNUSED(dirName);
   PDK_UNUSED(createParentDirectories);
   return false;
}

bool AbstractFileEngine::rmdir(const String &dirName, bool recurseParentDirectories) const
{
   PDK_UNUSED(dirName);
   PDK_UNUSED(recurseParentDirectories);
   return false;
}

bool AbstractFileEngine::setSize(pdk::pint64 size)
{
   PDK_UNUSED(size);
   return false;
}

bool AbstractFileEngine::caseSensitive() const
{
   return false;
}

bool AbstractFileEngine::isRelativePath() const
{
   return false;
}

StringList AbstractFileEngine::getEntryList(Dir::Filters filters, const StringList &filterNames) const
{
   StringList ret;
   DirIterator iter(getFileName(), filterNames, filters);
   while (iter.hasNext()) {
      iter.next();
      ret.push_back(iter.getFileName());
   }
   return ret;
}

AbstractFileEngine::FileFlags AbstractFileEngine::getFileFlags(FileFlags type) const
{
   PDK_UNUSED(type);
   return 0;
}

bool AbstractFileEngine::setPermissions(uint perms)
{
   PDK_UNUSED(perms);
   return false;
}

ByteArray AbstractFileEngine::getId() const
{
   return ByteArray();
}

String AbstractFileEngine::getFileName(FileName file) const
{
   PDK_UNUSED(file);
   return String();
}

uint AbstractFileEngine::getOwnerId(FileOwner owner) const
{
   PDK_UNUSED(owner);
   return 0;
}

String AbstractFileEngine::getOwner(FileOwner owner) const
{
   PDK_UNUSED(owner);
   return String();
}

bool AbstractFileEngine::setFileTime(const DateTime &newDate, FileTime time)
{
   PDK_UNUSED(newDate);
   PDK_UNUSED(time);
   return false;
}

DateTime AbstractFileEngine::getFileTime(FileTime time) const
{
   PDK_UNUSED(time);
   return DateTime();
}

void AbstractFileEngine::setFileName(const String &file)
{
   PDK_UNUSED(file);
}

int AbstractFileEngine::getHandle() const
{
   return -1;
}

bool AbstractFileEngine::atEnd() const
{
   return const_cast<AbstractFileEngine *>(this)->extension(AtEndExtension);
}

uchar *AbstractFileEngine::map(pdk::pint64 offset, pdk::pint64 size, File::MemoryMapFlags flags)
{
   MapExtensionOption option;
   option.offset = offset;
   option.size = size;
   option.flags = flags;
   MapExtensionReturn r;
   if (!extension(MapExtension, &option, &r)) {
      return 0;
   }
   return r.address;
}

bool AbstractFileEngine::unmap(uchar *address)
{
   UnMapExtensionOption options;
   options.address = address;
   return extension(UnMapExtension, &options);
}

bool AbstractFileEngine::cloneTo(AbstractFileEngine *target)
{
   PDK_UNUSED(target);
   return false;
}

class AbstractFileEngineIteratorPrivate
{
public:
   String m_path;
   Dir::Filters m_filters;
   StringList m_nameFilters;
   FileInfo m_fileInfo;
};

AbstractFileEngineIterator::AbstractFileEngineIterator(Dir::Filters filters,
                                                       const StringList &nameFilters)
   : m_implPtr(new AbstractFileEngineIteratorPrivate)
{
   m_implPtr->m_nameFilters = nameFilters;
   m_implPtr->m_filters = filters;
}

AbstractFileEngineIterator::~AbstractFileEngineIterator()
{
}

String AbstractFileEngineIterator::getPath() const
{
   return m_implPtr->m_path;
}

void AbstractFileEngineIterator::setPath(const String &path)
{
   m_implPtr->m_path = path;
}

StringList AbstractFileEngineIterator::getNameFilters() const
{
   return m_implPtr->m_nameFilters;
}

Dir::Filters AbstractFileEngineIterator::getFilters() const
{
   return m_implPtr->m_filters;
}

String AbstractFileEngineIterator::getCurrentFilePath() const
{
   String name = getCurrentFileName();
   if (!name.isNull()) {
      String tmp = getPath();
      if (!tmp.isEmpty()) {
         if (!tmp.endsWith(Latin1Character('/'))) {
            tmp.append(Latin1Character('/'));
         }
         name.prepend(tmp);
      }
   }
   return name;
}

FileInfo AbstractFileEngineIterator::getCurrentFileInfo() const
{
   String path = getCurrentFilePath();
   if (m_implPtr->m_fileInfo.getFilePath() != path) {
      m_implPtr->m_fileInfo.setFile(path);
   }
   // return a shallow copy
   return m_implPtr->m_fileInfo;
}

std::any AbstractFileEngineIterator::entryInfo(EntryInfoType type) const
{
   PDK_UNUSED(type);
   return std::any();
}

AbstractFileEngine::Iterator *AbstractFileEngine::beginEntryList(Dir::Filters filters, const StringList &filterNames)
{
   PDK_UNUSED(filters);
   PDK_UNUSED(filterNames);
   return 0;
}

AbstractFileEngine::Iterator *AbstractFileEngine::endEntryList()
{
   return 0;
}

pdk::pint64 AbstractFileEngine::read(char *data, pdk::pint64 maxlen)
{
   PDK_UNUSED(data);
   PDK_UNUSED(maxlen);
   return -1;
}

pdk::pint64 AbstractFileEngine::write(const char *data, pdk::pint64 len)
{
   PDK_UNUSED(data);
   PDK_UNUSED(len);
   return -1;
}

pdk::pint64 AbstractFileEngine::readLine(char *data, pdk::pint64 maxlen)
{
   pdk::pint64 readSoFar = 0;
   while (readSoFar < maxlen) {
      char c;
      pdk::pint64 readResult = read(&c, 1);
      if (readResult <= 0) {
         return (readSoFar > 0) ? readSoFar : -1;
      }
      ++readSoFar;
      *data++ = c;
      if (c == '\n') {
         return readSoFar;
      } 
   }
   return readSoFar;
}

bool AbstractFileEngine::extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output)
{
   PDK_UNUSED(extension);
   PDK_UNUSED(option);
   PDK_UNUSED(output);
   return false;
}

bool AbstractFileEngine::supportsExtension(Extension extension) const
{
   PDK_UNUSED(extension);
   return false;
}

File::FileError AbstractFileEngine::getError() const
{
   PDK_D(const AbstractFileEngine);
   return implPtr->m_fileError;
}

String AbstractFileEngine::getErrorString() const
{
   PDK_D(const AbstractFileEngine);
   return implPtr->m_errorString;
}

void AbstractFileEngine::setError(File::FileError error, const String &errorString)
{
   PDK_D(AbstractFileEngine);
   implPtr->m_fileError = error;
   implPtr->m_errorString = errorString;
}

} // internal
} // fs
} // io
} // pdk
