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
// Created by softboy on 2017/01/09.

// Copyright (C) 2014, Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/optional for documentation.
//
// You are welcome to contact the author at:
//  akrzemi1@gmail.com
//

#ifndef PDK_STDEXT_OPTIONAL_INTERNAL_RELATION_OPS_H
#define PDK_STDEXT_OPTIONAL_INTERNAL_RELATION_OPS_H

#include "pdk/stdext/optional/Optional.h"

namespace pdk {
namespace stdext {
namespace optional {
namespace internal {

using pdk::stdext::optional::Optional;

//
// Optional<T> vs Optional<T> cases
//
template <typename T>
inline bool operator ==(const Optional<T> &lhs, const Optional<T> &rhs)
{
   return static_cast<bool>(lhs) && static_cast<bool>(rhs) ?
            *lhs == *rhs : static_cast<bool>(lhs) == static_cast<bool>(rhs);
}

template <typename T>
inline bool operator <(const Optional<T> &lhs, const Optional<T> &rhs)
{
   return less_pointees(lhs, rhs);
}

template <typename T>
inline bool operator !=(const Optional<T> &lhs, const Optional<T> &rhs)
{
   return !(lhs == rhs);
}

template <typename T>
inline bool operator >(const Optional<T> &lhs, const Optional<T> &rhs)
{
   return rhs < lhs;
}

template <typename T>
inline bool operator <=(const Optional<T> &lhs, const Optional<T> &rhs)
{
   return !(lhs > rhs);
}

template <typename T>
inline bool operator >=(const Optional<T> &lhs, const Optional<T> &rhs)
{
   return !(lhs < rhs);
}

//
// Optional<T> vs T cases
//
template <typename T>
inline bool operator ==(const Optional<T> &lhs, const T &rhs)
{
   return equal_pointees(lhs, Optional<T>(rhs));
}

template <typename T>
inline bool operator <(const Optional<T> &lhs, const T &rhs)
{
   return less_pointees(lhs, Optional<T>(rhs));
}

template <typename T>
inline bool operator !=(const Optional<T> &lhs, const T &rhs)
{
   return !(lhs == rhs);
}

template <typename T>
inline bool operator >(const Optional<T> &lhs, const T &rhs)
{
   return rhs < lhs;
}

template <typename T>
inline bool operator <=(const Optional<T> &lhs, const T &rhs)
{
   return !(lhs > rhs);
}

template <typename T>
inline bool operator >=(const Optional<T> &lhs, const T &rhs)
{
   return !(lhs < rhs);
}

//
// T vs Optional<T> cases
//
template <typename T>
inline bool operator ==(const T &lhs, const Optional<T> &rhs)
{
   return equal_pointees(Optional<T>(lhs), rhs);
}

template <typename T>
inline bool operator <(const T &lhs, const Optional<T> &rhs)
{
   return less_pointees(Optional<T>(lhs), rhs);
}

template <typename T>
inline bool operator !=(const T &lhs, const Optional<T> &rhs)
{
   return !(lhs == rhs);
}

template <typename T>
inline bool operator >(const T &lhs, const Optional<T> &rhs)
{
   return rhs < lhs;
}

template <typename T>
inline bool operator <=(const T &lhs, const Optional<T> &rhs)
{
   return !(lhs > rhs);
}

template <typename T>
inline bool operator >=(const T &lhs, const Optional<T> &rhs)
{
   return !(lhs < rhs);
}

//
// Optional<T> vs none cases
//
template <typename T>
inline bool operator ==(const Optional<T> &lhs, None rhs)
{
   return !lhs;
}

template <typename T>
inline bool operator <(const Optional<T> &lhs, None rhs)
{
   return equal_pointees(lhs, Optional<T>(rhs));
}

template <typename T>
inline bool operator !=(const Optional<T> &lhs, None rhs)
{
   return static_cast<bool>(lhs);
}

template <typename T>
inline bool operator >(const Optional<T> &lhs, None rhs)
{
   return rhs < lhs;
}

template <typename T>
inline bool operator <=(const Optional<T> &lhs, None rhs)
{
   return !(lhs > rhs);
}

template <typename T>
inline bool operator >=(const Optional<T> &lhs, None rhs)
{
   return !(lhs < rhs);
}

//
// none vs Optional<T> cases
//
template <typename T>
inline bool operator ==(None lhs, const Optional<T> &rhs) noexcept
{
   return !rhs;
}

template <typename T>
inline bool operator <(None lhs, const Optional<T> &rhs)
{
   return equal_pointees(Optional<T>(lhs), rhs);
}

template <typename T>
inline bool operator !=(None lhs, const Optional<T> &rhs) noexcept
{
   return static_cast<bool>(rhs);
}

template <typename T>
inline bool operator >(None lhs, const Optional<T> &rhs)
{
   return rhs < lhs;
}

template <typename T>
inline bool operator <=(None lhs, const Optional<T> &rhs)
{
   return !(lhs > rhs);
}

template <typename T>
inline bool operator >=(None lhs, const Optional<T> &rhs)
{
   return !(lhs < rhs);
}

} // internal
} // optional
} // stdext
} // pdk

#endif // PDK_STDEXT_OPTIONAL_INTERNAL_RELATION_OPS_H
