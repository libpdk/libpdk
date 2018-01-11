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

#ifndef PDK_STDEXT_OPTIONAL_OPTIONAL_IO_H
#define PDK_STDEXT_OPTIONAL_OPTIONAL_IO_H

#include "pdk/stdext/optional/OptionalIo.h"

#include <istream>
#include <ostream>

namespace pdk {
namespace stdext {
namespace optional {

template<typename CharType, typename CharTrait>
inline std::basic_ostream<CharType, CharTrait>&
operator <<(std::basic_ostream<CharType, CharTrait>& out, None)
{
   if (out.good()) {
      out << "--";
   }
   return out;
}

template<typename CharType, typename CharTrait, typename T>
inline std::basic_ostream<CharType, CharTrait>&
operator <<(std::basic_ostream<CharType, CharTrait>& out, const Optional<T> &value)
{
   if (out.good()) {
      if (!value) {
         out << "--" ;
      } else {
         out << ' ' << *value;
      }
   }
   return out;
}

template<typename CharType, typename CharTrait, typename T>
inline std::basic_istream<CharType, CharTrait>&
operator>>(std::basic_istream<CharType, CharTrait> &in, Optional<T> &value)
{
   if (in.good()) {
      int d = in.get();
      if (d == ' ') {
         T x;
         in >> x;
         value = std::move(x);
      } else {
         if (d == '-') {
            d = in.get();
            if (d == '-') {
               value = none;
               return in;
            }
         }
         in.setstate(std::ios::failbit);
      }
   }
   return in;
}


} // optional
} // stdext
} // pdk

#endif // PDK_STDEXT_OPTIONAL_OPTIONAL_IO_H
