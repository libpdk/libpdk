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

#include "pdk/base/utils/json/JsonObject.h"
#include "pdk/base/utils/json/JsonValue.h"
#include "pdk/base/utils/json/JsonArray.h"
#include "pdk/base/utils/json/internal/JsonPrivate.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/io/Debug.h"

#include <any>

namespace pdk {
namespace utils {
namespace json {

using pdk::lang::String;
using pdk::lang::StringDataPtr;
using pdk::io::Debug;
using pdk::io::DebugStateSaver;

JsonValue::JsonValue(Type type)
   : m_ui(0),
     m_data(nullptr),
     m_type(type)
{}

JsonValue::JsonValue(jsonprivate::Data *data, jsonprivate::Base *base, const jsonprivate::LocalValue &value)
   : m_data(nullptr)
{
   m_type = (Type)(uint)value.m_type;
   switch (m_type) {
   case Type::Undefined:
   case Type::Null:
      m_dbl = 0;
      break;
   case Type::Bool:
      m_b = value.toBoolean();
      break;
   case Type::Double:
      m_dbl = value.toDouble(base);
      break;
   case Type::String: {
      String s = value.toString(base);
      m_stringData = s.getDataPtr();
      m_stringData->m_ref.ref();
      break;
   }
   case Type::Array:
   case Type::Object:
      m_data = data;
      this->m_base = value.base(base);
      break;
   }
   if (m_data) {
      m_data->m_ref.ref();
   }
}

JsonValue::JsonValue(bool value)
   : m_data(nullptr),
     m_type(Type::Bool)
{
   m_b = value;
}

JsonValue::JsonValue(double value)
   : m_data(nullptr), 
     m_type(Type::Double)
{
   m_dbl = value;
}

JsonValue::JsonValue(int value)
   : m_data(nullptr),
     m_type(Type::Double)
{
   m_dbl = value;
}

JsonValue::JsonValue(pdk::pint64 value)
   : m_data(nullptr),
     m_type(Type::Double)
{
   m_dbl = double(value);
}

JsonValue::JsonValue(const String &value)
   : m_data(nullptr),
     m_type(Type::String)
{
   stringDataFromStringHelper(value);
}

void JsonValue::stringDataFromStringHelper(const String &string)
{
   m_stringData = *(StringData **)(const_cast<String *>(&string));
   m_stringData->m_ref.ref();
}

JsonValue::JsonValue(Latin1String s)
   : m_data(nullptr),
     m_type(Type::String)
{
   // ### FIXME: Avoid creating the temp String below
   String str(s);
   stringDataFromStringHelper(str);
}

JsonValue::JsonValue(const JsonArray &value)
   : m_data(value.m_data),
     m_type(Type::Array)
{
   m_base = value.m_array;
   if (m_data) {
      m_data->m_ref.ref();
   }
}

JsonValue::JsonValue(const JsonObject &value)
   : m_data(value.m_data),
     m_type(Type::Object)
{
   m_base = value.m_object;
   if (m_data) {
      m_data->m_ref.ref();
   }
}

JsonValue::~JsonValue()
{
   if (m_type == Type::String && m_stringData && !m_stringData->m_ref.deref()) {
      free(m_stringData);
   }
   if (m_data && !m_data->m_ref.deref()) {
      delete m_data;
   }
}

JsonValue::JsonValue(const JsonValue &other)
{
   m_type = other.m_type;
   m_data = other.m_data;
   m_ui = other.m_ui;
   if (m_data) {
      m_data->m_ref.ref();
   }
   if (m_type == Type::String && m_stringData) {
      m_stringData->m_ref.ref();
   }
}

JsonValue &JsonValue::operator =(const JsonValue &other)
{
   JsonValue copy(other);
   swap(copy);
   return *this;
}

JsonValue JsonValue::fromStdAny(const std::any &anyValue)
{
   const std::type_info &typeInfo = anyValue.type();
   if (typeInfo == typeid(nullptr)) {
      return JsonValue(Type::Null);
   } else if (typeInfo == typeid(bool)) {
      return JsonValue(std::any_cast<bool>(anyValue));
   } else if (typeInfo == typeid(int) ||
              typeInfo == typeid(float) ||
              typeInfo == typeid(double) ||
              typeInfo == typeid(pdk::plonglong) ||
              typeInfo == typeid(pdk::pulonglong) ||
              typeInfo == typeid(uint)) {
      return JsonValue(std::any_cast<double>(anyValue));
   } else if (typeInfo == typeid(String)) {
      return JsonValue(std::any_cast<String>(anyValue));
   } else if (typeInfo == typeid(StringList)) {
      return JsonValue(JsonArray::fromStringList(std::any_cast<StringList>(anyValue)));
   } else if (typeInfo == typeid(AnyList)) {
      return JsonValue(JsonArray::fromAnyList(std::any_cast<AnyList>(anyValue)));
   } else if (typeInfo == typeid(AnyMap)) {
      return JsonValue(JsonObject::fromAnyMap(std::any_cast<AnyMap>(anyValue)));
   } else if (typeInfo == typeid(JsonValue)) {
      return std::any_cast<JsonValue>(anyValue);
   } else if (typeInfo == typeid(JsonObject)) {
      return std::any_cast<JsonObject>(anyValue);
   } else if (typeInfo == typeid(JsonArray)) {
      return std::any_cast<JsonArray>(anyValue);
   } else if (typeInfo == typeid(JsonDocument)) {
      JsonDocument doc = std::any_cast<JsonDocument>(anyValue);
      return doc.isArray() ? JsonValue(doc.getArray()) : JsonValue(doc.getObject());
   }
   String string = std::any_cast<String>(anyValue);
   if (string.isEmpty()) {
      return JsonValue();
   }
   return JsonValue(string);
}

std::any JsonValue::toStdAny() const
{
   switch (m_type) {
   case Type::Bool:
      return m_b;
   case Type::Double:
      return m_dbl;
   case Type::String:
      return toString();
   case Type::Array:
      return m_data ?
               JsonArray(m_data, static_cast<jsonprivate::LocalArray *>(m_base)).toAnyList() :
               AnyList();
   case Type::Object:
      return m_data ?
               JsonObject(m_data, static_cast<jsonprivate::LocalObject *>(m_base)).toAnyMap() :
               AnyMap();
   case Type::Null:
      return std::any(nullptr);
   case Type::Undefined:
      break;
   }
   return std::any();
}

JsonValue::Type JsonValue::getType() const
{
   return m_type;
}

bool JsonValue::toBool(bool defaultValue) const
{
   if (m_type != Type::Bool) {
      return defaultValue;
   }
   return m_b;
}

int JsonValue::toInt(int defaultValue) const
{
   if (m_type == Type::Double && int(m_dbl) == m_dbl) {
      return int(m_dbl);
   }
   return defaultValue;
}

double JsonValue::toDouble(double defaultValue) const
{
   if (m_type != Type::Double) {
      return defaultValue;
   }
   return m_dbl;
}

String JsonValue::toString(const String &defaultValue) const
{
   if (m_type != Type::String) {
      return defaultValue;
   }
   m_stringData->m_ref.ref(); // the constructor below doesn't add a ref.
   StringDataPtr holder = { m_stringData };
   return String(holder);
}

String JsonValue::toString() const
{
   if (m_type != Type::String) {
      return String();
   }
   m_stringData->m_ref.ref(); // the constructor below doesn't add a ref.
   StringDataPtr holder = { m_stringData };
   return String(holder);
}

JsonArray JsonValue::toArray(const JsonArray &defaultValue) const
{
   if (!m_data || m_type != Type::Array) {
      return defaultValue;
   }
   return JsonArray(m_data, static_cast<jsonprivate::LocalArray *>(m_base));
}

JsonArray JsonValue::toArray() const
{
   return toArray(JsonArray());
}

JsonObject JsonValue::toObject(const JsonObject &defaultValue) const
{
   if (!m_data || m_type != Type::Object) {
      return defaultValue;
   }
   return JsonObject(m_data, static_cast<jsonprivate::LocalObject *>(m_base));
}

JsonObject JsonValue::toObject() const
{
   return toObject(JsonObject());
}

const JsonValue JsonValue::operator[](const String &key) const
{
   if (!isObject()) {
      return JsonValue(JsonValue::Type::Undefined);
   }
   return toObject().getValue(key);
}

const JsonValue JsonValue::operator[](Latin1String key) const
{
   if (!isObject()) {
      return JsonValue(JsonValue::Type::Undefined);
   }
   return toObject().getValue(key);
}

const JsonValue JsonValue::operator[](int i) const
{
   if (!isArray()) {
      return JsonValue(JsonValue::Type::Undefined);
   }
   return toArray().at(i);
}

bool JsonValue::operator==(const JsonValue &other) const
{
   if (m_type != other.m_type) {
      return false;
   }
   switch (m_type) {
   case Type::Undefined:
   case Type::Null:
      break;
   case Type::Bool:
      return m_b == other.m_b;
   case Type::Double:
      return m_dbl == other.m_dbl;
   case Type::String:
      return toString() == other.toString();
   case Type::Array:
      if (m_base == other.m_base) {
         return true;
      }
      if (!m_base) {
         return !other.m_base->m_length;
      }
      if (!other.m_base) {
         return !m_base->m_length;
      }
      return JsonArray(m_data, static_cast<jsonprivate::LocalArray *>(m_base))
            == JsonArray(other.m_data, static_cast<jsonprivate::LocalArray *>(other.m_base));
   case Type::Object:
      if (m_base == other.m_base) {
         return true;
      }
      if (!m_base) {
         return !other.m_base->m_length;
      }
      if (!other.m_base) {
         return !m_base->m_length;
      }
      return JsonObject(m_data, static_cast<jsonprivate::LocalObject *>(m_base))
            == JsonObject(other.m_data, static_cast<jsonprivate::LocalObject *>(other.m_base));
   }
   return true;
}

bool JsonValue::operator!=(const JsonValue &other) const
{
   return !(*this == other);
}

void JsonValue::detach()
{
   if (!m_data) {
      return;
   }
   jsonprivate::Data *x = m_data->clone(m_base);
   x->m_ref.ref();
   if (!m_data->m_ref.deref()) {
      delete m_data;
   } 
   m_data = x;
   m_base = static_cast<jsonprivate::LocalObject *>(m_data->m_header->getRoot());
}


JsonValueRef &JsonValueRef::operator =(const JsonValue &value)
{
   if (m_isObject) {
      m_object->setValueAt(m_index, value);
   } else {
      m_array->replace(m_index, value);
   }
   return *this;
}

JsonValueRef &JsonValueRef::operator =(const JsonValueRef &ref)
{
   if (m_isObject) {
      m_object->setValueAt(m_index, ref);
   } else {
      m_array->replace(m_index, ref);
   }
   return *this;
}

std::any JsonValueRef::toStdAny() const
{
   return toValue().toStdAny();
}

JsonArray JsonValueRef::toArray() const
{
   return toValue().toArray();
}

JsonObject JsonValueRef::toObject() const
{
   return toValue().toObject();
}

JsonValue JsonValueRef::toValue() const
{
   if (!m_isObject) {
      return m_array->at(m_index);
   }
   return m_object->valueAt(m_index);
}

#if !defined(PDK_NO_DEBUG_STREAM) && !defined(PDK_JSON_READONLY)
Debug operator<<(Debug dbg, const JsonValue &value)
{
   DebugStateSaver saver(dbg);
   switch (value.m_type) {
   case JsonValue::Type::Undefined:
      dbg << "JsonValue(undefined)";
      break;
   case JsonValue::Type::Null:
      dbg << "JsonValue(null)";
      break;
   case JsonValue::Type::Bool:
      dbg.nospace() << "JsonValue(bool, " << value.toBool() << ')';
      break;
   case JsonValue::Type::Double:
      dbg.nospace() << "JsonValue(double, " << value.toDouble() << ')';
      break;
   case JsonValue::Type::String:
      dbg.nospace() << "JsonValue(string, " << value.toString() << ')';
      break;
   case JsonValue::Type::Array:
      dbg.nospace() << "JsonValue(array, ";
      dbg << value.toArray();
      dbg << ')';
      break;
   case JsonValue::Type::Object:
      dbg.nospace() << "JsonValue(object, ";
      dbg << value.toObject();
      dbg << ')';
      break;
   }
   return dbg;
}
#endif

} // json
} // utils
} // pdk
