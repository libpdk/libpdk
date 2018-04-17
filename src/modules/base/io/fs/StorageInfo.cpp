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
// Created by softboy on 2018/04/16.

#include "pdk/base/io/fs/StorageInfo.h"
#include "pdk/base/io/fs/internal/StorageInfoPrivate.h"
#include "pdk/base/io/Debug.h"
#include "pdk/global/GlobalStatic.h"

namespace pdk {
namespace io {
namespace fs {

StorageInfo::StorageInfo()
   : m_implPtr(new StorageInfoPrivate)
{
}

StorageInfo::StorageInfo(const String &path)
   : m_implPtr(new StorageInfoPrivate)
{
   setPath(path);
}

StorageInfo::StorageInfo(const Dir &dir)
   : m_implPtr(new StorageInfoPrivate)
{
   setPath(dir.getAbsolutePath());
}

StorageInfo::StorageInfo(const StorageInfo &other)
   : m_implPtr(other.m_implPtr)
{
}

StorageInfo::~StorageInfo()
{
}

StorageInfo &StorageInfo::operator=(const StorageInfo &other)
{
   m_implPtr = other.m_implPtr;
   return *this;
}

void StorageInfo::setPath(const String &path)
{
   if (m_implPtr->m_rootPath == path) {
      return;
   }
   m_implPtr.detach();
   m_implPtr->m_rootPath = path;
   m_implPtr->doStat();
}

String StorageInfo::getRootPath() const
{
   return m_implPtr->m_rootPath;
}

pdk::pint64 StorageInfo::getBytesAvailable() const
{
   return m_implPtr->m_bytesAvailable;
}

pdk::pint64 StorageInfo::getBytesFree() const
{
   return m_implPtr->m_bytesFree;
}

pdk::pint64 StorageInfo::getBytesTotal() const
{
   return m_implPtr->m_bytesTotal;
}

int StorageInfo::getBlockSize() const
{
   return m_implPtr->m_blockSize;
}

ByteArray StorageInfo::getFileSystemType() const
{
   return m_implPtr->m_fileSystemType;
}

ByteArray StorageInfo::getDevice() const
{
   return m_implPtr->m_device;
}

ByteArray StorageInfo::getSubvolume() const
{
   return m_implPtr->m_subvolume;
}

String StorageInfo::getName() const
{
   return m_implPtr->m_name;
}

String StorageInfo::getDisplayName() const
{
   if (!m_implPtr->m_name.isEmpty()) {
      return m_implPtr->m_name;
   }
   return m_implPtr->m_rootPath;
}

bool StorageInfo::isReadOnly() const
{
   return m_implPtr->m_readOnly;
}

bool StorageInfo::isReady() const
{
   return m_implPtr->m_ready;
}

bool StorageInfo::isValid() const
{
   return m_implPtr->m_valid;
}

void StorageInfo::refresh()
{
   m_implPtr.detach();
   m_implPtr->doStat();
}

std::list<StorageInfo> StorageInfo::getMountedVolumes()
{
   return StorageInfoPrivate::getMountedVolumes();
}

PDK_GLOBAL_STATIC_WITH_ARGS(StorageInfo, sg_storage_root, (StorageInfoPrivate::getRoot()));

StorageInfo StorageInfo::getRoot()
{
   return *sg_storage_root();
}

#ifndef PDK_NO_DEBUG_STREAM
Debug operator<<(Debug debug, const StorageInfo &storageInfo)
{
   DebugStateSaver saver(debug);
   debug.nospace();
   debug.noquote();
   debug << "StorageInfo(";
   if (storageInfo.isValid()) {
      const StorageInfoPrivate *implPtr = storageInfo.m_implPtr.constData();
      debug << '"' << implPtr->m_rootPath << '"';
      if (!implPtr->m_fileSystemType.isEmpty()) {
         debug << ", type=" << implPtr->m_fileSystemType;
      }
      if (!implPtr->m_name.isEmpty()) {
         debug << ", name=\"" << implPtr->m_name << '"';
      }
      if (!implPtr->m_device.isEmpty()) {
         debug << ", device=\"" << implPtr->m_device << '"';
      }
      if (!implPtr->m_subvolume.isEmpty()) {
         debug << ", subvolume=\"" << implPtr->m_subvolume << '"';
      }
      if (implPtr->m_readOnly) {
         debug << " [read only]";
      }
      debug << (implPtr->m_ready ? " [ready]" : " [not ready]");
      if (implPtr->m_bytesTotal > 0) {
         debug << ", bytesTotal=" << implPtr->m_bytesTotal << ", bytesFree=" << implPtr->m_bytesFree
               << ", bytesAvailable=" << implPtr->m_bytesAvailable;
      }
   } else {
      debug << "invalid";
   }
   debug<< ')';
   return debug;
}
#endif // !PDK_NO_DEBUG_STREAM

} // fs
} // io
} // pdk
