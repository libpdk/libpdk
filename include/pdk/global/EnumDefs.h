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
// Created by softboy on 2017/11/13.

#ifndef PDK_GLOBAL_ENUM_DEFS_H
#define PDK_GLOBAL_ENUM_DEFS_H

#include "pdk/global/Global.h"

namespace pdk {

enum class Initialization
{
   Uninitialized
};

static constexpr PDK_DECL_UNUSED Initialization Uninitialized = Initialization::Uninitialized;

enum class CaseSensitivity
{
   Insensitive,
   Sensitive
};

enum class TimerType
{
   PreciseTimer,
   CoarseTimer,
   VeryCoarseTimer
};

enum class EventPriority
{
   HighEventPriority = 1,
   NormalEventPriority = 0,
   LowEventPriority = -1
};

enum class DateFormat
{
   TextDate,      // default
   ISODate,       // ISO 8601
   SystemLocaleShortDate,
   SystemLocaleLongDate,
   DefaultLocaleShortDate,
   DefaultLocaleLongDate,
   RFC2822Date,        // RFC 2822 (+ 850 and 1036 during parsing)
   ISODateWithMs
};

enum class TimeSpec
{
   LocalTime,
   UTC,
   OffsetFromUTC,
   TimeZone
};

enum class DayOfWeek
{
   Monday = 1,
   Tuesday = 2,
   Wednesday = 3,
   Thursday = 4,
   Friday = 5,
   Saturday = 6,
   Sunday = 7
};

} // pdk

#endif // PDK_GLOBAL_ENUM_DEFS_H
