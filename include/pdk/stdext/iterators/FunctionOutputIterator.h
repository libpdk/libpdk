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
// Created by softboy on 2017/01/20.

// (C) Copyright Jeremy Siek 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History:

// 27 Feb 2001   Jeremy Siek
//      Initial checkin.

#ifndef PDK_STDEXT_ITERATORS_FUNCTION_OUTPUT_ITERATOR_H
#define PDK_STDEXT_ITERATORS_FUNCTION_OUTPUT_ITERATOR_H

#include <iterator>

namespace pdk {
namespace stdext {
namespace iterators {

template <typename UnaryFunction>
class FunctionOutputIterator
{
   using Self = FunctionOutputIterator;
public:
   using IteratorCategory = std::output_iterator_tag;
   using ValueType = void;
   using DifferenceType = void;
   using Pointer = void;
   using Reference = void;
   
   using iterator_category = IteratorCategory;
   using value_type = ValueType;
   using difference_type = DifferenceType;
   using pointer = Pointer;
   using reference = Reference;
   
   explicit FunctionOutputIterator()
   {}
   
   explicit FunctionOutputIterator(const UnaryFunction &func)
      : m_func(func)
   {}
   
   struct OutputProxy
   {
      OutputProxy(UnaryFunction &func)
         : m_func(func)
      {}
      
      template <typename T>
      OutputProxy &operator=(const T &value)
      {
         m_func(value);
         return *this;
      }
      UnaryFunction &m_func;
   };
   
   OutputProxy operator*()
   { 
      return OutputProxy(m_func);
   }
   
   Self &operator++()
   {
      return *this;
   }
   
   Self &operator++(int)
   {
      return *this;
   }
   
private:
   UnaryFunction m_func;
};

template <typename UnaryFunction>
inline FunctionOutputIterator<UnaryFunction>
make_function_output_iterator(const UnaryFunction &func = UnaryFunction())
{
   return FunctionOutputIterator<UnaryFunction>(func);
}

} // iterators
} // stdext
} // pdk

#endif // PDK_STDEXT_ITERATORS_FUNCTION_OUTPUT_ITERATOR_H
