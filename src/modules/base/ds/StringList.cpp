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

} // internal
} // ds
} // pdk
