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
// Created by softboy on 2017/01/10.

// Copyright (C) 2003, Fernando Luis Cacciola Carballal.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/optional for documentation.
//
// You are welcome to contact the author at:
//  fernando_cacciola@hotmail.com
//

#ifndef PDK_STDEXT_UTILITY_OPTIONAL_POINTEE_H
#define PDK_STDEXT_UTILITY_OPTIONAL_POINTEE_H

#include <functional>

namespace pdk {
namespace stdext {


// template<class OP> bool equal_pointees(const OP &lhs, const OP &rhs);
// template<class OP> struct EqualPointees;
//
// Being OP a model of OptionalPointee (either a pointer or an optional):
//
// If both x and y have valid pointees, returns the result of (*lhs == *rhs)
// If only one has a valid pointee, returns false.
// If none have valid pointees, returns true.
// No-throw
template <typename OptionalPointee>
inline bool equal_pointees(const OptionalPointee &lhs, const OptionalPointee &rhs)
{
   return (!lhs) != (!rhs) ? false : ( !lhs ? true : (*lhs) == (*rhs) ) ;
}

template<class OptionalPointee>
struct EqualPointees
{
   using ResultType = bool;
   using FirstArgumentType = OptionalPointee;
   using SecondArgumentType = OptionalPointee;
   
   bool operator() (const OptionalPointee &lhs, const OptionalPointee &rhs) const
   {
      return equal_pointees(lhs, rhs);
   }
};

// template<class OP> bool less_pointees(const OP &lhs, const OP &rhs);
// template<class OP> struct LessPointees;
//
// Being OP a model of OptionalPointee (either a pointer or an optional):
//
// If y has not a valid pointee, returns false.
// ElseIf x has not a valid pointee, returns true.
// ElseIf both x and y have valid pointees, returns the result of (*lhs < *rhs)
// No-throw
template<class OptionalPointee>
inline
bool less_pointees(const OptionalPointee &lhs, const OptionalPointee &rhs)
{
   return !rhs ? false : (!lhs ? true : (*lhs) < (*rhs));
}

template<class OptionalPointee>
struct LessPointees
{
   using ResultType = bool;
   using FirstArgumentType = OptionalPointee;
   using SecondArgumentType = OptionalPointee;
   
   bool operator() (const OptionalPointee &lhs, const OptionalPointee &rhs) const
   {
      return less_pointees(lhs, rhs);
   }
};


} // stdext
} // pdk

#endif // PDK_STDEXT_UTILITY_OPTIONAL_POINTEE_H
