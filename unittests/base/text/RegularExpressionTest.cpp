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

#include "gtest/gtest.h"
#include "pdk/base/text/RegularExpression.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/lang/StringView.h"
#include <map>

using pdk::text::RegularExpression;
using pdk::text::RegularExpressionMatch;
using pdk::text::RegularExpressionMatchIterator;
using pdk::lang::String;
using pdk::lang::StringRef;
using pdk::lang::StringView;
using pdk::ds::StringList;
using pdk::lang::Latin1String;
using pdk::lang::Character;

namespace {

struct Match
{
   Match()
   {
      clear();
   }
   
   void clear()
   {
      m_isValid = false;
      m_hasMatch = false;
      m_hasPartialMatch = false;
      m_captured.clear();
      m_namedCaptured.clear();
   }
   
   bool m_isValid;
   bool m_hasMatch;
   bool m_hasPartialMatch;
   StringList m_captured;
   std::map<String, String> m_namedCaptured;
};

bool operator==(const RegularExpressionMatch &rem, const Match &m)
{
   if (rem.isValid() != m.m_isValid) {
      return false;
   }
   if (!rem.isValid()) {
      return true;
   } 
   if ((rem.hasMatch() != m.m_hasMatch) || (rem.hasPartialMatch() != m.m_hasPartialMatch)) {
      return false;
   }
   if (rem.hasMatch() || rem.hasPartialMatch()) {
      if ((size_t)rem.getLastCapturedIndex() != (m.m_captured.size() - 1ul)) {
         return false;
      }
      for (int i = 0; i <= rem.getLastCapturedIndex(); ++i) {
         String remCaptured = rem.getCaptured(i);
         String mCaptured = m.m_captured.at(i);
         if (remCaptured != mCaptured
             || remCaptured.isNull() != mCaptured.isNull()
             || remCaptured.isEmpty() != mCaptured.isEmpty()) {
            return false;
         }
      }
      
      for (auto it = m.m_namedCaptured.begin(), end = m.m_namedCaptured.end(); it != end; ++it) {
         const String remCaptured = rem.getCaptured(it->first);
         const String mCaptured = it->second;
         if (remCaptured != mCaptured
             || remCaptured.isNull() != mCaptured.isNull()
             || remCaptured.isEmpty() != mCaptured.isEmpty()) {
            return false;
         }
      }
   }
   return true;
}

bool operator==(const Match &m, const RegularExpressionMatch &rem)
{
   return operator==(rem, m);
}

bool operator!=(const RegularExpressionMatch &rem, const Match &m)
{
   return !operator==(rem, m);
}

bool operator!=(const Match &m, const RegularExpressionMatch &rem)
{
   return !operator==(m, rem);
}


bool operator==(const RegularExpressionMatchIterator &iterator, const std::vector<Match> &expectedMatchList)
{
   RegularExpressionMatchIterator i = iterator;
   
   for (const Match &expectedMatch : expectedMatchList) {
      if (!i.hasNext()) {
         return false;
      }
      RegularExpressionMatch match = i.next();
      if (match != expectedMatch)
         return false;
   }
   
   if (i.hasNext()) {
      return false;
   }
   return true;
}

bool operator==(const std::vector<Match> &expectedMatchList, const RegularExpressionMatchIterator &iterator)
{
   return operator==(iterator, expectedMatchList);
}

bool operator!=(const RegularExpressionMatchIterator &iterator, const std::vector<Match> &expectedMatchList)
{
   return !operator==(iterator, expectedMatchList);
}

bool operator!=(const std::vector<Match> &expectedMatchList, const RegularExpressionMatchIterator &iterator)
{
   return !operator==(expectedMatchList, iterator);
}

void consistency_check(const RegularExpressionMatch &match)
{
   if (match.isValid()) {
      ASSERT_TRUE(match.getRegularExpression().isValid());
      ASSERT_TRUE(!(match.hasMatch() && match.hasPartialMatch()));
      
      if (match.hasMatch() || match.hasPartialMatch()) {
         ASSERT_TRUE(match.getLastCapturedIndex() >= 0);
         if (match.hasPartialMatch()) {
            ASSERT_TRUE(match.getLastCapturedIndex() == 0);
         }
         
         for (int i = 0; i <= match.getLastCapturedIndex(); ++i) {
            int startPos = match.getCapturedStart(i);
            int endPos = match.getCapturedEnd(i);
            int length = match.getCapturedLength(i);
            String captured = match.getCaptured(i);
            StringRef capturedRef = match.getCapturedRef(i);
            StringView capturedView = match.getCapturedView(i);
            
            if (!captured.isNull()) {
               ASSERT_TRUE(startPos >= 0);
               ASSERT_TRUE(endPos >= 0);
               ASSERT_TRUE(length >= 0);
               ASSERT_TRUE(endPos >= startPos);
               ASSERT_TRUE((endPos - startPos) == length);
               ASSERT_TRUE(captured == capturedRef);
               ASSERT_TRUE(captured == capturedView);
            } else {
               ASSERT_TRUE(startPos == -1);
               ASSERT_TRUE(endPos == -1);
               ASSERT_TRUE((endPos - startPos) == length);
               ASSERT_TRUE(capturedRef.isNull());
               ASSERT_TRUE(capturedView.isNull());
            }
         }
      }
   } else {
      ASSERT_TRUE(!match.hasMatch());
      ASSERT_TRUE(!match.hasPartialMatch());
      ASSERT_TRUE(match.getCaptured(0).isNull());
      ASSERT_TRUE(match.getCapturedStart(0) == -1);
      ASSERT_TRUE(match.getCapturedEnd(0) == -1);
      ASSERT_TRUE(match.getCapturedLength(0) == 0);
   }
}

void consistency_check(const RegularExpressionMatchIterator &iterator)
{
   RegularExpressionMatchIterator i(iterator); // make a copy, we modify it
   if (i.isValid()) {
      while (i.hasNext()) {
         RegularExpressionMatch peeked = i.peekNext();
         RegularExpressionMatch match = i.next();
         consistency_check(peeked);
         consistency_check(match);
         ASSERT_TRUE(match.isValid());
         ASSERT_TRUE(match.hasMatch() || match.hasPartialMatch());
         ASSERT_EQ(i.getRegularExpression(), match.getRegularExpression());
         ASSERT_EQ(i.getMatchOptions(), match.getMatchOptions());
         ASSERT_EQ(i.getMatchType(), match.getMatchType());
         
         ASSERT_TRUE(peeked.isValid() == match.isValid());
         ASSERT_TRUE(peeked.hasMatch() == match.hasMatch());
         ASSERT_TRUE(peeked.hasPartialMatch() == match.hasPartialMatch());
         ASSERT_TRUE(peeked.getLastCapturedIndex() == match.getLastCapturedIndex());
         for (int i = 0; i <= peeked.getLastCapturedIndex(); ++i) {
            ASSERT_TRUE(peeked.getCaptured(i) == match.getCaptured(i));
            ASSERT_TRUE(peeked.getCapturedStart(i) == match.getCapturedStart(i));
            ASSERT_TRUE(peeked.getCapturedEnd(i) == match.getCapturedEnd(i));
         }
      }
   } else {
      ASSERT_TRUE(!i.hasNext());
      
      RegularExpressionMatch peeked = i.peekNext();
      
      RegularExpressionMatch match = i.next();
      consistency_check(peeked);
      consistency_check(match);
      ASSERT_TRUE(!match.isValid());
      ASSERT_TRUE(!peeked.isValid());
   }
   
}

template<typename Result>
void prepare_result_for_no_match_type(Result *r, const Result &orig)
{
   PDK_UNUSED(r);
   PDK_UNUSED(orig);
}

static void prepare_result_for_no_match_type(Match *m, const Match &orig)
{
   m->m_isValid = orig.m_isValid;
}

template<typename PdkREMatch, typename PdkREMatchFunc, typename Subject, typename Result>
void test_match_impl(const RegularExpression &regexp,
                     PdkREMatchFunc matchingMethod,
                     const Subject &subject,
                     int offset,
                     RegularExpression::MatchType matchType,
                     RegularExpression::MatchOptions matchOptions,
                     const Result &result)
{
   {
      const PdkREMatch m = (regexp.*matchingMethod)(subject, offset, matchType, matchOptions);
      consistency_check(m);
      ASSERT_TRUE(m == result);
      ASSERT_EQ(m.getRegularExpression(), regexp);
      ASSERT_EQ(m.getMatchType(), matchType);
      ASSERT_EQ(m.getMatchOptions(), matchOptions);
   }
   {
      // ignore the expected results provided by the match object --
      // we'll never get any result when testing the NoMatch type.
      // Just check the validity of the match here.
      Result realMatch;
      prepare_result_for_no_match_type(&realMatch, result);
      
      const PdkREMatch m = (regexp.*matchingMethod)(subject, offset, RegularExpression::MatchType::NoMatch, matchOptions);
      consistency_check(m);
      ASSERT_TRUE(m == realMatch);
      ASSERT_EQ(m.getRegularExpression(), regexp);
      ASSERT_EQ(m.getMatchType(), RegularExpression::MatchType::NoMatch);
      ASSERT_EQ(m.getMatchOptions(), matchOptions);
   }
}

template<typename PdkREMatch, typename PdkREMatchFuncForString, typename PdkREMatchFuncForStringRef, typename Result>
void test_match(const RegularExpression &regexp,
                PdkREMatchFuncForString matchingMethodForString,
                PdkREMatchFuncForStringRef matchingMethodForStringRef,
                const String &subject,
                int offset,
                RegularExpression::MatchType matchType,
                RegularExpression::MatchOptions matchOptions,
                const Result &result,
                bool forceOptimize)
{
   if (forceOptimize)
      regexp.optimize();
   
   // test with String as subject type
   test_match_impl<PdkREMatch>(regexp, matchingMethodForString, subject, offset, matchType, matchOptions, result);
   
   // test with StringRef as subject type
   test_match_impl<PdkREMatch>(regexp,
                               matchingMethodForStringRef,
                               StringRef(&subject, 0, subject.length()),
                               offset,
                               matchType,
                               matchOptions,
                               result);
}

using REMatchStringPMF = RegularExpressionMatch (RegularExpression::*)(const String &, int, RegularExpression::MatchType, RegularExpression::MatchOptions) const;
using REMatchStringRefPMF = RegularExpressionMatch (RegularExpression::*)(const StringRef &, int, RegularExpression::MatchType, RegularExpression::MatchOptions) const;
using REGlobalMatchStringPMF = RegularExpressionMatchIterator (RegularExpression::*)(const String &, int, RegularExpression::MatchType, RegularExpression::MatchOptions) const;
using REGlobalMatchStringRefPMF = RegularExpressionMatchIterator (RegularExpression::*)(const StringRef &, int, RegularExpression::MatchType, RegularExpression::MatchOptions) const;

void provide_regular_expressions(std::list<std::tuple<String, RegularExpression::PatternOptions>> &data)
{
   data.push_back(std::make_tuple(String(), RegularExpression::PatternOptions(0)));
   data.push_back(std::make_tuple(String(), RegularExpression::PatternOptions(RegularExpression::PatternOption::CaseInsensitiveOption)
                                  | RegularExpression::PatternOption::DotMatchesEverythingOption
                                  | RegularExpression::PatternOption::MultilineOption));
   
   data.push_back(std::make_tuple(String(Latin1String("")), RegularExpression::PatternOptions(0)));
   data.push_back(std::make_tuple(String(Latin1String("")), RegularExpression::PatternOptions(RegularExpression::PatternOption::CaseInsensitiveOption)
                                  | RegularExpression::PatternOption::DotMatchesEverythingOption
                                  | RegularExpression::PatternOption::MultilineOption));
   
   data.push_back(std::make_tuple(String(Latin1String("a pattern")), RegularExpression::PatternOptions(0)));
   data.push_back(std::make_tuple(String(Latin1String("^a (.*) more complicated(?<P>pattern)$")), RegularExpression::PatternOptions(0)));
   data.push_back(std::make_tuple(String(Latin1String("(?:a) pAttErN")), RegularExpression::PatternOptions(RegularExpression::PatternOption::CaseInsensitiveOption)));
   data.push_back(std::make_tuple(String(Latin1String("a\nmultiline\npattern")), RegularExpression::PatternOptions(RegularExpression::PatternOption::MultilineOption)));
   data.push_back(std::make_tuple(String(Latin1String("an extended # IGNOREME\npattern")), RegularExpression::PatternOptions(RegularExpression::PatternOption::ExtendedPatternSyntaxOption)));
   data.push_back(std::make_tuple(String(Latin1String("a [sS]ingleline .* match")), RegularExpression::PatternOptions(RegularExpression::PatternOption::DotMatchesEverythingOption)));
   
   data.push_back(std::make_tuple(String(Latin1String("multiple.*options")), RegularExpression::PatternOptions(RegularExpression::PatternOption::CaseInsensitiveOption)
                                  | RegularExpression::PatternOption::DotMatchesEverythingOption
                                  | RegularExpression::PatternOption::MultilineOption
                                  | RegularExpression::PatternOption::DontCaptureOption
                                  | RegularExpression::PatternOption::InvertedGreedinessOption));
   
   data.push_back(std::make_tuple(String::fromUtf8("Unicode \xf0\x9d\x85\x9d \xf0\x9d\x85\x9e\xf0\x9d\x85\x9f"), RegularExpression::PatternOptions(0)));
   data.push_back(std::make_tuple(String::fromUtf8("Unicode \xf0\x9d\x85\x9d \xf0\x9d\x85\x9e\xf0\x9d\x85\x9f"), RegularExpression::PatternOptions(RegularExpression::PatternOption::CaseInsensitiveOption)
                                  | RegularExpression::PatternOption::DotMatchesEverythingOption
                                  | RegularExpression::PatternOption::InvertedGreedinessOption));
}

} // anonymous namespace

