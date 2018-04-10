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

#ifndef PDK_M_BASE_TEXT_REGULAR_EXPRESSION_H
#define PDK_M_BASE_TEXT_REGULAR_EXPRESSION_H

#include "pdk/global/Global.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/utils/SharedData.h"

// forward declare class with namespace
namespace pdk{
namespace io {

class Debug;
class DataStream;

} // io
} // pdk

namespace pdk {
namespace text {

using pdk::lang::StringView;
using pdk::lang::String;
using pdk::lang::StringRef;
using pdk::ds::StringList;
using pdk::utils::ExplicitlySharedDataPointer;
using pdk::utils::SharedDataPointer;

class RegularExpressionMatch;
class RegularExpressionMatchIterator;
class RegularExpression;

using pdk::io::DataStream;
using pdk::io::Debug;

namespace internal {
struct RegularExpressionPrivate;
struct RegularExpressionMatchPrivate;
struct RegularExpressionMatchIteratorPrivate;
} // internal

using internal::RegularExpressionPrivate;
using internal::RegularExpressionMatchPrivate;
using internal::RegularExpressionMatchIteratorPrivate;

PDK_CORE_EXPORT uint pdk_hash(const RegularExpression &key, uint seed = 0) noexcept;

class PDK_CORE_EXPORT RegularExpression
{
public:
   enum class PatternOption
   {
      NoPatternOption                = 0x0000,
      CaseInsensitiveOption          = 0x0001,
      DotMatchesEverythingOption     = 0x0002,
      MultilineOption                = 0x0004,
      ExtendedPatternSyntaxOption    = 0x0008,
      InvertedGreedinessOption       = 0x0010,
      DontCaptureOption              = 0x0020,
      UseUnicodePropertiesOption     = 0x0040,
      OptimizeOnFirstUsageOption     = 0x0080,
      DontAutomaticallyOptimizeOption = 0x0100
   };
   PDK_DECLARE_FLAGS(PatternOptions, PatternOption);
   enum class MatchType
   {
      NormalMatch = 0,
      PartialPreferCompleteMatch,
      PartialPreferFirstMatch,
      NoMatch
   };
   
   enum class MatchOption
   {
      NoMatchOption              = 0x0000,
      AnchoredMatchOption        = 0x0001,
      DontCheckSubjectStringMatchOption = 0x0002
   };
   PDK_DECLARE_FLAGS(MatchOptions, MatchOption);
   
   PatternOptions getPatternOptions() const;
   void setPatternOptions(PatternOptions options);
   
   RegularExpression();
   explicit RegularExpression(const String &pattern, PatternOptions options = PatternOption::NoPatternOption);
   RegularExpression(const RegularExpression &regex);
   ~RegularExpression();
   RegularExpression &operator=(const RegularExpression &regex);
   
   RegularExpression &operator=(RegularExpression &&regex) noexcept
   {
      m_implPtr.swap(regex.m_implPtr);
      return *this;
   }
   
   void swap(RegularExpression &other) noexcept
   {
      m_implPtr.swap(other.m_implPtr);
   }
   
   String getPattern() const;
   void setPattern(const String &pattern);
   
   bool isValid() const;
   int getPatternErrorOffset() const;
   String getErrorString() const;
   
   int getCaptureCount() const;
   StringList getNamedCaptureGroups() const;
   
   RegularExpressionMatch match(const String &subject,
                                int offset                = 0,
                                MatchType matchType       = MatchType::NormalMatch,
                                MatchOptions matchOptions = MatchOption::NoMatchOption) const;
   
   RegularExpressionMatch match(const StringRef &subjectRef,
                                int offset = 0,
                                MatchType matchType = MatchType::NormalMatch,
                                MatchOptions matchOptions = MatchOption::NoMatchOption) const;
   
   RegularExpressionMatchIterator globalMatch(const String &subject,
                                              int offset = 0,
                                              MatchType matchType = MatchType::NormalMatch,
                                              MatchOptions matchOptions = MatchOption::NoMatchOption) const;
   
   RegularExpressionMatchIterator globalMatch(const StringRef &subjectRef,
                                              int offset = 0,
                                              MatchType matchType = MatchType::NormalMatch,
                                              MatchOptions matchOptions = MatchOption::NoMatchOption) const;
   
   void optimize() const;
   
   static String escape(const String &str);
   
   bool operator==(const RegularExpression &regex) const;
   inline bool operator!=(const RegularExpression &regex) const
   {
      return !operator==(regex);
   }
private:
   friend struct RegularExpressionPrivate;
   friend class RegularExpressionMatch;
   friend struct RegularExpressionMatchPrivate;
   friend class RegularExpressionMatchIterator;
   friend PDK_CORE_EXPORT uint pdk_hash(const RegularExpression &key, uint seed) noexcept;
   
