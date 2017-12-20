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

#ifndef PDK_M_BASE_LANG_STRING_H
#define PDK_M_BASE_LANG_STRING_H

#include "pdk/global/Global.h"
#include "pdk/base/lang/Character.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/utils/RefCount.h"

#include <stdarg.h>
#include <string>
#include <iterator>

#ifdef truncate
#error String.h must be included before any header file that defines truncate
#endif

#ifdef PDK_OS_DARWIN
PDK_FORWARD_DECLARE_CF_TYPE(CFString);
PDK_FORWARD_DECLARE_OBJC_CLASS(NSString);
#endif

namespace pdk {
namespace lang {

using pdk::ds::ByteArray;

class CharacterRef;
class String;
class StringRef;
class StringList;

class Latin1String
{
public:
   constexpr inline Latin1String() noexcept
      : m_size(0),
        m_data(nullptr)
   {}
   
   constexpr inline explicit Latin1String(const char *str) noexcept
      : m_size(str ? std::strlen(str) : 0),
        m_data(str)
   {}
   
   constexpr inline explicit Latin1String(const char *str, int size) noexcept
      : m_size(size),
        m_data(str)
   {}
   
   inline explicit Latin1String(const ByteArray &str) noexcept
      : m_size(pdk::strnlen(str.getConstRawData(), str.size())),
        m_data(str.getConstRawData())
   {}
   
   constexpr const char *latin1() const noexcept
   {
      return m_data;
   }
   
   constexpr int size() const noexcept
   {
      return m_size;
   }
   
   constexpr const char *getRawData() const noexcept
   {
      return m_data;
   }
   
   constexpr Latin1Character at(int i) const
   {
      return Latin1Character(m_data[i]);
   }
   
   constexpr Latin1Character operator[](int i) const
   {
      return at(i);
   }
   
   constexpr Latin1String substring(int pos) const
   {
      return Latin1String(m_data + pos, m_size - pos);
   }
   
   constexpr Latin1String substring(int pos, int n) const
   {
      return Latin1String(m_data + pos, n);
   }
   
   constexpr Latin1String left(int n) const
   {
      return Latin1String(m_data, n);
   }
   
   constexpr Latin1String right(int n) const
   {
      return Latin1String(m_data + m_size - n, n);
   }
   
   inline bool operator==(const String &s) const noexcept;
   inline bool operator!=(const String &s) const noexcept;
   inline bool operator>(const String &s) const noexcept;
   inline bool operator<(const String &s) const noexcept;
   inline bool operator>=(const String &s) const noexcept;
   inline bool operator<=(const String &s) const noexcept;
   
private:
   int m_size;
   const char *m_data;
};

class PDK_CORE_EXPORT String
{
public:
   String()
   {}
};

} // lang
} // pdk

#endif // PDK_M_BASE_LANG_STRING_H
