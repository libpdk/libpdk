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
# include "pdk/kernel/CoreUnix.h"
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
   StringList children(int node) const;
   virtual String mappingRoot() const
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
            StringList relatedChildren = res->children(node);
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
      String root = mappingRoot();
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

StringList ResourceRoot::children(int node) const
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
   const String root = mappingRoot();
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

using internal::ResourcePrivate;

Resource::Resource(const String &file, const Locale &locale)
   : m_implPtr(new ResourcePrivate(this))
{
   PDK_D(Resource);
   implPtr->m_fileName = file;
   implPtr->m_locale = locale;
}

Resource::~Resource()
{
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

} // fs
} // io
} // pdk

PDK_DECLARE_TYPEINFO(pdk::io::fs::internal::ResourceRoot, PDK_MOVABLE_TYPE);