   RegularExpression(RegularExpressionPrivate &dd);
   ExplicitlySharedDataPointer<RegularExpressionPrivate> m_implPtr;
};

PDK_DECLARE_OPERATORS_FOR_FLAGS(RegularExpression::PatternOptions)
PDK_DECLARE_OPERATORS_FOR_FLAGS(RegularExpression::MatchOptions)

#ifndef PDK_NO_DATASTREAM
PDK_CORE_EXPORT DataStream &operator <<(DataStream &out, const RegularExpression &regex);
PDK_CORE_EXPORT DataStream &operator >>(DataStream &in, RegularExpression &regex);
#endif // PDK_NO_DATASTREAM

#ifndef PDK_NO_DEBUG_STREAM
PDK_CORE_EXPORT Debug operator <<(Debug debug, const RegularExpression &regex);
PDK_CORE_EXPORT Debug operator <<(Debug debug, RegularExpression::PatternOptions patternOptions);
#endif // PDK_NO_DEBUG_STREAM

class PDK_CORE_EXPORT RegularExpressionMatch
{
public:
   RegularExpressionMatch();
   ~RegularExpressionMatch();
   RegularExpressionMatch(const RegularExpressionMatch &match);
   RegularExpressionMatch &operator=(const RegularExpressionMatch &match);
   RegularExpressionMatch &operator=(RegularExpressionMatch &&match) noexcept
   {
      m_implPtr.swap(match.m_implPtr);
      return *this;
   }
   void swap(RegularExpressionMatch &other) noexcept
   {
      m_implPtr.swap(other.m_implPtr);
   }
   
   RegularExpression getRegularExpression() const;
   RegularExpression::MatchType getMatchType() const;
   RegularExpression::MatchOptions getMatchOptions() const;
   
   bool hasMatch() const;
   bool hasPartialMatch() const;
   
   bool isValid() const;
   
   int getLastCapturedIndex() const;
   
   String getCaptured(int nth = 0) const;
   StringRef getCapturedRef(int nth = 0) const;
   StringView getCapturedView(int nth = 0) const;
   
#if PDK_STRINGVIEW_LEVEL < 2
   String getCaptured(const String &name) const;
   StringRef getCapturedRef(const String &name) const;
#endif
   
   String getCaptured(StringView name) const;
   StringRef getCapturedRef(StringView name) const;
   StringView getCapturedView(StringView name) const;
   
   StringList getCapturedTexts() const;
   
   int getCapturedStart(int nth = 0) const;
   int getCapturedLength(int nth = 0) const;
   int getCapturedEnd(int nth = 0) const;
   
#if PDK_STRINGVIEW_LEVEL < 2
   int getCapturedStart(const String &name) const;
   int getCapturedLength(const String &name) const;
   int getCapturedEnd(const String &name) const;
#endif
   
   int getCapturedStart(StringView name) const;
   int getCapturedLength(StringView name) const;
   int getCapturedEnd(StringView name) const;
private:
   friend class RegularExpression;
   friend struct RegularExpressionMatchPrivate;
   friend class RegularExpressionMatchIterator;
   
   RegularExpressionMatch(RegularExpressionMatchPrivate &dd);
   SharedDataPointer<RegularExpressionMatchPrivate> m_implPtr;
   
};

class PDK_CORE_EXPORT RegularExpressionMatchIterator
{
public:
   RegularExpressionMatchIterator();
   ~RegularExpressionMatchIterator();
   RegularExpressionMatchIterator(const RegularExpressionMatchIterator &iterator);
   RegularExpressionMatchIterator &operator=(const RegularExpressionMatchIterator &iterator);
   
   RegularExpressionMatchIterator &operator=(RegularExpressionMatchIterator &&iterator) noexcept
   {
      m_implPtr.swap(iterator.m_implPtr);
      return *this;
   }
   
   void swap(RegularExpressionMatchIterator &other) noexcept
   {
      m_implPtr.swap(other.m_implPtr);
   }
   
   bool isValid() const;
   
   bool hasNext() const;
   RegularExpressionMatch next();
   RegularExpressionMatch peekNext() const;
   
   RegularExpression getRegularExpression() const;
   RegularExpression::MatchType getMatchType() const;
   RegularExpression::MatchOptions getMatchOptions() const;
   
private:
   friend class RegularExpression;
   
   RegularExpressionMatchIterator(RegularExpressionMatchIteratorPrivate &dd);
   SharedDataPointer<RegularExpressionMatchIteratorPrivate> m_implPtr;
};

} // text
} // pdk

PDK_DECLARE_SHARED(pdk::text::RegularExpression)
PDK_DECLARE_SHARED(pdk::text::RegularExpressionMatch)
PDK_DECLARE_SHARED(pdk::text::RegularExpressionMatchIterator)

#endif // PDK_M_BASE_TEXT_REGULAR_EXPRESSION_H
