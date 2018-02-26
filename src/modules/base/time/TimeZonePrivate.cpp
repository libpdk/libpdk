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
#include "pdk/base/time/internal/TimeZoneDataPrivate.h"

#include "pdk/base/io/DataStream.h"
#include "pdk/base/io/Debug.h"
#include <algorithm>

namespace pdk {
namespace time {
namespace internal {

using pdk::ds::ByteArray;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;

// Static utilities for looking up Windows ID tables

static const int sg_windowsDataTableSize = sizeof(sg_windowsDataTable) / sizeof(WindowsData) - 1;
static const int sg_zoneDataTableSize = sizeof(sg_zoneDataTable) / sizeof(ZoneData) - 1;
static const int sg_utcDataTableSize = sizeof(sg_utcDataTable) / sizeof(UtcData) - 1;

namespace {

const ZoneData *zone_data(pdk::puint16 index)
{
   PDK_ASSERT(index < sg_zoneDataTableSize);
   return &sg_zoneDataTable[index];
}

const WindowsData *windows_data(pdk::puint16 index)
{
   PDK_ASSERT(index < sg_windowsDataTableSize);
   return &sg_windowsDataTable[index];
}

const UtcData *utc_data(pdk::puint16 index)
{
   PDK_ASSERT(index < sg_utcDataTableSize);
   return &sg_utcDataTable[index];
}

// Return the Windows ID literal for a given WindowsData
ByteArray windows_id(const WindowsData *windowsData)
{
   return (sg_windowsIdData + windowsData->m_windowsIdIndex);
}

// Return the IANA ID literal for a given WindowsData
ByteArray iana_id(const WindowsData *windowsData)
{
   return (sg_ianaIdData + windowsData->m_ianaIdIndex);
}

// Return the IANA ID literal for a given ZoneData
ByteArray iana_id(const ZoneData *zoneData)
{
   return (sg_ianaIdData + zoneData->m_ianaIdIndex);
}

ByteArray utc_id(const UtcData *utcData)
{
   return (sg_ianaIdData + utcData->m_ianaIdIndex);
}

pdk::puint16 to_windows_id_key(const ByteArray &winId)
{
   for (pdk::puint16 i = 0; i < sg_windowsDataTableSize; ++i) {
      const WindowsData *data = windows_data(i);
      if (windows_id(data) == winId) {
         return data->m_windowsIdKey;
      }
   }
   return 0;
}

ByteArray to_windows_id_literal(pdk::puint16 windowsIdKey)
{
   for (pdk::puint16 i = 0; i < sg_windowsDataTableSize; ++i) {
      const WindowsData *data = windows_data(i);
      if (data->m_windowsIdKey == windowsIdKey) {
         return windows_id(data);
      }
   }
   return ByteArray();
}

} // anonymous namespace

TimeZonePrivate::TimeZonePrivate()
{
}

TimeZonePrivate::TimeZonePrivate(const TimeZonePrivate &other)
   : SharedData(other),
     m_id(other.m_id)
{}

TimeZonePrivate::~TimeZonePrivate()
{}

TimeZonePrivate *TimeZonePrivate::clone() const
{
   return new TimeZonePrivate(*this);
}

bool TimeZonePrivate::operator==(const TimeZonePrivate &other) const
{
   // TODO Too simple, but need to solve problem of comparing different derived classes
   // Should work for all System and ICU classes as names guaranteed unique, but not for Simple.
   // Perhaps once all classes have working transitions can compare full list?
   return (m_id == other.m_id);
}

bool TimeZonePrivate::operator!=(const TimeZonePrivate &other) const
{
   return !(*this == other);
}

bool TimeZonePrivate::isValid() const
{
   return !m_id.isEmpty();
}

ByteArray TimeZonePrivate::getId() const
{
   return m_id;
}

Locale::Country TimeZonePrivate::getCountry() const
{
   // Default fall-back mode, use the zoneTable to find Region of known Zones
   for (int i = 0; i < sg_zoneDataTableSize; ++i) {
      const ZoneData *data = zone_data(i);
      std::list<ByteArray> parts = iana_id(data).split(' ');
      if (std::find(parts.begin(), parts.end(), m_id) != parts.end()) {
         return (Locale::Country)data->m_country;
      }
      
   }
   return Locale::Country::AnyCountry;
}

String TimeZonePrivate::getComment() const
{
   return String();
}

String TimeZonePrivate::displayName(pdk::pint64 atMSecsSinceEpoch,
                                    TimeZone::NameType nameType,
                                    const Locale &locale) const
{
   if (nameType == TimeZone::NameType::OffsetName) {
      return isoOffsetFormat(offsetFromUtc(atMSecsSinceEpoch));
   }
   if (isDaylightTime(atMSecsSinceEpoch)) {
      return displayName(TimeZone::TimeType::DaylightTime, nameType, locale);
   } else {
      return displayName(TimeZone::TimeType::StandardTime, nameType, locale);
   }
}

String TimeZonePrivate::displayName(TimeZone::TimeType timeType,
                                    TimeZone::NameType nameType,
                                    const Locale &locale) const
{
   PDK_UNUSED(timeType);
   PDK_UNUSED(nameType);
   PDK_UNUSED(locale);
   return String();
}

String TimeZonePrivate::abbreviation(pdk::pint64 atMSecsSinceEpoch) const
{
   PDK_UNUSED(atMSecsSinceEpoch);
   return String();
}

int TimeZonePrivate::offsetFromUtc(pdk::pint64 atMSecsSinceEpoch) const
{
   return standardTimeOffset(atMSecsSinceEpoch) + daylightTimeOffset(atMSecsSinceEpoch);
}

int TimeZonePrivate::standardTimeOffset(pdk::pint64 atMSecsSinceEpoch) const
{
   PDK_UNUSED(atMSecsSinceEpoch);
   return getInvalidSeconds();
}

int TimeZonePrivate::daylightTimeOffset(pdk::pint64 atMSecsSinceEpoch) const
{
   PDK_UNUSED(atMSecsSinceEpoch);
   return getInvalidSeconds();
}

bool TimeZonePrivate::hasDaylightTime() const
{
   return false;
}

bool TimeZonePrivate::isDaylightTime(pdk::pint64 atMSecsSinceEpoch) const
{
   PDK_UNUSED(atMSecsSinceEpoch);
   return false;
}

TimeZonePrivate::Data TimeZonePrivate::data(pdk::pint64 forMSecsSinceEpoch) const
{
   PDK_UNUSED(forMSecsSinceEpoch);
   return getInvalidData();
}

// Private only method for use by DateTime to convert local msecs to epoch msecs
TimeZonePrivate::Data TimeZonePrivate::dataForLocalTime(pdk::pint64 forLocalMSecs, int hint) const
{
   if (!hasDaylightTime()) // No DST means same offset for all local msecs
      return data(forLocalMSecs - standardTimeOffset(forLocalMSecs) * 1000);
   
   /*
      We need a UTC time at which to ask for the offset, in order to be able to
      add that offset to forLocalMSecs, to get the UTC time we
      need. Fortunately, no time-zone offset is more than 14 hours; and DST
      transitions happen (much) more than thirty-two hours apart.  So sampling
      offset sixteen hours each side gives us information we can be sure
      brackets the correct time and at most one DST transition.
    */
   const pdk::pint64 sixteenHoursInMSecs(16 * 3600 * 1000);
   PDK_STATIC_ASSERT(-sixteenHoursInMSecs / 1000 < TimeZone::MinUtcOffsetSecs
                     && sixteenHoursInMSecs / 1000 > TimeZone::MaxUtcOffsetSecs);
   /*
      Offsets are Local - UTC, positive to the east of Greenwich, negative to
      the west; DST offset always exceeds standard offset, when DST applies.
      When we have offsets on either side of a transition, the lower one is
      standard, the higher is DST.
      
      Non-DST transitions (jurisdictions changing time-zone and time-zones
      changing their standard offset, typically) are described below as if they
      were DST transitions (since these are more usual and familiar); the code
      mostly concerns itself with offsets from UTC, described in terms of the
      common case for changes in that.  If there is no actual change in offset
      (e.g. a DST transition cancelled by a standard offset change), this code
      should handle it gracefully; without transitions, it'll see early == late
      and take the easy path; with transitions, tran and nextTran get the
      correct UTC time as atMSecsSinceEpoch so comparing to nextStart selects
      the right one.  In all other cases, the transition changes offset and the
      reasoning that applies to DST applies just the same.  Aside from hinting,
      the only thing that looks at DST-ness at all, other than inferred from
      offset changes, is the case without transition data handling an invalid
      time in the gap that a transition passed over.
      
      The handling of hint (see below) is apt to go wrong in non-DST
      transitions.  There isn't really a great deal we can hope to do about that
      without adding yet more unreliable complexity to the heuristics in use for
      already obscure corner-cases.
     */
   
   /*
      The hint (really a DateTimePrivate::DaylightStatus) is > 0 if caller
      thinks we're in DST, 0 if in standard.  A value of -2 means never-DST, so
      should have been handled above; if it slips through, it's wrong but we
      should probably treat it as standard anyway (never-DST means
      always-standard, after all).  If the hint turns out to be wrong, fall back
      on trying the other possibility: which makes it harmless to treat -1
      (meaning unknown) as standard (i.e. try standard first, then try DST).  In
      practice, away from a transition, the only difference hint makes is to
      which candidate we try first: if the hint is wrong (or unknown and
      standard fails), we'll try the other candidate and it'll work.
      
      For the obscure (and invalid) case where forLocalMSecs falls in a
      spring-forward's missing hour, a common case is that we started with a
      date/time for which the hint was valid and adjusted it naively; for that
      case, we should correct the adjustment by shunting across the transition
      into where hint is wrong.  So half-way through the gap, arrived at from
      the DST side, should be read as an hour earlier, in standard time; but, if
      arrived at from the standard side, should be read as an hour later, in
      DST.  (This shall be wrong in some cases; for example, when a country
      changes its transition dates and changing a date/time by more than six
      months lands it on a transition.  However, these cases are even more
      obscure than those where the heuristic is good.)
     */
   
   if (hasTransitions()) {
      /*
          We have transitions.
          
          Each transition gives the offsets to use until the next; so we need the
          most recent transition before the time forLocalMSecs describes.  If it
          describes a time *in* a transition, we'll need both that transition and
          the one before it.  So find one transition that's probably after (and not
          much before, otherwise) and another that's definitely before, then work
          out which one to use.  When both or neither work on forLocalMSecs, use
          hint to disambiguate.
        */
      
      // Get a transition definitely before the local MSecs; usually all we need.
      // Only around the transition times might we need another.
      Data tran = previousTransition(forLocalMSecs - sixteenHoursInMSecs);
      PDK_ASSERT(forLocalMSecs < 0 || // Pre-epoch TZ info may be unavailable
                 forLocalMSecs >= tran.m_atMSecsSinceEpoch + tran.m_offsetFromUtc * 1000);
      Data nextTran = nextTransition(tran.m_atMSecsSinceEpoch);
      /*
          Now walk those forward until they bracket forLocalMSecs with transitions.
          
          One of the transitions should then be telling us the right offset to use.
          In a transition, we need the transition before it (to describe the run-up
          to the transition) and the transition itself; so we need to stop when
          nextTran is that transition.
        */
      while (nextTran.m_atMSecsSinceEpoch != getInvalidMSecs()
             && forLocalMSecs > nextTran.m_atMSecsSinceEpoch + nextTran.m_offsetFromUtc * 1000) {
         Data newTran = nextTransition(nextTran.m_atMSecsSinceEpoch);
         if (newTran.m_atMSecsSinceEpoch == getInvalidMSecs()
             || newTran.m_atMSecsSinceEpoch + newTran.m_offsetFromUtc * 1000
             > forLocalMSecs + sixteenHoursInMSecs) {
            // Definitely not a relevant tansition: too far in the future.
            break;
         }
         tran = nextTran;
         nextTran = newTran;
      }
      
      // Check we do *really* have transitions for this zone:
      if (tran.m_atMSecsSinceEpoch != getInvalidMSecs()) {
         
         /*
              So now tran is definitely before and nextTran is either after or only
              slightly before.  The one with the larger offset is in DST; the other in
              standard time.  Our hint tells us which of those to use (defaulting to
              standard if no hint): try it first; if that fails, try the other; if both
              fail life's tricky.
            */
         PDK_ASSERT(forLocalMSecs < 0
                    || forLocalMSecs > tran.m_atMSecsSinceEpoch + tran.m_offsetFromUtc * 1000);
         const pdk::pint64 nextStart = nextTran.m_atMSecsSinceEpoch;
         // Work out the UTC values it might make sense to return:
         nextTran.m_atMSecsSinceEpoch = forLocalMSecs - nextTran.m_offsetFromUtc * 1000;
         tran.m_atMSecsSinceEpoch = forLocalMSecs - tran.m_offsetFromUtc * 1000;
         
         const bool nextIsDst = tran.m_offsetFromUtc < nextTran.m_offsetFromUtc;
         // If that agrees with hint > 0, our first guess is to use nextTran; else tran.
         const bool nextFirst = nextIsDst == (hint > 0) && nextStart != getInvalidMSecs();
         for (int i = 0; i < 2; i++) {
            /*
                  On the first pass, the case we consider is what hint told us to expect
                  (except when hint was -1 and didn't actually tell us what to expect),
                  so it's likely right.  We only get a second pass if the first failed,
                  by which time the second case, that we're trying, is likely right.  If
                  an overwhelming majority of calls have hint == -1, the PDK_LIKELY here
                  shall be wrong half the time; otherwise, its errors shall be rarer
                  than that.
                */
            if (nextFirst ? i == 0 : i) {
               PDK_ASSERT(nextStart != getInvalidMSecs());
               if (PDK_LIKELY(nextStart <= nextTran.m_atMSecsSinceEpoch))
                  return nextTran;
            } else {
               // If next is invalid, nextFirst is false, to route us here first:
               if (nextStart == getInvalidMSecs() || PDK_LIKELY(nextStart > tran.m_atMSecsSinceEpoch))
                  return tran;
            }
         }
         
         /*
              Neither is valid (e.g. in a spring-forward's gap) and
              nextTran.m_atMSecsSinceEpoch < nextStart <= tran.atMSecsSinceEpoch, so
              0 < tran.atMSecsSinceEpoch - nextTran.m_atMSecsSinceEpoch
              = (nextTran.m_offsetFromUtc - tran.offsetFromUtc) * 1000
            */
         int dstStep = nextTran.m_offsetFromUtc - tran.m_offsetFromUtc;
         PDK_ASSERT(dstStep > 0); // How else could we get here ?
         if (nextFirst) { // hint thought we needed nextTran, so use tran
            tran.m_atMSecsSinceEpoch -= dstStep;
            return tran;
         }
         nextTran.m_atMSecsSinceEpoch += dstStep;
         return nextTran;
      }
      // System has transitions but not for this zone.
      // Try falling back to offsetFromUtc
   }
   
   /* Bracket and refine to discover offset. */
   pdk::pint64 utcEpochMSecs;
   
   int early = offsetFromUtc(forLocalMSecs - sixteenHoursInMSecs);
   int late = offsetFromUtc(forLocalMSecs + sixteenHoursInMSecs);
   if (PDK_LIKELY(early == late)) { // > 99% of the time
      utcEpochMSecs = forLocalMSecs - early * 1000;
   } else {
      // Close to a DST transition: early > late is near a fall-back,
      // early < late is near a spring-forward.
      const int offsetInDst = std::max(early, late);
      const int offsetInStd = std::min(early, late);
      // Candidate values for utcEpochMSecs (if forLocalMSecs is valid):
      const pdk::pint64 forDst = forLocalMSecs - offsetInDst * 1000;
      const pdk::pint64 forStd = forLocalMSecs - offsetInStd * 1000;
      // Best guess at the answer:
      const pdk::pint64 hinted = hint > 0 ? forDst : forStd;
      if (PDK_LIKELY(offsetFromUtc(hinted) == (hint > 0 ? offsetInDst : offsetInStd))) {
         utcEpochMSecs = hinted;
      } else if (hint <= 0 && offsetFromUtc(forDst) == offsetInDst) {
         utcEpochMSecs = forDst;
      } else if (hint > 0 && offsetFromUtc(forStd) == offsetInStd) {
         utcEpochMSecs = forStd;
      } else {
         // Invalid forLocalMSecs: in spring-forward gap.
         const int dstStep = daylightTimeOffset(early < late ?
                                                   forLocalMSecs + sixteenHoursInMSecs :
                                                   forLocalMSecs - sixteenHoursInMSecs);
         PDK_ASSERT(dstStep); // There can't be a transition without it !
         utcEpochMSecs = (hint > 0) ? forStd - dstStep : forDst + dstStep;
      }
   }
   
   return data(utcEpochMSecs);
}

bool TimeZonePrivate::hasTransitions() const
{
   return false;
}

TimeZonePrivate::Data TimeZonePrivate::nextTransition(pdk::pint64 afterMSecsSinceEpoch) const
{
   PDK_UNUSED(afterMSecsSinceEpoch);
   return getInvalidData();
}

TimeZonePrivate::Data TimeZonePrivate::previousTransition(pdk::pint64 beforeMSecsSinceEpoch) const
{
   PDK_UNUSED(beforeMSecsSinceEpoch);
   return getInvalidData();
}

TimeZonePrivate::DataList TimeZonePrivate::transitions(pdk::pint64 fromMSecsSinceEpoch,
                                                       pdk::pint64 toMSecsSinceEpoch) const
{
   DataList list;
   if (toMSecsSinceEpoch >= fromMSecsSinceEpoch) {
      // fromMSecsSinceEpoch is inclusive but nextTransitionTime() is exclusive so go back 1 msec
      Data next = nextTransition(fromMSecsSinceEpoch - 1);
      while (next.m_atMSecsSinceEpoch != getInvalidMSecs()
             && next.m_atMSecsSinceEpoch <= toMSecsSinceEpoch) {
         list.push_back(next);
         next = nextTransition(next.m_atMSecsSinceEpoch);
      }
   }
   return list;
}

ByteArray TimeZonePrivate::getSystemTimeZoneId() const
{
   return ByteArray();
}

std::list<ByteArray> TimeZonePrivate::getAvailableTimeZoneIds() const
{
   return std::list<ByteArray>();
}

std::list<ByteArray> TimeZonePrivate::getAvailableTimeZoneIds(Locale::Country country) const
{
   // Default fall-back mode, use the zoneTable to find Region of know Zones
   std::list<ByteArray> regions;
   // First get all Zones in the Zones table belonging to the Region
   for (int i = 0; i < sg_zoneDataTableSize; ++i) {
      if (zone_data(i)->m_country == pdk::as_integer<Locale::Country>(country)) {
         for (const ByteArray &item : iana_id(zone_data(i)).split(' ')) {
            regions.push_back(item);
         }
      }
   }
   
   regions.sort();
   regions.erase(std::unique(regions.begin(), regions.end()), regions.end());
   // Then select just those that are available
   const std::list<ByteArray> all = getAvailableTimeZoneIds();
   std::list<ByteArray> result;
   result.resize(std::min(all.size(), regions.size()));
   std::set_intersection(all.begin(), all.end(), regions.cbegin(), regions.cend(),
                         std::back_inserter(result));
   return result;
}

std::list<ByteArray> TimeZonePrivate::getAvailableTimeZoneIds(int offsetFromUtc) const
{
   // Default fall-back mode, use the zoneTable to find Offset of know Zones
   std::list<ByteArray> offsets;
   // First get all Zones in the table using the Offset
   for (int i = 0; i < sg_windowsDataTableSize; ++i) {
      const WindowsData *winData = windows_data(i);
      if (winData->m_offsetFromUtc == offsetFromUtc) {
         for (int j = 0; j < sg_zoneDataTableSize; ++j) {
            const ZoneData *data = zone_data(j);
            if (data->m_windowsIdKey == winData->m_windowsIdKey) {
               for (const ByteArray &item : iana_id(data).split(' ')) {
                  offsets.push_back(item);
               }
            }
         }
      }
   }
   offsets.sort();
   offsets.erase(std::unique(offsets.begin(), offsets.end()), offsets.end());
   
   // Then select just those that are available
   const std::list<ByteArray> all = getAvailableTimeZoneIds();
   std::list<ByteArray> result;
   result.resize(std::min(all.size(), offsets.size()));
   std::set_intersection(all.begin(), all.end(), offsets.cbegin(), offsets.cend(),
                         std::back_inserter(result));
   return result;
}

#ifndef PDK_NO_DATASTREAM
void TimeZonePrivate::serialize(DataStream &ds) const
{
   ds << String::fromUtf8(m_id);
}
#endif // PDK_NO_DATASTREAM

// Static Utility Methods

TimeZonePrivate::Data TimeZonePrivate::getInvalidData()
{
   Data data;
   data.m_atMSecsSinceEpoch = getInvalidMSecs();
   data.m_offsetFromUtc = getInvalidSeconds();
   data.m_standardTimeOffset = getInvalidSeconds();
   data.m_daylightTimeOffset = getInvalidSeconds();
   return data;
}

TimeZone::OffsetData TimeZonePrivate::getInvalidOffsetData()
{
   TimeZone::OffsetData offsetData;
   offsetData.m_atUtc = DateTime();
   offsetData.m_offsetFromUtc = getInvalidSeconds();
   offsetData.m_standardTimeOffset = getInvalidSeconds();
   offsetData.m_daylightTimeOffset = getInvalidSeconds();
   return offsetData;
}

TimeZone::OffsetData TimeZonePrivate::toOffsetData(const TimeZonePrivate::Data &data)
{
   TimeZone::OffsetData offsetData = getInvalidOffsetData();
   if (data.m_atMSecsSinceEpoch != getInvalidMSecs()) {
      offsetData.m_atUtc = DateTime::fromMSecsSinceEpoch(data.m_atMSecsSinceEpoch, pdk::TimeSpec::UTC);
      offsetData.m_offsetFromUtc = data.m_offsetFromUtc;
      offsetData.m_standardTimeOffset = data.m_standardTimeOffset;
      offsetData.m_daylightTimeOffset = data.m_daylightTimeOffset;
      offsetData.m_abbreviation = data.m_abbreviation;
   }
   return offsetData;
}

// Is the format of the ID valid ?
bool TimeZonePrivate::isValidId(const ByteArray &ianaId)
{
   /*
      Main rules for defining TZ/IANA names as per ftp://ftp.iana.org/tz/code/Theory
       1. Use only valid POSIX file name components
       2. Within a file name component, use only ASCII letters, `.', `-' and `_'.
       3. Do not use digits (except in a [+-]\d+ suffix, when used).
       4. A file name component must not exceed 14 characters or start with `-'
      However, the rules are really guidelines - a later one says
       - Do not change established names if they only marginally violate the
         above rules.
      We may, therefore, need to be a bit slack in our check here, if we hit
      legitimate exceptions in real time-zone databases.
      
      In particular, aliases such as "Etc/GMT+7" and "SystemV/EST5EDT" are valid
      so we need to accept digits, ':', and '+'; aliases typically have the form
      of POSIX TZ strings, which allow a suffix to a proper IANA name.  A POSIX
      suffix starts with an offset (as in GMT+7) and may continue with another
      name (as in EST5EDT, giving the DST name of the zone); a further offset is
      allowed (for DST).  The ("hard to describe and [...] error-prone in
      practice") POSIX form even allows a suffix giving the dates (and
      optionally times) of the annual DST transitions.  Hopefully, no TZ aliases
      go that far, but we at least need to accept an offset and (single
      fragment) DST-name.
      
      But for the legacy complications, the following would be preferable if
      QRegExp would work on ByteArrays directly:
          const QRegExp rx(StringLiteral("[a-z+._][a-z+._-]{,13}"
                                      "(?:/[a-z+._][a-z+._-]{,13})*"
                                          // Optional suffix:
                                          "(?:[+-]?\d{1,2}(?::\d{1,2}){,2}" // offset
                                             // one name fragment (DST):
                                             "(?:[a-z+._][a-z+._-]{,13})?)"),
                           Qt::CaseInsensitive);
          return rx.exactMatch(ianaId);
    */
   
   // Somewhat slack hand-rolled version:
   const int MinSectionLength = 1;
   const int MaxSectionLength = 14;
   int sectionLength = 0;
   for (const char *it = ianaId.begin(), * const end = ianaId.end(); it != end; ++it, ++sectionLength) {
      const char ch = *it;
      if (ch == '/') {
         if (sectionLength < MinSectionLength || sectionLength > MaxSectionLength)
            return false; // violates (4)
         sectionLength = -1;
      } else if (ch == '-') {
         if (sectionLength == 0)
            return false; // violates (4)
      } else if (!(ch >= 'a' && ch <= 'z')
                 && !(ch >= 'A' && ch <= 'Z')
                 && !(ch == '_')
                 && !(ch == '.')
                 // Should ideally check these only happen as an offset:
                 && !(ch >= '0' && ch <= '9')
                 && !(ch == '+')
                 && !(ch == ':')) {
         return false; // violates (2)
      }
   }
   if (sectionLength < MinSectionLength || sectionLength > MaxSectionLength)
      return false; // violates (4)
   return true;
}

String TimeZonePrivate::isoOffsetFormat(int offsetFromUtc)
{
   const int mins = offsetFromUtc / 60;
   return String::fromUtf8("UTC%1%2:%3").arg(mins >= 0 ? Latin1Character('+') : Latin1Character('-'))
         .arg(std::abs(mins) / 60, 2, 10, Latin1Character('0'))
         .arg(std::abs(mins) % 60, 2, 10, Latin1Character('0'));
}

ByteArray TimeZonePrivate::ianaIdToWindowsId(const ByteArray &id)
{
   for (int i = 0; i < sg_zoneDataTableSize; ++i) {
      const ZoneData *data = zone_data(i);
      std::list<ByteArray> parts = iana_id(data).split(' ');
      if (std::find(parts.begin(), parts.end(), id) != parts.end()) {
         return to_windows_id_literal(data->m_windowsIdKey);
      }
   }
   return ByteArray();
}

ByteArray TimeZonePrivate::windowsIdToDefaultIanaId(const ByteArray &windowsId)
{
   const pdk::puint16 windowsIdKey = to_windows_id_key(windowsId);
   for (int i = 0; i < sg_windowsDataTableSize; ++i) {
      const WindowsData *data = windows_data(i);
      if (data->m_windowsIdKey == windowsIdKey) {
         return iana_id(data);
      }
   }
   return ByteArray();
}

ByteArray TimeZonePrivate::windowsIdToDefaultIanaId(const ByteArray &windowsId,
                                                    Locale::Country country)
{
   const std::list<ByteArray> list = windowsIdToIanaIds(windowsId, country);
   if (list.size() > 0) {
      return list.front();
   } else {
      return ByteArray();
   }
}

std::list<ByteArray> TimeZonePrivate::windowsIdToIanaIds(const ByteArray &windowsId)
{
   const pdk::puint16 windowsIdKey = to_windows_id_key(windowsId);
   std::list<ByteArray> list;
   for (int i = 0; i < sg_zoneDataTableSize; ++i) {
      const ZoneData *data = zone_data(i);
      if (data->m_windowsIdKey == windowsIdKey) {
         for (const ByteArray &item : iana_id(data).split(' ')) {
            list.push_back(item);
         }
      }
   }
   // Return the full list in alpha order
   list.sort();
   return list;
}

std::list<ByteArray> TimeZonePrivate::windowsIdToIanaIds(const ByteArray &windowsId,
                                                         Locale::Country country)
{
   const pdk::puint16 windowsIdKey = to_windows_id_key(windowsId);
   for (int i = 0; i < sg_zoneDataTableSize; ++i) {
      const ZoneData *data = zone_data(i);
      // Return the region matches in preference order
      if (data->m_windowsIdKey == windowsIdKey && data->m_country == (pdk::puint16) country){
         return iana_id(data).split(' ');
      }
   }
   return std::list<ByteArray>();
}

UtcTimeZonePrivate::UtcTimeZonePrivate()
{
   const String name = getUtcString();
   init(getUtcByteArray(), 0, name, name, Locale::Country::AnyCountry, name);
}

// Create a named UTC time zone
UtcTimeZonePrivate::UtcTimeZonePrivate(const ByteArray &id)
{
   // Look for the name in the UTC list, if found set the values
   for (int i = 0; i < sg_utcDataTableSize; ++i) {
      const UtcData *data = utc_data(i);
      const ByteArray uid = utc_id(data);
      if (uid == id) {
         String name = String::fromUtf8(id);
         init(id, data->m_offsetFromUtc, name, name, Locale::Country::AnyCountry, name);
         break;
      }
   }
}

// Create offset from UTC
UtcTimeZonePrivate::UtcTimeZonePrivate(pdk::pint32 offsetSeconds)
{
   String utcId;
   
   if (offsetSeconds == 0) {
      utcId = getUtcString();
   }else {
      utcId = isoOffsetFormat(offsetSeconds);
   }
   init(utcId.toUtf8(), offsetSeconds, utcId, utcId, Locale::Country::AnyCountry, utcId);
}

UtcTimeZonePrivate::UtcTimeZonePrivate(const ByteArray &zoneId, int offsetSeconds,
                                       const String &name, const String &abbreviation,
                                       Locale::Country country, const String &comment)
{
   init(zoneId, offsetSeconds, name, abbreviation, country, comment);
}

UtcTimeZonePrivate::UtcTimeZonePrivate(const UtcTimeZonePrivate &other)
   : TimeZonePrivate(other), 
     m_name(other.m_name),
     m_abbreviation(other.m_abbreviation),
     m_comment(other.m_comment),
     m_country(other.m_country),
     m_offsetFromUtc(other.m_offsetFromUtc)
{
}

UtcTimeZonePrivate::~UtcTimeZonePrivate()
{
}

UtcTimeZonePrivate *UtcTimeZonePrivate::clone() const
{
   return new UtcTimeZonePrivate(*this);
}

TimeZonePrivate::Data UtcTimeZonePrivate::data(pdk::pint64 forMSecsSinceEpoch) const
{
   Data d;
   d.m_abbreviation = m_abbreviation;
   d.m_atMSecsSinceEpoch = forMSecsSinceEpoch;
   d.m_standardTimeOffset = d.m_offsetFromUtc = m_offsetFromUtc;
   d.m_daylightTimeOffset = 0;
   return d;
}

void UtcTimeZonePrivate::init(const ByteArray &zoneId)
{
   m_id = zoneId;
}

void UtcTimeZonePrivate::init(const ByteArray &zoneId, int offsetSeconds, const String &name,
                              const String &abbreviation, Locale::Country country,
                              const String &comment)
{
   m_id = zoneId;
   m_offsetFromUtc = offsetSeconds;
   m_name = name;
   m_abbreviation = abbreviation;
   m_country = country;
   m_comment = comment;
}

Locale::Country UtcTimeZonePrivate::getCountry() const
{
   return m_country;
}

String UtcTimeZonePrivate::getComment() const
{
   return m_comment;
}

String UtcTimeZonePrivate::displayName(TimeZone::TimeType timeType,
                                       TimeZone::NameType nameType,
                                       const Locale &locale) const
{
   PDK_UNUSED(timeType);
   PDK_UNUSED(locale);
   if (nameType == TimeZone::NameType::ShortName) {
      return m_abbreviation;
   } else if (nameType == TimeZone::NameType::OffsetName) {
      return isoOffsetFormat(m_offsetFromUtc);
   }
   return m_name;
}

String UtcTimeZonePrivate::abbreviation(pdk::pint64 atMSecsSinceEpoch) const
{
   PDK_UNUSED(atMSecsSinceEpoch);
   return m_abbreviation;
}

pdk::pint32 UtcTimeZonePrivate::standardTimeOffset(pdk::pint64 atMSecsSinceEpoch) const
{
   PDK_UNUSED(atMSecsSinceEpoch);
   return m_offsetFromUtc;
}

pdk::pint32 UtcTimeZonePrivate::daylightTimeOffset(pdk::pint64 atMSecsSinceEpoch) const
{
   PDK_UNUSED(atMSecsSinceEpoch);
   return 0;
}

ByteArray UtcTimeZonePrivate::getSystemTimeZoneId() const
{
   return getUtcByteArray();
}

std::list<ByteArray> UtcTimeZonePrivate::getAvailableTimeZoneIds() const
{
   std::list<ByteArray> result;
   result.resize(sg_utcDataTableSize);
   for (int i = 0; i < sg_utcDataTableSize; ++i) {
      result.push_back(utc_id(utc_data(i)));
   }
   result.sort(); // ### or already sorted??
   // ### assuming no duplicates
   return result;
}

std::list<ByteArray> UtcTimeZonePrivate::getAvailableTimeZoneIds(Locale::Country country) const
{
   // If AnyCountry then is request for all non-region offset codes
   if (country == Locale::Country::AnyCountry) {
      return getAvailableTimeZoneIds();
   }
   return std::list<ByteArray>();
}

std::list<ByteArray> UtcTimeZonePrivate::getAvailableTimeZoneIds(pdk::pint32 offsetSeconds) const
{
   std::list<ByteArray> result;
   for (int i = 0; i < sg_utcDataTableSize; ++i) {
      const UtcData *data = utc_data(i);
      if (data->m_offsetFromUtc == offsetSeconds) {
         result.push_back(utc_id(data));
      }
   }
   result.sort(); // ### or already sorted??
   // ### assuming no duplicates
   return result;
}

#ifndef PDK_NO_DATASTREAM
void UtcTimeZonePrivate::serialize(DataStream &ds) const
{
   ds << StringLiteral("OffsetFromUtc") << String::fromUtf8(m_id) << m_offsetFromUtc << m_name
      << m_abbreviation << (pdk::pint32) m_country << m_comment;
}
#endif // PDK_NO_DATASTREAM

} // internal
} // time

namespace utils {
template<> TimeZonePrivate *SharedDataPointer<TimeZonePrivate>::clone()
{
   return m_implPtr->clone();
}
} // utils

} // pdk
