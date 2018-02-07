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
// Created by softboy on 2018/02/06.

#ifndef PDK_KERNEL_INTERNAL_SYSTEM_ERROR_PRIVATE_H
#define PDK_KERNEL_INTERNAL_SYSTEM_ERROR_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/lang/String.h"

namespace pdk {
namespace kernel {
namespace internal {

using pdk::lang::String;

class PDK_CORE_EXPORT SystemError
{
public:
   enum class ErrorScope
   {
      NoError,
      StandardLibraryError,
      NativeError
   };
   
   inline SystemError(int error, ErrorScope scope);
   inline SystemError();
   
   inline String toString() const;
   inline ErrorScope getScope() const;
   inline int getError() const;
   
   static String getString(ErrorScope errorScope, int errorCode);
   static String getStdString(int errorCode = -1);
#ifdef PDK_OS_WIN
   static String getWindowsString(int errorCode = -1);
#endif
   
   //data members
   int m_errorCode;
   ErrorScope m_errorScope;
};

SystemError::SystemError(int error, SystemError::ErrorScope scope)
   : m_errorCode(error),
     m_errorScope(scope)
{
   
}

SystemError::SystemError()
   : m_errorCode(0),
     m_errorScope(ErrorScope::NoError)
{}

String SystemError::toString() const
{
   return getString(m_errorScope, m_errorCode);
}

SystemError::ErrorScope SystemError::getScope() const
{
   return m_errorScope;
}

int SystemError::getError() const
{
   return m_errorCode;
}

String pdk_error_string(int code);

} // internal
} // kernel
} // pdk

#endif // PDK_KERNEL_INTERNAL_SYSTEM_ERROR_PRIVATE_H
