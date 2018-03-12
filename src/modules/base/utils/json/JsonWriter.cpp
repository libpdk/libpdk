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

#include <cmath>
#include "pdk/utils/Locale.h"
#include "pdk/base/utils/json/internal/JsonWriterPrivate.h"
#include "pdk/base/utils/json/internal/JsonPrivate.h"
#include "pdk/base/text/codecs/internal/UtfCodecPrivate.h"

namespace pdk {
namespace utils {
namespace json {
namespace jsonprivate {

namespace codecinternal = pdk::text::codecs::internal;
namespace utf8funcs = codecinternal::Utf8Functions;
using pdk::utils::Locale;

namespace {

void object_content_to_json(const LocalObject *o, ByteArray &json, int indent, bool compact);
void array_content_to_json(const LocalArray *a, ByteArray &json, int indent, bool compact);

static inline uchar hexdig(uint u)
{
   return (u < 0xa ? '0' + u : 'a' + u - 0xa);
}

ByteArray escaped_string(const String &s)
{
   const uchar replacement = '?';
   ByteArray ba(s.length(), pdk::Uninitialized);
   uchar *cursor = reinterpret_cast<uchar *>(const_cast<char *>(ba.getConstRawData()));
   const uchar *baEnd = cursor + ba.length();
   const char16_t *src = reinterpret_cast<const char16_t *>(s.constBegin());
   const char16_t *const end = reinterpret_cast<const char16_t *>(s.constEnd());
   while (src != end) {
      if (cursor >= baEnd - 6) {
         // ensure we have enough space
         int pos = cursor - (const uchar *)ba.getConstRawData();
         ba.resize(ba.size() * 2);
         cursor = (uchar *)ba.getRawData() + pos;
         baEnd = (const uchar *)ba.getConstRawData() + ba.length();
      }
      uint u = *src++;
      if (u < 0x80) {
         if (u < 0x20 || u == 0x22 || u == 0x5c) {
            *cursor++ = '\\';
            switch (u) {
            case 0x22:
               *cursor++ = '"';
               break;
            case 0x5c:
               *cursor++ = '\\';
               break;
            case 0x8:
               *cursor++ = 'b';
               break;
            case 0xc:
               *cursor++ = 'f';
               break;
            case 0xa:
               *cursor++ = 'n';
               break;
            case 0xd:
               *cursor++ = 'r';
               break;
            case 0x9:
               *cursor++ = 't';
               break;
            default:
               *cursor++ = 'u';
               *cursor++ = '0';
               *cursor++ = '0';
               *cursor++ = hexdig(u>>4);
               *cursor++ = hexdig(u & 0xf);
            }
         } else {
            *cursor++ = (uchar)u;
         }
      } else {
         if (utf8funcs::toUtf8<codecinternal::Utf8BaseTraits>(u, cursor, src, end) < 0) {
            *cursor++ = replacement;
         }
      }
   }
   ba.resize(cursor - (const uchar *)ba.getConstRawData());
   return ba;
}

void value_to_json(const jsonprivate::Base *base, const jsonprivate::LocalValue &value, ByteArray &json, int indent, bool compact)
{
   JsonValue::Type type = (JsonValue::Type)(uint)value.m_type;
   switch (type) {
   case JsonValue::Type::Bool:
      json += value.toBoolean() ? "true" : "false";
      break;
   case JsonValue::Type::Double: {
      const double d = value.toDouble(base);
      if (std::isfinite(d)) { // +2 to format to ensure the expected precision
         const double abs = std::abs(d);
         json += ByteArray::number(d, abs == static_cast<pdk::puint64>(abs) ? 'f' : 'g', Locale::FloatingPointShortest);
      } else {
         json += "null"; // +INF || -INF || NaN (see RFC4627#section2.4)
      }
      break;
   }
   case JsonValue::Type::String:
      json += '"';
      json += escaped_string(value.toString(base));
      json += '"';
      break;
   case JsonValue::Type::Array:
      json += compact ? "[" : "[\n";
      array_content_to_json(static_cast<jsonprivate::LocalArray *>(value.base(base)), json, indent + (compact ? 0 : 1), compact);
      json += ByteArray(4 * indent, ' ');
      json += ']';
      break;
   case JsonValue::Type::Object:
      json += compact ? "{" : "{\n";
      object_content_to_json(static_cast<jsonprivate::LocalObject *>(value.base(base)), json, indent + (compact ? 0 : 1), compact);
      json += ByteArray(4 * indent, ' ');
      json += '}';
      break;
   case JsonValue::Type::Null:
   default:
      json += "null";
   }
}

void array_content_to_json(const jsonprivate::LocalArray *array, ByteArray &json, int indent, bool compact)
{
   if (!array || !array->m_length) {
      return;
   }
   ByteArray indentString(4 * indent, ' ');
   uint i = 0;
   while (1) {
      json += indentString;
      value_to_json(array, array->at(i), json, indent, compact);
      if (++i == array->m_length) {
         if (!compact) {
            json += '\n';
         }
         break;
      }
      json += compact ? "," : ",\n";
   }
}

void object_content_to_json(const jsonprivate::LocalObject *object, ByteArray &json, int indent, bool compact)
{
   if (!object || !object->m_length) {
      return;
   }
   ByteArray indentString(4 * indent, ' ');
   uint i = 0;
   while (1) {
      jsonprivate::LocalEntry *e = object->entryAt(i);
      json += indentString;
      json += '"';
      json += escaped_string(e->getKey());
      json += compact ? "\":" : "\": ";
      value_to_json(object, e->m_value, json, indent, compact);
      if (++i == object->m_length) {
         if (!compact) {
            json += '\n';
         }
         break;
      }
      json += compact ? "," : ",\n";
   }
}

} // anonymous namespace

void Writer::objectToJson(const jsonprivate::LocalObject *object, ByteArray &json, int indent, bool compact)
{
    json.reserve(json.size() + (object ? (int)object->m_size : 16));
    json += compact ? "{" : "{\n";
    object_content_to_json(object, json, indent + (compact ? 0 : 1), compact);
    json += ByteArray(4*indent, ' ');
    json += compact ? "}" : "}\n";
}

void Writer::arrayToJson(const jsonprivate::LocalArray *array, ByteArray &json, int indent, bool compact)
{
    json.reserve(json.size() + (array ? (int)array->m_size : 16));
    json += compact ? "[" : "[\n";
    array_content_to_json(array, json, indent + (compact ? 0 : 1), compact);
    json += ByteArray(4*indent, ' ');
    json += compact ? "]" : "]\n";
}

} // jsonprivate
} // json
} // utils
} // pdk
