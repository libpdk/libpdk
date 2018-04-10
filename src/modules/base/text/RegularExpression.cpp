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
// Created by softboy on 2018/04/09.

#include "pdk/global/Global.h"
#include "pdk/base/text/RegularExpression.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/HashFuncs.h"
#include "pdk/base/os/thread/ReadWriteLock.h"
#include "pdk/base/os/thread/ThreadStorage.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/io/DataStream.h"
#include "pdk/global/Logging.h"
#include "pdk/global/GlobalStatic.h"

#include <vector>

#define PCRE2_CODE_UNIT_WIDTH 16
#include <pcre2.h>

namespace pdk {
namespace text {

using pdk::utils::SharedData;
using pdk::os::thread::ReadWriteLock;
using pdk::os::thread::WriteLocker;
using pdk::os::thread::ReadLocker;
using pdk::os::thread::ThreadStorage;
using pdk::ds::ByteArray;
using pdk::lang::Latin1Character;
using pdk::lang::Character;
using pdk::kernel::CoreApplication;
using pdk::io::DebugStateSaver;

#ifdef PDK_BUILD_INTERNAL
PDK_UNITTEST_EXPORT unsigned int sg_regex_optimize_after_use_count = 10;
#else
static const unsigned int sg_regex_optimize_after_use_count = 10;
#endif // PDK_BUILD_INTERNAL

namespace {

int convert_to_pcre_options(RegularExpression::PatternOptions patternOptions)
{
   int options = 0;
   if (patternOptions & RegularExpression::PatternOption::CaseInsensitiveOption) {
      options |= PCRE2_CASELESS;
   }
   if (patternOptions & RegularExpression::PatternOption::DotMatchesEverythingOption) {
      options |= PCRE2_DOTALL;
   }
   if (patternOptions & RegularExpression::PatternOption::MultilineOption) {
      options |= PCRE2_MULTILINE;
   }
   if (patternOptions & RegularExpression::PatternOption::ExtendedPatternSyntaxOption) {
      options |= PCRE2_EXTENDED;
   }
   if (patternOptions & RegularExpression::PatternOption::InvertedGreedinessOption) {
      options |= PCRE2_UNGREEDY;
   }
   if (patternOptions & RegularExpression::PatternOption::DontCaptureOption) {
      options |= PCRE2_NO_AUTO_CAPTURE;
   }
   if (patternOptions & RegularExpression::PatternOption::UseUnicodePropertiesOption) {
      options |= PCRE2_UCP;
   }
   return options;
}

int convert_to_pcre_options(RegularExpression::MatchOptions matchOptions)
{
   int options = 0;
   if (matchOptions & RegularExpression::MatchOption::AnchoredMatchOption) {
      options |= PCRE2_ANCHORED;
   }
   if (matchOptions & RegularExpression::MatchOption::DontCheckSubjectStringMatchOption) {
      options |= PCRE2_NO_UTF_CHECK;
   }
   return options;
}

} // anonymous namespacd

namespace internal {

struct RegularExpressionPrivate : public SharedData
{
   RegularExpressionPrivate();
   ~RegularExpressionPrivate();
   RegularExpressionPrivate(const RegularExpressionPrivate &other);
   
   void cleanCompiledPattern();
   void compilePattern();
   void getPatternInfo();
   
   enum class OptimizePatternOption
   {
      LazyOptimizeOption,
      ImmediateOptimizeOption
   };
   
   void optimizePattern(OptimizePatternOption option);
   
   enum class CheckSubjectStringOption
   {
      CheckSubjectString,
      DontCheckSubjectString
   };
   
   RegularExpressionMatchPrivate *doMatch(const String &subject,
                                          int subjectStartPos,
                                          int subjectLength,
                                          int offset,
                                          RegularExpression::MatchType matchType,
                                          RegularExpression::MatchOptions matchOptions,
                                          CheckSubjectStringOption checkSubjectStringOption = CheckSubjectStringOption::CheckSubjectString,
                                          const RegularExpressionMatchPrivate *previous = nullptr) const;
   
   int captureIndexForName(StringView name) const;
   
   // sizeof(SharedData) == 4, so start our members with an enum
   RegularExpression::PatternOptions m_patternOptions;
   String m_pattern;
   
   // *All* of the following members are managed while holding this mutex,
   // except for isDirty which is set to true by RegularExpression setters
   // (right after a detach happened).
   mutable ReadWriteLock m_mutex;
   
   // The PCRE code pointer is reference-counted by the RegularExpressionPrivate
   // objects themselves; when the private is copied (i.e. a detach happened)
   // they are set to nullptr
   pcre2_code_16 *m_compiledPattern;
   int m_errorCode;
   int m_errorOffset;
   int m_capturingCount;
   unsigned int m_usedCount;
   bool m_usingCrLfNewlines;
   bool m_isDirty;
};

struct RegularExpressionMatchPrivate : SharedData
{
   RegularExpressionMatchPrivate(const RegularExpression &regex,
                                 const String &subject,
                                 int subjectStart,
                                 int subjectLength,
                                 RegularExpression::MatchType matchType,
                                 RegularExpression::MatchOptions matchOptions);
   
   RegularExpressionMatch nextMatch() const;
   
   const RegularExpression m_regularExpression;
   const String m_subject;
   // the capturedOffsets vector contains pairs of (start, end) positions
   // for each captured substring
   std::vector<int> m_capturedOffsets;
   
   const int m_subjectStart;
   const int m_subjectLength;
   
