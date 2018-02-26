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
// Created by softboy on 2018/02/25.

#include "pdk/kernel/internal/CoreMacPrivate.h"
#include "pdk/base/ds/VarLengthArray.h"
#include <new>

namespace pdk {
namespace kernel {

using pdk::lang::String;

CFString::operator String() const
{
   if (m_string.isEmpty() && m_value) {
      const_cast<CFString*>(this)->m_string = String::fromCFString(m_value);
   }
   return m_string;
}

CFString::operator CFStringRef() const
{
   if (!m_value) {
      const_cast<CFString*>(this)->m_value = m_string.toCFString();
   }
   return m_value;
}

} // kernel
} // pdk