PDK_DECLARE_TYPEINFO(Match, PDK_MOVABLE_TYPE);

TEST(RegularExpressionTest, testDefaultConstructors)
{
   RegularExpression re;
   ASSERT_EQ(re.getPattern(), String());
   ASSERT_EQ(re.getPatternOptions(), RegularExpression::PatternOption::NoPatternOption);
   
   RegularExpressionMatch match;
   ASSERT_EQ(match.getRegularExpression(), RegularExpression());
   ASSERT_EQ(match.getRegularExpression(), re);
   ASSERT_EQ(match.getMatchType(), RegularExpression::MatchType::NoMatch);
   ASSERT_EQ(match.getMatchOptions(), RegularExpression::MatchOption::NoMatchOption);
   ASSERT_EQ(match.hasMatch(), false);
   ASSERT_EQ(match.hasPartialMatch(), false);
   ASSERT_EQ(match.isValid(), true);
   ASSERT_EQ(match.getLastCapturedIndex(), -1);
   
   RegularExpressionMatchIterator iterator;
   ASSERT_EQ(iterator.getRegularExpression(), RegularExpression());
   ASSERT_EQ(iterator.getRegularExpression(), re);
   ASSERT_EQ(iterator.getMatchType(), RegularExpression::MatchType::NoMatch);
   ASSERT_EQ(iterator.getMatchOptions(), RegularExpression::MatchOption::NoMatchOption);
   ASSERT_EQ(iterator.isValid(), true);
   ASSERT_EQ(iterator.hasNext(), false);
}

