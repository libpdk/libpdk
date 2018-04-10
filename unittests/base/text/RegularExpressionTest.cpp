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


