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

#include "pdk/base/utils/json/JsonDocument.h"
#include "pdk/base/utils/json/JsonObject.h"
#include "pdk/base/utils/json/JsonValue.h"
#include "pdk/base/utils/json/JsonArray.h"
#include "pdk/base/utils/json/internal/JsonWriterPrivate.h"
#include "pdk/base/utils/json/internal/JsonParserPrivate.h"
#include "pdk/base/utils/json/internal/JsonPrivate.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/io/Debug.h"

namespace pdk {
namespace utils {
namespace json {

using pdk::io::Debug;
using pdk::io::DebugStateSaver;
using pdk::lang::String;

JsonDocument::JsonDocument()
   : m_data(nullptr)
{}

JsonDocument::JsonDocument(const JsonObject &object)
   : m_data(nullptr)
{
   setObject(object);
}

JsonDocument::JsonDocument(const JsonArray &array)
   : m_data(nullptr)
{
   setArray(array);
}

JsonDocument::JsonDocument(jsonprivate::Data *data)
   : m_data(data)
{
   PDK_ASSERT(m_data);
   m_data->m_ref.ref();
}

JsonDocument::~JsonDocument()
{
   if (m_data && !m_data->m_ref.deref()) {
      delete m_data;
   }
}

JsonDocument::JsonDocument(const JsonDocument &other)
{
   m_data = other.m_data;
   if (m_data) {
      m_data->m_ref.ref();
   }
}

JsonDocument &JsonDocument::operator =(const JsonDocument &other)
{
   if (m_data != other.m_data) {
      if (m_data && !m_data->m_ref.deref()) {
         delete m_data;
      }
      m_data = other.m_data;
      if (m_data) {
         m_data->m_ref.ref();
      }
   }
   return *this;
}

JsonDocument JsonDocument::fromRawData(const char *data, int size, DataValidation validation)
{
   if (pdk::uintptr(data) & 3) {
      warning_stream("JsonDocument::fromRawData: data has to have 4 byte alignment");
      return JsonDocument();
   }
   jsonprivate::Data *d = new jsonprivate::Data(const_cast<char *>(data), size);
   d->m_ownsData = false;
   if (validation != DataValidation::BypassValidation && !d->valid()) {
      delete d;
      return JsonDocument();
   }
   return JsonDocument(d);
}

const char *JsonDocument::getRawData(int *size) const
{
   if (!m_data) {
      *size = 0;
      return 0;
   }
   *size = m_data->m_alloc;
   return m_data->m_rawData;
}

JsonDocument JsonDocument::fromBinaryData(const ByteArray &data, DataValidation validation)
{
   if (data.size() < (int)(sizeof(jsonprivate::Header) + sizeof(jsonprivate::Base))) {
      return JsonDocument();
   }
   jsonprivate::Header h;
   memcpy(&h, data.getConstRawData(), sizeof(jsonprivate::Header));
   jsonprivate::Base root;
   memcpy(&root, data.getConstRawData() + sizeof(jsonprivate::Header), sizeof(jsonprivate::Base));
   
   // do basic checks here, so we don't try to allocate more memory than we can.
   if (h.m_tag != JsonDocument::BinaryFormatTag || h.m_version != 1u ||
       sizeof(jsonprivate::Header) + root.m_size > (uint)data.size()) {
      return JsonDocument();
   }
   const uint size = sizeof(jsonprivate::Header) + root.m_size;
   char *raw = (char *)malloc(size);
   if (!raw) {
      return JsonDocument();
   }
   memcpy(raw, data.getConstRawData(), size);
   jsonprivate::Data *d = new jsonprivate::Data(raw, size);
   if (validation != DataValidation::BypassValidation && !d->valid()) {
      delete d;
      return JsonDocument();
   }
   return JsonDocument(d);
}

JsonDocument JsonDocument::fromAny(const std::any &anyValue)
{
   JsonDocument doc;
   const std::type_info &typeInfo = anyValue.type();
   if (typeInfo == typeid(AnyMap)) {
      doc.setObject(JsonObject::fromAnyMap(std::any_cast<AnyMap>(anyValue)));
   } else if (typeInfo == typeid(AnyList)) {
      doc.setArray(JsonArray::fromAnyList(std::any_cast<AnyList>(anyValue)));
   } else if (typeInfo == typeid(StringList)) {
      doc.setArray(JsonArray::fromStringList(std::any_cast<StringList>(anyValue)));
   }
   return doc;
}

std::any JsonDocument::toAny() const
{
   if (!m_data) {
      return std::any();
   }
   if (m_data->m_header->getRoot()->isArray()) {
      return JsonArray(m_data, static_cast<jsonprivate::LocalArray *>(m_data->m_header->getRoot())).toAnyList();
   } else {
      return JsonObject(m_data, static_cast<jsonprivate::LocalObject *>(m_data->m_header->getRoot())).toAnyMap();
   }  
}

#ifndef PDK_JSON_READONLY
ByteArray JsonDocument::toJson() const
{
   return toJson(JsonFormat::Indented);
}
#endif

#ifndef PDK_JSON_READONLY
ByteArray JsonDocument::toJson(JsonFormat format) const
{
   ByteArray json;
   if (!m_data) {
      return json;
   }
   if (m_data->m_header->getRoot()->isArray()) {
      jsonprivate::Writer::arrayToJson(static_cast<jsonprivate::LocalArray *>(m_data->m_header->getRoot()), json, 0, (format == JsonFormat::Compact));
   } else {
      jsonprivate::Writer::objectToJson(static_cast<jsonprivate::LocalObject *>(m_data->m_header->getRoot()), json, 0, (format == JsonFormat::Compact));
   }   
   return json;
}
#endif

JsonDocument JsonDocument::fromJson(const ByteArray &json, JsonParseError *error)
{
   jsonprivate::Parser parser(json.getConstRawData(), json.length());
   return parser.parse(error);
}

bool JsonDocument::isEmpty() const
{
   if (!m_data) {
      return true;
   }
   return false;
}

ByteArray JsonDocument::toBinaryData() const
{
   if (!m_data || !m_data->m_rawData) {
      return ByteArray();
   }
   return ByteArray(m_data->m_rawData, m_data->m_header->getRoot()->m_size + sizeof(jsonprivate::Header));
}

bool JsonDocument::isArray() const
{
   if (!m_data) {
      return false;
   }
   jsonprivate::Header *h = (jsonprivate::Header *)m_data->m_rawData;
   return h->getRoot()->isArray();
}

bool JsonDocument::isObject() const
{
   if (!m_data) {
      return false;
   } 
   jsonprivate::Header *h = (jsonprivate::Header *)m_data->m_rawData;
   return h->getRoot()->isObject();
}

JsonObject JsonDocument::getObject() const
{
   if (m_data) {
      jsonprivate::Base *base = m_data->m_header->getRoot();
      if (base->isObject()) {
         return JsonObject(m_data, static_cast<jsonprivate::LocalObject *>(base));
      }
   }
   return JsonObject();
}

JsonArray JsonDocument::getArray() const
{
   if (m_data) {
      jsonprivate::Base *base = m_data->m_header->getRoot();
      if (base->isArray()) {
         return JsonArray(m_data, static_cast<jsonprivate::LocalArray *>(base));
      }
   }
   return JsonArray();
}

void JsonDocument::setObject(const JsonObject &object)
{
   if (m_data && !m_data->m_ref.deref()) {
      delete m_data;
   }
   m_data = object.m_data;
   
   if (!m_data) {
      m_data = new jsonprivate::Data(0, JsonValue::Type::Object);
   } else if (m_data->m_compactionCounter || object.m_object != m_data->m_header->getRoot()) {
      JsonObject o(object);
      if (m_data->m_compactionCounter){
         o.compact();
      } else {
         o.detach();
      } 
      m_data = o.m_data;
      m_data->m_ref.ref();
      return;
   }
   m_data->m_ref.ref();
}

void JsonDocument::setArray(const JsonArray &array)
{
   if (m_data && !m_data->m_ref.deref()) {
      delete m_data;
   }
   m_data = array.m_data;
   if (!m_data) {
      m_data = new jsonprivate::Data(0, JsonValue::Type::Array);
   } else if (m_data->m_compactionCounter || array.m_array != m_data->m_header->getRoot()) {
      JsonArray a(array);
      if (m_data->m_compactionCounter) {
         a.compact();
      } else {
         a.detach();
      }
      m_data = a.m_data;
      m_data->m_ref.ref();
      return;
   }
   m_data->m_ref.ref();
}

const JsonValue JsonDocument::operator[](const String &key) const
{
   if (!isObject()) {
      return JsonValue(JsonValue::Type::Undefined);
   }
   return getObject().getValue(key);
}

const JsonValue JsonDocument::operator[](Latin1String key) const
{
   if (!isObject()) {
      return JsonValue(JsonValue::Type::Undefined);
   }
   return getObject().getValue(key);
}

const JsonValue JsonDocument::operator[](int i) const
{
   if (!isArray()) {
      return JsonValue(JsonValue::Type::Undefined);
   }
   return getArray().at(i);
}

bool JsonDocument::operator==(const JsonDocument &other) const
{
   if (m_data == other.m_data) {
      return true;
   }
   if (!m_data || !other.m_data) {
      return false;
   }
   if (m_data->m_header->getRoot()->isArray() != other.m_data->m_header->getRoot()->isArray()) {
      return false;
   }
   if (m_data->m_header->getRoot()->isObject()) {
      return JsonObject(m_data, static_cast<jsonprivate::LocalObject *>(m_data->m_header->getRoot()))
            == JsonObject(other.m_data, static_cast<jsonprivate::LocalObject *>(other.m_data->m_header->getRoot()));
   } else {
      return JsonArray(m_data, static_cast<jsonprivate::LocalArray *>(m_data->m_header->getRoot()))
            == JsonArray(other.m_data, static_cast<jsonprivate::LocalArray *>(other.m_data->m_header->getRoot()));
   }
}

bool JsonDocument::isNull() const
{
   return (m_data == nullptr);
}

#if !defined(PDK_NO_DEBUG_STREAM) && !defined(PDK_JSON_READONLY)
Debug operator<<(Debug dbg, const JsonDocument &other)
{
   DebugStateSaver saver(dbg);
   if (!other.m_data) {
      dbg << "JsonDocument()";
      return dbg;
   }
   ByteArray json;
   if (other.m_data->m_header->getRoot()->isArray()) {
      jsonprivate::Writer::arrayToJson(static_cast<jsonprivate::LocalArray *>(other.m_data->m_header->getRoot()), json, 0, true);
   } else {
      jsonprivate::Writer::objectToJson(static_cast<jsonprivate::LocalObject *>(other.m_data->m_header->getRoot()), json, 0, true);
   }
   dbg.nospace() << "JsonDocument("
                 << json.getConstRawData() // print as utf-8 string without extra quotation marks
                 << ')';
   return dbg;
}
#endif

} // json
} // utils
} // pdk