namespace {

void init_getters_setters_data(std::list<std::tuple<String, RegularExpression::PatternOptions>> &data)
{
   provide_regular_expressions(data);
}

void do_test_getters_setters(bool forceOptimize)
{
   std::list<std::tuple<String, RegularExpression::PatternOptions>> data;
   init_getters_setters_data(data);
   for (auto &item : data) {
      String pattern = std::get<0>(item);
      RegularExpression::PatternOptions patternOptions = std::get<1>(item);
      {
         RegularExpression re;
         re.setPattern(pattern);
         if (forceOptimize) {
            re.optimize();
         }
         ASSERT_EQ(re.getPattern(), pattern);
         ASSERT_EQ(re.getPatternOptions(), RegularExpression::PatternOption::NoPatternOption);
      }
      
      {
         RegularExpression re;
         re.setPatternOptions(patternOptions);
         if (forceOptimize) {
            re.optimize();
         }
         ASSERT_EQ(re.getPattern(), String());
         ASSERT_EQ(re.getPatternOptions(), patternOptions);
      }
      {
         RegularExpression re(pattern);
         if (forceOptimize) {
            re.optimize();
         }
         ASSERT_EQ(re.getPattern(), pattern);
         ASSERT_EQ(re.getPatternOptions(), RegularExpression::PatternOption::NoPatternOption);
      }
      {
         RegularExpression re(pattern, patternOptions);
         if (forceOptimize) {
            re.optimize();
         }
         ASSERT_EQ(re.getPattern(), pattern);
         ASSERT_EQ(re.getPatternOptions(), patternOptions);
      }
   }
}

void init_escape_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(String(Latin1String("a normal pattern")),
                                  String(Latin1String("a\\ normal\\ pattern"))));
   
   data.push_back(std::make_tuple(String(Latin1String("abcdefghijklmnopqrstuvzABCDEFGHIJKLMNOPQRSTUVZ1234567890_")),
                                  String(Latin1String("abcdefghijklmnopqrstuvzABCDEFGHIJKLMNOPQRSTUVZ1234567890_"))));
   
   data.push_back(std::make_tuple(String(Latin1String("^\\ba\\b.*(?<NAME>reg|exp)$")),
                                  String(Latin1String("\\^\\\\ba\\\\b\\.\\*\\(\\?\\<NAME\\>reg\\|exp\\)\\$"))));
   
   String nulString(Latin1String("abcXabcXXabc"));
   nulString[3] = nulString[7] = nulString[8] = Character(0, 0);
   
   data.push_back(std::make_tuple(nulString,
                                  String(Latin1String("abc\\0abc\\0\\0abc"))));
   
   data.push_back(std::make_tuple(String::fromUtf8("^s[ome] latin-1 \xc3\x80\xc3\x88\xc3\x8c\xc3\x92\xc3\x99 chars$"),
                                  String::fromUtf8("\\^s\\[ome\\]\\ latin\\-1\\ \\\xc3\x80\\\xc3\x88\\\xc3\x8c\\\xc3\x92\\\xc3\x99\\ chars\\$")));
   
   data.push_back(std::make_tuple(String::fromUtf8("Unicode \xf0\x9d\x85\x9d \xf0\x9d\x85\x9e\xf0\x9d\x85\x9f"),
                                  String::fromUtf8("Unicode\\ \\\xf0\x9d\x85\x9d\\ \\\xf0\x9d\x85\x9e\\\xf0\x9d\x85\x9f")));
   
   String unicodeAndNulString = String::fromUtf8("^\xc3\x80\xc3\x88\xc3\x8cN\xc3\x92NN\xc3\x99 chars$");
   unicodeAndNulString[4] = unicodeAndNulString[6] = unicodeAndNulString[7] = Character(0, 0);
   data.push_back(std::make_tuple(unicodeAndNulString,
                                  String::fromUtf8("\\^\\\xc3\x80\\\xc3\x88\\\xc3\x8c\\0\\\xc3\x92\\0\\0\\\xc3\x99\\ chars\\$")));
}

