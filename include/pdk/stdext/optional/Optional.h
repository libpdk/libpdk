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

#ifndef PDK_STDEXT_OPTIONAL_BAD_ACCESS_H
#define PDK_STDEXT_OPTIONAL_BAD_ACCESS_H

#include <type_traits>

namespace pdk {
namespace stdext {
namespace optional {

struct InPlaceInit
{
   struct InitTag {};
   explicit InPlaceInit(InitTag) {}
};

const InPlaceInit in_place_init((InPlaceInit::InitTag()));

struct InPlaceInitIf
{
   struct InitTag{};
   explicit InPlaceInitIf(InitTag) {}
};

const InPlaceInitIf in_place_init_if((InPlaceInitIf::InitTag()));

namespace internal {

struct OptionalTag {};

template <typename T>
class OptionalBase : public OptionalTag
{
private:
   using StorageType = std::aligned_storage<T>;
   using ThisType = OptionalBase<T>;
   
protected:
   using ValueType = T;
   using value_type = ValueType;
   using ReferenceType = T &;
   using ReferenceConstType = T const &;
   using RvalReferenceType = T &&;
   using ReferenceTypeOfTemporaryWrapper = T &&;
   using PointerType = T *;
   using PointerConstType = const T *;
   using ArgumentType = const T &;
   
   using reference_type = ReferenceType;
   using reference_const_type = ReferenceConstType;
   using rval_reference_type = RvalReferenceType;
   using reference_type_of_temporary_wrapper = ReferenceTypeOfTemporaryWrapper;
   using pointer_type = PointerType;
   using pointer_const_type = PointerConstType;
   using argument_type = ArgumentType;
   
   OptionalBase()
      : m_initialized(false)
   {}
   
   
   
   bool m_initialized;
   StorageType m_storage;
};

} // internal

} // optional
} // stdext
} // pdk
