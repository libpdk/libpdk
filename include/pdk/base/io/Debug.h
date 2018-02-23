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

#ifndef PDK_M_BASE_IO_DEBUG_H
#define PDK_M_BASE_IO_DEBUG_H

#include "pdk/kernel/Algorithms.h"
#include "pdk/base/io/TextStream.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/lang/StringView.h"
#include "pdk/global/Flags.h"
#include "pdk/kernel/internal/CoreMacPrivate.h"
#include "pdk/global/Logging.h"

#include <vector>
#include <list>
#include <map>
#include <utility>

namespace pdk {

// forward declare class with namespace
namespace ds {
class ByteArray;
} // lang

namespace io {

using pdk::ds::ByteArray;
using pdk::lang::Latin1String;
using pdk::lang::String;
using pdk::lang::StringRef;
using pdk::lang::StringView;
using pdk::MessageLogContext;

namespace internal
{
class DebugStateSaverPrivate;
} // internal

using internal::DebugStateSaverPrivate;

class PDK_CORE_EXPORT Debug
{
   friend class MessageLogger;
   friend class DebugStateSaverPrivate;
   struct Stream
   {
      constexpr static const int DefaultVerbosity = 2;
      constexpr static const int VerbosityShift = 29;
      constexpr static const int VerbosityMask = 0x7;
      
      Stream(IoDevice *device)
         : m_ts(device),
           m_ref(1), 
           m_type(pdk::MsgType::DebugMsg),
           m_space(true),
           m_messageOutput(false),
           m_flags(DefaultVerbosity << VerbosityShift)
      {}
      
      Stream(String *string)
         : m_ts(string, IoDevice::OpenMode::WriteOnly),
           m_ref(1),
           m_type(pdk::MsgType::DebugMsg),
           m_space(true), 
           m_messageOutput(false),
           m_flags(DefaultVerbosity << VerbosityShift)
      {}
      
      Stream(pdk::MsgType t)
         : m_ts(&m_buffer, IoDevice::OpenMode::WriteOnly),
           m_ref(1),
           m_type(t),
           m_space(true),
           m_messageOutput(true),
           m_flags(DefaultVerbosity << VerbosityShift)
      {}
      
      TextStream m_ts;
      String m_buffer;
      int m_ref;
      pdk::MsgType m_type;
      bool m_space;
      bool m_messageOutput;
      MessageLogContext m_context;
      
      enum FormatFlag
      { // Note: Bits 29..31 are reserved for the verbose level introduced in 5.6.
         NoQuotes = 0x1
      };
      
      // ### Qt 6: unify with space, introduce own version member
      bool testFlag(FormatFlag flag) const
      {
         return (m_context.m_version > 1) ? (m_flags & flag) : false;
      }
      
      void setFlag(FormatFlag flag) 
      {
         if (m_context.m_version > 1) {
            m_flags |= flag;
         }
      }
      void unsetFlag(FormatFlag flag)
      {
         if (m_context.m_version > 1) {
            m_flags &= ~flag;
         }
      }
      
      int verbosity() const
      {
         return m_context.m_version > 1 ? (m_flags >> VerbosityShift) & VerbosityMask : int(Stream::DefaultVerbosity);
      }
      void setVerbosity(int v)
      {
         if (m_context.m_version > 1) {
            m_flags &= ~(VerbosityMask << VerbosityShift);
            m_flags |= (v & VerbosityMask) << VerbosityShift;
         }
      }
      // added in 5.4
      int m_flags;
   } *m_stream;
   
   enum class Latin1Content
   {
      ContainsBinary = 0,
      ContainsLatin1
   };
   
   void putUcs4(uint ucs4);
   void putString(const Character *begin, size_t length);
   void putByteArray(const char *begin, size_t length, Latin1Content content);
public:
   inline Debug(IoDevice *device)
      : m_stream(new Stream(device))
   {}
   
   inline Debug(String *string)
      : m_stream(new Stream(string))
   {}
   
   inline Debug(pdk::MsgType t)
      : m_stream(new Stream(t))
   {}
   
   inline Debug(const Debug &other)
      : m_stream(other.m_stream) 
   {
      ++m_stream->m_ref;
   }
   
   inline Debug &operator=(const Debug &other);
   ~Debug();
   inline void swap(Debug &other) noexcept
   {
      std::swap(m_stream, other.m_stream);
   }
   
   Debug &resetFormat();
   
   inline Debug &space()
   {
      m_stream->m_space = true;
      m_stream->m_ts << ' ';
      return *this;
   }
   
   inline Debug &nospace()
   {
      m_stream->m_space = false;
      return *this;
   }
   
   inline Debug &maybeSpace()
   {
      if (m_stream->m_space) {
         m_stream->m_ts << ' ';
      }
      return *this;
   }
   
   int verbosity() const 
   {
      return m_stream->verbosity();
   }
   
   void setVerbosity(int verbosityLevel)
   {
      m_stream->setVerbosity(verbosityLevel);
   }
   
