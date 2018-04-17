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

#include "pdk/base/io/fs/internal/StorageInfoPrivate.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/TextStream.h"
#include "pdk/kernel/internal/CoreUnixPrivate.h"
#include <cerrno>
#include <sys/stat.h>

#if defined(PDK_OS_BSD4)
#  include <sys/mount.h>
#  include <sys/statvfs.h>
#elif defined(PDK_OS_LINUX) || defined(PDK_OS_HURD)
#  include <mntent.h>
#  include <sys/statvfs.h>
#elif defined(PDK_OS_SOLARIS)
#  include <sys/mnttab.h>
#  include <sys/statvfs.h>
#elif defined(PDK_OS_HAIKU)
#  include <Directory.h>
#  include <Path.h>
#  include <Volume.h>
#  include <VolumeRoster.h>
#  include <fs_info.h>
#  include <sys/statvfs.h>
#else
#  include <sys/statvfs.h>
#endif

#if defined(PDK_OS_BSD4)
#  if defined(PDK_OS_NETBSD)
#    define PDK_STATFSBUF struct statvfs
#    define PDK_STATFS    ::statvfs
#  else
#    define PDK_STATFSBUF struct statfs
#    define PDK_STATFS    ::statfs
#  endif

#  if !defined(ST_RDONLY)
#    define ST_RDONLY MNT_RDONLY
#  endif
#  if !defined(_STATFS_F_FLAGS) && !defined(PDK_OS_NETBSD)
#    define _STATFS_F_FLAGS 1
#  endif
#elif defined(PDK_OS_HAIKU)
#  define PDK_STATFSBUF struct statvfs
#  define PDK_STATFS    ::statvfs
#else
#  if defined(PDK_LARGEFILE_SUPPORT)
#    define PDK_STATFSBUF struct statvfs64
#    define PDK_STATFS    ::statvfs64
#  else
#    define PDK_STATFSBUF struct statvfs
#    define PDK_STATFS    ::statvfs
#  endif // PDK_LARGEFILE_SUPPORT
#endif // PDK_OS_BSD4

#if PDK_HAS_INCLUDE(<paths.h>)
#  include <paths.h>
#endif
#ifndef _PATH_MOUNTED
#  define _PATH_MOUNTED     "/etc/mnttab"
#endif

namespace pdk {
namespace io {
namespace fs {

namespace internal {

class StorageIterator
{
public:
   StorageIterator();
   ~StorageIterator();
   
