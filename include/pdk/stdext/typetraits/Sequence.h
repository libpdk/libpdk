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
// Created by softboy on 2018/03/22.

#ifndef PDK_STDEXT_TYPE_TRAITS_SEQUENCE_H
#define PDK_STDEXT_TYPE_TRAITS_SEQUENCE_H

#include <tuple>

namespace pdk {
namespace stdext {

namespace internal {

template <size_t... Index, typename... ArgTypes>
decltype(auto) constexpr extract_first_n_items_impl(const std::tuple<ArgTypes...> &tuple, const std::index_sequence<Index...> &)
{
   return std::make_tuple(std::get<Index>(tuple)...);
}

} // internal

template <size_t N, typename... ArgTypes>
decltype(auto) constexpr extract_first_n_items(const std::tuple<ArgTypes...>& tuple)
{
   if constexpr(N == 0) {
      return std::tuple{};
   } else {
      return internal::extract_first_n_items_impl(tuple, std::make_index_sequence<N>{});
   }
}

} // stdext
} // pdk

#endif // PDK_STDEXT_TYPE_TRAITS_SEQUENCE_H
