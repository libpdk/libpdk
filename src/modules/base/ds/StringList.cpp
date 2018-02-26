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
#include <set>

namespace pdk {
namespace ds {
namespace internal {

using pdk::ds::StringList;

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
      if (i) {
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
      that->erase(that->begin() + j, that->end());
   }
   return n - j;
}

} // internal
} // ds
} // pdk
