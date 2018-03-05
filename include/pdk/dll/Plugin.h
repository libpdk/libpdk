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
// Created by softboy on 2018/03/05.

#ifndef PDK_DLL_PLUGIN_H
#define PDK_DLL_PLUGIN_H

#include "pdk/kernel/Object.h"

namespace pdk {
namespace dll {

using pdk::kernel::Object;

#ifndef PDK_EXTERN_C
#  ifdef __cplusplus
#    define PDK_EXTERN_C extern "C"
#  else
#    define PDK_EXTERN_C extern
#  endif
#endif

using PdkPluginInstanceFunc = Object *(*)();
using PdkPluginMetaDataFunc = const char *(*)();

struct PDK_CORE_EXPORT StaticPlugin
{
    // Note: This struct is initialized using an initializer list.
    // As such, it cannot have any new constructors or variables.
    PdkPluginInstanceFunc m_instance;
    PdkPluginMetaDataFunc m_rawMetaData;
//    QJsonObject metaData() const;
};

} // dll
} // pdk

#endif // PDK_DLL_PLUGIN_H
