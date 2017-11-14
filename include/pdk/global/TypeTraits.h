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
// Created by softboy on 2017/11/14.

#ifndef PDK_GLOBAL_TYPE_TRAITS
#define PDK_GLOBAL_TYPE_TRAITS

#include "pdk/global/Global.h"

namespace pdk
{
namespace internal
{

template <typename T>
struct InvertResult : std::integral_constant<bool, !T::value>
{};

template <typename T>
struct IsUnsignedClassicEnum : std::integral_constant<bool, (T(0) < T(-1))>
{};

template <typename T>
struct IsSignedClassicEnum : InvertResult<IsUnsignedClassicEnum<T>>
{};

// some static assert

PDK_STATIC_ASSERT((IsUnsignedClassicEnum<puint8>::value));
PDK_STATIC_ASSERT((!IsUnsignedClassicEnum<pint8>::value));

PDK_STATIC_ASSERT((!IsSignedEnum<puint8>::value));
PDK_STATIC_ASSERT((IsSignedEnum<pint8>::value));

PDK_STATIC_ASSERT((IsUnsignedClassicEnum<puint16>::value));
PDK_STATIC_ASSERT((!IsUnsignedClassicEnum<pint16>::value));

PDK_STATIC_ASSERT((!IsSignedEnum<puint16>::value));
PDK_STATIC_ASSERT((IsSignedEnum<pint16>::value));

PDK_STATIC_ASSERT((IsUnsignedClassicEnum<puint32>::value));
PDK_STATIC_ASSERT((!IsUnsignedClassicEnum<pint32>::value));

PDK_STATIC_ASSERT((!IsSignedEnum<puint32>::value));
PDK_STATIC_ASSERT((IsSignedEnum<pint32>::value));

PDK_STATIC_ASSERT((IsUnsignedClassicEnum<puint64>::value));
PDK_STATIC_ASSERT((!IsUnsignedClassicEnum<pint64>::value));

PDK_STATIC_ASSERT((!IsSignedEnum<puint64>::value));
PDK_STATIC_ASSERT((IsSignedEnum<pint64>::value));

} // internal
} // pdk

#endif