   const RegularExpression::MatchType m_matchType;
   const RegularExpression::MatchOptions m_matchOptions;
   
   int m_capturedCount;
   
   bool m_hasMatch;
   bool m_hasPartialMatch;
   bool m_isValid;
};

struct RegularExpressionMatchIteratorPrivate : SharedData
{
   RegularExpressionMatchIteratorPrivate(const RegularExpression &regex,
                                         RegularExpression::MatchType matchType,
                                         RegularExpression::MatchOptions matchOptions,
                                         const RegularExpressionMatch &next);
   
   bool hasNext() const;
   RegularExpressionMatch m_next;
   const RegularExpression m_regularExpression;
   const RegularExpression::MatchType m_matchType;
   const RegularExpression::MatchOptions m_matchOptions;
};

RegularExpressionPrivate::RegularExpressionPrivate()
   : m_patternOptions(0),
     m_pattern(),
     m_mutex(),
     m_compiledPattern(nullptr),
     m_errorCode(0),
     m_errorOffset(-1),
     m_capturingCount(0),
     m_usedCount(0),
     m_usingCrLfNewlines(false),
     m_isDirty(true)
{}

RegularExpressionPrivate::~RegularExpressionPrivate()
{
   cleanCompiledPattern();
}

RegularExpressionPrivate::RegularExpressionPrivate(const RegularExpressionPrivate &other)
   : SharedData(other),
     m_patternOptions(other.m_patternOptions),
     m_pattern(other.m_pattern),
     m_mutex(),
     m_compiledPattern(nullptr),
     m_errorCode(0),
     m_errorOffset(-1),
     m_capturingCount(0),
     m_usedCount(0),
     m_usingCrLfNewlines(false),
     m_isDirty(true)
{}

void RegularExpressionPrivate::cleanCompiledPattern()
{
   pcre2_code_free_16(m_compiledPattern);
   m_compiledPattern = nullptr;
   m_errorCode = 0;
   m_errorOffset = -1;
   m_capturingCount = 0;
   m_usedCount = 0;
   m_usingCrLfNewlines = false;
}

void RegularExpressionPrivate::compilePattern()
{
   const WriteLocker lock(&m_mutex);
   if (!m_isDirty) {
      return;
   }
   m_isDirty = false;
   cleanCompiledPattern();
   int options = convert_to_pcre_options(m_patternOptions);
   options |= PCRE2_UTF;
   PCRE2_SIZE patternErrorOffset;
   m_compiledPattern = pcre2_compile_16(reinterpret_cast<const ushort *>(m_pattern.utf16()),
                                        m_pattern.length(),
                                        options,
                                        &m_errorCode,
                                        &patternErrorOffset,
                                        NULL);
   
   if (!m_compiledPattern) {
      m_errorOffset = static_cast<int>(patternErrorOffset);
      return;
   } else {
      // ignore whatever PCRE2 wrote into errorCode -- leave it to 0 to mean "no error"
      m_errorCode = 0;
   }
   getPatternInfo();
}

void RegularExpressionPrivate::getPatternInfo()
{
   PDK_ASSERT(m_compiledPattern);
   pcre2_pattern_info_16(m_compiledPattern, PCRE2_INFO_CAPTURECOUNT, &m_capturingCount);
   // detect the settings for the newline
   unsigned int patternNewlineSetting;
   if (pcre2_pattern_info_16(m_compiledPattern, PCRE2_INFO_NEWLINE, &patternNewlineSetting) != 0) {
      // no option was specified in the regexp, grab PCRE build defaults
      pcre2_config_16(PCRE2_CONFIG_NEWLINE, &patternNewlineSetting);
   }
   
   m_usingCrLfNewlines = (patternNewlineSetting == PCRE2_NEWLINE_CRLF) ||
         (patternNewlineSetting == PCRE2_NEWLINE_ANY) ||
         (patternNewlineSetting == PCRE2_NEWLINE_ANYCRLF);
   
   unsigned int hasJOptionChanged;
   pcre2_pattern_info_16(m_compiledPattern, PCRE2_INFO_JCHANGED, &hasJOptionChanged);
   if (PDK_UNLIKELY(hasJOptionChanged)) {
      warning_stream("RegularExpressionPrivate::getPatternInfo(): the pattern '%s'\n    is using the (?J) option; duplicate capturing group names are not supported by libpdk",
                     pdk_printable(m_pattern));
   }
}

/*
    Simple "smartpointer" wrapper around a pcre2_jit_stack_16, to be used with
    ThreadStorage.
*/
class PcreJitStackPointer
{
   PDK_DISABLE_COPY(PcreJitStackPointer);
   
public:
   
   PcreJitStackPointer()
   {
      // The default JIT stack size in PCRE is 32K,
      // we allocate from 32K up to 512K.
      m_stack = pcre2_jit_stack_create_16(32 * 1024, 512 * 1024, NULL);
   }
   
   ~PcreJitStackPointer()
   {
      if (m_stack) {
         pcre2_jit_stack_free_16(m_stack);
      }
   }
   
