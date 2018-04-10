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
// Created by softboy on 2018/02/01.

#include "pdk/base/ds/StringList.h"
#include "pdk/base/text/RegularExpression.h"
#include <set>

namespace pdk {
namespace ds {
namespace internal {

using pdk::ds::StringList;
using pdk::text::RegularExpressionMatch;

namespace {

int accumulated_size(const StringList &list, int seplen)
{
   int result = 0;
   if (!list.empty()) {
      for (const auto &e : list) {
         result += e.size() + seplen;
      }
      result -= seplen;
   }
   return result;
}

} // anonymous namespace

String stringlist_join(const StringList *that, const Character *sep, int seplen)
{
   const int totalLength = accumulated_size(*that, seplen);
   const int size = that->size();
   String res;
   if (totalLength == 0) {
      return res;
   }
   res.reserve(totalLength);
   for (int i = 0; i < size; ++i) {
      if (i != 0) {
         res.append(sep, seplen);
      }
      res += that->at(i);
   }
   return res;
}

String stringlist_join(const StringList &list, Latin1String sep)
{
   String result;
   if (!list.empty()) {
      result.reserve(accumulated_size(list, sep.size()));
      const auto end = list.end();
      auto it = list.begin();
      result += *it;
      while (++it != end) {
         result += sep;
         result += *it;
      }
   }
   return result;
}

namespace {

template<typename T>
static bool stringlist_contains(const StringList &stringList, const T &str, pdk::CaseSensitivity cs)
{
   for (const auto &string : stringList) {
      if (string.size() == str.size() && string.compare(str, cs) == 0) {
         return true;
      }
   }
   return false;
}

} // anonymous namespace

bool stringlist_contains(const StringList *that, const String &str, pdk::CaseSensitivity cs)
{
   return stringlist_contains(*that, str, cs);
}

bool stringlist_contains(const StringList *that, Latin1String str, pdk::CaseSensitivity cs)
{
   return stringlist_contains(*that, str, cs);
}

int stringlist_remove_duplicates(StringList *that)
{
   int n = that->size();
   int j = 0;
   std::set<String> seen;
   size_t setSize = 0;
   for (int i = 0; i < n; ++i) {
      const String &s = that->at(i);
      seen.insert(s);
      if (setSize == seen.size()) {// unchanged size => was already seen
         continue;
      }
      ++setSize;
      if (j != i) {
         that->swap(i, j);
      }
      ++j;
   }
   if (n != j) {
      auto iter = that->begin();
      std::advance(iter, j);
      that->erase(iter, that->end());
   }
   return n - j;
}

#ifndef PDK_NO_REGULAREXPRESSION
void stringlist_replace_in_strings(StringList *that, const RegularExpression &regex, const String &after)
{
   for (size_t i = 0; i < that->size(); ++i) {
      (*that)[i].replace(regex, after);
   }
}

StringList stringlist_filter(const StringList *that, const RegularExpression &regex)
{
   StringList res;
   for (size_t i = 0; i < that->size(); ++i) {
      if (that->at(i).contains(regex)) {
         res << that->at(i);
      }
   }
   return res;
}

int stringlist_index_of(const StringList *that, const RegularExpression &regex, int from)
{
   if (from < 0) {
      from = std::max(from + static_cast<int>(that->size()), 0);
   }
   
   
   String exactPattern = Latin1String("\\A(?:") + regex.getPattern() + Latin1String(")\\z");
   RegularExpression exactRe(exactPattern, regex.getPatternOptions());
   
   for (size_t i = from; i < that->size(); ++i) {
      RegularExpressionMatch m = exactRe.match(that->at(i));
      if (m.hasMatch()) {
         return i;
      }
      
   }
   return -1;
}

int stringlist_last_index_of(const StringList *that, const RegularExpression &regex, int from)
{
   if (from < 0) {
      from += that->size();
   } else if (static_cast<size_t>(from) >= that->size()) {
      from = that->size() - 1;
   }
   String exactPattern = Latin1String("\\A(?:") + regex.getPattern() + Latin1String(")\\z");
   RegularExpression exactRe(exactPattern, regex.getPatternOptions());
   
   for (int i = from; i >= 0; --i) {
      RegularExpressionMatch m = exactRe.match(that->at(i));
      if (m.hasMatch()) {
         return i;
      }
   }
   return -1;
}

#endif // PDK_NO_REGULAREXPRESSION

} // internal
} // ds
} // pdk