   inline bool isValid() const;
   inline bool next();
   inline String getRootPath() const;
   inline ByteArray getFileSystemType() const;
   inline ByteArray getDevice() const;
   inline ByteArray getOptions() const;
private:
#if defined(PDK_OS_BSD4)
   PDK_STATFSBUF *m_statBuf;
   int m_entryCount;
   int m_currentIndex;
#elif defined(PDK_OS_SOLARIS)
   FILE *m_fp;
   mnttab m_mnt;
#elif defined(PDK_OS_LINUX) || defined(PDK_OS_HURD)
   FILE *m_fp;
   mntent m_mnt;
   ByteArray m_buffer;
#elif defined(PDK_OS_HAIKU)
   BVolumeRoster m_volumeRoster;
   ByteArray m_rootPath;
   ByteArray m_fileSystemType;
   ByteArray m_device;
#endif
};

namespace {

template <typename StringType>
bool is_parent_of(const StringType &parent, const String &dirName)
{
   return dirName.startsWith(parent) &&
         (dirName.size() == parent.size() || dirName.at(parent.size()) == Latin1Character('/') ||
          parent.size() == 1);
}

bool should_include_fs(const StorageIterator &iter)
{
   /*
     * This function implements a heuristic algorithm to determine whether a
     * given mount should be reported to the user. Our objective is to list
     * only entries that the end-user would find useful.
     *
     * We therefore ignore:
     *  - mounted in /dev, /proc, /sys: special mounts
     *    (this will catch /sys/fs/cgroup, /proc/sys/fs/binfmt_misc, /dev/pts,
     *    some of which are tmpfs on Linux)
     *  - mounted in /var/run or /var/lock: most likely pseudofs
     *    (on earlier systemd versions, /var/run was a bind-mount of /run, so
     *    everything would be unnecessarily duplicated)
     *  - filesystem type is "rootfs": artifact of the root-pivot on some Linux
     *    initrd
     *  - if the filesystem total size is zero, it's a pseudo-fs (not checked here).
     */
   
   String mountDir = iter.getRootPath();
   if (is_parent_of(Latin1String("/dev"), mountDir)
       || is_parent_of(Latin1String("/proc"), mountDir)
       || is_parent_of(Latin1String("/sys"), mountDir)
       || is_parent_of(Latin1String("/var/run"), mountDir)
       || is_parent_of(Latin1String("/var/lock"), mountDir)) {
      return false;
   }
   
#ifdef PDK_OS_LINUX
   if (it.getFileSystemType() == "rootfs")
      return false;
#endif
   // size checking in mountedVolumes()
   return true;
}

} // anonymous namespace

#if defined(PDK_OS_BSD4)

#ifndef MNT_NOWAIT
#  define MNT_NOWAIT 0
#endif

inline StorageIterator::StorageIterator()
   : m_entryCount(::getmntinfo(&m_statBuf, MNT_NOWAIT)),
     m_currentIndex(-1)
{
}

inline StorageIterator::~StorageIterator()
{
}

inline bool StorageIterator::isValid() const
{
   return m_entryCount != -1;
}

inline bool StorageIterator::next()
{
   return ++m_currentIndex < m_entryCount;
}

inline String StorageIterator::getRootPath() const
{
   return File::decodeName(m_statBuf[m_currentIndex].f_mntonname);
}

inline ByteArray StorageIterator::getFileSystemType() const
{
   return ByteArray(m_statBuf[m_currentIndex].f_fstypename);
}

inline ByteArray StorageIterator::getDevice() const
{
   return ByteArray(m_statBuf[m_currentIndex].f_mntfromname);
}

inline ByteArray StorageIterator::getOptions() const
{
   return ByteArray();
}

#elif defined(PDK_OS_SOLARIS)

inline StorageIterator::StorageIterator()
{
   const int fd = pdk::kernel::safe_open(_PATH_MOUNTED, O_RDONLY);
   m_fp = ::fdopen(fd, "r");
}

inline StorageIterator::~StorageIterator()
{
   if (m_fp) {
      ::fclose(m_fp);
   }      
}

inline bool StorageIterator::isValid() const
{
   return m_fp != nullptr;
}

inline bool StorageIterator::next()
{
   return ::getmntent(m_fp, &m_mnt) == 0;
}

inline String StorageIterator::getRootPath() const
{
   return File::decodeName(m_mnt.mnt_mountp);
}

inline ByteArray StorageIterator::getFileSystemType() const
{
   return ByteArray(m_mnt.mnt_fstype);
}

inline ByteArray StorageIterator::getDevice() const
{
   return ByteArray(m_mnt.mnt_mntopts);
}

#elif defined(PDK_OS_LINUX) || defined(PDK_OS_HURD)

static const int bufferSize = 1024; // 2 paths (mount point+device) and metainfo;
// should be enough

inline StorageIterator::StorageIterator() :
   buffer(ByteArray(m_bufferSize, 0))
{
   m_fp = ::setmntent(_PATH_MOUNTED, "r");
}

inline StorageIterator::~StorageIterator()
{
   if (m_fp) {
      ::endmntent(m_fp);
   }
}

inline bool StorageIterator::isValid() const
{
   return m_fp != nullptr;
}

inline bool StorageIterator::next()
{
   return ::getmntent_r(m_fp, &m_mnt, m_buffer.getConstRawData), m_buffer.size()) != nullptr;
}

inline String StorageIterator::rootPath() const
{
   return File::decodeName(m_mnt.mnt_dir);
}

inline ByteArray StorageIterator::getFileSystemType() const
{
   return ByteArray(m_mnt.mnt_type);
}

inline ByteArray StorageIterator::getDevice() const
{
   return ByteArray(m_mnt.mnt_fsname);
}

inline ByteArray StorageIterator::getOptions() const
{
   return ByteArray(m_mnt.mnt_opts);
}

#elif defined(PDK_OS_HAIKU)
inline StorageIterator::StorageIterator()
{
}

inline StorageIterator::~StorageIterator()
{
}

inline bool StorageIterator::isValid() const
{
   return true;
}

inline bool StorageIterator::next()
{
   BVolume volume;
   
   if (m_volumeRoster.GetNextVolume(&volume) != B_OK) {
      return false;
   }
   BDirectory directory;
   if (volume.GetRootDirectory(&directory) != B_OK) {
      return false;
   }
   const BPath path(&directory);
   
   fs_info fsInfo;
   memset(&fsInfo, 0, sizeof(fsInfo));
   
   if (fs_stat_dev(volume.Device(), &fsInfo) != 0) {
      return false;
   }
   m_rootPath = path.Path();
   m_fileSystemType = ByteArray(fsInfo.fsh_name);
   
   const ByteArray deviceName(fsInfo.device_name);
   m_device = (deviceName.isEmpty() ? ByteArray::number(qint32(volume.Device())) : deviceName);
   
   return true;
}

inline String StorageIterator::getRootPath() const
{
   return File::decodeName(m_rootPath);
}

inline ByteArray StorageIterator::getFileSystemType() const
{
   return m_fileSystemType;
}

inline ByteArray StorageIterator::getDevice() const
{
   return m_device;
}

inline ByteArray StorageIterator::getOptions() const
{
   return ByteArray();
}

#else

inline StorageIterator::StorageIterator()
{
}

inline StorageIterator::~StorageIterator()
{
}

inline bool StorageIterator::isValid() const
{
   return false;
}

inline bool StorageIterator::next()
{
   return false;
}

inline String StorageIterator::getRootPath() const
{
   return String();
}

inline ByteArray StorageIterator::getFileSystemType() const
{
   return ByteArray();
}

inline ByteArray StorageIterator::getDevice() const
{
   return ByteArray();
}

inline ByteArray StorageIterator::getOptions() const
{
   return ByteArray();
}

#endif

namespace {

ByteArray extract_sub_volume(const StorageIterator &iter)
{
#ifdef PDK_OS_LINUX
   if (iter.fileSystemType() == "btrfs") {
      const ByteArrayList opts = iter.getOptions().split(',');
      ByteArray id;
      for (const ByteArray &opt : opts) {
         static const char subvol[] = "subvol=";
         static const char subvolid[] = "subvolid=";
         if (opt.startsWith(subvol)) {
            return std::move(opt).mid(strlen(subvol));
         }   
         if (opt.startsWith(subvolid)) {
            id = std::move(opt).mid(strlen(subvolid));
         }
      }
      // if we didn't find the subvolume name, return the subvolume ID
      return id;
   }
#else
   PDK_UNUSED(iter);
#endif
   return ByteArray();
}

} // anonymous namespace

void StorageInfoPrivate::initRootPath()
{
   m_rootPath = FileInfo(m_rootPath).getCanonicalFilePath();
   if (m_rootPath.isEmpty()) {
      return;
   }
   StorageIterator iter;
   if (!iter.isValid()) {
      m_rootPath = StringLiteral("/");
      return;
   }
   
   int maxLength = 0;
   const String oldRootPath = m_rootPath;
   m_rootPath.clear();
   
   while (iter.next()) {
      const String mountDir = iter.getRootPath();
      const ByteArray fsName = iter.getFileSystemType();
      // we try to find most suitable entry
      if (is_parent_of(mountDir, oldRootPath) && maxLength < mountDir.length()) {
         maxLength = mountDir.length();
         m_rootPath = mountDir;
         m_device = iter.getDevice();
         m_fileSystemType = fsName;
         m_subvolume = extract_sub_volume(iter);
      }
   }
}

namespace {

#ifdef PDK_OS_LINUX
// udev encodes the labels with ID_LABEL_FS_ENC which is done with
// blkid_encode_string(). Within this function some 1-byte utf-8
// characters not considered safe (e.g. '\' or ' ') are encoded as hex
String decode_fs_enc_string(const String &str)
{
   String decoded;
   decoded.reserve(str.size());
   int i = 0;
   while (i < str.size()) {
      if (i <= str.size() - 4) {    // we need at least four characters \xAB
         if (str.at(i) == Latin1Character('\\') &&
             str.at(i+1) == Latin1Character('x')) {
            bool bOk;
            const int code = str.substringRef(i+2, 2).toInt(&bOk, 16);
            // only decode characters between 0x20 and 0x7f but not
            // the backslash to prevent collisions
            if (bOk && code >= 0x20 && code < 0x80 && code != '\\') {
               decoded += Character(code);
               i += 4;
               continue;
            }
         }
      }
      decoded += str.at(i);
      ++i;
   }
   return decoded;
}
#endif

inline String retrieve_label(const ByteArray &device)
{
#ifdef PDK_OS_LINUX
   static const char pathDiskByLabel[] = "/dev/disk/by-label";
   
   FileInfo devinfo(File::decodeName(device));
   String devicePath = devinfo.getCanonicalFilePath();
   
   DirIterator iter(Latin1String(pathDiskByLabel), Dir::Filter::NoDotAndDotDot);
   while (iter.hasNext()) {
      iter.next();
      FileInfo fileInfo(iter.getFileInfo());
      if (fileInfo.isSymLink() && fileInfo.getSymLinkTarget() == devicePath) {
         return decodeFsEncString(fileInfo.getFileName());
      }
   }
#elif defined PDK_OS_HAIKU
   fs_info fsInfo;
   memset(&fsInfo, 0, sizeof(fsInfo));
   
   int32 pos = 0;
   dev_t dev;
   while ((dev = next_dev(&pos)) >= 0) {
      if (fs_stat_dev(dev, &fsInfo) != 0) {
         continue;
      }
      if (pdk::strcmp(fsInfo.device_name, device.getConstRawData()) == 0) {
         return String::fromLocal8Bit(fsInfo.volume_name);
      }
      
   }
#else
   PDK_UNUSED(device);
#endif
   
   return String();
}

} // anonymous namespace

void StorageInfoPrivate::doStat()
{
   initRootPath();
   if (m_rootPath.isEmpty()) {
      return;
   }
   retrieveVolumeInfo();
   m_name = retrieve_label(m_device);
}

void StorageInfoPrivate::retrieveVolumeInfo()
{
   PDK_STATFSBUF statfs_buf;
   int result;
   PDK_EINTR_LOOP(result, PDK_STATFS(File::encodeName(m_rootPath).getConstRawData(), &statfs_buf));
   if (result == 0) {
      m_valid = true;
      m_ready = true;
      
#if defined(PDK_OS_INTEGRITY) || (defined(PDK_OS_BSD4) && !defined(PDK_OS_NETBSD))
      m_bytesTotal = statfs_buf.f_blocks * statfs_buf.f_bsize;
      m_bytesFree = statfs_buf.f_bfree * statfs_buf.f_bsize;
      m_bytesAvailable = statfs_buf.f_bavail * statfs_buf.f_bsize;
#else
      m_bytesTotal = statfs_buf.f_blocks * statfs_buf.f_frsize;
      m_bytesFree = statfs_buf.f_bfree * statfs_buf.f_frsize;
      m_bytesAvailable = statfs_buf.f_bavail * statfs_buf.f_frsize;
#endif
      m_blockSize = statfs_buf.f_bsize;
#if defined(PDK_OS_BSD4) || defined(PDK_OS_INTEGRITY)
#if defined(_STATFS_F_FLAGS)
      m_readOnly = (statfs_buf.f_flags & ST_RDONLY) != 0;
#endif
#else
      m_readOnly = (statfs_buf.f_flag & ST_RDONLY) != 0;
#endif
   }
}

std::list<StorageInfo> StorageInfoPrivate::getMountedVolumes()
{
   StorageIterator iter;
   if (!iter.isValid()) {
      return std::list<StorageInfo>{getRoot()};
   }
   std::list<StorageInfo> volumes;
   while (iter.next()) {
      if (!should_include_fs(iter)) {
         continue;
      }
      const String mountDir = iter.getRootPath();
      StorageInfo info(mountDir);
      if (info.getBytesTotal() == 0) {
         continue;
      }
      volumes.push_back(info);
   }
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
