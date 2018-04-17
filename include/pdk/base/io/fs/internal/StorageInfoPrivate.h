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

#ifndef PDK_M_BASE_IO_FS_INTERNAL_STORAGE_INFO_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_STORAGE_INFO_PRIVATE_H

#include "pdk/base/io/fs/StorageInfo.h"

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::utils::SharedData;

class StorageInfoPrivate : public SharedData
{
public:
   inline StorageInfoPrivate() :
      m_bytesTotal(-1),
      m_bytesFree(-1),
      m_bytesAvailable(-1),
      m_blockSize(-1),
      m_readOnly(false),
      m_ready(false),
      m_valid(false)
   {}
   
   void initRootPath();
   void doStat();
   
   static std::list<StorageInfo> getMountedVolumes();
   static StorageInfo getRoot();
   
protected:
#if defined(PDK_OS_WIN)
   void retrieveVolumeInfo();
   void retrieveDiskFreeSpace();
#elif defined(PDK_OS_UNIX)
   void retrieveVolumeInfo();
#elif defined(PDK_OS_MAC)
   void retrievePosixInfo();
   void retrieveUrlProperties(bool initRootPath = false);
   void retrieveLabel();
#endif
   
public:
   String m_rootPath;
   ByteArray m_device;
   ByteArray m_subvolume;
   ByteArray m_fileSystemType;
   String m_name;
   
   pdk::pint64 m_bytesTotal;
   pdk::pint64 m_bytesFree;
   pdk::pint64 m_bytesAvailable;
   int m_blockSize;
   
   bool m_readOnly;
   bool m_ready;
   bool m_valid;
};

} // internal
} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_INTERNAL_STORAGE_INFO_PRIVATE_H
