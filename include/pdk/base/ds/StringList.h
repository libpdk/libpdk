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

#ifndef PDK_M_BASE_DS_STRING_LIST_H
#define PDK_M_BASE_DS_STRING_LIST_H

#include "pdk/base/lang/String.h"
#include "pdk/base/lang/StringMatcher.h"
#include <list>

namespace pdk {
namespace ds {

using pdk::lang::String;

class StringList : public std::list<String>
{
public:
   const_reference at(size_type idx) const noexcept
   {
      PDK_ASSERT((idx >= 0 && idx < size()));
      auto iter = cbegin();
      std::advance(iter, idx);
      return *iter;
   }
};

} // ds
} // pdk


#endif // PDK_M_BASE_DS_STRING_LIST_H
