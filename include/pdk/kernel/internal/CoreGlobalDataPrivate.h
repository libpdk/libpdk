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
// Created by softboy on 2018/02/01.

#ifndef PDK_KERNEL_INTERNAL_CORE_GLOBAL_DATA_PRIVATE_H
#define PDK_KERNEL_INTERNAL_CORE_GLOBAL_DATA_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/os/thread/ReadWriteLock.h"
#include "pdk/base/lang/StringMatcher.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/text/codecs/TextCodec.h"

#include <map>
#include <mutex>

namespace pdk {
namespace kernel {
namespace internal {

using pdk::ds::ByteArray;
//using pdk::ds::StringList;
using pdk::lang::String;
using pdk::text::codecs::TextCodec;
using pdk::os::thread::ReadWriteLock;
using pdk::os::thread::AtomicPointer;

using TextCodecCache = std::map<ByteArray, TextCodec *>;

struct CoreGlobalData {
    CoreGlobalData();
    ~CoreGlobalData();

//    std::map<String, StringList> m_dirSearchPaths;
    ReadWriteLock m_dirSearchPathsLock;

#if PDK_CONFIG(TEXT_CODEC)
    std::list<TextCodec *> m_allCodecs;
    AtomicPointer<TextCodec> m_codecForLocale;
    TextCodecCache m_codecCache;
#endif

    static CoreGlobalData *getInstance();
};

} // internal
} // kernel
} // pdk

#endif // PDK_KERNEL_INTERNAL_CORE_GLOBAL_DATA_PRIVATE_H
