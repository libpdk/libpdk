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
// Created by softboy on 2018/04/17.

#include "pdk/base/io/fs/internal/StorageInfoPrivate.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/kernel/internal/CoreMacPrivate.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFURLEnumerator.h>

#include <sys/mount.h>
#include <list>

#define PDK_STATFSBUF struct statfs
#define PDK_STATFS    ::statfs

using pdk::io::fs::FileInfo;

namespace pdk {
namespace io {
namespace fs {
namespace internal {

void StorageInfoPrivate::initRootPath()
{
   m_rootPath = FileInfo(m_rootPath).getCanonicalFilePath();
   if (m_rootPath.isEmpty()) {
      return;
   }
   retrieveUrlProperties(true);
}

void StorageInfoPrivate::doStat()
{
   initRootPath();
   if (m_rootPath.isEmpty()) {
      return;
   }
   retrieveLabel();
   retrievePosixInfo();
   retrieveUrlProperties();
}

void StorageInfoPrivate::retrievePosixInfo()
{
   PDK_STATFSBUF statfs_buf;
   int result = PDK_STATFS(File::encodeName(m_rootPath).getConstRawData(), &statfs_buf);
   if (result == 0) {
      m_device = ByteArray(statfs_buf.f_mntfromname);
      m_readOnly = (statfs_buf.f_flags & MNT_RDONLY) != 0;
      m_fileSystemType = ByteArray(statfs_buf.f_fstypename);
      m_blockSize = statfs_buf.f_bsize;
   }
}

static inline pdk::pint64 CFDictionaryGetInt64(CFDictionaryRef dictionary, const void *key)
{
   CFNumberRef cfNumber = (CFNumberRef)CFDictionaryGetValue(dictionary, key);
   if (!cfNumber) {
      return -1;
   }      
   pdk::pint64 result;
   bool ok = CFNumberGetValue(cfNumber, kCFNumberSInt64Type, &result);
   if (!ok) {
      return -1;
   }
   return result;
}

void StorageInfoPrivate::retrieveUrlProperties(bool initRootPath)
{
   static const void *rootPathKeys[] = { kCFURLVolumeURLKey };
   static const void *propertyKeys[] = {
      // kCFURLVolumeNameKey, // 10.7
      // kCFURLVolumeLocalizedNameKey, // 10.7
      kCFURLVolumeTotalCapacityKey,
      kCFURLVolumeAvailableCapacityKey,
      // kCFURLVolumeIsReadOnlyKey // 10.7
   };
   size_t size = (initRootPath ? sizeof(rootPathKeys) : sizeof(propertyKeys)) / sizeof(void*);
   pdk::kernel::CFType<CFArrayRef> keys = CFArrayCreate(kCFAllocatorDefault,
                                                        initRootPath ? rootPathKeys : propertyKeys,
                                                        size,
                                                        nullptr);
   if (!keys) {
      return;
   }
   const pdk::kernel::CFString cfPath = m_rootPath;
   if (initRootPath) {
      m_rootPath.clear();
   }
   pdk::kernel::CFType<CFURLRef> url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                                     cfPath,
                                                                     kCFURLPOSIXPathStyle,
                                                                     true);
   if (!url) {
      return;
   }
   CFErrorRef error;
   pdk::kernel::CFType<CFDictionaryRef> map = CFURLCopyResourcePropertiesForKeys(url, keys, &error);
   if (!map) {
      return;
   }
   if (initRootPath) {
      const CFURLRef rootUrl = (CFURLRef)CFDictionaryGetValue(map, kCFURLVolumeURLKey);
      if (!rootUrl) {
         return;
      }
      m_rootPath = pdk::kernel::CFString(CFURLCopyFileSystemPath(rootUrl, kCFURLPOSIXPathStyle));
      m_valid = true;
      m_ready = true;
      return;
   }
   
   m_bytesTotal = CFDictionaryGetInt64(map, kCFURLVolumeTotalCapacityKey);
   m_bytesAvailable = CFDictionaryGetInt64(map, kCFURLVolumeAvailableCapacityKey);
   m_bytesFree = m_bytesAvailable;
}

void StorageInfoPrivate::retrieveLabel()
{
   pdk::kernel::CFString path = CFStringCreateWithFileSystemRepresentation(0,
                                                                           File::encodeName(m_rootPath).getConstRawData());
   if (!path) {
      return;
   }
   pdk::kernel::CFType<CFURLRef> url = CFURLCreateWithFileSystemPath(0, path, kCFURLPOSIXPathStyle, true);
   if (!url) {
      return;
   }
   pdk::kernel::CFType<CFURLRef> volumeUrl;
   if (!CFURLCopyResourcePropertyForKey(url, kCFURLVolumeURLKey, &volumeUrl, NULL)) {
      return;
   }
   pdk::kernel::CFString volumeName;
   if (!CFURLCopyResourcePropertyForKey(url, kCFURLNameKey, &volumeName, NULL)) {
      return;
   }
   m_name = volumeName;
}

std::list<StorageInfo> StorageInfoPrivate::getMountedVolumes()
{
   std::list<StorageInfo> volumes;
   
   pdk::kernel::CFType<CFURLEnumeratorRef> enumerator;
   enumerator = CFURLEnumeratorCreateForMountedVolumes(nullptr,
                                                       kCFURLEnumeratorSkipInvisibles,
                                                       nullptr);
   CFURLEnumeratorResult result = kCFURLEnumeratorSuccess;
   do {
      CFURLRef url;
      CFErrorRef error;
      result = CFURLEnumeratorGetNextURL(enumerator, &url, &error);
      if (result == kCFURLEnumeratorSuccess) {
         const pdk::kernel::CFString urlString = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
         volumes.push_back(StorageInfo(urlString));
      }
   } while (result != kCFURLEnumeratorEnd);
   
   return volumes;
}

StorageInfo StorageInfoPrivate::getRoot()
{
   return StorageInfo(StringLiteral("/"));
}

} // internal
} // fs
} // io
} // pdk