void do_test_escape(bool forceOptimize)
{
   std::list<std::tuple<String, String>> data;
   init_escape_data(data);
   for (auto &item : data) {
      String string = std::get<0>(item);
      String escaped = std::get<1>(item);
      ASSERT_EQ(RegularExpression::escape(string), escaped);
      RegularExpression re(escaped);
      if (forceOptimize) {
         re.optimize();
      }
      ASSERT_EQ(re.isValid(), true);
   }
}

void init_validity_data(std::list<std::tuple<String, bool>> &data)
{
   data.push_back(std::make_tuple(String(Latin1String("a pattern")), true));
   data.push_back(std::make_tuple(String(Latin1String("(a|pattern)")), true));
   data.push_back(std::make_tuple(String(Latin1String("a [pP]attern")), true));
   data.push_back(std::make_tuple(String(Latin1String("^(?<article>a).*(?<noun>pattern)$")), true));
   data.push_back(std::make_tuple(String(Latin1String("a \\P{Ll}attern")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("a pattern\\")), false));
   data.push_back(std::make_tuple(String(Latin1String("(a|pattern")), false));
   data.push_back(std::make_tuple(String(Latin1String("a \\P{BLAH}attern")), false));
   
   String pattern;
   // 0xD800 (high surrogate) not followed by a low surrogate
   pattern = Latin1String("abcdef");
   pattern[3] = Character(0x00, 0xD8);
   data.push_back(std::make_tuple(pattern, false));
}

