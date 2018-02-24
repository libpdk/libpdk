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
// Created by softboy on 2018/02/23.

#ifndef PDK_UTILS_VERSION_NUMBER_H
#define PDK_UTILS_VERSION_NUMBER_H

#include "pdk/base/lang/String.h"
#ifndef PDK_NO_DEBUG_STREAM
#include "pdk/base/io/Debug.h"
#endif

#include <vector>

namespace pdk {

// forward declare class with namespace
namespace io {
class DataStream;
} // io

namespace utils {

using pdk::io::DataStream;
using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::StringView;
using pdk::io::Debug;

class VersionNumber;
PDK_CORE_EXPORT uint pdk_hash(const VersionNumber &key, uint seed = 0);

#ifndef PDK_NO_DATASTREAM
PDK_CORE_EXPORT DataStream &operator<<(DataStream &out, const VersionNumber &version);
PDK_CORE_EXPORT DataStream &operator>>(DataStream &in, VersionNumber &version);
#endif

class VersionNumber
{
   // VersionNumber stores small values inline, without memory allocation.
   // We do that by setting the LSB in the pointer that would otherwise hold
   // the longer form of the segments.
   // The constants below help us deal with the permutations for 32- and 64-bit,
   // little- and big-endian architectures.
   enum {
      // in little-endian, inline_segments[0] is shared with the pointer's LSB, while
      // in big-endian, it's inline_segments[7]
      InlineSegmentMarker = PDK_BYTE_ORDER == PDK_LITTLE_ENDIAN ? 0 : sizeof(void*) - 1,
      InlineSegmentStartIdx = !InlineSegmentMarker, // 0 for BE, 1 for LE
      InlineSegmentCount = sizeof(void*) - 1
   };
   PDK_STATIC_ASSERT(InlineSegmentCount >= 3);   // at least major, minor, micro
   
   struct SegmentStorage {
      // Note: we alias the use of dummy and inline_segments in the use of the
      // union below. This is undefined behavior in C++98, but most compilers implement
      // the C++11 behavior. The one known exception is older versions of Sun Studio.
      union {
         pdk::uintptr m_dummy;
         pdk::pint8 m_inlineSegments[sizeof(void*)];
         std::vector<int> *m_pointerSegments;
      };
      
      // set the InlineSegmentMarker and set length to zero
      SegmentStorage() noexcept
         : m_dummy(1) 
      {}
      
      SegmentStorage(const std::vector<int> &seg)
      {
         if (dataFitsInline(seg.data(), seg.size())) {
            setInlineData(seg.data(), seg.size());
         } else {
            m_pointerSegments = new std::vector<int>(seg);
         }
      }
      
      SegmentStorage(const SegmentStorage &other)
      {
         if (other.isUsingPointer()) {
            m_pointerSegments = new std::vector<int>(*other.m_pointerSegments);
         } else {
            m_dummy = other.m_dummy;
         }
      }
      
      SegmentStorage &operator=(const SegmentStorage &other)
      {
         if (isUsingPointer() && other.isUsingPointer()) {
            *m_pointerSegments = *other.m_pointerSegments;
         } else if (other.isUsingPointer()) {
            m_pointerSegments = new std::vector<int>(*other.m_pointerSegments);
         } else {
            if (isUsingPointer()) {
               delete m_pointerSegments;
            }
            m_dummy = other.m_dummy;
         }
         return *this;
      }
      
      SegmentStorage(SegmentStorage &&other) noexcept
         : m_dummy(other.m_dummy)
      {
         other.m_dummy = 1;
      }
      
      SegmentStorage &operator=(SegmentStorage &&other) noexcept
      {
         std::swap(m_dummy, other.m_dummy);
         return *this;
      }
      
      explicit SegmentStorage(std::vector<int> &&seg)
      {
         if (dataFitsInline(seg.data(), seg.size())) {
            setInlineData(seg.data(), seg.size());
         } else {
            m_pointerSegments = new std::vector<int>(std::move(seg));
         }
      }
      
