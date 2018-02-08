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
   
   const uchar *data(int node, pdk::pint64 *size) const;
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
   inline void setSource(int version, const uchar *tree, const uchar *name, const uchar *data) {
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
               m_data = res->data(node, &m_size);
               m_compressed = res->isCompressed(node);
            } else {
               m_data = nullptr;
               m_size = 0;
               m_compressed = 0;
            }
            m_lastModified = res->lastModified(node);
         } else if(res->isContainer(node) != m_container) {
            //qWarning("ResourceInfo: Resource [%s] has both data and children!", file.toLatin1().constData());
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

} // internal

} // fs
} // io
} // pdk

PDK_DECLARE_TYPEINFO(pdk::io::fs::internal::ResourceRoot, PDK_MOVABLE_TYPE);
