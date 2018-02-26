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
// Created by softboy on 2018/02/26.

#include "pdk/base/time/TimeZone.h"
#include "pdk/base/time/internal/TimeZonePrivate.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/lang/String.h"
#include "pdk/kernel/internal/CoreMacPrivate.h"
#include "pdk/base/io/Debug.h"
#include <algorithm>
#include <Foundation/NSTimeZone.h>

namespace pdk {
namespace time {
namespace internal {

using pdk::utils::Locale;
using pdk::ds::ByteArray;
using pdk::lang::String;

/*
    Private
    
    OS X system implementation
*/

// Create the system default time zone
MacTimeZonePrivate::MacTimeZonePrivate()
   : m_nstz(0)
{
   init(getSystemTimeZoneId());
}

// Create a named time zone
MacTimeZonePrivate::MacTimeZonePrivate(const ByteArray &ianaId)
   : m_nstz(0)
{
   init(ianaId);
}

MacTimeZonePrivate::MacTimeZonePrivate(const MacTimeZonePrivate &other)
   : TimeZonePrivate(other), m_nstz(0)
{
   m_nstz = [other.m_nstz copy];
}

MacTimeZonePrivate::~MacTimeZonePrivate()
{
   [m_nstz release];
}

MacTimeZonePrivate *MacTimeZonePrivate::clone() const
{
   return new MacTimeZonePrivate(*this);
}

void MacTimeZonePrivate::init(const ByteArray &ianaId)
{
   std::list<ByteArray> ids = getAvailableTimeZoneIds();
   if (std::find(ids.begin(), ids.end(), ianaId) != ids.end()) {
      m_nstz = [[NSTimeZone timeZoneWithName:String::fromUtf8(ianaId).toNSString()] retain];
      if (m_nstz) {
         m_id = ianaId;
      }
   }
}

String MacTimeZonePrivate::getComment() const
{
   return String::fromNSString([m_nstz description]);
}

String MacTimeZonePrivate::displayName(TimeZone::TimeType timeType,
                                       TimeZone::NameType nameType,
                                       const Locale &locale) const
{
   // TODO Mac doesn't support OffsetName yet so use standard offset name
   if (nameType == TimeZone::NameType::OffsetName) {
      const Data nowData = data(DateTime::getCurrentMSecsSinceEpoch());
      // TODO Cheat for now, assume if has dst the offset if 1 hour
      if (timeType == TimeZone::TimeType::DaylightTime && hasDaylightTime()) {
         return isoOffsetFormat(nowData.m_standardTimeOffset + 3600);
      } else {
         return isoOffsetFormat(nowData.m_standardTimeOffset);
      }
   }
   
   NSTimeZoneNameStyle style = NSTimeZoneNameStyleStandard;
   
   switch (nameType) {
   case TimeZone::NameType::ShortName :
      if (timeType == TimeZone::TimeType::DaylightTime) {
         style = NSTimeZoneNameStyleShortDaylightSaving;
      } else if (timeType == TimeZone::TimeType::GenericTime) {
         style = NSTimeZoneNameStyleShortGeneric;
      } else {
         style = NSTimeZoneNameStyleShortStandard;
      }
      break;
   case TimeZone::NameType::DefaultName :
   case TimeZone::NameType::LongName :
      if (timeType == TimeZone::TimeType::DaylightTime)
         style = NSTimeZoneNameStyleDaylightSaving;
      else if (timeType == TimeZone::TimeType::GenericTime)
         style = NSTimeZoneNameStyleGeneric;
      else
         style = NSTimeZoneNameStyleStandard;
      break;
   case TimeZone::NameType::OffsetName :
      // Unreachable
      break;
   }
   
   NSString *macLocaleCode = locale.getName().toNSString();
   NSLocale *macLocale = [[NSLocale alloc] initWithLocaleIdentifier:macLocaleCode];
   const String result = String::fromNSString([m_nstz localizedName:style locale:macLocale]);
   [macLocale release];
   return result;
}

String MacTimeZonePrivate::abbreviation(pdk::pint64 atMSecsSinceEpoch) const
{
   const NSTimeInterval seconds = atMSecsSinceEpoch / 1000.0;
   return String::fromNSString([m_nstz abbreviationForDate:[NSDate dateWithTimeIntervalSince1970:seconds]]);
}

int MacTimeZonePrivate::offsetFromUtc(pdk::pint64 atMSecsSinceEpoch) const
{
   const NSTimeInterval seconds = atMSecsSinceEpoch / 1000.0;
   return [m_nstz secondsFromGMTForDate:[NSDate dateWithTimeIntervalSince1970:seconds]];
}

int MacTimeZonePrivate::standardTimeOffset(pdk::pint64 atMSecsSinceEpoch) const
{
   return offsetFromUtc(atMSecsSinceEpoch) - daylightTimeOffset(atMSecsSinceEpoch);
}

int MacTimeZonePrivate::daylightTimeOffset(pdk::pint64 atMSecsSinceEpoch) const
{
   const NSTimeInterval seconds = atMSecsSinceEpoch / 1000.0;
   return [m_nstz daylightSavingTimeOffsetForDate:[NSDate dateWithTimeIntervalSince1970:seconds]];
}

bool MacTimeZonePrivate::hasDaylightTime() const
{
   // TODO No Mac API, assume if has transitions
   return hasTransitions();
}

bool MacTimeZonePrivate::isDaylightTime(pdk::pint64 atMSecsSinceEpoch) const
{
   const NSTimeInterval seconds = atMSecsSinceEpoch / 1000.0;
   return [m_nstz isDaylightSavingTimeForDate:[NSDate dateWithTimeIntervalSince1970:seconds]];
}

TimeZonePrivate::Data MacTimeZonePrivate::data(pdk::pint64 forMSecsSinceEpoch) const
{
   const NSTimeInterval seconds = forMSecsSinceEpoch / 1000.0;
   NSDate *date = [NSDate dateWithTimeIntervalSince1970:seconds];
   Data data;
   data.m_atMSecsSinceEpoch = forMSecsSinceEpoch;
   data.m_offsetFromUtc = [m_nstz secondsFromGMTForDate:date];
   data.m_daylightTimeOffset = [m_nstz daylightSavingTimeOffsetForDate:date];
   data.m_standardTimeOffset = data.m_offsetFromUtc - data.m_daylightTimeOffset;
   data.m_abbreviation = String::fromNSString([m_nstz abbreviationForDate:date]);
   return data;
}

bool MacTimeZonePrivate::hasTransitions() const
{
   // TODO No direct Mac API, so return if has next after 1970, i.e. since start of tz
   // TODO Not sure what is returned in event of no transitions, assume will be before requested date
   NSDate *epoch = [NSDate dateWithTimeIntervalSince1970:0];
   const NSDate *date = [m_nstz nextDaylightSavingTimeTransitionAfterDate:epoch];
   const bool result = ([date timeIntervalSince1970] > [epoch timeIntervalSince1970]);
   return result;
}

TimeZonePrivate::Data MacTimeZonePrivate::nextTransition(pdk::pint64 afterMSecsSinceEpoch) const
{
   TimeZonePrivate::Data tran;
   const NSTimeInterval seconds = afterMSecsSinceEpoch / 1000.0;
   NSDate *nextDate = [NSDate dateWithTimeIntervalSince1970:seconds];
   nextDate = [m_nstz nextDaylightSavingTimeTransitionAfterDate:nextDate];
   const NSTimeInterval nextSecs = [nextDate timeIntervalSince1970];
   if (nextDate == nil || nextSecs <= seconds) {
      [nextDate release];
      return getInvalidData();
   }
   tran.m_atMSecsSinceEpoch = nextSecs * 1000;
   tran.m_offsetFromUtc = [m_nstz secondsFromGMTForDate:nextDate];
   tran.m_daylightTimeOffset = [m_nstz daylightSavingTimeOffsetForDate:nextDate];
   tran.m_standardTimeOffset = tran.m_offsetFromUtc - tran.m_daylightTimeOffset;
   tran.m_abbreviation = String::fromNSString([m_nstz abbreviationForDate:nextDate]);
   return tran;
}

TimeZonePrivate::Data MacTimeZonePrivate::previousTransition(pdk::pint64 beforeMSecsSinceEpoch) const
{
   // The native API only lets us search forward, so we need to find an early-enough start:
   const NSTimeInterval lowerBound = std::numeric_limits<NSTimeInterval>::min();
   const pdk::pint64 endSecs = beforeMSecsSinceEpoch / 1000;
   const int year = 366 * 24 * 3600; // a (long) year, in seconds
   NSTimeInterval prevSecs = endSecs; // sentinel for later check
   NSTimeInterval nextSecs = prevSecs - year;
   NSTimeInterval tranSecs = lowerBound; // time at a transition; may be > endSecs
   
   NSDate *nextDate = [NSDate dateWithTimeIntervalSince1970:nextSecs];
   nextDate = [m_nstz nextDaylightSavingTimeTransitionAfterDate:nextDate];
   if (nextDate != nil
       && (tranSecs = [nextDate timeIntervalSince1970]) < endSecs) {
      // There's a transition within the last year before endSecs:
      nextSecs = tranSecs;
   } else {
      // Need to start our search earlier:
      nextDate = [NSDate dateWithTimeIntervalSince1970:lowerBound];
      nextDate = [m_nstz nextDaylightSavingTimeTransitionAfterDate:nextDate];
      if (nextDate != nil) {
         NSTimeInterval lateSecs = nextSecs;
         nextSecs = [nextDate timeIntervalSince1970];
         PDK_ASSERT(nextSecs <= endSecs - year || nextSecs == tranSecs);
         /*
              We're looking at the first ever transition for our zone, at
              nextSecs (and our zone *does* have at least one transition).  If
              it's later than endSecs - year, then we must have found it on the
              initial check and therefore set tranSecs to the same transition
              time (which, we can infer here, is >= endSecs).  In this case, we
              won't enter the binary-chop loop, below.
              
              In the loop, nextSecs < lateSecs < endSecs: we have a transition
              at nextSecs and there is no transition between lateSecs and
              endSecs.  The loop narrows the interval between nextSecs and
              lateSecs by looking for a transition after their mid-point; if it
              finds one < endSecs, nextSecs moves to this transition; otherwise,
              lateSecs moves to the mid-point.  This soon enough narrows the gap
              to within a year, after which walking forward one transition at a
              time (the "Wind through" loop, below) is good enough.
            */
         
         // Binary chop to within a year of last transition before endSecs:
         while (nextSecs + year < lateSecs) {
            // Careful about overflow, not fussy about rounding errors:
            NSTimeInterval middle = nextSecs / 2 + lateSecs / 2;
            NSDate *split = [NSDate dateWithTimeIntervalSince1970:middle];
            split = [m_nstz nextDaylightSavingTimeTransitionAfterDate:split];
            if (split != nil
                && (tranSecs = [split timeIntervalSince1970]) < endSecs) {
               nextDate = split;
               nextSecs = tranSecs;
            } else {
               lateSecs = middle;
            }
         }
         PDK_ASSERT(nextDate != nil);
         // ... and nextSecs < endSecs unless first transition ever was >= endSecs.
      } // else: we have no data - prevSecs is still endSecs, nextDate is still nil
   }
   // Either nextDate is nil or nextSecs is at its transition.
   
   // Wind through remaining transitions (spanning at most a year), one at a time:
   while (nextDate != nil && nextSecs < endSecs) {
      prevSecs = nextSecs;
      nextDate = [m_nstz nextDaylightSavingTimeTransitionAfterDate:nextDate];
      nextSecs = [nextDate timeIntervalSince1970];
      if (nextSecs <= prevSecs) // presumably no later data available
         break;
   }
   if (prevSecs < endSecs) // i.e. we did make it into that while loop
      return data(pdk::pint64(prevSecs * 1e3));
   
   // No transition data; or first transition later than requested time.
   return getInvalidData();
}

ByteArray MacTimeZonePrivate::getSystemTimeZoneId() const
{
   // Reset the cached system tz then return the name
   [NSTimeZone resetSystemTimeZone];
   return String::fromNSString([[NSTimeZone systemTimeZone] name]).toUtf8();
}

std::list<ByteArray> MacTimeZonePrivate::getAvailableTimeZoneIds() const
{
   NSEnumerator *enumerator = [[NSTimeZone knownTimeZoneNames] objectEnumerator];
   ByteArray tzid = String::fromNSString([enumerator nextObject]).toUtf8();
   
   std::list<ByteArray> list;
   while (!tzid.isEmpty()) {
      list.push_back(tzid);
      tzid = String::fromNSString([enumerator nextObject]).toUtf8();
   }
   list.sort();
   list.erase(std::unique(list.begin(), list.end()), list.end());
   return list;
}

NSTimeZone *MacTimeZonePrivate::getNsTimeZone() const
{
   return m_nstz;
}

} // internal
} // time
} // pdk
