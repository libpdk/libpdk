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
// Created by softboy on 2018/02/08.

#include "pdk/base/io/fs/Resource.h"
#include "pdk/base/io/fs/internal/ResourcePrivate.h"
#include "pdk/base/io/fs/internal/ResourceIteratorPrivate.h"
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/lang/StringView.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/utils/Locale.h"
#include "pdk/utils/SharedData.h"
#include "pdk/global/Endian.h"
#include "pdk/global/PlatformDefs.h"
#include "pdk/global/Global.h"
#include "pdk/global/GlobalStatic.h"
#include "pdk/kernel/internal/SystemErrorPrivate.h"

#include <mutex>
#include <list>
#include <set>

#ifdef PDK_OS_UNIX
#include "pdk/kernel/internal/CoreUnixPrivate.h"
#endif

namespace pdk {
namespace io {
namespace fs {

using pdk::lang::StringView;
using pdk::lang::Latin1Character;
using pdk::lang::Latin1String;
using pdk::lang::String;
using pdk::lang::StringRef;
using pdk::os::thread::AtomicInt;

namespace {

String clean_path(const String &path)
{
   String cleanedPath = Dir::cleanPath(path);
   // Dir::cleanPath does not remove two trailing slashes under _Windows_
   // due to support for UNC paths. Remove those manually.
   if (cleanedPath.startsWith(Latin1String("//"))) {
      cleanedPath.remove(0, 1);
   }
   return cleanedPath;
}

} // anonymous namespace

namespace internal {

class StringSplitter
{
public:
   explicit StringSplitter(StringView sv)
      : m_data(sv.data()), 
        m_len(sv.size())
   {
   }
   
   inline bool hasNext()
   {
      while (m_pos < m_len && m_data[m_pos] == m_splitChar) {
         ++m_pos;
      } 
      return m_pos < m_len;
   }
   
   inline StringView next()
   {
      int start = m_pos;
      while (m_pos < m_len && m_data[m_pos] != m_splitChar) {
         ++m_pos;
      }
      return StringView(m_data + start, m_pos - start);
   }
   
   const Character *m_data;
   pdk::sizetype m_len;
   pdk::sizetype m_pos = 0;
   Character m_splitChar = Latin1Character('/');
};

//resource glue
class ResourceRoot
{
   enum class Flags
   {
      Compressed = 0x01,
      Directory = 0x02
   };
   const uchar *m_tree;
   const uchar *m_names;
   const uchar *m_payloads;
   int m_version;
   inline int findOffset(int node) const
   {
      return node * (14 + (m_version >= 0x02 ? 8 : 0));
   } //sizeof each tree element
   
   uint hash(int node) const;
   String getName(int node) const;
   short getFlags(int node) const;
public:
   mutable AtomicInt m_ref;
   
   inline ResourceRoot()
      : m_tree(0),
        m_names(0),
        m_payloads(0),
        m_version(0)
   {}
   
   inline ResourceRoot(int version, const uchar *t, const uchar *n, const uchar *d)
   {
      setSource(version, t, n, d);
   }
   
   virtual ~ResourceRoot()
   {}
   
   int findNode(const String &path, const Locale &locale = Locale()) const;
   inline bool isContainer(int node) const
   {
      return getFlags(node) & pdk::as_integer<Flags>(Flags::Directory);
   }
   
   inline bool isCompressed(int node) const
   {
      return getFlags(node) & pdk::as_integer<Flags>(Flags::Compressed);
   }
   
   const uchar *getData(int node, pdk::pint64 *size) const;
   DateTime lastModified(int node) const;
   StringList getChildren(int node) const;
   virtual String getMappingRoot() const
   {
      return String();
   }
   
   bool mappingRootSubdir(const String &path, String *match=0) const;
   inline bool operator==(const ResourceRoot &other) const
   {
      return m_tree == other.m_tree && m_names == other.m_names && 
            m_payloads == other.m_payloads && 
            m_version == other.m_version; 
   }
   inline bool operator!=(const ResourceRoot &other) const
   {
      return !operator==(other);
   }
   
   enum class ResourceRootType 
   {
      Builtin, 
      File,
      Buffer
   };
   
   virtual ResourceRootType getType() const
   {
      return ResourceRootType::Builtin;
   }
   
protected:
   inline void setSource(int version, const uchar *tree, 
                         const uchar *name, const uchar *data)
   {
      m_tree = tree;
      m_names = name;
      m_payloads = data;
      m_version = version;
   }
};

} // internal

using internal::ResourceRoot;

using ResourceList = std::list<ResourceRoot *>;
struct ResourceGlobalData
{
   std::recursive_mutex m_resourceMutex;
   ResourceList m_resourceList;
   StringList m_resourceSearchPaths;
};
PDK_GLOBAL_STATIC(ResourceGlobalData, sg_resourceGlobalData);

namespace {

inline std::recursive_mutex &resource_mutex()
{
   return sg_resourceGlobalData->m_resourceMutex;
}

inline ResourceList *resource_list()
{
   return &sg_resourceGlobalData->m_resourceList;
}

inline StringList *resource_search_paths()
{
   return &sg_resourceGlobalData->m_resourceSearchPaths;
}

} // anonymous namespace

namespace internal {

using pdk::io::fs::Resource;
class ResourcePrivate {
public:
   inline ResourcePrivate(Resource *apiPtr)
      : m_apiPtr(apiPtr)
   {
      clear();
   }
   inline ~ResourcePrivate()
   {
      clear();
   }
   
