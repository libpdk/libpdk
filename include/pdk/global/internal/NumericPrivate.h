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
// Created by softboy on 2018/02/03.

#ifndef PDK_GLOBAL_INTERNAL_NUMERIC_PRIVATE_H
#define PDK_GLOBAL_INTERNAL_NUMERIC_PRIVATE_H

#include "pdk/global/Global.h"

#include <limits>

namespace pdk {

constexpr PDK_DECL_CONST_FUNCTION static inline double inf() noexcept
{
   PDK_STATIC_ASSERT_X(std::numeric_limits<double>::has_infinity,
                       "platform has no definition for infinity for type double");
   return std::numeric_limits<double>::infinity();
}

constexpr PDK_DECL_CONST_FUNCTION static inline double snan() noexcept
{
   PDK_STATIC_ASSERT_X(std::numeric_limits<double>::has_signaling_NaN,
                       "platform has no definition for signaling NaN for type double");
   return std::numeric_limits<double>::signaling_NaN();
}

constexpr PDK_DECL_CONST_FUNCTION static inline double qnan() noexcept
{
   PDK_STATIC_ASSERT_X(std::numeric_limits<double>::has_quiet_NaN,
                       "platform has no definition for quiet NaN for type double");
   return std::numeric_limits<double>::quiet_NaN();
}

} // pdk

#endif // PDK_GLOBAL_INTERNAL_NUMERIC_PRIVATE_H
