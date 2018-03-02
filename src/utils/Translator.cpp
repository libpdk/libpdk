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
// Created by softboy on 2018/03/02.

#ifndef PDK_NO_TRANSLATION

#include "pdk/global/PlatformDefs.h"
#include "pdk/utils/Translator.h"
#include "pdk/utils/internal/TranslatorPrivate.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/Resource.h"
#include "pdk/base/io/DataStream.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/kernel/Algorithms.h"
#include "pdk/kernel/Object.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/utils/Locale.h"
#include "pdk/global/Endian.h"
#include "pdk/stdext/utility/Algorithms.h"

#if defined(PDK_OS_UNIX) && !defined(PDK_OS_INTEGRITY)
#define PDK_USE_MMAP
#include "pdk/kernel/internal/CoreUnixPrivate.h"
#endif

// most of the headers below are already included in qplatformdefs.h
// also this lacks Large File support but that's probably irrelevant
#if defined(PDK_USE_MMAP)
// for mmap
#include <sys/mman.h>
#include <cerrno>
#endif

#include <cstdlib>

namespace pdk {
namespace utils {

using pdk::io::fs::Resource;
using pdk::io::fs::File;
using pdk::io::fs::FileInfo;
using pdk::io::IoDevice;
using pdk::io::DataStream;
using pdk::lang::Latin1Character;
using pdk::lang::String;
using pdk::kernel::CoreApplication;
using pdk::kernel::internal::CoreApplicationPrivate;
using pdk::kernel::Event;

enum Tag
{
   Tag_End = 1,
   Tag_SourceText16,
   Tag_Translation,
   Tag_Context16,
   Tag_Obsolete1,
   Tag_SourceText,
   Tag_Context,
   Tag_Comment,
   Tag_Obsolete2
};

static const int MagicLength = 16;
static const uchar sg_magic[MagicLength] = {
   0x3c, 0xb8, 0x64, 0x18, 0xca, 0xef, 0x9c, 0x95,
   0xcd, 0x21, 0x1c, 0xbf, 0x60, 0xa1, 0xbd, 0xdd
};

namespace {

inline String dot_qm_literal()
{
   return StringLiteral(".qm");
}

bool match(const uchar *found, uint foundLen, const char *target, uint targetLen)
{
   // catch the case if \a found has a zero-terminating symbol and \a len includes it.
   // (normalize it to be without the zero-terminating symbol)
   if (foundLen > 0 && found[foundLen-1] == '\0') {
      --foundLen;
   }
   
   return ((targetLen == foundLen) && memcmp(found, target, foundLen) == 0);
}

void elf_hash_continue(const char *name, uint &h)
{
   const uchar *k;
   uint g;
   k = (const uchar *) name;
   while (*k) {
      h = (h << 4) + *k++;
      if ((g = (h & 0xf0000000)) != 0) {
         h ^= g >> 24;
      }
      h &= ~g;
   }
}

void elf_hash_finish(uint &h)
{
   if (!h) {
      h = 1;
   }
}

uint elf_hash(const char *name)
{
   uint hash = 0;
   elf_hash_continue(name, hash);
   elf_hash_finish(hash);
   return hash;
}

/*
   Determines whether \a rules are valid "numerus rules". Test input with this
   function before calling numerusHelper, below.
 */
bool is_valid_numerus_rules(const uchar *rules, uint rulesSize)
{
   // Disabled computation of maximum numerus return value
   // pdk::puint32 numerus = 0;
   
   if (rulesSize == 0) {
      return true;
   }
   pdk::puint32 offset = 0;
   do {
      uchar opcode = rules[offset];
      uchar op = opcode & P_OP_MASK;
      if (opcode & 0x80) {
         return false; // Bad op
      }
      if (++offset == rulesSize) {
         return false; // Missing operand
      }
      // right operand
      ++offset;      
      switch (op)
      {
      case P_EQ:
      case P_LT:
      case P_LEQ:
         break;
         
      case P_BETWEEN:
         if (offset != rulesSize) {
            // third operand
            ++offset;
            break;
         }
         return false; // Missing operand
         
      default:
         return false; // Bad op (0)
      }
      
      // ++numerus;
      if (offset == rulesSize)
         return true;
      
   } while (((rules[offset] == P_AND)
             || (rules[offset] == P_OR)
             || (rules[offset] == P_NEWRULE))
            && ++offset != rulesSize);
   
   // Bad op
   return false;
}

/*
  
   This function does no validation of input and assumes it is well-behaved,
   these assumptions can be checked with isValidNumerusRules, above.
   
   Determines which translation to use based on the value of \a n. The return
   value is an index identifying the translation to be used.
   
   \a rules is a character array of size \a rulesSize containing bytecode that
   operates on the value of \a n and ultimately determines the result.
   
   This function has O(1) space and O(rulesSize) time complexity.
 */
uint numerus_helper(int n, const uchar *rules, uint rulesSize)
{
   uint result = 0;
   uint i = 0;
   
   if (rulesSize == 0) {
      return 0;
   }
   for (;;) {
      bool orExprTruthValue = false;
      for (;;) {
         bool andExprTruthValue = true;
         for (;;) {
            bool truthValue = true;
            int opcode = rules[i++];
            int leftOperand = n;
            if (opcode & P_MOD_10) {
               leftOperand %= 10;
            } else if (opcode & P_MOD_100) {
               leftOperand %= 100;
            } else if (opcode & P_LEAD_1000) {
               while (leftOperand >= 1000)
                  leftOperand /= 1000;
            }
            int op = opcode & P_OP_MASK;
            int rightOperand = rules[i++];
            switch (op) {
            case P_EQ:
               truthValue = (leftOperand == rightOperand);
               break;
            case P_LT:
               truthValue = (leftOperand < rightOperand);
               break;
            case P_LEQ:
               truthValue = (leftOperand <= rightOperand);
               break;
            case P_BETWEEN:
               int bottom = rightOperand;
               int top = rules[i++];
               truthValue = (leftOperand >= bottom && leftOperand <= top);
            }
            
            if (opcode & P_NOT) {
               truthValue = !truthValue;
            }
            andExprTruthValue = andExprTruthValue && truthValue;
            if (i == rulesSize || rules[i] != P_AND) {
               break;
            }
            ++i;
         }
         orExprTruthValue = orExprTruthValue || andExprTruthValue;
         if (i == rulesSize || rules[i] != P_OR) {
            break;
         }
         ++i;
      }      
      if (orExprTruthValue) {
         return result;
      }
      ++result;
      if (i == rulesSize) {
         return result;
      }
      i++; // P_NEWRULE
   }
   
   PDK_ASSERT(false);
   return 0;
}
} // anonymous namespace

namespace internal {

using pdk::kernel::internal::ObjectPrivate;

class TranslatorPrivate : public ObjectPrivate
{
   PDK_DECLARE_PUBLIC(Translator);
public:
   enum { Contexts = 0x2f, Hashes = 0x42, Messages = 0x69, NumerusRules = 0x88, Dependencies = 0x96 };
   