void do_test_validity(bool forceOptimize)
{
   std::list<std::tuple<String, bool>> data;
   init_validity_data(data);
   for (auto &item : data) {
      String &pattern = std::get<0>(item);
      bool validity = std::get<1>(item);
      RegularExpression re(pattern);
      if (forceOptimize) {
         re.optimize();
      }
      ASSERT_EQ(re.isValid(), validity);
      if (!validity)
      {}
      RegularExpressionMatch match = re.match(Latin1String("a pattern"));
      ASSERT_EQ(match.isValid(), validity);
      consistency_check(match);
      if (!validity)
      {}
      RegularExpressionMatchIterator iterator = re.globalMatch(Latin1String("a pattern"));
      ASSERT_EQ(iterator.isValid(), validity);
   }
}

void init_pattern_options_data(std::list<std::tuple<RegularExpression, String, Match>> &data)
{
   // none of these would successfully match if the respective
   // pattern option is not set
   Match m;
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String::fromUtf8("AbC\xc3\xa0");
   data.push_back(std::make_tuple(RegularExpression(String::fromUtf8("abc\xc3\x80"), RegularExpression::PatternOption::CaseInsensitiveOption),
                                  String::fromUtf8("AbC\xc3\xa0"),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String("abc123\n678def"));
   data.push_back(std::make_tuple(RegularExpression(Latin1String("\\Aabc.*def\\z"), RegularExpression::PatternOption::DotMatchesEverythingOption),
                                  String(Latin1String("abc123\n678def")),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String("jumped over"));
   data.push_back(std::make_tuple(RegularExpression(Latin1String("^\\w+ \\w+$"), RegularExpression::PatternOption::MultilineOption),
                                  String(Latin1String("the quick fox\njumped over\nthe lazy\ndog")),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String("abc 123456"));
   data.push_back(std::make_tuple(RegularExpression(Latin1String("\\w+  # a word\n"
                                                                 "\\ # a space\n"
                                                                 "\\w+ # another word"), RegularExpression::PatternOption::ExtendedPatternSyntaxOption),
                                  String(Latin1String("abc 123456 def")),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String("the quick fox")) 
                << String(Latin1String("the"))
                << String(Latin1String("quick fox"));
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(.+) (.+?)"), RegularExpression::PatternOption::InvertedGreedinessOption),
                                  String(Latin1String("the quick fox")),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String("the quick fox")) 
                << String(Latin1String("quick"));
   m.m_namedCaptured[Latin1String("named")] = String(Latin1String("quick"));
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(\\w+) (?<named>\\w+) (\\w+)"), RegularExpression::PatternOption::DontCaptureOption),
                                  String(Latin1String("the quick fox")),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String::fromUtf8("abc\xc3\x80\xc3\xa0 12\xdb\xb1\xdb\xb2\xf0\x9d\x9f\x98")
                << String::fromUtf8("abc\xc3\x80\xc3\xa0")
                << String::fromUtf8("12\xdb\xb1\xdb\xb2\xf0\x9d\x9f\x98");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(\\w+) (\\d+)"), RegularExpression::PatternOption::UseUnicodePropertiesOption),
                                  String::fromUtf8("abc\xc3\x80\xc3\xa0 12\xdb\xb1\xdb\xb2\xf0\x9d\x9f\x98"),
                                  m));
}

void do_test_pattern_options(bool forceOptimize)
{
   std::list<std::tuple<RegularExpression, String, Match>> data;
   init_pattern_options_data(data);
   for (auto &item : data) {
      RegularExpression &regexp = std::get<0>(item);
      String &subject = std::get<1>(item);
      Match &match = std::get<2>(item);
      if (forceOptimize) {
         regexp.optimize();
      }
      RegularExpressionMatch m = regexp.match(subject);
      consistency_check(m);
      ASSERT_TRUE(m == match);
   }
}

using NormalMatchDataType = std::list<std::tuple<RegularExpression, String, int, RegularExpression::MatchOptions, Match>>;

void init_normal_match_data(NormalMatchDataType &data)
{
   Match m;
   int offset = 0;
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String("string")) << String(Latin1String("string"));
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(\\bstring\\b)")),
                                  String(Latin1String("a string")),
                                  0,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String("a string"))
                << String(Latin1String("a")) 
                << String(Latin1String("string"));
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(\\w+) (\\w+)")),
                                  String(Latin1String("a string")),
                                  0,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String("a string"))
                << String(Latin1String("a")) 
                << String(Latin1String("string"));
   m.m_namedCaptured[Latin1String("article")] = Latin1String("a");
   m.m_namedCaptured[Latin1String("noun")] = Latin1String("string");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(?<article>\\w+) (?<noun>\\w+)")),
                                  String(Latin1String("a string")),
                                  0,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String(" string"))
                << String()
                << String(Latin1String("string"));
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(\\w+)? (\\w+)")),
                                  String(Latin1String(" string")),
                                  0,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String(" string"))
                << String(Latin1String(""))
                << String(Latin1String("string"));
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(\\w*) (\\w+)")),
                                  String(Latin1String(" string")),
                                  0,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   offset = 2;
   m.m_captured << String(Latin1String("c123def"))
                << String(Latin1String("c12"))
                << String(Latin1String("3"))
                << String(Latin1String("def"));
   for (int i = 0; i <= offset; ++i) {
      data.push_back(std::make_tuple(RegularExpression(Latin1String("(\\w*)(\\d+)(\\w*)")),
                                     StringLiteral("abc123def").substring(offset - i),
                                     i,
                                     RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                     m));
   }
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   offset = 9;
   m.m_captured << String(Latin1String(""));
   for (int i = 0; i <= offset; ++i) {
      data.push_back(std::make_tuple(RegularExpression(Latin1String("\\w*")),
                                     StringLiteral("abc123def").substring(offset - i),
                                     i,
                                     RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                     m));
   }
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String("a string"))
                << String(Latin1String("a string"))
                << String(Latin1String(""));
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(.*)(.*)")),
                                  String(Latin1String("a string")),
                                  0,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String("a string"))
                << String(Latin1String(""))
                << String(Latin1String("a string"));
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(.*?)(.*)")),
                                  String(Latin1String("a string")),
                                  0,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String("a string"))
                << String(Latin1String("a"))
                << String(Latin1String("string"));
   m.m_namedCaptured[Latin1String("article")] = Latin1String("a");
   m.m_namedCaptured[Latin1String("noun")] = Latin1String("string");
   m.m_namedCaptured[Latin1String("nonexisting1")] = String();
   m.m_namedCaptured[Latin1String("nonexisting2")] = String();
   m.m_namedCaptured[Latin1String("nonexisting3")] = String();
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(?<article>\\w+) (?<noun>\\w+)")),
                                  String(Latin1String("a string")),
                                  0,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String(""))
                << String(Latin1String(""));
   m.m_namedCaptured[Latin1String("digits")] = Latin1String("");
   m.m_namedCaptured[Latin1String("nonexisting")] = String();
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(?<digits>\\d*)")),
                                  String(Latin1String("abcde")),
                                  0,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << String(Latin1String("bcd"));
   data.push_back(std::make_tuple(RegularExpression(Latin1String("\\Bbcd\\B")),
                                  String(Latin1String("abcde")),
                                  1,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   data.push_back(std::make_tuple(RegularExpression(Latin1String("\\d+")),
                                  String(Latin1String("a string")),
                                  0,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   offset = 1;
   for (int i = 0; i <= offset; ++i) {
      data.push_back(std::make_tuple(RegularExpression(Latin1String("(\\w+) (\\w+)")),
                                     StringLiteral("a string").substring(offset - i),
                                     i,
                                     RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                     m));
   }
   
   m.clear();
   m.m_isValid = true;
   offset = 9;
   for (int i = 0; i <= offset; ++i) {
      data.push_back(std::make_tuple(RegularExpression(Latin1String("\\w+")),
                                     StringLiteral("abc123def").substring(offset - i),
                                     i,
                                     RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                     m));
   }
   
   m.clear();
   m.m_isValid = true;
   data.push_back(std::make_tuple(RegularExpression(Latin1String("\\d+")),
                                  StringLiteral("abc123def"),
                                  0,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::AnchoredMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << Latin1String("678");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("\\d+")),
                                  StringLiteral("abc123def678ghi"),
                                  -6,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << Latin1String("678");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("\\d+")),
                                  StringLiteral("abc123def678ghi"),
                                  -8,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << Latin1String("678ghi")
                << Latin1String("678")
                << Latin1String("ghi");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(\\d+)(\\w+)")),
                                  StringLiteral("abc123def678ghi"),
                                  -8,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   data.push_back(std::make_tuple(RegularExpression(Latin1String("\\d+")),
                                  StringLiteral("abc123def678ghi"),
                                  -3,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << Latin1String("678");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("^\\d+"), RegularExpression::PatternOption::MultilineOption),
                                  StringLiteral("a\nbc123\ndef\n678gh\ni"),
                                  -10,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
}

using PartialMatchDataType = std::list<std::tuple<RegularExpression, String, int, RegularExpression::MatchType, RegularExpression::MatchOptions, Match>>;

void init_partial_atch_data(PartialMatchDataType &data)
{
   Match m;
   int offset = 0;
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String("str");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("string")),
                                  Latin1String("a str"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferCompleteMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String(" str");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("\\bstring\\b")),
                                  Latin1String("a str"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferCompleteMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String(" str");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(\\bstring\\b)")),
                                  Latin1String("a str"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferCompleteMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String("8 Dec 19");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("^(\\d{1,2}) (\\w{3}) (\\d{4})$")),
                                  Latin1String("8 Dec 19"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferCompleteMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << Latin1String("8 Dec 1985")
                << Latin1String("8")
                << Latin1String("Dec")
                << Latin1String("1985");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("^(\\d{1,2}) (\\w{3}) (\\d{4})$")),
                                  Latin1String("8 Dec 1985"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferCompleteMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasMatch = true;
   m.m_captured << Latin1String("def");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("abc\\w+X|def")),
                                  Latin1String("abcdef"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferCompleteMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String("abcdef");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("abc\\w+X|defY")),
                                  Latin1String("abcdef"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferCompleteMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String("def");
   offset = 1;
   for (int i = 0; i <= offset; ++i) {
      data.push_back(std::make_tuple(RegularExpression(Latin1String("abc\\w+X|defY")),
                                     StringLiteral("abcdef").substring(offset - i),
                                     i,
                                     RegularExpression::MatchType::PartialPreferCompleteMatch,
                                     RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                     m));
   }
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String("str");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("string")),
                                  Latin1String("a str"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferFirstMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String(" str");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("\\bstring\\b")),
                                  Latin1String("a str"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferFirstMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String(" str");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(\\bstring\\b)")),
                                  Latin1String("a str"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferFirstMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String("8 Dec 19");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("^(\\d{1,2}) (\\w{3}) (\\d{4})$")),
                                  Latin1String("8 Dec 19"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferFirstMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String("8 Dec 1985");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("^(\\d{1,2}) (\\w{3}) (\\d{4})$")),
                                  Latin1String("8 Dec 1985"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferFirstMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String("abcdef");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("abc\\w+X|def")),
                                  Latin1String("abcdef"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferFirstMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String("abcdef");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("abc\\w+X|defY")),
                                  Latin1String("abcdef"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferFirstMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String("def");
   offset = 1;
   for (int i = 0; i <= offset; ++i) {
      data.push_back(std::make_tuple(RegularExpression(Latin1String("abc\\w+X|defY")),
                                     StringLiteral("abcdef").substring(offset - i),
                                     i,
                                     RegularExpression::MatchType::PartialPreferFirstMatch,
                                     RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                     m));
   }
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String("ab");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("abc|ab")),
                                  Latin1String("ab"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferFirstMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String("abc");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("abc(def)?")),
                                  Latin1String("abc"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferFirstMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   m.m_hasPartialMatch = true;
   m.m_captured << Latin1String("abc");
   data.push_back(std::make_tuple(RegularExpression(Latin1String("(abc)*")),
                                  Latin1String("abc"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferFirstMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   data.push_back(std::make_tuple(RegularExpression(Latin1String("abc\\w+X|defY")),
                                  Latin1String("123456"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferCompleteMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   data.push_back(std::make_tuple(RegularExpression(Latin1String("abc\\w+X|defY")),
                                  Latin1String("123456"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferFirstMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   data.push_back(std::make_tuple(RegularExpression(Latin1String("abc\\w+X|defY")),
                                  Latin1String("ab123"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferCompleteMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
   
   m.clear();
   m.m_isValid = true;
   data.push_back(std::make_tuple(RegularExpression(Latin1String("abc\\w+X|defY")),
                                  Latin1String("ab123"),
                                  0,
                                  RegularExpression::MatchType::PartialPreferFirstMatch,
                                  RegularExpression::MatchOptions(RegularExpression::MatchOption::NoMatchOption),
                                  m));
}

} // anonymous namespace

TEST(RegularExpressionTest, testGettersSetters)
{
   do_test_getters_setters(false);
   do_test_getters_setters(true);
}

TEST(RegularExpressionTest, testEscape)
{
   do_test_escape(false);
   do_test_escape(true);
}

TEST(RegularExpressionTest, testValidity)
{
   do_test_validity(false);
   do_test_validity(true);
}

TEST(RegularExpressionTest, testPatternOptions)
{
   do_test_pattern_options(false);
   do_test_pattern_options(true);
}

TEST(RegularExpressionTest, testNormalMatch)
{
   NormalMatchDataType data;
   init_normal_match_data(data);
   for (auto &item : data) {
      RegularExpression &regex = std::get<0>(item);
      String &subject = std::get<1>(item);
      int offset = std::get<2>(item);
      RegularExpression::MatchOptions matchOptions = std::get<3>(item);
      Match match = std::get<4>(item);
      test_match<RegularExpressionMatch>(regex,
                                         static_cast<REMatchStringPMF>(&RegularExpression::match),
                                         static_cast<REMatchStringRefPMF>(&RegularExpression::match),
                                         subject,
                                         offset,
                                         RegularExpression::MatchType::NormalMatch,
                                         matchOptions,
                                         match, false);
      test_match<RegularExpressionMatch>(regex,
                                         static_cast<REMatchStringPMF>(&RegularExpression::match),
                                         static_cast<REMatchStringRefPMF>(&RegularExpression::match),
                                         subject,
                                         offset,
                                         RegularExpression::MatchType::NormalMatch,
                                         matchOptions,
                                         match, true);
   }
}

TEST(RegularExpressionTest, testPartialMatch)
{
   PartialMatchDataType data;
   init_partial_atch_data(data);
   for (auto &item : data) {
      RegularExpression &regex = std::get<0>(item);
      String &subject = std::get<1>(item);
      int offset = std::get<2>(item);
      RegularExpression::MatchType matchType = std::get<3>(item);
      RegularExpression::MatchOptions matchOptions = std::get<4>(item);
      Match match = std::get<5>(item);
      
      test_match<RegularExpressionMatch>(regex,
                                         static_cast<REMatchStringPMF>(&RegularExpression::match),
                                         static_cast<REMatchStringRefPMF>(&RegularExpression::match),
                                         subject,
                                         offset,
                                         matchType,
                                         matchOptions,
                                         match, true);
      
      test_match<RegularExpressionMatch>(regex,
                                         static_cast<REMatchStringPMF>(&RegularExpression::match),
                                         static_cast<REMatchStringRefPMF>(&RegularExpression::match),
                                         subject,
                                         offset,
                                         matchType,
                                         matchOptions,
                                         match, false);
   }
}