   void ensureInitialized() const;
   void ensureChildren() const;
   
   bool load(const String &file);
   void clear();
   
   Locale m_locale;
   String m_fileName;
   String m_absoluteFilePath;
   ResourceList m_related;
   uint m_container : 1;
   mutable uint m_compressed : 1;
   mutable pdk::pint64 m_size;
   mutable const uchar *m_data;
   mutable StringList m_children;
   mutable DateTime m_lastModified;
   
   Resource *m_apiPtr;
   PDK_DECLARE_PUBLIC(Resource);
};

void ResourcePrivate::clear()
{
   m_absoluteFilePath.clear();
   m_compressed = 0;
   m_data = 0;
   m_size = 0;
   m_children.clear();
   m_lastModified = DateTime();
   m_container = 0;
   for(std::list<ResourceRoot *>::size_type i = 0; i < m_related.size(); ++i) {
      ResourceRoot *root = nullptr;
      auto iter = m_related.begin();
      std::advance(iter, i);
      root = *iter;
      if(!root->m_ref.deref()) {
         delete root;
      }
   }
   m_related.clear();
}

bool ResourcePrivate::load(const String &file)
{
   m_related.clear();
   std::lock_guard<std::recursive_mutex> lock(resource_mutex());
   const ResourceList *list = resource_list();
   String cleaned = clean_path(file);
   for(ResourceList::size_type i = 0; i < list->size(); ++i) {
      ResourceRoot *res = nullptr;
      auto iter = list->begin();
      std::advance(iter, i);
      res = *iter;
      const int node = res->findNode(cleaned, m_locale);
      if(node != -1) {
         if(m_related.empty()) {
            m_container = res->isContainer(node);
            if(!m_container) {
               m_data = res->getData(node, &m_size);
               m_compressed = res->isCompressed(node);
            } else {
               m_data = nullptr;
               m_size = 0;
               m_compressed = 0;
            }
            m_lastModified = res->lastModified(node);
         } else if(res->isContainer(node) != m_container) {
            //warning_stream("ResourceInfo: Resource [%s] has both data and children!", file.toLatin1().constData());
         }
         res->m_ref.ref();
         m_related.push_back(res);
      } else if(res->mappingRootSubdir(file)) {
         m_container = true;
         m_data = 0;
         m_size = 0;
         m_compressed = 0;
         m_lastModified = DateTime();
         res->m_ref.ref();
         m_related.push_back(res);
      }
   }
   return !m_related.empty();
}

void ResourcePrivate::ensureInitialized() const
{
   if(!m_related.empty()) {
      return;
   }
   ResourcePrivate *that = const_cast<ResourcePrivate *>(this);
   if(m_fileName == Latin1String(":")) {
      that->m_fileName += Latin1Character('/');
   }
   that->m_absoluteFilePath = m_fileName;
   if(!that->m_absoluteFilePath.startsWith(Latin1Character(':'))) {
      that->m_absoluteFilePath.prepend(Latin1Character(':'));
   }
   StringRef path(&m_fileName);
   if(path.startsWith(Latin1Character(':'))) {
      path = path.substring(1);
   }
   if(path.startsWith(Latin1Character('/'))) {
      that->load(path.toString());
   } else {
      std::lock_guard<std::recursive_mutex> lock(resource_mutex());
      StringList searchPaths = *resource_search_paths();
      searchPaths.push_back(Latin1String(""));
      for(StringList::size_type i = 0; i < searchPaths.size(); ++i) {
         const String searchPath(searchPaths.at(i) + Latin1Character('/') + path);
         if(that->load(searchPath)) {
            that->m_absoluteFilePath = Latin1Character(':') + searchPath;
            break;
         }
      }
   }
}

void ResourcePrivate::ensureChildren() const
{
   ensureInitialized();
   if(!m_children.empty() || !m_container || m_related.empty()) {
      return;
   }
   String path = m_absoluteFilePath;
   String k;
   if(path.startsWith(Latin1Character(':'))) {
      path = path.substring(1);
   }
   
   std::set<String> kids;
   String cleaned = clean_path(path);
   for(ResourceList::size_type i = 0; i < m_related.size(); ++i) {
      ResourceRoot *res = nullptr;
      auto iter = m_related.begin();
      std::advance(iter, i);
      res = *iter;
      if(res->mappingRootSubdir(path, &k) && !k.isEmpty()) {
         if(kids.find(k) == kids.end()) {
            m_children.push_back(k);
            kids.insert(k);
         }
      } else {
         const int node = res->findNode(cleaned);
         if(node != -1) {
            StringList relatedChildren = res->getChildren(node);
            for(StringList::size_type kid = 0; kid < relatedChildren.size(); ++kid) {
               k = relatedChildren.at(kid);
               if(kids.find(k) == kids.end()) {
                  m_children.push_back(k);
                  kids.insert(k);
               }
            }
         }
      }
   }
}

inline uint ResourceRoot::hash(int node) const
{
   if(!node) {
      //root 
      return 0;
   }
   const int offset = findOffset(node);
   pdk::pint32 nameOffset = pdk::pdk_from_big_endian<pdk::pint32>(m_tree + offset);
   nameOffset += 2; //jump past name length
   return pdk::pdk_from_big_endian<pdk::puint32>(m_names + nameOffset);
}

inline String ResourceRoot::getName(int node) const
{
   if(!node) {
      // root
      return String();
   }
   const int offset = findOffset(node);
   String ret;
   pdk::pint32 nameOffset = pdk::pdk_from_big_endian<pdk::pint32>(m_tree + offset);
   const pdk::pint16 nameLength = pdk::pdk_from_big_endian<pdk::pint16>(m_names + nameOffset);
   nameOffset += 2;
   nameOffset += 4; //jump past hash
   
   ret.resize(nameLength);
   Character *strData = ret.getRawData();
   for(int i = 0; i < nameLength * 2; i += 2) {
      Character c(m_names[nameOffset + i + 1], m_names[nameOffset + i]);
      *strData = c;
      ++strData;
   }
   return ret;
}

int ResourceRoot::findNode(const String &path, const Locale &locale) const
{
   String ppath = path;
   {
      String root = getMappingRoot();
      if(!root.isEmpty()) {
         if(root == ppath) {
            ppath = Latin1Character('/');
         } else {
            if(!root.endsWith(Latin1Character('/'))) {
               root += Latin1Character('/');
            }
            if(ppath.size() >= root.size() && path.startsWith(root)) {
               ppath = path.substring(root.length() - 1);
            }
            if(path.isEmpty()) {
               ppath = Latin1Character('/');
            }
         }
      }
   }
#ifdef DEBUG_RESOURCE_MATCH
   qDebug() << "!!!!" << "START" << path << locale.getCountry() << locale.getLanguage();
#endif
   
   if(path == Latin1String("/"))
      return 0;
   
   //the root node is always first
   pdk::pint32 childCount = pdk::pdk_from_big_endian<pdk::pint32>(m_tree + 6);
   pdk::pint32 child       = pdk::pdk_from_big_endian<pdk::pint32>(m_tree + 10);
   
   //now iterate up the tree
   int node = -1;
   
   StringSplitter splitter(path);
   while (childCount && splitter.hasNext()) {
      StringView segment = splitter.next();
      
#ifdef DEBUG_RESOURCE_MATCH
      qDebug() << "  CHILDREN" << segment;
      for(int j = 0; j < childCount; ++j) {
         qDebug() << "   " << child+j << " :: " << getName(child+j);
      }
#endif
      const uint h = pdk_internal_hash(segment);
      
      //do the binary search for the hash
      int l = 0, r = childCount - 1;
      int subNode = (l + r + 1) / 2;
      while(r != l) {
         const uint subNodeHash = hash(child + subNode);
         if(h == subNodeHash) {
            break;
         } else if(h < subNodeHash) {
            r = subNode - 1;
         } else {
            l = subNode;
         } 
         subNode = (l + r + 1) / 2;
      }
      subNode += child;
      
      //now do the "harder" compares
      bool found = false;
      if(hash(subNode) == h) {
         while(subNode > child && hash(subNode-1) == h) //backup for collisions
            --subNode;
         for(; subNode < child+ childCount && hash(subNode) == h; ++subNode) { //here we go...
            if(getName(subNode) == segment) {
               found = true;
               int offset = findOffset(subNode);
#ifdef DEBUG_RESOURCE_MATCH
               qDebug() << "  TRY" << subNode << name(subNode) << offset;
#endif
               offset += 4;  //jump past name
               
               const pdk::pint16 flags = pdk::pdk_from_big_endian<pdk::pint16>(m_tree + offset);
               offset += 2;
               
               if(!splitter.hasNext()) {
                  if(!(flags & pdk::as_integer<Flags>(Flags::Directory))) {
                     const pdk::pint16 country = pdk::pdk_from_big_endian<pdk::pint16>(m_tree + offset);
                     offset += 2;
                     const pdk::pint16 language = pdk::pdk_from_big_endian<pdk::pint16>(m_tree + offset);
                     offset += 2;
#ifdef DEBUG_RESOURCE_MATCH
                     qDebug() << "    " << "LOCALE" << country << language;
#endif
                     if(country == static_cast<pdk::pint16>(locale.getCountry()) && 
                           language == static_cast<pdk::pint16>(locale.getLanguage())) {
#ifdef DEBUG_RESOURCE_MATCH
                        qDebug() << "!!!!" << "FINISHED" << __LINE__ << subNode;
#endif
                        return subNode;
                     } else if((country == static_cast<pdk::pint16>(Locale::Country::AnyCountry) && 
                                language == static_cast<pdk::pint16>(locale.getLanguage())) ||
                               (country == static_cast<pdk::pint16>(Locale::Country::AnyCountry) && 
                                language == static_cast<pdk::pint16>(Locale::Language::C) && node == -1)) {
                        node = subNode;
                     }
                     continue;
                  } else {
#ifdef DEBUG_RESOURCE_MATCH
                     qDebug() << "!!!!" << "FINISHED" << __LINE__ << subNode;
#endif
                     
                     return subNode;
                  }
               }
               
               if(!(flags & pdk::as_integer<Flags>(Flags::Directory))) {
                  return -1;
               }
               
               
               childCount = pdk::pdk_from_big_endian<pdk::pint32>(m_tree + offset);
               offset += 4;
               child = pdk::pdk_from_big_endian<pdk::pint32>(m_tree + offset);
               break;
            }
         }
      }
      if(!found) {
         break;
      }
      
   }
#ifdef DEBUG_RESOURCE_MATCH
   qDebug() << "!!!!" << "FINISHED" << __LINE__ << node;
#endif
   return node;
}

short ResourceRoot::getFlags(int node) const
{
   if(node == -1)
      return 0;
   const int offset = findOffset(node) + 4; //jump past name
   return pdk::pdk_from_big_endian<pdk::pint16>(m_tree + offset);
}

const uchar *ResourceRoot::getData(int node, pdk::pint64 *size) const
{
   if(node == -1) {
      *size = 0;
      return 0;
   }
   int offset = findOffset(node) + 4; //jump past name
   
   const pdk::pint16 flags = pdk::pdk_from_big_endian<pdk::pint16>(m_tree + offset);
   offset += 2;
   
   offset += 4; //jump past locale
   
   if(!(flags & pdk::as_integer<Flags>(Flags::Directory))) {
      const pdk::pint32 dataOffset = pdk::pdk_from_big_endian<pdk::pint32>(m_tree + offset);
      const pdk::puint32 dataLength = pdk::pdk_from_big_endian<pdk::puint32>(m_payloads + dataOffset);
      const uchar *ret = m_payloads + dataLength + 4;
      *size = dataLength;
      return ret;
   }
   *size = 0;
   return 0;
}

DateTime ResourceRoot::lastModified(int node) const
{
   if (node == -1 || m_version < 0x02) {
      return DateTime();      
   }
   
   const int offset = findOffset(node) + 14;
   
   const pdk::puint64 timeStamp = pdk::pdk_from_big_endian<pdk::puint64>(m_tree + offset);
   if (timeStamp == 0) {
      return DateTime();
   }
   return DateTime::fromMSecsSinceEpoch(timeStamp);
}

StringList ResourceRoot::getChildren(int node) const
{
   if(node == -1)
      return StringList();
   int offset = findOffset(node) + 4; //jump past name
   
   const pdk::pint16 flags = pdk::pdk_from_big_endian<pdk::pint16>(m_tree + offset);
   offset += 2;
   
   StringList ret;
   if(flags & pdk::as_integer<Flags>(Flags::Directory)) {
      const pdk::pint32 childCount = pdk::pdk_from_big_endian<pdk::pint32>(m_tree + offset);
      offset += 4;
      const pdk::pint32 childOff = pdk::pdk_from_big_endian<pdk::pint32>(m_tree + offset);
      ret.resize(childCount);
      for(int i = childOff; i < childOff + childCount; ++i) {
         ret.push_back(getName(i));
      }
   }
   return ret;
}
bool ResourceRoot::mappingRootSubdir(const String &path, String *match) const
{
   const String root = getMappingRoot();
   if (root.isEmpty())
      return false;
   
   StringSplitter rootIt(root);
   StringSplitter pathIt(path);
   while (rootIt.hasNext()) {
      if (pathIt.hasNext()) {
         if (rootIt.next() != pathIt.next()) // mismatch
            return false;
      } else {
         // end of path, but not of root:
         if (match)
            *match = rootIt.next().toString();
         return true;
      }
   }
   // end of root
   return !pathIt.hasNext();
}

} // internal

PDK_CORE_EXPORT bool register_resource_data(int version, const unsigned char *tree,
                                            const unsigned char *name, const unsigned char *data)
{
   std::lock_guard<std::recursive_mutex> lock(resource_mutex());
   if ((version == 0x01 || version == 0x2) && resource_list()) {
      bool found = false;
      ResourceRoot res(version, tree, name, data);
      for(size_t i = 0; i < resource_list()->size(); ++i) {
         auto iter = resource_list()->begin();
         std::advance(iter, i);
         if(**iter == res) {
            found = true;
            break;
         }
      }
      if(!found) {
         ResourceRoot *root = new ResourceRoot(version, tree, name, data);
         root->m_ref.ref();
         resource_list()->push_back(root);
      }
      return true;
   }
   return false;
}

PDK_CORE_EXPORT bool unregister_resource_data(int version, const unsigned char *tree,
                                              const unsigned char *name, const unsigned char *data)
{
   if (sg_resourceGlobalData.isDestroyed()) {
      return false;
   }
   std::lock_guard<std::recursive_mutex> lock(resource_mutex());
   if ((version == 0x01 || version == 0x02) && resource_list()) {
      ResourceRoot res(version, tree, name, data);
      for(size_t i = 0; i < resource_list()->size(); ) {
         auto iter = resource_list()->begin();
         std::advance(iter, i);
         if(**iter == res) {
            ResourceRoot *root = *iter;
            resource_list()->erase(iter);
            if(!root->m_ref.deref())
               delete root;
         } else {
            ++i;
         }
      }
      return true;
   }
   return false;
}

namespace internal {

class DynamicBufferResourceRoot: public ResourceRoot
{
   String m_root;
   const uchar *m_buffer;
   
public:
   inline DynamicBufferResourceRoot(const String &root)
      : m_root(root),
        m_buffer(nullptr)
   {}
   
