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

#include "pdk/utils/VersionNumber.h"
#include "pdk/kernel/HashFuncs.h"
#include "pdk/utils/internal/LocaleToolsPrivate.h"
#include "pdk/utils/Collator.h"

#ifndef PDK_NO_DATASTREAM
#  include "pdk/base/io/DataStream.h"
#endif // PDK_NO_DATASTREAM

#ifndef PDK_NO_DEBUG_STREAM
#  include "pdk/base/io/Debug.h"
#endif

#include <algorithm>
#include <limits>

namespace pdk {
namespace utils {

using pdk::lang::Latin1Character;
using pdk::lang::Latin1String;
using pdk::utils::internal::pdk_strtoull;
using pdk::io::DebugStateSaver;

std::vector<int> VersionNumber::getSegments() const
{
   if (m_segments.isUsingPointer()) {
      return *m_segments.m_pointerSegments;
   }
   std::vector<int> result;
   result.resize(segmentCount());
   for (int i = 0; i < segmentCount(); ++i) {
      result[i] = segmentAt(i);
   }
   return result;
}

VersionNumber VersionNumber::normalized() const
{
   int i;
   for (i = m_segments.size(); i; --i) {
      if (m_segments.at(i - 1) != 0) {
         break;
      }
   }
   VersionNumber result(*this);
   result.m_segments.resize(i);
   return result;
}

bool VersionNumber::isPrefixOf(const VersionNumber &other) const noexcept
{
   if (segmentCount() > other.segmentCount()) {
      return false;
   }
   
   for (int i = 0; i < segmentCount(); ++i) {
      if (segmentAt(i) != other.segmentAt(i)) {
         return false;
      } 
   }
   return true;
}

int VersionNumber::compare(const VersionNumber &v1, const VersionNumber &v2) noexcept
{
   int commonlen;
   if (PDK_LIKELY(!v1.m_segments.isUsingPointer() && !v2.m_segments.isUsingPointer())) {
      // we can't use memcmp because it interprets the data as unsigned bytes
      const pdk::pint8 *ptr1 = v1.m_segments.m_inlineSegments + InlineSegmentStartIdx;
      const pdk::pint8 *ptr2 = v2.m_segments.m_inlineSegments + InlineSegmentStartIdx;
      commonlen = std::min(v1.m_segments.size(),
                           v2.m_segments.size());
      for (int i = 0; i < commonlen; ++i) {
         if (int x = ptr1[i] - ptr2[i]) {
            return x;
         }
      }
   } else {
      commonlen = std::min(v1.segmentCount(), v2.segmentCount());
      for (int i = 0; i < commonlen; ++i) {
         if (v1.segmentAt(i) != v2.segmentAt(i)) {
            return v1.segmentAt(i) - v2.segmentAt(i);
         }
      }
   }
   
   // ran out of segments in v1 and/or v2 and need to check the first trailing
   // segment to finish the compare
   if (v1.segmentCount() > commonlen) {
      // v1 is longer
      if (v1.segmentAt(commonlen) != 0) {
         return v1.segmentAt(commonlen);
      } else {
         return 1;
      }
   } else if (v2.segmentCount() > commonlen) {
      // v2 is longer
      if (v2.segmentAt(commonlen) != 0) {
         return -v2.segmentAt(commonlen);
      } else {
         return -1;
      }
   }
   // the two version numbers are the same
   return 0;
}

VersionNumber VersionNumber::commonPrefix(const VersionNumber &v1,
                                          const VersionNumber &v2)
{
   int commonlen = std::min(v1.segmentCount(), v2.segmentCount());
   int i;
   for (i = 0; i < commonlen; ++i) {
      if (v1.segmentAt(i) != v2.segmentAt(i)) {
         break;
      }
   }
   
   if (i == 0) {
      return VersionNumber();
   }
   // try to use the one with inline segments, if there's one
   VersionNumber result(!v1.m_segments.isUsingPointer() ? v1 : v2);
   result.m_segments.resize(i);
   return result;
}

String VersionNumber::toString() const
{
   String version;
   version.reserve(std::max(segmentCount() * 2 - 1, 0));
   bool first = true;
   for (int i = 0; i < segmentCount(); ++i) {
      if (!first) {
         version += Latin1Character('.');
      }
      version += String::number(segmentAt(i));
      first = false;
   }
   return version;
}

#if PDK_STRINGVIEW_LEVEL < 2
VersionNumber VersionNumber::fromString(const String &string, int *suffixIndex)
{
   return fromString(Latin1String(string.toLatin1()), suffixIndex);
}
#endif

VersionNumber VersionNumber::fromString(StringView string, int *suffixIndex)
{
   return fromString(Latin1String(string.toLatin1()), suffixIndex);
}

VersionNumber VersionNumber::fromString(Latin1String string, int *suffixIndex)
{
   std::vector<int> seg;
   const char *start = string.begin();
   const char *end = start;
   const char *lastGoodEnd = start;
   const char *endOfString = string.end();
   
   do {
      bool ok = false;
      const pdk::pulonglong value = pdk_strtoull(start, &end, 10, &ok);
      if (!ok || value > pdk::pulonglong(std::numeric_limits<int>::max())) {
         break;
      }
      seg.push_back(int(value));
      start = end + 1;
      lastGoodEnd = end;
   } while (start < endOfString && (end < endOfString && *end == '.'));
   
   if (suffixIndex) {
      *suffixIndex = int(lastGoodEnd - string.begin());
   }
   return VersionNumber(std::move(seg));
}

void VersionNumber::SegmentStorage::setVector(int len, int major, int minor, int mic)
{
   m_pointerSegments = new std::vector<int>;
   m_pointerSegments->resize(len);
   m_pointerSegments->data()[0] = major;
   if (len > 1) {
      m_pointerSegments->data()[1] = minor;
      if (len > 2) {
         m_pointerSegments->data()[2] = mic;
      }
   }
}

#ifndef PDK_NO_DATASTREAM
DataStream &operator<<(DataStream &out, const VersionNumber &version)
{
   out << version.getSegments();
   return out;
}

DataStream &operator>>(DataStream &in, VersionNumber &version)
{
   if (!version.m_segments.isUsingPointer()) {
      version.m_segments.m_pointerSegments = new std::vector<int>;
   }
   in >> *version.m_segments.m_pointerSegments;
   return in;
}
#endif

#ifndef PDK_NO_DEBUG_STREAM
Debug operator<<(Debug debug, const VersionNumber &version)
{
   DebugStateSaver saver(debug);
   debug.noquote() << version.toString();
   return debug;
}
#endif

uint pdk_hash(const VersionNumber &key, uint seed)
{
   pdk::internal::HashCombine pdk_hash;
   for (int i = 0; i < key.segmentCount(); ++i) {
      seed = pdk_hash(seed, key.segmentAt(i));
   }
   return seed;
}

} // utils
} // pdk
