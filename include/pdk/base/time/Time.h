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
// Created by softboy on 2018/01/30.

#ifndef PDK_M_BASE_TIME_TIME_H
#define PDK_M_BASE_TIME_TIME_H

#include "pdk/base/lang/String.h"

namespace pdk {

#ifndef PDK_NO_DATASTREAM
namespace io {
class DataStream;
}
#endif

namespace time {

class DateTime;
namespace internal {
class DateTimePrivate;
} // internal

using internal::DateTimePrivate;
using pdk::lang::String;
using pdk::lang::StringView;
#ifndef PDK_NO_DATASTREAM
using pdk::io::DataStream;
#endif

class PDK_CORE_EXPORT Time
{
private:
   static constexpr int NULL_TIME = -1;
   explicit constexpr Time(int ms)
      : m_mds(ms)
   {}
   
public:
   constexpr Time()
      : m_mds(NULL_TIME)
   {}
   
   Time(int hours, int minutes, int seconds = 0, int ms = 0);
   
   constexpr bool isNull() const
   {
      return m_mds == NULL_TIME;
   }
   
   bool isValid() const;
   
   int getHour() const;
   int getMinute() const;
   int getSecond() const;
   int getMsec() const;
#ifndef PDK_NO_DATESTRING
   String toString(pdk::DateFormat f = pdk::DateFormat::TextDate) const;
#if PDK_STRINGVIEW_LEVEL < 2
   String toString(const String &format) const;
#endif
   String toString(StringView format) const;
#endif
   
   bool setHMS(int hour, int minute, int second, int ms = 0);
   
   Time addSecs(int secs) const PDK_REQUIRED_RESULT;
   int secsTo(const Time &) const;
   Time addMSecs(int ms) const PDK_REQUIRED_RESULT;
   int msecsTo(const Time &) const;
   void start();
   int restart();
   int elapsed() const;
   constexpr bool operator==(const Time &other) const
   {
      return m_mds == other.m_mds;
   }
   
   constexpr bool operator!=(const Time &other) const
   {
      return m_mds != other.m_mds;
   }
   
   constexpr bool operator< (const Time &other) const
   {
      return m_mds <  other.m_mds;
   }
   
   constexpr bool operator<=(const Time &other) const
   {
      return m_mds <= other.m_mds;
   }
   
   constexpr bool operator> (const Time &other) const
   { 
      return m_mds > other.m_mds;
   }
   
   constexpr bool operator>=(const Time &other) const
   {
      return m_mds >= other.m_mds;
   }
   
   static constexpr inline Time fromMSecsSinceStartOfDay(int msecs)
   {
      return Time(msecs);
   }
   
   constexpr inline int msecsSinceStartOfDay() const
   {
      return m_mds == NULL_TIME ? 0 : m_mds;
   }
   
   static Time getCurrentTime();
   
#ifndef PDK_NO_DATESTRING
   static Time fromString(const String &s, pdk::DateFormat f = pdk::DateFormat::TextDate);
   static Time fromString(const String &s, const String &format);
#endif
   static bool isValid(int hour, int minute, int second, int ms = 0);
private:
   constexpr inline int ds() const
   {
      return m_mds == -1 ? 0 : m_mds;
   }
   friend class DateTime;
   friend class DateTimePrivate;
#ifndef PDK_NO_DATASTREAM
   friend PDK_CORE_EXPORT DataStream &operator<<(DataStream &, const Time &);
   friend PDK_CORE_EXPORT DataStream &operator>>(DataStream &, Time &);
#endif
private:
   int m_mds;
};

PDK_CORE_EXPORT uint pdk_hash(const Time &key, uint seed = 0) noexcept;

} // time
} // pdk

PDK_DECLARE_TYPEINFO(pdk::time::Time, PDK_MOVABLE_TYPE);

#endif // PDK_M_BASE_TIME_TIME_H