   inline ~DynamicBufferResourceRoot()
   {}
   
   inline const uchar *getMappingBuffer() const
   {
      return m_buffer;
   }
   virtual String getMappingRoot() const override
   {
      return m_root;
   }
   
   virtual ResourceRootType getType() const override
   {
      return ResourceRootType::Buffer;
   }
   
   // size == -1 means "unknown"
   bool registerSelf(const uchar *buffer, int size)
   {
      // 5 int "pointers"
      if (size >= 0 && size < 20) {
         return false;
      }
      //setup the data now
      int offset = 0;
      //magic number
      if(buffer[offset+0] != 'q' || buffer[offset+1] != 'r' ||
            buffer[offset+2] != 'e' || buffer[offset+3] != 's') {
         return false;
      }
      offset += 4;
      const int version = pdk::pdk_from_big_endian<pdk::pint32>(buffer + offset);
      offset += 4;
      const int treeOffset = pdk::pdk_from_big_endian<pdk::pint32>(buffer + offset);
      offset += 4;
      const int dataOffset = pdk::pdk_from_big_endian<pdk::pint32>(buffer + offset);
      offset += 4;
      const int nameOffset = pdk::pdk_from_big_endian<pdk::pint32>(buffer + offset);
      offset += 4;
      // Some sanity checking for sizes. This is _not_ a security measure.
      if (size >= 0 && (treeOffset >= size || dataOffset >= size || nameOffset >= size))
         return false;
      if (version == 0x01 || version == 0x02) {
         m_buffer = buffer;
         setSource(version, buffer + treeOffset, buffer + nameOffset, buffer + dataOffset);
         return true;
      }
      return false;
   }
};

} // internal

#if defined(PDK_OS_UNIX) && !defined(PDK_OS_INTEGRITY)
#define PDK_USE_MMAP
#endif

// most of the headers below are already included in qplatformdefs.h
// also this lacks Large File support but that's probably irrelevant
#if defined(PDK_USE_MMAP)
// for mmap
} // fs
} // io
} // pdk