   bool autoInsertSpaces() const
   {
      return m_stream->m_space;
   }
   
   void setAutoInsertSpaces(bool b)
   {
      m_stream->m_space = b;
   }
   
   inline Debug &quote()
   {
      m_stream->unsetFlag(Stream::NoQuotes);
      return *this;
   }
   
   inline Debug &noquote()
   { 
      m_stream->setFlag(Stream::NoQuotes); 
      return *this; 
   }
   inline Debug &maybeQuote(char c = '"') 
   { 
      if (!(m_stream->testFlag(Stream::NoQuotes))) {
         m_stream->m_ts << c; 
      }
      return *this; 
   }
   
   inline Debug &operator<<(Character t)
   {
      putUcs4(t.unicode());
      return maybeSpace();
   }
   
   inline Debug &operator<<(bool t)
   {
      m_stream->m_ts << (t ? "true" : "false");
      return maybeSpace();
   }
   
   inline Debug &operator<<(char t)
   {
      m_stream->m_ts << t;
      return maybeSpace();
   }
   
   inline Debug &operator<<(signed short t)
   {
      m_stream->m_ts << t;
      return maybeSpace();
   }
   
   inline Debug &operator<<(unsigned short t)
   {
      m_stream->m_ts << t;
      return maybeSpace();
   }
   
   inline Debug &operator<<(char16_t t)
   {
      return *this << Character(ushort(t));
   }
   
   inline Debug &operator<<(char32_t t)
   {
      putUcs4(t); 
      return maybeSpace();
   }
   
   inline Debug &operator<<(signed int t)
   {
      m_stream->m_ts << t;
      return maybeSpace();
   }
   
   inline Debug &operator<<(unsigned int t)
   {
      m_stream->m_ts << t;
      return maybeSpace();
   }
   
   inline Debug &operator<<(signed long t)
   { 
      m_stream->m_ts << t; 
      return maybeSpace();
   }
   
   inline Debug &operator<<(unsigned long t)
   {
      m_stream->m_ts << t;
      return maybeSpace();
   }
   
   inline Debug &operator<<(pdk::pint64 t)
   {
      m_stream->m_ts << t;
      return maybeSpace();
   }
   inline Debug &operator<<(pdk::puint64 t)
   {
      m_stream->m_ts << t;
      return maybeSpace();
   }
   
   inline Debug &operator<<(float t)
   {
      m_stream->m_ts << t;
      return maybeSpace();
   }
   
   inline Debug &operator<<(double t) 
   {
      m_stream->m_ts << t; 
      return maybeSpace(); 
   }
   
   inline Debug &operator<<(const char *t) 
   { 
      m_stream->m_ts << String::fromUtf8(t); 
      return maybeSpace(); 
   }
#if PDK_STRINGVIEW_LEVEL < 2
   inline Debug &operator<<(const String &t) 
   { 
      putString(t.getConstRawData(), uint(t.length())); 
      return maybeSpace(); 
   }
   
   inline Debug &operator<<(const StringRef &t) 
   { 
      putString(t.getConstRawData(), uint(t.length())); 
      return maybeSpace(); 
   }
#endif
   inline Debug &operator<<(StringView s) 
   { 
      putString(s.data(), size_t(s.size())); 
      return maybeSpace(); 
   }
   inline Debug &operator<<(Latin1String t) 
   {
      putByteArray(t.latin1(), t.size(), Latin1Content::ContainsLatin1);
      return maybeSpace(); 
   }
   
   inline Debug &operator<<(const ByteArray &t) 
   {
      putByteArray(t.getConstRawData(), t.size(), Latin1Content::ContainsBinary);
      return maybeSpace(); 
   }
   
   inline Debug &operator<<(const void *t) 
   { 
      m_stream->m_ts << t;
      return maybeSpace();
   }
   
   inline Debug &operator<<(std::nullptr_t) 
   { 
      m_stream->m_ts << "(nullptr)";
      return maybeSpace();
   }
   
   inline Debug &operator<<(TextStreamFunc f) {
      m_stream->m_ts << f;
      return *this;
   }
   
   inline Debug &operator<<(TextStreamManipulator m)
   { 
      m_stream->m_ts << m;
      return *this;
   }
};

class PDK_CORE_EXPORT DebugStateSaver
{
public:
   DebugStateSaver(Debug &dbg);
   ~DebugStateSaver();
private:
   PDK_DISABLE_COPY(DebugStateSaver);
   pdk::utils::ScopedPointer<DebugStateSaverPrivate> m_implPtr;
};

class NoDebug
{
public:
   inline NoDebug &operator<<(TextStreamFunc)
   {
      return *this;
   }
   
   inline NoDebug &operator<<(TextStreamManipulator)
   {
      return *this;
   }
   
   inline NoDebug &space()
   {
      return *this;
   }
   
   inline NoDebug &nospace() 
   { 
      return *this; 
   }
   
   inline NoDebug &maybeSpace() 
   { 
      return *this; 
   }
   
