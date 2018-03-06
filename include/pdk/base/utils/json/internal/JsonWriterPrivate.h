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

#ifndef PDK_M_BASE_JSON_INTERNAL_JSON_WRITER_PRIVATE_H
#define PDK_M_BASE_JSON_INTERNAL_JSON_WRITER_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/utils/json/JsonValue.h"

namespace pdk {

// forward declare class with namespace
namespace ds {
class ByteArray;
} // ds

namespace utils {
namespace json {
namespace jsonprivate {

using pdk::ds::ByteArray;

class Writer
{
public:
    static void objectToJson(const LocalObject *object, ByteArray &json, int indent, bool compact = false);
    static void arrayToJson(const LocalArray *array, ByteArray &json, int indent, bool compact = false);
};

} // jsonprivate
} // json
} // utils
} // pdk

#endif // PDK_M_BASE_JSON_INTERNAL_JSON_WRITER_PRIVATE_H