#include <sys/mman.h>
#include <errno.h>

namespace pdk {
namespace io {
namespace fs {
#endif

namespace internal {

class DynamicFileResourceRoot: public DynamicBufferResourceRoot
{
   String m_fileName;
   // for mmap'ed files, this is what needs to be unmapped.
   uchar *m_unmapPointer;
   unsigned int m_unmapLength;
   
public:
   inline DynamicFileResourceRoot(const String &root) 
      : DynamicBufferResourceRoot(root),
        m_unmapPointer(nullptr),
        m_unmapLength(0)
   {}
   
   ~DynamicFileResourceRoot() {
#if defined(PDK_USE_MMAP)
      if (m_unmapPointer) {
         munmap((char*)m_unmapPointer, m_unmapLength);
         m_unmapPointer = nullptr;
         m_unmapLength = 0;
      } else
#endif
      {
         delete [] getMappingBuffer();
      }
   }
   String getMappingFile() const
   {
      return m_fileName;
   }
   
   virtual ResourceRootType getType() const override
   {
      return ResourceRootType::File;
   }
   
   bool registerSelf(const String &f) 
   {
      bool fromMM = false;
      uchar *data = nullptr;
      unsigned int dataLen = 0;
      
#ifdef PDK_USE_MMAP
      
#ifndef MAP_FILE
#define MAP_FILE 0
#endif
#ifndef MAP_FAILED
#define MAP_FAILED -1
#endif
      
      int fd = PDK_OPEN(File::encodeName(f), O_RDONLY,
                  #if defined(PDK_OS_WIN)
                        _S_IREAD | _S_IWRITE
                  #else
                        0666
                  #endif
                        );
      if (fd >= 0) {
         PDK_STATBUF st;
         if (!PDK_FSTAT(fd, &st)) {
            uchar *ptr;
            ptr = reinterpret_cast<uchar *>(
                     mmap(0, st.st_size,             // any address, whole file
                          PROT_READ,                 // read-only memory
                          MAP_FILE | MAP_PRIVATE,    // swap-backed map from file
                          fd, 0));                   // from offset 0 of fd
            if (ptr && ptr != reinterpret_cast<uchar *>(MAP_FAILED)) {
               data = ptr;
               dataLen = st.st_size;
               fromMM = true;
            }
         }
         ::close(fd);
      }
#endif // PDK_USE_MMAP
      if(!data) {
         File file(f);
         if (!file.exists()) {
            return false;
         }
         dataLen = file.getSize();
         data = new uchar[dataLen];
         bool ok = false;
         if (file.open(IoDevice::OpenMode::ReadOnly))
            ok = (dataLen == (uint)file.read((char*)data, dataLen));
         if (!ok) {
            delete [] data;
            data = 0;
            dataLen = 0;
            return false;
         }
         fromMM = false;
      }
      if (data && DynamicBufferResourceRoot::registerSelf(data, dataLen)) {
         if(fromMM) {
            m_unmapPointer = data;
            m_unmapLength = dataLen;
         }
         m_fileName = f;
         return true;
      }
      return false;
   }
};

} // internal

namespace {

String resource_fix_resource_root(String resource)
{
   if(!resource.isEmpty()) {
      if(resource.startsWith(Latin1Character(':'))) {
         resource = resource.substring(1);
      }
      if(!resource.isEmpty()) {
         resource = Dir::cleanPath(resource);
      } 
   }
   return resource;
}

} // anonymous namespace

using internal::ResourcePrivate;
using internal::DynamicFileResourceRoot;
using internal::DynamicBufferResourceRoot;

Resource::Resource(const String &file, const Locale &locale)
   : m_implPtr(new ResourcePrivate(this))
{
   PDK_D(Resource);
   implPtr->m_fileName = file;
   implPtr->m_locale = locale;
}

Resource::~Resource()
{}

bool Resource::registerResource(const String &rccFilename, const String &resourceRoot)
{
   String r = resource_fix_resource_root(resourceRoot);
   if(!r.isEmpty() && r[0] != Latin1Character('/')) {
      warning_stream("Dir::registerResource: Registering a resource [%s] must be rooted in an absolute path (start with /) [%s]",
                     rccFilename.toLocal8Bit().getRawData(), resourceRoot.toLocal8Bit().getRawData());
      return false;
   }
   DynamicFileResourceRoot *root = new DynamicFileResourceRoot(r);
   if(root->registerSelf(rccFilename)) {
      root->m_ref.ref();
      std::lock_guard<std::recursive_mutex> lock(resource_mutex());
      resource_list()->push_back(root);
      return true;
   }
   delete root;
   return false;
}

bool Resource::unregisterResource(const String &rccFilename, const String &resourceRoot)
{
   String r = resource_fix_resource_root(resourceRoot);
   std::lock_guard<std::recursive_mutex> lock(resource_mutex());
   ResourceList *list = resource_list();
   for(size_t i = 0; i < list->size(); ++i) {
      auto iter = list->begin();
      std::advance(iter, i);
      ResourceRoot *res = *iter;
      if(res->getType() == ResourceRoot::ResourceRootType::File) {
         DynamicFileResourceRoot *root = reinterpret_cast<DynamicFileResourceRoot*>(res);
         if (root->getMappingFile() == rccFilename && root->getMappingRoot() == r) {
            resource_list()->erase(iter);
            if(!root->m_ref.deref()) {
               delete root;
               return true;
            }
            return false;
         }
      }
   }
   return false;
}

bool Resource::registerResource(const uchar *rccData, const String &resourceRoot)
{
   String r = resource_fix_resource_root(resourceRoot);
   if(!r.isEmpty() && r[0] != Latin1Character('/')) {
      warning_stream("Dir::registerResource: Registering a resource [%p] must be rooted in an absolute path (start with /) [%s]",
                     (void *)rccData, resourceRoot.toLocal8Bit().getRawData());
      return false;
   }
   
   DynamicBufferResourceRoot *root = new DynamicBufferResourceRoot(r);
   if (root->registerSelf(rccData, -1)) {
      root->m_ref.ref();
      std::lock_guard<std::recursive_mutex> lock(resource_mutex());
      resource_list()->push_back(root);
      return true;
   }
   delete root;
   return false;
}

bool Resource::unregisterResource(const uchar *rccData, const String &resourceRoot)
{
   String r = resource_fix_resource_root(resourceRoot);
   std::lock_guard<std::recursive_mutex> lock(resource_mutex());
   ResourceList *list = resource_list();
   for(size_t i = 0; i < list->size(); ++i) {
      auto iter = list->begin();
      std::advance(iter, i);
      ResourceRoot *res = *iter;
      if(res->getType() == ResourceRoot::ResourceRootType::Buffer) {
         DynamicBufferResourceRoot *root = reinterpret_cast<DynamicBufferResourceRoot*>(res);
         if (root->getMappingBuffer() == rccData && root->getMappingRoot() == r) {
            resource_list()->erase(iter);
            if(!root->m_ref.deref()) {
               delete root;
               return true;
            }
            return false;
         }
      }
   }
   return false;
}

void Resource::setLocale(const Locale &locale)
{
   PDK_D(Resource);
   implPtr->clear();
   implPtr->m_locale = locale;
}

Locale Resource::getLocale() const
{
   PDK_D(const Resource);
   return implPtr->m_locale;
}

void Resource::setFileName(const String &file)
{
   PDK_D(Resource);
   implPtr->clear();
   implPtr->m_fileName = file;
}

String Resource::getFileName() const
{
   PDK_D(const Resource);
   implPtr->ensureInitialized();
   return implPtr->m_fileName;
}

String Resource::getAbsoluteFilePath() const
{
   PDK_D(const Resource);
   implPtr->ensureInitialized();
   return implPtr->m_absoluteFilePath;
}

bool Resource::isValid() const
{
   PDK_D(const Resource);
   implPtr->ensureInitialized();
   return !implPtr->m_related.empty();
}

bool Resource::isCompressed() const
{
   PDK_D(const Resource);
   implPtr->ensureInitialized();
   return implPtr->m_compressed;
}

pdk::pint64 Resource::getSize() const
{
   PDK_D(const Resource);
   implPtr->ensureInitialized();
   return implPtr->m_size;
}

const uchar *Resource::getData() const
{
   PDK_D(const Resource);
   implPtr->ensureInitialized();
   return implPtr->m_data;
}

DateTime Resource::lastModified() const
{
   PDK_D(const Resource);
   implPtr->ensureInitialized();
   return implPtr->m_lastModified;
}

bool Resource::isDir() const
{
   PDK_D(const Resource);
   implPtr->ensureInitialized();
   return implPtr->m_container;
}

StringList Resource::getChildren() const
{
   PDK_D(const Resource);
   implPtr->ensureChildren();
   return implPtr->m_children;
}

namespace internal {

using pdk::kernel::internal::SystemError;

class ResourceFileEnginePrivate : public AbstractFileEnginePrivate
{
protected:
   PDK_DECLARE_PUBLIC(ResourceFileEngine);
private:
   uchar *map(pdk::pint64 offset, pdk::pint64 size, File::MemoryMapFlags flags);
   bool unmap(uchar *ptr);
   void uncompress() const;
   pdk::pint64 m_offset;
   Resource m_resource;
   mutable ByteArray m_uncompressed;
protected:
   ResourceFileEnginePrivate()
      : m_offset(0)
   {}
};

bool ResourceFileEngine::mkdir(const String &, bool) const
{
   return false;
}

bool ResourceFileEngine::rmdir(const String &, bool) const
{
   return false;
}

bool ResourceFileEngine::setSize(pdk::pint64)
{
   return false;
}

StringList ResourceFileEngine::getEntryList(Dir::Filters filters, const StringList &filterNames) const
{
   return AbstractFileEngine::getEntryList(filters, filterNames);
}

bool ResourceFileEngine::caseSensitive() const
{
   return true;
}

ResourceFileEngine::ResourceFileEngine(const String &file) :
   AbstractFileEngine(*new ResourceFileEnginePrivate)
{
   PDK_D(ResourceFileEngine);
   implPtr->m_resource.setFileName(file);
}

ResourceFileEngine::~ResourceFileEngine()
{
}

void ResourceFileEngine::setFileName(const String &file)
{
   PDK_D(ResourceFileEngine);
   implPtr->m_resource.setFileName(file);
}

bool ResourceFileEngine::open(IoDevice::OpenModes flags)
{
   PDK_D(ResourceFileEngine);
   if (implPtr->m_resource.getFileName().isEmpty()) {
      warning_stream("ResourceFileEngine::open: Missing file name");
      return false;
   }
   if(flags & IoDevice::OpenMode::WriteOnly) {
      return false;
   }
   implPtr->uncompress();
   if (!implPtr->m_resource.isValid()) {
      implPtr->m_errorString = SystemError::getStdString(ENOENT);
      return false;
   }
   return true;
}

bool ResourceFileEngine::close()
{
   PDK_D(ResourceFileEngine);
   implPtr->m_offset = 0;
   implPtr->m_uncompressed.clear();
   return true;
}

bool ResourceFileEngine::flush()
{
   return true;
}

pdk::pint64 ResourceFileEngine::read(char *data, pdk::pint64 len)
{
   PDK_D(ResourceFileEngine);
   if(len > getSize() - implPtr->m_offset) {
      len = getSize() - implPtr->m_offset;
   }
   if(len <= 0) {
      return 0;
   }
   if(implPtr->m_resource.isCompressed()) {
      memcpy(data, implPtr->m_uncompressed.getConstRawData() + implPtr->m_offset, len);
   } else {
      memcpy(data, implPtr->m_resource.getData() + implPtr->m_offset, len);
   }
   implPtr->m_offset += len;
   return len;
}

pdk::pint64 ResourceFileEngine::write(const char *, pdk::pint64)
{
   return -1;
}

bool ResourceFileEngine::remove()
{
   return false;
}

bool ResourceFileEngine::copy(const String &)
{
   return false;
}

bool ResourceFileEngine::rename(const String &)
{
   return false;
}

bool ResourceFileEngine::link(const String &)
{
   return false;
}

pdk::pint64 ResourceFileEngine::getSize() const
{
   PDK_D(const ResourceFileEngine);
   if(!implPtr->m_resource.isValid()) {
      return 0;
   }
   
   if (implPtr->m_resource.isCompressed()) {
      implPtr->uncompress();
      return implPtr->m_uncompressed.size();
   }
   return implPtr->m_resource.getSize();
}

pdk::pint64 ResourceFileEngine::getPosition() const
{
   PDK_D(const ResourceFileEngine);
   return implPtr->m_offset;
}

bool ResourceFileEngine::atEnd() const
{
   PDK_D(const ResourceFileEngine);
   if(!implPtr->m_resource.isValid()) {
      return true;
   }
   return implPtr->m_offset == getSize();
}

bool ResourceFileEngine::seek(pdk::pint64 pos)
{
   PDK_D(ResourceFileEngine);
   if(!implPtr->m_resource.isValid()) {
      return false;
   }
   if(implPtr->m_offset > getSize()) {
      return false;
   }
   implPtr->m_offset = pos;
   return true;
}

bool ResourceFileEngine::isSequential() const
{
   return false;
}

AbstractFileEngine::FileFlags ResourceFileEngine::getFileFlags(AbstractFileEngine::FileFlags type) const
{
   PDK_D(const ResourceFileEngine);
   AbstractFileEngine::FileFlags ret = 0;
   if(!implPtr->m_resource.isValid()) {
      return ret;
   }
   if(type & AbstractFileEngine::FileFlag::PermsMask) {
      ret |= AbstractFileEngine::FileFlag::ReadOwnerPerm;
      ret |= AbstractFileEngine::FileFlag::ReadUserPerm;
      ret |= AbstractFileEngine::FileFlag::ReadGroupPerm;
      ret |= AbstractFileEngine::FileFlag::ReadOtherPerm;
   }
   if(type & AbstractFileEngine::FileFlag::TypesMask) {
      if(implPtr->m_resource.isDir()) {
         ret |= AbstractFileEngine::FileFlag::DirectoryType;
      } else {
         ret |= AbstractFileEngine::FileFlag::FileType;
      }
   }
   if(type & AbstractFileEngine::FileFlag::FlagsMask) {
      ret |= AbstractFileEngine::FileFlag::ExistsFlag;
      if(implPtr->m_resource.getAbsoluteFilePath() == Latin1String(":/")) {
         ret |= AbstractFileEngine::FileFlag::RootFlag;
      }
   }
   return ret;
}

bool ResourceFileEngine::setPermissions(uint)
{
   return false;
}

String ResourceFileEngine::getFileName(FileName file) const
{
   PDK_D(const ResourceFileEngine);
   if(file == FileName::BaseName) {
      int slash = implPtr->m_resource.getFileName().lastIndexOf(Latin1Character('/'));
      if (slash == -1) {
         return implPtr->m_resource.getFileName();
      }
      return implPtr->m_resource.getFileName().substring(slash + 1);
   } else if(file == FileName::PathName || file == FileName::AbsolutePathName) {
      const String path = (file == FileName::AbsolutePathName) ? implPtr->m_resource.getAbsoluteFilePath() : implPtr->m_resource.getFileName();
      const int slash = path.lastIndexOf(Latin1Character('/'));
      if (slash == -1) {
         return Latin1String(":");
      } else if (slash <= 1) {
         return Latin1String(":/");
      } 
      return path.left(slash);
      
   } else if(file == FileName::CanonicalName || file == FileName::CanonicalPathName) {
      const String absoluteFilePath = implPtr->m_resource.getAbsoluteFilePath();
      if(file == FileName::CanonicalPathName) {
         const int slash = absoluteFilePath.lastIndexOf(Latin1Character('/'));
         if (slash != -1) {
            return absoluteFilePath.left(slash);
         }
      }
      return absoluteFilePath;
   }
   return implPtr->m_resource.getFileName();
}

bool ResourceFileEngine::isRelativePath() const
{
   return false;
}

uint ResourceFileEngine::getOwnerId(FileOwner) const
{
   static const uint nobodyID = (uint) -2;
   return nobodyID;
}

String ResourceFileEngine::getOwner(FileOwner) const
{
   return String();
}

DateTime ResourceFileEngine::getFileTime(FileTime time) const
{
   PDK_D(const ResourceFileEngine);
   if (time == FileTime::ModificationTime) {
      return implPtr->m_resource.lastModified();
   }
   return DateTime();
}

AbstractFileEngine::Iterator *ResourceFileEngine::beginEntryList(Dir::Filters filters,
                                                                 const StringList &filterNames)
{
   return new ResourceFileEngineIterator(filters, filterNames);
}

AbstractFileEngine::Iterator *ResourceFileEngine::endEntryList()
{
   return 0;
}

bool ResourceFileEngine::extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output)
{
   PDK_D(ResourceFileEngine);
   if (extension == MapExtension) {
      const MapExtensionOption *options = (const MapExtensionOption*)(option);
      MapExtensionReturn *returnValue = static_cast<MapExtensionReturn*>(output);
      returnValue->address = implPtr->map(options->offset, options->size, options->flags);
      return (returnValue->address != 0);
   }
   if (extension == UnMapExtension) {
      const UnMapExtensionOption *options = (const UnMapExtensionOption*)option;
      return implPtr->unmap(options->address);
   }
   return false;
}

bool ResourceFileEngine::supportsExtension(Extension extension) const
{
   return (extension == UnMapExtension || extension == MapExtension);
}

uchar *ResourceFileEnginePrivate::map(pdk::pint64 offset, pdk::pint64 size, File::MemoryMapFlags flags)
{
   PDK_Q(ResourceFileEngine);
   PDK_UNUSED(flags);
   if (offset < 0 || size <= 0 || !m_resource.isValid() || offset + size > m_resource.getSize()) {
      apiPtr->setError(File::FileError::UnspecifiedError, String());
      return 0;
   }
   uchar *address = const_cast<uchar *>(m_resource.getData());
   return (address + offset);
}

bool ResourceFileEnginePrivate::unmap(uchar *ptr)
{
   PDK_UNUSED(ptr);
   return true;
}

void ResourceFileEnginePrivate::uncompress() const
{
   if (m_resource.isCompressed() && m_uncompressed.isEmpty() && m_resource.getSize()) {
#ifndef PDK_NO_COMPRESS
//      m_uncompressed = qUncompress(resource.data(), resource.size());
#else
      PDK_ASSERT(!"ResourceFileEngine::open: pdk built without support for compression");
#endif
   }
}

} // internal

} // fs
} // io
} // pdk

PDK_DECLARE_TYPEINFO(pdk::io::fs::internal::ResourceRoot, PDK_MOVABLE_TYPE);