   TranslatorPrivate() :
   #if defined(PDK_USE_MMAP)
      m_usedmmap(false),
   #endif
      m_unmapPointer(nullptr),
      m_unmapLength(0),
      m_resource(nullptr),
      m_messageArray(nullptr),
      m_offsetArray(nullptr), 
      m_contextArray(nullptr),
      m_numerusRulesArray(nullptr),
      m_messageLength(0),
      m_offsetLength(0),
      m_contextLength(0),
      m_numerusRulesLength(0)
   {}
   
#if defined(PDK_USE_MMAP)
   bool m_usedmmap : 1;
#endif
   char *m_unmapPointer;     // used memory (mmap, new or resource file)
   pdk::puint32 m_unmapLength;
   
   // The resource object in case we loaded the translations from a resource
   Resource *m_resource;
   
   // used if the translator has dependencies
   std::list<Translator*> m_subTranslators;
   
   // Pointers and offsets into unmapPointer[unmapLength] array, or user
   // provided data array
   const uchar *m_messageArray;
   const uchar *m_offsetArray;
   const uchar *m_contextArray;
   const uchar *m_numerusRulesArray;
   uint m_messageLength;
   uint m_offsetLength;
   uint m_contextLength;
   uint m_numerusRulesLength;
   
   bool doLoad(const String &filename, const String &directory);
   bool doLoad(const uchar *data, int len, const String &directory);
   String doTranslate(const char *context, const char *sourceText, const char *comment,
                      int n) const;
   void clear();
};

bool TranslatorPrivate::doLoad(const String &realname, const String &directory)
{
   bool ok = false;
   
   if (realname.startsWith(Latin1Character(':'))) {
      // If the translation is in a non-compressed resource file, the data is already in
      // memory, so no need to use QFile to copy it again.
      PDK_ASSERT(!m_resource);
      m_resource = new Resource(realname);
      if (m_resource->isValid() && !m_resource->isCompressed() && m_resource->getSize() > MagicLength
          && !memcmp(m_resource->getData(), sg_magic, MagicLength)) {
         m_unmapLength = m_resource->getSize();
         m_unmapPointer = reinterpret_cast<char *>(const_cast<uchar *>(m_resource->getData()));
#if defined(PDK_USE_MMAP)
         m_usedmmap = false;
#endif
         ok = true;
      } else {
         delete m_resource;
         m_resource = nullptr;
      }
   }
   
   if (!ok) {
      File file(realname);
      if (!file.open(IoDevice::OpenMode::ReadOnly | IoDevice::OpenMode::Unbuffered)) {
         return false;
      }
      pdk::pint64 fileSize = file.getSize();
      if (fileSize <= MagicLength || pdk::puint32(-1) <= fileSize) {
         return false;
      }
      {
         char magicBuffer[MagicLength];
         if (MagicLength != file.read(magicBuffer, MagicLength)
             || memcmp(magicBuffer, sg_magic, MagicLength)) {
            return false;
         }
      }
      m_unmapLength = pdk::puint32(fileSize);
      
#ifdef PDK_USE_MMAP
      
#ifndef MAP_FILE
#define MAP_FILE 0
#endif
#ifndef MAP_FAILED
#define MAP_FAILED -1
#endif
      
      int fd = file.getHandle();
      if (fd >= 0) {
         char *ptr;
         ptr = reinterpret_cast<char *>(
                  mmap(0, m_unmapLength,         // any address, whole file
                       PROT_READ,                 // read-only memory
                       MAP_FILE | MAP_PRIVATE,    // swap-backed map from file
                       fd, 0));                   // from offset 0 of fd
         if (ptr && ptr != reinterpret_cast<char *>(MAP_FAILED)) {
            file.close();
            m_usedmmap = true;
            m_unmapPointer = ptr;
            ok = true;
         }
      }
#endif // PDK_USE_MMAP
      
      if (!ok) {
         m_unmapPointer = new char[m_unmapLength];
         if (m_unmapPointer) {
            file.seek(0);
            pdk::pint64 readResult = file.read(m_unmapPointer, m_unmapLength);
            if (readResult == pdk::pint64(m_unmapLength)) {
               ok = true;
            }
         }
      }
   }
   
   if (ok && doLoad(reinterpret_cast<const uchar *>(m_unmapPointer), m_unmapLength, directory)) {
      return true;
   }
#if defined(PDK_USE_MMAP)
   if (m_usedmmap) {
      m_usedmmap = false;
      munmap(m_unmapPointer, m_unmapLength);
   } else
#endif
      if (!m_resource)
         delete [] m_unmapPointer;
   
   delete m_resource;
   m_resource = nullptr;
   m_unmapPointer = nullptr;
   m_unmapLength = 0;
   
   return false;
}

namespace {

pdk::puint8 read8(const uchar *data)
{
   return pdk::from_big_endian<pdk::puint8>(data);
}

pdk::puint16 read16(const uchar *data)
{
   return pdk::from_big_endian<pdk::puint16>(data);
}

pdk::puint32 read32(const uchar *data)
{
   return pdk::from_big_endian<pdk::puint32>(data);
}
} // anonymous namespace

bool TranslatorPrivate::doLoad(const uchar *data, int len, const String &directory)
{
   bool ok = true;
   const uchar *end = data + len;
   
   data += MagicLength;
   
   StringList dependencies;
   while (data < end - 4) {
      pdk::puint8 tag = read8(data++);
      pdk::puint32 blockLen = read32(data);
      data += 4;
      if (!tag || !blockLen)
         break;
      if (pdk::puint32(end - data) < blockLen) {
         ok = false;
         break;
      }
      if (tag == TranslatorPrivate::Contexts) {
         m_contextArray = data;
         m_contextLength = blockLen;
      } else if (tag == TranslatorPrivate::Hashes) {
         m_offsetArray = data;
         m_offsetLength = blockLen;
      } else if (tag == TranslatorPrivate::Messages) {
         m_messageArray = data;
         m_messageLength = blockLen;
      } else if (tag == TranslatorPrivate::NumerusRules) {
         m_numerusRulesArray = data;
         m_numerusRulesLength = blockLen;
      } else if (tag == TranslatorPrivate::Dependencies) {
         DataStream stream(ByteArray::fromRawData((const char*)data, blockLen));
         String dep;
         while (!stream.atEnd()) {
            stream >> dep;
            dependencies.push_back(dep);
         }
      }
      data += blockLen;
   }
   
   if (dependencies.empty() && (!m_offsetArray || !m_messageArray)) {
      ok = false;
   }
   if (ok && !is_valid_numerus_rules(m_numerusRulesArray, m_numerusRulesLength)) {
      ok = false;
   }
   if (ok) {
      const int dependenciesCount = dependencies.size();
      m_subTranslators.resize(dependenciesCount);
      for (int i = 0 ; i < dependenciesCount; ++i) {
         Translator *translator = new Translator;
         m_subTranslators.push_back(translator);
         ok = translator->load(dependencies.at(i), directory);
         if (!ok) {
            break;
         } 
      }
      // In case some dependencies fail to load, unload all the other ones too.
      if (!ok) {
         pdk::stdext::delete_all(m_subTranslators);
         m_subTranslators.clear();
      }
   }
   
   if (!ok) {
      m_messageArray = nullptr;
      m_contextArray = nullptr;
      m_offsetArray = nullptr;
      m_numerusRulesArray = nullptr;
      m_messageLength = 0;
      m_contextLength = 0;
      m_offsetLength = 0;
      m_numerusRulesLength = 0;
   }
   
   return ok;
}

namespace {

String get_message(const uchar *m, const uchar *end, const char *context,
                   const char *sourceText, const char *comment, uint numerus)
{
   const uchar *tn = 0;
   uint tnlength = 0;
   const uint sourceTextLen = uint(strlen(sourceText));
   const uint contextLen = uint(strlen(context));
   const uint commentLen = uint(strlen(comment));
   
   for (;;) {
      uchar tag = 0;
      if (m < end) {
         tag = read8(m++);
      }
      switch((Tag)tag) {
      case Tag_End:
         goto end;
      case Tag_Translation: {
         int len = read32(m);
         if (len % 1) {
            return String();
         }
         m += 4;
         if (!numerus--) {
            tnlength = len;
            tn = m;
         }
         m += len;
         break;
      }
      case Tag_Obsolete1:
         m += 4;
         break;
      case Tag_SourceText: {
         pdk::puint32 len = read32(m);
         m += 4;
         if (!match(m, len, sourceText, sourceTextLen)) {
            return String();
         }
         m += len;
      }
         break;
      case Tag_Context: {
         pdk::puint32 len = read32(m);
         m += 4;
         if (!match(m, len, context, contextLen)) {
            return String();
         }
         m += len;
      }
         break;
      case Tag_Comment: {
         pdk::puint32 len = read32(m);
         m += 4;
         if (*m && !match(m, len, comment, commentLen)) {
            return String();
         }
         m += len;
      }
         break;
      default:
         return String();
      }
   }
end:
   if (!tn) {
      return String();
   }
   String str = String((const Character *)tn, tnlength/2);
   if (SysInfo::ByteOrder == SysInfo::LittleEndian) {
      for (int i = 0; i < str.length(); ++i) {
         str[i] = Character((str.at(i).unicode() >> 8) + ((str.at(i).unicode() << 8) & 0xff00));
      }
   }
   return str;
}


} // anonymous namespace

String TranslatorPrivate::doTranslate(const char *context, const char *sourceText,
                                      const char *comment, int n) const
{
   if (context == 0) {
      context = "";
   }
   if (sourceText == 0) {
      sourceText = "";
   }
   if (comment == 0) {
      comment = "";
   }
   uint numerus = 0;
   size_t numItems = 0;
   if (!m_offsetLength) {
      goto searchDependencies;
   }
   /*
        Check if the context belongs to this Translator. If many
        translators are installed, this step is necessary.
    */
   if (m_contextLength) {
      pdk::puint16 hTableSize = read16(m_contextArray);
      uint g = elf_hash(context) % hTableSize;
      const uchar *c = m_contextArray + 2 + (g << 1);
      pdk::puint16 off = read16(c);
      c += 2;
      if (off == 0) {
         return String();
      }
      c = m_contextArray + (2 + (hTableSize << 1) + (off << 1));
      
      const uint contextLen = uint(strlen(context));
      for (;;) {
         pdk::puint8 len = read8(c++);
         if (len == 0) {
            return String();
         }
         if (match(c, len, context, contextLen)) {
            break;
         }
         c += len;
      }
   }
   
   numItems = m_offsetLength / (2 * sizeof(pdk::puint32));
   if (!numItems) {
      goto searchDependencies;
   }
   if (n >= 0) {
      numerus = numerus_helper(n, m_numerusRulesArray, m_numerusRulesLength);
   }
   
   for (;;) {
      pdk::puint32 h = 0;
      elf_hash_continue(sourceText, h);
      elf_hash_continue(comment, h);
      elf_hash_finish(h);
      
      const uchar *start = m_offsetArray;
      const uchar *end = start + ((numItems-1) << 3);
      while (start <= end) {
         const uchar *middle = start + (((end - start) >> 4) << 3);
         uint hash = read32(middle);
         if (h == hash) {
            start = middle;
            break;
         } else if (hash < h) {
            start = middle + 8;
         } else {
            end = middle - 8;
         }
      }
      
      if (start <= end) {
         // go back on equal key
         while (start != m_offsetArray && read32(start) == read32(start-8)) {
            start -= 8;
         }
         while (start < m_offsetArray + m_offsetLength) {
            pdk::puint32 rh = read32(start);
            start += 4;
            if (rh != h) {
               break;
            }
            pdk::puint32 ro = read32(start);
            start += 4;
            String tn = get_message(m_messageArray + ro, m_messageArray + m_messageLength, context,
                                    sourceText, comment, numerus);
            if (!tn.isNull()) {
               return tn;
            }
         }
      }
      if (!comment[0]) {
         break;
      }
      comment = "";
   }
   
searchDependencies:
   for (Translator *translator : m_subTranslators) {
      String tn = translator->translate(context, sourceText, comment, n);
      if (!tn.isNull()) {
         return tn;
      }
   }
   return String();
}

void TranslatorPrivate::clear()
{
   PDK_Q(Translator);
   if (m_unmapPointer && m_unmapLength) {
#if defined(PDK_USE_MMAP)
      if (m_usedmmap) {
         m_usedmmap = false;
         munmap(m_unmapPointer, m_unmapLength);
      } else
#endif
         if (!m_resource) {
            delete [] m_unmapPointer;
         }
   }
   
   delete m_resource;
   m_resource = nullptr;
   m_unmapPointer = nullptr;
   m_unmapLength = 0;
   m_messageArray = nullptr;
   m_contextArray = nullptr;
   m_offsetArray = nullptr;
   m_numerusRulesArray = nullptr;
   m_messageLength = 0;
   m_contextLength = 0;
   m_offsetLength = 0;
   m_numerusRulesLength = 0;
   
   pdk::stdext::delete_all(m_subTranslators);
   m_subTranslators.clear();
   
   if (CoreApplicationPrivate::isTranslatorInstalled(apiPtr)) {
      CoreApplication::postEvent(CoreApplication::getInstance(),
                                 new Event(Event::Type::LanguageChange));
   }
   
}


} // internal

Translator::Translator(Object * parent)
   : Object(*new TranslatorPrivate, parent)
{}

Translator::~Translator()
{
   if (CoreApplication::getInstance()) {
      CoreApplication::removeTranslator(this);
   }
   PDK_D(Translator);
   implPtr->clear();
}

bool Translator::load(const String & filename, const String & directory,
                      const String & searchDelimiters,
                      const String & suffix)
{
   PDK_D(Translator);
   implPtr->clear();
   String prefix;
   if (FileInfo(filename).isRelative()) {
      prefix = directory;
      if (prefix.length() && !prefix.endsWith(Latin1Character('/'))) {
         prefix += Latin1Character('/');
      }        
   }
   
   const String suffixOrDotQM = suffix.isNull() ? dot_qm_literal() : suffix;
   StringRef fname(&filename);
   String realname;
   const String delims = searchDelimiters.isNull() ? StringLiteral("_.") : searchDelimiters;
   for (;;) {
      FileInfo fi;
      realname = prefix + fname + suffixOrDotQM;
      fi.setFile(realname);
      if (fi.isReadable() && fi.isFile()) {
         break;
      }
      realname = prefix + fname;
      fi.setFile(realname);
      if (fi.isReadable() && fi.isFile()) {
         break;
      }
      int rightmost = 0;
      for (int i = 0; i < (int)delims.length(); i++) {
         int k = fname.lastIndexOf(delims[i]);
         if (k > rightmost) {
            rightmost = k;
         }  
      }
      // no truncations? fail
      if (rightmost == 0) {
         return false;
      }
      fname.truncate(rightmost);
   }
   // realname is now the fully qualified name of a readable file.
   return implPtr->doLoad(realname, directory);
}

namespace {

PDK_NEVER_INLINE
bool is_readable_file(const String &name)
{
   const FileInfo fi(name);
   return fi.isReadable() && fi.isFile();
}

String find_translation(const Locale & locale,
                        const String &filename,
                        const String &prefix,
                        const String &directory,
                        const String &suffix)
{
   String path;
   if (FileInfo(filename).isRelative()) {
      path = directory;
      if (!path.isEmpty() && !path.endsWith(Latin1Character('/'))) {
         path += Latin1Character('/');
      }
   }
   const String suffixOrDotQM = suffix.isNull() ? dot_qm_literal() : suffix;
   
   String realname;
   realname += path + filename + prefix; // using += in the hope for some reserve capacity
   const int realNameBaseSize = realname.size();
   StringList fuzzyLocales;
   
   // see http://www.unicode.org/reports/tr35/#LanguageMatching for inspiration
   
   StringList languages = locale.getUiLanguages();
#if defined(PDK_OS_UNIX)
   for (int i = languages.size()-1; i >= 0; --i) {
      String lang = languages.at(i);
      String lowerLang = lang.toLower();
      if (lang != lowerLang) {
         auto iter = languages.begin();
         std::advance(iter, i+1);
         languages.insert(iter, lowerLang);
      }
   }
#endif
   
   // try explicit locales names first
   for (String localeName : std::as_const(languages)) {
      localeName.replace(Latin1Character('-'), Latin1Character('_'));
      
      realname += localeName + suffixOrDotQM;
      if (is_readable_file(realname)) {
         return realname;
      }
      realname.truncate(realNameBaseSize + localeName.size());
      if (is_readable_file(realname)) {
         return realname;
      }
      realname.truncate(realNameBaseSize);
      fuzzyLocales.push_back(localeName);
   }
   
   // start guessing
   for (const String &fuzzyLocale : std::as_const(fuzzyLocales)) {
      StringRef localeName(&fuzzyLocale);
      for (;;) {
         int rightmost = localeName.lastIndexOf(Latin1Character('_'));
         // no truncations? fail
         if (rightmost <= 0) {
            break;
         }
         localeName.truncate(rightmost);
         realname += localeName + suffixOrDotQM;
         if (is_readable_file(realname)) {
            return realname;
         }
         realname.truncate(realNameBaseSize + localeName.size());
         if (is_readable_file(realname)) {
            return realname;
         }
         realname.truncate(realNameBaseSize);
      }
   }
   const int realNameBaseSizeFallbacks = path.size() + filename.size();
   // realname == path + filename + prefix;
   if (!suffix.isNull()) {
      realname.replace(realNameBaseSizeFallbacks, prefix.size(), suffix);
      // realname == path + filename;
      if (is_readable_file(realname)) {
         return realname;
      }
      realname.replace(realNameBaseSizeFallbacks, suffix.size(), prefix);
   }
   // realname == path + filename + prefix;
   if (is_readable_file(realname)) {
      return realname;
   }
   realname.truncate(realNameBaseSizeFallbacks);
   // realname == path + filename;
   if (is_readable_file(realname)) {
      return realname;
   }
   realname.truncate(0);
   return realname;
}

} // anonymous namespace

bool Translator::load(const Locale &locale,
                      const String &filename,
                      const String &prefix,
                      const String &directory,
                      const String &suffix)
{
   PDK_D(Translator);
   implPtr->clear();
   String fname = find_translation(locale, filename, prefix, directory, suffix);
   return !fname.isEmpty() && implPtr->doLoad(fname, directory);
}

bool Translator::load(const uchar *data, int len, const String &directory)
{
   PDK_D(Translator);
   implPtr->clear();
   if (!data || len < MagicLength || memcmp(data, sg_magic, MagicLength)) {
      return false;
   }   
   return implPtr->doLoad(data, len, directory);
}

String Translator::translate(const char *context, const char *sourceText, const char *disambiguation,
                             int n) const
{
   PDK_D(const Translator);
   return implPtr->doTranslate(context, sourceText, disambiguation, n);
}

bool Translator::isEmpty() const
{
   PDK_D(const Translator);
   return !implPtr->m_unmapPointer && !implPtr->m_unmapLength && !implPtr->m_messageArray &&
         !implPtr->m_offsetArray && !implPtr->m_contextArray && implPtr->m_subTranslators.empty();
}

} // utils
} // pdk

#endif // PDK_NO_TRANSLATION