      SegmentStorage(std::initializer_list<int> args)
      {
         if (dataFitsInline(args.begin(), int(args.size()))) {
            setInlineData(args.begin(), int(args.size()));
         } else {
            m_pointerSegments = new std::vector<int>(args);
         }
      }
      
      ~SegmentStorage()
      {
         if (isUsingPointer()) {
            delete m_pointerSegments; 
         }
      }
      
      bool isUsingPointer() const noexcept
      {
         return (m_inlineSegments[InlineSegmentMarker] & 1) == 0;
      }
      
      int size() const noexcept
      {
         return isUsingPointer() 
               ? m_pointerSegments->size() 
               : (m_inlineSegments[InlineSegmentMarker] >> 1); 
      }
      
      void setInlineSize(int len)
      { 
         m_inlineSegments[InlineSegmentMarker] = 1 + 2 * len; 
      }
      
      void resize(int len)
      {
         if (isUsingPointer()) {
            m_pointerSegments->resize(len);
         } else {
            setInlineSize(len);
         }
      }
      
      int at(int index) const
      {
         return isUsingPointer() ?
                  m_pointerSegments->at(index) :
                  m_inlineSegments[InlineSegmentStartIdx + index];
      }
      
      void setSegments(int len, int maj, int min = 0, int mic = 0)
      {
         if (maj == pdk::pint8(maj) && min == pdk::pint8(min) && mic == pdk::pint8(mic)) {
            int data[] = { maj, min, mic };
            setInlineData(data, len);
         } else {
            setVector(len, maj, min, mic);
         }
      }
      
   private:
      static bool dataFitsInline(const int *data, int len)
      {
         if (len > InlineSegmentCount) {
            return false;
         }
         for (int i = 0; i < len; ++i) {
            if (data[i] != pdk::pint8(data[i])) {
               return false;
            }
         }
         return true;
      }
      
      void setInlineData(const int *data, int len)
      {
         m_dummy = 1 + len * 2;
#if PDK_BYTE_ORDER == PDK_LITTLE_ENDIAN
         for (int i = 0; i < len; ++i) {
            m_dummy |= pdk::uintptr(data[i] & 0xFF) << (8 * (i + 1));
         }
#elif PDK_BYTE_ORDER == PDK_BIG_ENDIAN
         for (int i = 0; i < len; ++i) {
            m_dummy |= pdk::uintptr(data[i] & 0xFF) << (8 * (sizeof(void *) - i - 1));
         }
#else
         // the code above is equivalent to:
         setInlineSize(len);
         for (int i = 0; i < len; ++i) {
            m_inlineSegments[InlineSegmentStartIdx + i] = data[i] & 0xFF;
         }
#endif
      }
      PDK_CORE_EXPORT void setVector(int len, int maj, int min, int mic);
   } m_segments;
   
public:
   inline VersionNumber() noexcept
      : m_segments()
   {}
   inline explicit VersionNumber(const std::vector<int> &seg)
      : m_segments(seg)
   {}
   
   // compiler-generated copy/move ctor/assignment operators and the destructor are ok
   
   explicit VersionNumber(std::vector<int> &&seg)
      : m_segments(std::move(seg))
   {}
   
   inline VersionNumber(std::initializer_list<int> args)
      : m_segments(args)
   {}
   
   inline explicit VersionNumber(int major)
   {
      m_segments.setSegments(1, major);
   }
   
   inline explicit VersionNumber(int major, int minor)
   {
      m_segments.setSegments(2, major, minor);
   }
   
   inline explicit VersionNumber(int major, int minor, int mic)
   {
      m_segments.setSegments(3, major, minor, mic);
   }
   
   PDK_REQUIRED_RESULT inline bool isNull() const noexcept
   {
      return segmentCount() == 0;
   }
   
   PDK_REQUIRED_RESULT inline bool isNormalized() const noexcept
   {
      return isNull() || segmentAt(segmentCount() - 1) != 0;
   }
   
   PDK_REQUIRED_RESULT inline int majorVersion() const noexcept
   {
      return segmentAt(0);
   }
   
