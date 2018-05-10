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
// Created by softboy on 2018/05/10.

#include "pdk/base/os/process/internal/ProcessPrivate.h"
#import <Foundation/Foundation.h>

namespace pdk {
namespace os {
namespace process {

ProcessEnvironment ProcessEnvironment::getSystemEnvironment()
{
   __block ProcessEnvironment env;
   [[[NSProcessInfo processInfo] environment]
         enumerateKeysAndObjectsUsingBlock:^(NSString *name, NSString *value, BOOL *__unused stop) {
      env.m_implPtr->m_vars[ProcessEnvironmentPrivate::Key(String::fromNSString(name).toLocal8Bit())]
            = ProcessEnvironmentPrivate::Value(String::fromNSString(value).toLocal8Bit());
   }];
   return env;
}

} // process
} // os
} // pdk