   pcre2_jit_stack_16 *m_stack = nullptr;
};

PDK_GLOBAL_STATIC(ThreadStorage<PcreJitStackPointer *>, sg_jitStacks);

namespace {

pcre2_jit_stack_16 *pdk_pcre_callback(void *)
{
   if (sg_jitStacks()->hasLocalData()) {
      return sg_jitStacks()->getLocalData()->m_stack;
   }
   return nullptr;
}

bool is_jit_enabled()
{
   ByteArray jitEnvironment = pdk_getenv("PDK_ENABLE_REGEXP_JIT");
   if (!jitEnvironment.isEmpty()) {
      bool ok;
      int enableJit = jitEnvironment.toInt(&ok);
      return ok ? (enableJit != 0) : true;
   }
   
#ifdef PDK_DEBUG
   return false;
#else
   return true;
#endif
}

/*!
  
    This is a simple wrapper for pcre2_match_16 for handling the case in which the
    JIT runs out of memory. In that case, we allocate a thread-local JIT stack
    and re-run pcre2_match_16.
*/
static int safe_pcre2_match_16(const pcre2_code_16 *code,
                               const unsigned short *subject, int length,
                               int startOffset, int options,
                               pcre2_match_data_16 *matchData,
                               pcre2_match_context_16 *matchContext)
{
   int result = pcre2_match_16(code, subject, length,
                               startOffset, options, matchData, matchContext);
   if (result == PCRE2_ERROR_JIT_STACKLIMIT && !sg_jitStacks()->hasLocalData()) {
      PcreJitStackPointer *ptr = new PcreJitStackPointer;
      sg_jitStacks()->setLocalData(ptr);
      
      result = pcre2_match_16(code, subject, length,
                              startOffset, options, matchData, matchContext);
   }
   return result;
}

} // anonymous namespace

void RegularExpressionPrivate::optimizePattern(OptimizePatternOption option)
{
   PDK_ASSERT(m_compiledPattern);
   static const bool enableJit = is_jit_enabled();
   if (!enableJit) {
      return;
   }
   const WriteLocker lock(&m_mutex);
   if ((option == OptimizePatternOption::LazyOptimizeOption) && (++m_usedCount != sg_regex_optimize_after_use_count)) {
      return;
   }
   pcre2_jit_compile_16(m_compiledPattern, PCRE2_JIT_COMPLETE | PCRE2_JIT_PARTIAL_SOFT | PCRE2_JIT_PARTIAL_HARD);
}

int RegularExpressionPrivate::captureIndexForName(StringView name) const
{
   PDK_ASSERT(!name.isEmpty());
   if (!m_compiledPattern) {
      return -1;
   }
   int index = pcre2_substring_number_from_name_16(m_compiledPattern, reinterpret_cast<PCRE2_SPTR16>(name.utf16()));
   if (index >= 0) {
      return index;
   }
   return -1;
}

RegularExpressionMatchPrivate *RegularExpressionPrivate::doMatch(const String &subject,
                                                                 int subjectStart,
                                                                 int subjectLength,
                                                                 int offset,
                                                                 RegularExpression::MatchType matchType,
                                                                 RegularExpression::MatchOptions matchOptions,
                                                                 CheckSubjectStringOption checkSubjectStringOption,
                                                                 const RegularExpressionMatchPrivate *previous) const
{
   if (offset < 0) {
      offset += subjectLength;
   }
   RegularExpression re(*const_cast<RegularExpressionPrivate *>(this));
   RegularExpressionMatchPrivate *priv = new RegularExpressionMatchPrivate(re, subject,
                                                                           subjectStart, subjectLength,
                                                                           matchType, matchOptions);
   if (offset < 0 || offset > subjectLength) {
      return priv;
   }
   if (PDK_UNLIKELY(!m_compiledPattern)) {
      warning_stream("RegularExpressionPrivate::doMatch(): called on an invalid RegularExpression object");
      return priv;
   }
   
   // skip optimizing and doing the actual matching if NoMatch type was requested
   if (matchType == RegularExpression::MatchType::NoMatch) {
      priv->m_isValid = true;
      return priv;
   }
   
   if (!(m_patternOptions & RegularExpression::PatternOption::DontAutomaticallyOptimizeOption)) {
      const OptimizePatternOption optimizePatternOption =
            (m_patternOptions & RegularExpression::PatternOption::OptimizeOnFirstUsageOption)
            ? OptimizePatternOption::ImmediateOptimizeOption
            : OptimizePatternOption::LazyOptimizeOption;
      
      // this is mutex protected
      const_cast<RegularExpressionPrivate *>(this)->optimizePattern(optimizePatternOption);
   }
   
   int pcreOptions = convert_to_pcre_options(matchOptions);
   
   if (matchType == RegularExpression::MatchType::PartialPreferCompleteMatch) {
      pcreOptions |= PCRE2_PARTIAL_SOFT;
   } else if (matchType == RegularExpression::MatchType::PartialPreferFirstMatch) {
      pcreOptions |= PCRE2_PARTIAL_HARD;
   }
   if (checkSubjectStringOption == CheckSubjectStringOption::DontCheckSubjectString) {
      pcreOptions |= PCRE2_NO_UTF_CHECK;
   }
   
   bool previousMatchWasEmpty = false;
   if (previous && previous->m_hasMatch &&
       (previous->m_capturedOffsets.at(0) == previous->m_capturedOffsets.at(1))) {
      previousMatchWasEmpty = true;
   }
   
   pcre2_match_context_16 *matchContext = pcre2_match_context_create_16(NULL);
   pcre2_jit_stack_assign_16(matchContext, &pdk_pcre_callback, NULL);
   pcre2_match_data_16 *matchData = pcre2_match_data_create_from_pattern_16(m_compiledPattern, NULL);
   const unsigned short * const subjectUtf16 = reinterpret_cast<const unsigned short *>(subject.utf16()) + subjectStart;
   
   int result;
   
   ReadLocker lock(&m_mutex);
   
   if (!previousMatchWasEmpty) {
      result = safe_pcre2_match_16(m_compiledPattern,
                                   subjectUtf16, subjectLength,
                                   offset, pcreOptions,
                                   matchData, matchContext);
   } else {
      result = safe_pcre2_match_16(m_compiledPattern,
                                   subjectUtf16, subjectLength,
                                   offset, pcreOptions | PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED,
                                   matchData, matchContext);
      
      if (result == PCRE2_ERROR_NOMATCH) {
         ++offset;
         if (m_usingCrLfNewlines
             && offset < subjectLength
             && subjectUtf16[offset - 1] == Latin1Character('\r')
             && subjectUtf16[offset] == Latin1Character('\n')) {
            ++offset;
         } else if (offset < subjectLength
                    && Character::isLowSurrogate(subjectUtf16[offset])) {
            ++offset;
         }
         result = safe_pcre2_match_16(m_compiledPattern,
                                      subjectUtf16, subjectLength,
                                      offset, pcreOptions,
                                      matchData, matchContext);
      }
   }
   
   lock.unlock();
   
#ifdef PDK_REGULAR_EXPRESSION_DEBUG
   debug_stream() << "Matching" <<  m_pattern << "against" << subject
                  << "starting at" << subjectStart << "len" << subjectLength
                  << "offset" << offset
                  << matchType << matchOptions << previousMatchWasEmpty
                  << "result" << result;
#endif
   
   // result == 0 means not enough space in captureOffsets; should never happen
   PDK_ASSERT(result != 0);
   
   if (result > 0) {
      // full match
      priv->m_isValid = true;
      priv->m_hasMatch = true;
      priv->m_capturedCount = result;
      priv->m_capturedOffsets.resize(result * 2);
   } else {
      // no match, partial match or error
      priv->m_hasPartialMatch = (result == PCRE2_ERROR_PARTIAL);
      priv->m_isValid = (result == PCRE2_ERROR_NOMATCH || result == PCRE2_ERROR_PARTIAL);
      
      if (result == PCRE2_ERROR_PARTIAL) {
         // partial match:
         // leave the start and end capture offsets (i.e. cap(0))
         priv->m_capturedCount = 1;
         priv->m_capturedOffsets.resize(2);
      } else {
         // no match or error
         priv->m_capturedCount = 0;
         priv->m_capturedOffsets.clear();
      }
   }
   
   // copy the captured substrings offsets, if any
   if (priv->m_capturedCount) {
      PCRE2_SIZE *ovector = pcre2_get_ovector_pointer_16(matchData);
      int * const capturedOffsets = priv->m_capturedOffsets.data();
      
      for (int i = 0; i < priv->m_capturedCount * 2; ++i)
         capturedOffsets[i] = static_cast<int>(ovector[i]);
      
      // For partial matches, PCRE2 and PCRE1 differ in behavior when lookbehinds
      // are involved. PCRE2 reports the real begin of the match and the maximum
      // used lookbehind as distinct information; PCRE1 instead automatically
      // adjusted ovector[0] to include the maximum lookbehind.
      //
      // For instance, given the pattern "\bstring\b", and the subject "a str":
      // * PCRE1 reports partial, capturing " str"
      // * PCRE2 reports partial, capturing "str" with a lookbehind of 1
      //
      // To keep behavior, emulate PCRE1 here.
      // (Eventually, we could expose the lookbehind info in a future patch.)
      if (result == PCRE2_ERROR_PARTIAL) {
         unsigned int maximumLookBehind;
         pcre2_pattern_info_16(m_compiledPattern, PCRE2_INFO_MAXLOOKBEHIND, &maximumLookBehind);
         capturedOffsets[0] -= maximumLookBehind;
      }
   }
   pcre2_match_data_free_16(matchData);
   pcre2_match_context_free_16(matchContext);
   return priv;
}

RegularExpressionMatchPrivate::RegularExpressionMatchPrivate(const RegularExpression &re,
                                                             const String &subject,
                                                             int subjectStart,
                                                             int subjectLength,
                                                             RegularExpression::MatchType matchType,
                                                             RegularExpression::MatchOptions matchOptions)
   : m_regularExpression(re),
     m_subject(subject),
     m_subjectStart(subjectStart),
     m_subjectLength(subjectLength),
     m_matchType(matchType),
     m_matchOptions(matchOptions),
     m_capturedCount(0),
     m_hasMatch(false),
     m_hasPartialMatch(false),
     m_isValid(false)
{}

RegularExpressionMatch RegularExpressionMatchPrivate::nextMatch() const
{
   PDK_ASSERT(m_isValid);
   PDK_ASSERT(m_hasMatch || m_hasPartialMatch);
   
   // Note the DontCheckSubjectString passed for the check of the subject string:
   // if we're advancing a match on the same subject,
   // then that subject was already checked at least once (when this object
   // was created, or when the object that created this one was created, etc.)
   RegularExpressionMatchPrivate *nextPrivate = m_regularExpression.m_implPtr->doMatch(m_subject,
                                                                                       m_subjectStart,
                                                                                       m_subjectLength,
                                                                                       m_capturedOffsets.at(1),
                                                                                       m_matchType,
                                                                                       m_matchOptions,
                                                                                       RegularExpressionPrivate::CheckSubjectStringOption::DontCheckSubjectString,
                                                                                       this);
   return RegularExpressionMatch(*nextPrivate);
}

RegularExpressionMatchIteratorPrivate::RegularExpressionMatchIteratorPrivate(const RegularExpression &regex,
                                                                             RegularExpression::MatchType matchType,
                                                                             RegularExpression::MatchOptions matchOptions,
                                                                             const RegularExpressionMatch &next)
   : m_next(next),
     m_regularExpression(regex),
     m_matchType(matchType),
     m_matchOptions(matchOptions)
{}

bool RegularExpressionMatchIteratorPrivate::hasNext() const
{
   return m_next.isValid() && (m_next.hasMatch() || m_next.hasPartialMatch());
}

} // internal

RegularExpression::RegularExpression(RegularExpressionPrivate &dd)
   : m_implPtr(&dd)
{}

RegularExpression::RegularExpression()
   : m_implPtr(new RegularExpressionPrivate)
{}

RegularExpression::RegularExpression(const String &pattern, PatternOptions options)
   : m_implPtr(new RegularExpressionPrivate)
{
   m_implPtr->m_pattern = pattern;
   m_implPtr->m_patternOptions = options;
}

RegularExpression::RegularExpression(const RegularExpression &regex)
   : m_implPtr(regex.m_implPtr)
{}

RegularExpression::~RegularExpression()
{}

RegularExpression &RegularExpression::operator=(const RegularExpression &regex)
{
   m_implPtr = regex.m_implPtr;
   return *this;
}

String RegularExpression::getPattern() const
{
   return m_implPtr->m_pattern;
}

void RegularExpression::setPattern(const String &pattern)
{
   m_implPtr.detach();
   m_implPtr->m_isDirty = true;
   m_implPtr->m_pattern = pattern;
}

RegularExpression::PatternOptions RegularExpression::getPatternOptions() const
{
   return m_implPtr->m_patternOptions;
}

void RegularExpression::setPatternOptions(PatternOptions options)
{
   m_implPtr.detach();
   m_implPtr->m_isDirty = true;
   m_implPtr->m_patternOptions = options;
}

int RegularExpression::getCaptureCount() const
{
   if (!isValid()) {// will compile the pattern
      return -1;
   }
   return m_implPtr->m_capturingCount;
}

StringList RegularExpression::getNamedCaptureGroups() const
{
   if (!isValid()) {// isValid() will compile the pattern
      return StringList();
   }
   
   // namedCapturingTable will point to a table of
   // namedCapturingTableEntryCount entries, each one of which
   // contains one ushort followed by the name, NUL terminated.
   // The ushort is the numerical index of the name in the pattern.
   // The length of each entry is namedCapturingTableEntrySize.
   PCRE2_SPTR16 *namedCapturingTable;
   unsigned int namedCapturingTableEntryCount;
   unsigned int namedCapturingTableEntrySize;
   
   pcre2_pattern_info_16(m_implPtr->m_compiledPattern, PCRE2_INFO_NAMETABLE, &namedCapturingTable);
   pcre2_pattern_info_16(m_implPtr->m_compiledPattern, PCRE2_INFO_NAMECOUNT, &namedCapturingTableEntryCount);
   pcre2_pattern_info_16(m_implPtr->m_compiledPattern, PCRE2_INFO_NAMEENTRYSIZE, &namedCapturingTableEntrySize);
   
   StringList result;
   
   // no List::resize nor fill is available. The +1 is for the implicit group #0
   result.resize(m_implPtr->m_capturingCount + 1);
   for (int i = 0; i < m_implPtr->m_capturingCount + 1; ++i) {
      result.push_back(String());      
   }
   for (unsigned int i = 0; i < namedCapturingTableEntryCount; ++i) {
      const ushort * const currentNamedCapturingTableRow =
            reinterpret_cast<const ushort *>(namedCapturingTable) + namedCapturingTableEntrySize * i;
      
      const int index = *currentNamedCapturingTableRow;
      result[index] = String::fromUtf16(currentNamedCapturingTableRow + 1);
   }
   return result;
}

bool RegularExpression::isValid() const
{
   m_implPtr.data()->compilePattern();
   return m_implPtr->m_compiledPattern;
}

String RegularExpression::getErrorString() const
{
   m_implPtr.data()->compilePattern();
   if (m_implPtr->m_errorCode) {
      String errorString;
      int errorStringLength;
      do {
         errorString.resize(errorString.length() + 64);
         errorStringLength = pcre2_get_error_message_16(m_implPtr->m_errorCode,
                                                        reinterpret_cast<ushort *>(errorString.getRawData()),
                                                        errorString.length());
      } while (errorStringLength < 0);
      errorString.resize(errorStringLength);
      return CoreApplication::translate("RegularExpression", std::move(errorString).toLatin1().getConstRawData());
   }
   return CoreApplication::translate("RegularExpression", "no error");
}

int RegularExpression::getPatternErrorOffset() const
{
   m_implPtr.data()->compilePattern();
   return m_implPtr->m_errorOffset;
}

RegularExpressionMatch RegularExpression::match(const String &subject,
                                                int offset,
                                                MatchType matchType,
                                                MatchOptions matchOptions) const
{
   m_implPtr.data()->compilePattern();
   RegularExpressionMatchPrivate *priv = m_implPtr->doMatch(subject, 0, subject.length(), offset, matchType, matchOptions);
   return RegularExpressionMatch(*priv);
}

RegularExpressionMatch RegularExpression::match(const StringRef &subjectRef,
                                                int offset,
                                                MatchType matchType,
                                                MatchOptions matchOptions) const
{
   m_implPtr.data()->compilePattern();
   const String subject = subjectRef.getStr() ? *subjectRef.getStr() : String();
   RegularExpressionMatchPrivate *priv = m_implPtr->doMatch(subject, subjectRef.getPosition(), subjectRef.length(), offset, matchType, matchOptions);
   return RegularExpressionMatch(*priv);
}

RegularExpressionMatchIterator RegularExpression::globalMatch(const String &subject,
                                                              int offset,
                                                              MatchType matchType,
                                                              MatchOptions matchOptions) const
{
   RegularExpressionMatchIteratorPrivate *priv =
         new RegularExpressionMatchIteratorPrivate(*this,
                                                   matchType,
                                                   matchOptions,
                                                   match(subject, offset, matchType, matchOptions));
   
   return RegularExpressionMatchIterator(*priv);
}

RegularExpressionMatchIterator RegularExpression::globalMatch(const StringRef &subjectRef,
                                                              int offset,
                                                              MatchType matchType,
                                                              MatchOptions matchOptions) const
{
   RegularExpressionMatchIteratorPrivate *priv =
         new RegularExpressionMatchIteratorPrivate(*this,
                                                   matchType,
                                                   matchOptions,
                                                   match(subjectRef, offset, matchType, matchOptions));
   
   return RegularExpressionMatchIterator(*priv);
}

void RegularExpression::optimize() const
{
   if (!isValid()) {// will compile the pattern
      return;
   }
   m_implPtr->optimizePattern(RegularExpressionPrivate::OptimizePatternOption::ImmediateOptimizeOption);
}

bool RegularExpression::operator==(const RegularExpression &regex) const
{
   return (m_implPtr == regex.m_implPtr) ||
         (m_implPtr->m_pattern == regex.m_implPtr->m_pattern && 
          m_implPtr->m_patternOptions == regex.m_implPtr->m_patternOptions);
}

uint pdk_hash(const RegularExpression &key, uint seed) noexcept
{
   pdk::internal::HashCombine hash;
   seed = hash(key.m_implPtr->m_pattern, seed);
   seed = hash(key.m_implPtr->m_patternOptions.getUnderData(), seed);
   return seed;
}

String RegularExpression::escape(const String &str)
{
   String result;
   const int count = str.size();
   result.reserve(count * 2);
   
   // everything but [a-zA-Z0-9_] gets escaped,
   // cf. perldoc -f quotemeta
   for (int i = 0; i < count; ++i) {
      const Character current = str.at(i);
      
      if (current == Character::Null) {
         // unlike Perl, a literal NUL must be escaped with
         // "\\0" (backslash + 0) and not "\\\0" (backslash + NUL),
         // because pcre16_compile uses a NUL-terminated string
         result.append(Latin1Character('\\'));
         result.append(Latin1Character('0'));
      } else if ((current < Latin1Character('a') || current > Latin1Character('z')) &&
                 (current < Latin1Character('A') || current > Latin1Character('Z')) &&
                 (current < Latin1Character('0') || current > Latin1Character('9')) &&
                 current != Latin1Character('_'))
      {
         result.append(Latin1Character('\\'));
         result.append(current);
         if (current.isHighSurrogate() && i < (count - 1)) {
            result.append(str.at(++i));
         }
      } else {
         result.append(current);
      }
   }
   result.squeeze();
   return result;
}

RegularExpressionMatch::RegularExpressionMatch()
   : m_implPtr(new RegularExpressionMatchPrivate(RegularExpression(),
                                                 String(),
                                                 0,
                                                 0,
                                                 RegularExpression::MatchType::NoMatch,
                                                 RegularExpression::MatchOption::NoMatchOption))
{
   m_implPtr->m_isValid = true;
}

RegularExpressionMatch::~RegularExpressionMatch()
{
}

RegularExpressionMatch::RegularExpressionMatch(const RegularExpressionMatch &match)
   : m_implPtr(match.m_implPtr)
{
}

RegularExpressionMatch &RegularExpressionMatch::operator=(const RegularExpressionMatch &match)
{
   m_implPtr = match.m_implPtr;
   return *this;
}

RegularExpressionMatch::RegularExpressionMatch(RegularExpressionMatchPrivate &dd)
   : m_implPtr(&dd)
{}

RegularExpression RegularExpressionMatch::getRegularExpression() const
{
   return m_implPtr->m_regularExpression;
}

RegularExpression::MatchType RegularExpressionMatch::getMatchType() const
{
   return m_implPtr->m_matchType;
}

RegularExpression::MatchOptions RegularExpressionMatch::getMatchOptions() const
{
   return m_implPtr->m_matchOptions;
}

int RegularExpressionMatch::getLastCapturedIndex() const
{
   return m_implPtr->m_capturedCount - 1;
}

String RegularExpressionMatch::getCaptured(int nth) const
{
   if (nth < 0 || nth > getLastCapturedIndex()) {
      return String();
   }
   int start = getCapturedStart(nth);
   if (start == -1) {// didn't capture
      return String();
   }
   return m_implPtr->m_subject.substring(start + m_implPtr->m_subjectStart, getCapturedLength(nth));
}

StringRef RegularExpressionMatch::getCapturedRef(int nth) const
{
   if (nth < 0 || nth > getLastCapturedIndex()) {
      return StringRef();
   }
   int start = getCapturedStart(nth);
   if (start == -1) {
      // didn't capture
      return StringRef();
   }
   return m_implPtr->m_subject.substringRef(start + m_implPtr->m_subjectStart, getCapturedLength(nth));
}

StringView RegularExpressionMatch::getCapturedView(int nth) const
{
   return getCapturedRef(nth);
}

#if PDK_STRINGVIEW_LEVEL < 2
String RegularExpressionMatch::getCaptured(const String &name) const
{
   return getCaptured(pdk::lang::to_string_view_ignoring_null(name));
}

StringRef RegularExpressionMatch::getCapturedRef(const String &name) const
{
   return getCapturedRef(pdk::lang::to_string_view_ignoring_null(name));
}
#endif // PDK_STRINGVIEW_LEVEL < 2

String RegularExpressionMatch::getCaptured(StringView name) const
{
   if (name.isEmpty()) {
      warning_stream("RegularExpressionMatch::captured: empty capturing group name passed");
      return String();
   }
   int nth = m_implPtr->m_regularExpression.m_implPtr->captureIndexForName(name);
   if (nth == -1) {
      return String();
   }
   return getCaptured(nth);
}

StringRef RegularExpressionMatch::getCapturedRef(StringView name) const
{
   if (name.isEmpty()) {
      warning_stream("RegularExpressionMatch::capturedRef: empty capturing group name passed");
      return StringRef();
   }
   int nth = m_implPtr->m_regularExpression.m_implPtr->captureIndexForName(name);
   if (nth == -1) {
      return StringRef();
   }
   return getCapturedRef(nth);
}

StringView RegularExpressionMatch::getCapturedView(StringView name) const
{
   if (name.isEmpty()) {
      warning_stream("RegularExpressionMatch::capturedView: empty capturing group name passed");
      return StringView();
   }
   int nth = m_implPtr->m_regularExpression.m_implPtr->captureIndexForName(name);
   if (nth == -1) {
      return StringView();
   }
   return getCapturedView(nth);
}

StringList RegularExpressionMatch::getCapturedTexts() const
{
   StringList texts;
   texts.resize(m_implPtr->m_capturedCount);
   for (int i = 0; i < m_implPtr->m_capturedCount; ++i) {
      texts << getCaptured(i);
   }
   return texts;
}

int RegularExpressionMatch::getCapturedStart(int nth) const
{
   if (nth < 0 || nth > getLastCapturedIndex()) {
      return -1;      
   }
   return m_implPtr->m_capturedOffsets.at(nth * 2);
}

int RegularExpressionMatch::getCapturedLength(int nth) const
{
   // bound checking performed by these two functions
   return getCapturedEnd(nth) - getCapturedStart(nth);
}

int RegularExpressionMatch::getCapturedEnd(int nth) const
{
   if (nth < 0 || nth > getLastCapturedIndex())
      return -1;
   
   return m_implPtr->m_capturedOffsets.at(nth * 2 + 1);
}

#if PDK_STRINGVIEW_LEVEL < 2
int RegularExpressionMatch::getCapturedStart(const String &name) const
{
   return getCapturedStart(pdk::lang::to_string_view_ignoring_null(name));
}
int RegularExpressionMatch::getCapturedLength(const String &name) const
{
   return getCapturedLength(pdk::lang::to_string_view_ignoring_null(name));
}

int RegularExpressionMatch::getCapturedEnd(const String &name) const
{
   return getCapturedEnd(pdk::lang::to_string_view_ignoring_null(name));
}
#endif // PDK_STRINGVIEW_LEVEL < 2

int RegularExpressionMatch::getCapturedStart(StringView name) const
{
   if (name.isEmpty()) {
      warning_stream("RegularExpressionMatch::capturedStart: empty capturing group name passed");
      return -1;
   }
   int nth = m_implPtr->m_regularExpression.m_implPtr->captureIndexForName(name);
   if (nth == -1) {
      return -1;
   }
   return getCapturedStart(nth);
}

int RegularExpressionMatch::getCapturedLength(StringView name) const
{
   if (name.isEmpty()) {
      warning_stream("RegularExpressionMatch::capturedLength: empty capturing group name passed");
      return 0;
   }
   int nth = m_implPtr->m_regularExpression.m_implPtr->captureIndexForName(name);
   if (nth == -1) {
      return 0;
   }
   return getCapturedLength(nth);
}

int RegularExpressionMatch::getCapturedEnd(StringView name) const
{
   if (name.isEmpty()) {
      warning_stream("RegularExpressionMatch::capturedEnd: empty capturing group name passed");
      return -1;
   }
   int nth = m_implPtr->m_regularExpression.m_implPtr->captureIndexForName(name);
   if (nth == -1)
      return -1;
   return getCapturedEnd(nth);
}

bool RegularExpressionMatch::hasMatch() const
{
   return m_implPtr->m_hasMatch;
}

bool RegularExpressionMatch::hasPartialMatch() const
{
   return m_implPtr->m_hasPartialMatch;
}

bool RegularExpressionMatch::isValid() const
{
   return m_implPtr->m_isValid;
}

RegularExpressionMatchIterator::RegularExpressionMatchIterator(RegularExpressionMatchIteratorPrivate &dd)
   : m_implPtr(&dd)
{
}

RegularExpressionMatchIterator::RegularExpressionMatchIterator()
   : m_implPtr(new RegularExpressionMatchIteratorPrivate(RegularExpression(),
                                                         RegularExpression::MatchType::NoMatch,
                                                         RegularExpression::MatchOption::NoMatchOption,
                                                         RegularExpressionMatch()))
{
}

RegularExpressionMatchIterator::~RegularExpressionMatchIterator()
{
}

RegularExpressionMatchIterator::RegularExpressionMatchIterator(const RegularExpressionMatchIterator &iterator)
   : m_implPtr(iterator.m_implPtr)
{
}

RegularExpressionMatchIterator &RegularExpressionMatchIterator::operator=(const RegularExpressionMatchIterator &iterator)
{
   m_implPtr = iterator.m_implPtr;
   return *this;
}

bool RegularExpressionMatchIterator::isValid() const
{
   return m_implPtr->m_next.isValid();
}

bool RegularExpressionMatchIterator::hasNext() const
{
   return m_implPtr->hasNext();
}

RegularExpressionMatch RegularExpressionMatchIterator::peekNext() const
{
   if (!hasNext()) {
      warning_stream("RegularExpressionMatchIterator::peekNext() called on an iterator already at end");
   }
   return m_implPtr->m_next;
}

RegularExpressionMatch RegularExpressionMatchIterator::next()
{
   if (!hasNext()) {
      warning_stream("RegularExpressionMatchIterator::next() called on an iterator already at end");
      return m_implPtr->m_next;
   }
   
   RegularExpressionMatch current = m_implPtr->m_next;
   m_implPtr->m_next = m_implPtr->m_next.m_implPtr.constData()->nextMatch();
   return current;
}

RegularExpression RegularExpressionMatchIterator::getRegularExpression() const
{
   return m_implPtr->m_regularExpression;
}

RegularExpression::MatchType RegularExpressionMatchIterator::getMatchType() const
{
   return m_implPtr->m_matchType;
}

RegularExpression::MatchOptions RegularExpressionMatchIterator::getMatchOptions() const
{
   return m_implPtr->m_matchOptions;
}

#ifndef PDK_NO_DATASTREAM
DataStream &operator<<(DataStream &out, const RegularExpression &regex)
{
   out << regex.getPattern() << pdk::puint32(regex.getPatternOptions());
   return out;
}

DataStream &operator>>(DataStream &in, RegularExpression &regex)
{
   String pattern;
   pdk::puint32 patternOptions;
   in >> pattern >> patternOptions;
   regex.setPattern(pattern);
   regex.setPatternOptions(RegularExpression::PatternOptions(patternOptions));
   return in;
}
#endif

#ifndef PDK_NO_DEBUG_STREAM
Debug operator<<(Debug debug, const RegularExpression &regex)
{
   DebugStateSaver saver(debug);
   debug.nospace() << "RegularExpression(" << regex.getPattern() << ", " << regex.getPatternOptions() << ')';
   return debug;
}

Debug operator<<(Debug debug, RegularExpression::PatternOptions patternOptions)
{
   DebugStateSaver saver(debug);
   ByteArray flags;
   
   if (patternOptions == RegularExpression::PatternOption::NoPatternOption) {
      flags = "NoPatternOption";
   } else {
      flags.reserve(200); // worst case...
      if (patternOptions & RegularExpression::PatternOption::CaseInsensitiveOption) {
         flags.append("CaseInsensitiveOption|");
      }
      if (patternOptions & RegularExpression::PatternOption::DotMatchesEverythingOption) {
         flags.append("DotMatchesEverythingOption|");
      }
      if (patternOptions & RegularExpression::PatternOption::MultilineOption) {
         flags.append("MultilineOption|");
      }
      if (patternOptions & RegularExpression::PatternOption::ExtendedPatternSyntaxOption) {
         flags.append("ExtendedPatternSyntaxOption|");
      }
      if (patternOptions & RegularExpression::PatternOption::InvertedGreedinessOption) {
         flags.append("InvertedGreedinessOption|");
      }
      if (patternOptions & RegularExpression::PatternOption::DontCaptureOption) {
         flags.append("DontCaptureOption|");
      }
      if (patternOptions & RegularExpression::PatternOption::UseUnicodePropertiesOption) {
         flags.append("UseUnicodePropertiesOption|");
      }
      if (patternOptions & RegularExpression::PatternOption::OptimizeOnFirstUsageOption) {
         flags.append("OptimizeOnFirstUsageOption|");
      }
      if (patternOptions & RegularExpression::PatternOption::DontAutomaticallyOptimizeOption) {
         flags.append("DontAutomaticallyOptimizeOption|");
      }
      flags.chop(1);
   }
   debug.nospace() << "RegularExpression::PatternOptions(" << flags << ')';
   return debug;
}

Debug operator<<(Debug debug, const RegularExpressionMatch &match)
{
   DebugStateSaver saver(debug);
   debug.nospace() << "RegularExpressionMatch(";
   
   if (!match.isValid()) {
      debug << "Invalid)";
      return debug;
   }
   debug << "Valid";
   if (match.hasMatch()) {
      debug << ", has match: ";
      for (int i = 0; i <= match.getLastCapturedIndex(); ++i) {
         debug << i
               << ":(" << match.getCapturedStart(i) << ", " << match.getCapturedEnd(i)
               << ", " << match.getCaptured(i) << ')';
         if (i < match.getLastCapturedIndex()) {
            debug << ", ";
         }
      }
   } else if (match.hasPartialMatch()) {
      debug << ", has partial match: ("
            << match.getCapturedStart(0) << ", "
            << match.getCapturedEnd(0) << ", "
            << match.getCaptured(0) << ')';
   } else {
      debug << ", no match";
   }
   debug << ')';
   return debug;
}
#endif

} // text
} // pdk