   PDK_REQUIRED_RESULT inline int minorVersion() const noexcept
   {
      return segmentAt(1);
   }
   
   PDK_REQUIRED_RESULT inline int microVersion() const noexcept
   {
      return segmentAt(2);
   }
   
   PDK_REQUIRED_RESULT PDK_CORE_EXPORT VersionNumber normalized() const;
   
   PDK_REQUIRED_RESULT PDK_CORE_EXPORT std::vector<int> getSegments() const;
   
   PDK_REQUIRED_RESULT inline int segmentAt(int index) const noexcept
   {
      return (m_segments.size() > index) ? m_segments.at(index) : 0;
   }
   
   PDK_REQUIRED_RESULT inline int segmentCount() const noexcept
   {
      return m_segments.size();
   }
   
   PDK_REQUIRED_RESULT PDK_CORE_EXPORT bool isPrefixOf(const VersionNumber &other) const noexcept;
   
   PDK_REQUIRED_RESULT PDK_CORE_EXPORT static int compare(const VersionNumber &v1, const VersionNumber &v2) noexcept;
   
   PDK_REQUIRED_RESULT PDK_CORE_EXPORT static PDK_DECL_PURE_FUNCTION 
   VersionNumber commonPrefix(const VersionNumber &v1, const VersionNumber &v2);
   
   PDK_REQUIRED_RESULT PDK_CORE_EXPORT String toString() const;
#if PDK_STRINGVIEW_LEVEL < 2
   PDK_REQUIRED_RESULT PDK_CORE_EXPORT static PDK_DECL_PURE_FUNCTION 
   VersionNumber fromString(const String &string, int *suffixIndex = nullptr);
#endif
   PDK_REQUIRED_RESULT PDK_CORE_EXPORT static PDK_DECL_PURE_FUNCTION 
   VersionNumber fromString(Latin1String string, int *suffixIndex = nullptr);
   
   PDK_REQUIRED_RESULT PDK_CORE_EXPORT static PDK_DECL_PURE_FUNCTION 
   VersionNumber fromString(StringView string, int *suffixIndex = nullptr);
   
private:
#ifndef PDK_NO_DATASTREAM
   friend PDK_CORE_EXPORT DataStream& operator>>(DataStream &in, VersionNumber &version);
#endif
   friend PDK_CORE_EXPORT uint pdk_hash(const VersionNumber &key, uint seed);
};

#ifndef PDK_NO_DEBUG_STREAM
PDK_CORE_EXPORT Debug operator<<(Debug, const VersionNumber &version);
#endif

PDK_REQUIRED_RESULT inline bool operator> (const VersionNumber &lhs, const VersionNumber &rhs) noexcept
{
   return VersionNumber::compare(lhs, rhs) > 0;
}

PDK_REQUIRED_RESULT inline bool operator>=(const VersionNumber &lhs, const VersionNumber &rhs) noexcept
{
   return VersionNumber::compare(lhs, rhs) >= 0;
}

PDK_REQUIRED_RESULT inline bool operator< (const VersionNumber &lhs, const VersionNumber &rhs) noexcept
{
   return VersionNumber::compare(lhs, rhs) < 0;
}

PDK_REQUIRED_RESULT inline bool operator<=(const VersionNumber &lhs, const VersionNumber &rhs) noexcept
{
   return VersionNumber::compare(lhs, rhs) <= 0;
}

PDK_REQUIRED_RESULT inline bool operator==(const VersionNumber &lhs, const VersionNumber &rhs) noexcept
{
   return VersionNumber::compare(lhs, rhs) == 0;
}

PDK_REQUIRED_RESULT inline bool operator!=(const VersionNumber &lhs, const VersionNumber &rhs) noexcept
{
   return VersionNumber::compare(lhs, rhs) != 0;
}

} // utils
} // pdk

PDK_DECLARE_TYPEINFO(pdk::utils::VersionNumber, PDK_MOVABLE_TYPE);

#endif // PDK_UTILS_VERSION_NUMBER_H