   inline NoDebug &quote() 
   { 
      return *this; 
   }
   
   inline NoDebug &noquote() 
   { 
      return *this;
   }
   
   inline NoDebug &maybeQuote(const char = '"') 
   { 
      return *this;
   }
   
   template<typename T>
   inline NoDebug &operator<<(const T &)
   { 
      return *this; 
   }
};

inline Debug &Debug::operator=(const Debug &other)
{
   if (this != &other) {
      Debug copy(other);
      std::swap(m_stream, copy.m_stream);
   }
   return *this;
}

namespace internal {

template <typename SequentialContainer>
inline Debug print_sequential_container(Debug debug, const char *which, const SequentialContainer &c)
{
   const bool oldSetting = debug.autoInsertSpaces();
   debug.nospace() << which << '(';
   typename SequentialContainer::const_iterator it = c.begin(), end = c.end();
   if (it != end) {
      debug << *it;
      ++it;
   }
   while (it != end) {
      debug << ", " << *it;
      ++it;
   }
   debug << ')';
   debug.setAutoInsertSpaces(oldSetting);
   return debug.maybeSpace();
}

} // namespace internal

template <typename T, typename Alloc>
inline Debug operator<<(Debug debug, const std::vector<T, Alloc> &vec)
{
   return internal::print_sequential_container(debug, "std::vector", vec);
}

template <typename T, typename Alloc>
inline Debug operator<<(Debug debug, const std::list<T, Alloc> &vec)
{
   return internal::print_sequential_container(debug, "std::list", vec);
}

template <typename Key, typename T, typename Compare, typename Alloc>
inline Debug operator<<(Debug debug, const std::map<Key, T, Compare, Alloc> &map)
{
   return internal::print_sequential_container(debug, "std::map", map); // yes, sequential: *it is std::pair
}

template <typename Key, typename T, typename Compare, typename Alloc>
inline Debug operator<<(Debug debug, const std::multimap<Key, T, Compare, Alloc> &map)
{
   return internal::print_sequential_container(debug, "std::multimap", map); // yes, sequential: *it is std::pair
}

template <class T1, class T2>
inline Debug operator<<(Debug debug, const std::pair<T1, T2> &pair)
{
   const bool oldSetting = debug.autoInsertSpaces();
   debug.nospace() << "std::pair(" << pair.first << ',' << pair.second << ')';
   debug.setAutoInsertSpaces(oldSetting);
   return debug.maybeSpace();
}

template <typename Key, typename Compare, typename Alloc>
inline Debug operator<<(Debug debug, const std::set<Key, Compare, Alloc> &set)
{
   return internal::print_sequential_container(debug, "std::set", set);
}

//template <class T>
//inline Debug operator<<(Debug debug, const ContiguousCache<T> &cache)
//{
//   const bool oldSetting = debug.autoInsertSpaces();
//   debug.nospace() << "ContiguousCache(";
//   for (int i = cache.firstIndex(); i <= cache.lastIndex(); ++i) {
//      debug << cache[i];
//      if (i != cache.lastIndex())
//         debug << ", ";
//   }
//   debug << ')';
//   debug.setAutoInsertSpaces(oldSetting);
//   return debug.maybeSpace();
//}

template <class T>
inline Debug operator<<(Debug debug, const std::shared_ptr<T> &ptr)
{
   DebugStateSaver saver(debug);
   debug.nospace() << "std::shared_ptr(" << ptr.get() << ")";
   return debug;
}

PDK_CORE_EXPORT void pdk_meta_enum_flag_debug_operator(Debug &debug, size_t sizeofT, int value);

template <typename Int>
void pdk_meta_enum_flag_debug_operator(Debug &debug, size_t sizeofT, Int value)
{
   const DebugStateSaver saver(debug);
   debug.resetFormat();
   debug.nospace() << "QFlags(" << hex << showbase;
   bool needSeparator = false;
   for (uint i = 0; i < sizeofT * 8; ++i) {
      if (value & (Int(1) << i)) {
         if (needSeparator) {
            debug << '|';
         } else {
            needSeparator = true;
         }
         debug << (Int(1) << i);
      }
   }
   debug << ')';
}

template <class T>
inline Debug pdk_meta_enum_flag_debug_operator_helper(Debug debug, const pdk::Flags<T> &flags)
{
   pdk_meta_enum_flag_debug_operator(debug, sizeof(T), typename pdk::Flags<T>::UnderType(flags));
   return debug;
}

template<typename T>
inline Debug operator<<(Debug debug, const pdk::Flags<T> &flags)
{
   // We have to use an indirection otherwise specialisation of some other overload of the
   // operator<< the compiler would try to instantiate QFlags<T> for the std::enable_if
   return pdk_meta_enum_flag_debug_operator_helper(debug, flags);
}
         
} // io
} // pdk

PDK_DECLARE_SHARED(pdk::io::Debug)

#endif // PDK_M_BASE_IO_DEBUG_H
