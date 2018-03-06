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
#include "pdk/base/utils/json/internal/JsonWriterPrivate.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/io/Debug.h"
#include <vector>
#include <any>

namespace pdk {
namespace utils {
namespace json {

using pdk::ds::ByteArray;
using pdk::lang::String;
using pdk::io::DebugStateSaver;

JsonArray::JsonArray()
   : m_data(nullptr),
     m_array(nullptr)
{}

JsonArray::JsonArray(jsonprivate::Data *data, jsonprivate::LocalArray *array)
   : m_data(data),
     m_array(array)
{
   PDK_ASSERT(data);
   PDK_ASSERT(array);
   m_data->m_ref.ref();
}

void JsonArray::initialize()
{
   m_data = nullptr;
   m_array = nullptr;
}

JsonArray::~JsonArray()
{
   if (m_data && !m_data->m_ref.deref()) {
      delete m_data;
   }      
}

JsonArray::JsonArray(const JsonArray &other)
{
   m_data = other.m_data;
   m_array = other.m_array;
   if (m_data) {
      m_data->m_ref.ref();
   }
}

JsonArray &JsonArray::operator =(const JsonArray &other)
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
   m_array = other.m_array;
   return *this;
}

JsonArray JsonArray::fromStringList(const StringList &list)
{
   JsonArray array;
   for (StringList::const_iterator iter = list.cbegin(); iter != list.cend(); ++iter) {
      array.append(JsonValue(*iter));
   }
   return array;
}

JsonArray JsonArray::fromAnyList(const AnyList &list)
{
   JsonArray array;
   if (list.empty()) {
      return array;
   }
   array.detach(1024);
   std::vector<jsonprivate::LocalValue> values;
   values.reserve(list.size());
   jsonprivate::LocalValue *valueData = values.data();
   uint currentOffset = sizeof(jsonprivate::Base);
   for (size_t i = 0; i < list.size(); ++i) {
      auto iter = list.begin();
      std::advance(iter, i);
      JsonValue val = JsonValue::fromStdAny(*iter);
      bool latinOrIntValue;
      int valueSize = jsonprivate::LocalValue::requiredStorage(val, &latinOrIntValue);
      if (!array.detach(valueSize)) {
         return JsonArray();
      }
      jsonprivate::LocalValue *value = valueData + i;
      value->m_type = pdk::as_integer<JsonValue::Type>((val.m_type == JsonValue::Type::Undefined ? JsonValue::Type::Null : val.m_type));
      value->m_latinOrIntValue = latinOrIntValue;
      value->m_latinKey = false;
      value->m_value = jsonprivate::LocalValue::valueToStore(val, currentOffset);
      if (valueSize) {
         jsonprivate::LocalValue::copyData(val, (char *)array.m_array + currentOffset, latinOrIntValue);
      }
      currentOffset += valueSize;
      array.m_array->m_size = currentOffset;
   }
   // write table
   array.m_array->m_tableOffset = currentOffset;
   if (!array.detach(sizeof(jsonprivate::offset)*values.size())) {
      return JsonArray();
   }
   memcpy(array.m_array->getTable(), values.data(), values.size() * sizeof(uint));
   array.m_array->m_length = values.size();
   array.m_array->m_size = currentOffset + sizeof(jsonprivate::offset)*values.size();
   return array;
}

AnyList JsonArray::toAnyList() const
{
   AnyList list;
   if (m_array) {
      list.resize(m_array->m_length);
      for (int i = 0; i < (int)m_array->m_length; ++i) {
         list.push_back(JsonValue(m_data, m_array, m_array->at(i)).toStdAny());
      } 
   }
   return list;
}

int JsonArray::getSize() const
{
   if (!m_data) {
      return 0;
   }
   return (int)m_array->m_length;
}

bool JsonArray::isEmpty() const
{
   if (!m_data) {
      return true;
   }
   return !m_array->m_length;
}

JsonValue JsonArray::at(int i) const
{
   if (!m_array || i < 0 || i >= (int)m_array->m_length) {
      return JsonValue(JsonValue::Type::Undefined);
   }
   return JsonValue(m_data, m_array, m_array->at(i));
}

JsonValue JsonArray::first() const
{
   return at(0);
}

JsonValue JsonArray::last() const
{
   return at(m_array ? (m_array->m_length - 1) : 0);
}

void JsonArray::prepend(const JsonValue &value)
{
   insert(0, value);
}

void JsonArray::append(const JsonValue &value)
{
   insert(m_array ? (int)m_array->m_length : 0, value);
}

void JsonArray::removeAt(int i)
{
   if (!m_array || i < 0 || i >= (int)m_array->m_length) {
      return;
   }
   detach();
   m_array->removeItems(i, 1);
   ++m_data->m_compactionCounter;
   if (m_data->m_compactionCounter > 32u && m_data->m_compactionCounter >= unsigned(m_array->m_length) / 2u) {
      compact();
   }
   
}

JsonValue JsonArray::takeAt(int i)
{
   if (!m_array || i < 0 || i >= (int)m_array->m_length) {
      return JsonValue(JsonValue::Type::Undefined);
   }
   JsonValue v(m_data, m_array, m_array->at(i));
   removeAt(i); // detaches
   return v;
}

void JsonArray::insert(int i, const JsonValue &value)
{
   PDK_ASSERT (i >= 0 && i <= (m_array ? (int)m_array->m_length : 0));
   JsonValue val = value;
   bool compressed;
   int valueSize = jsonprivate::LocalValue::requiredStorage(val, &compressed);
   
   if (!detach(valueSize + sizeof(jsonprivate::LocalValue))) {
      return;
   }
   if (!m_array->m_length) {
      m_array->m_tableOffset = sizeof(jsonprivate::LocalArray);
   }
   int valueOffset = m_array->reserveSpace(valueSize, i, 1, false);
   if (!valueOffset) {
      return;
   }
   jsonprivate::LocalValue &v = (*m_array)[i];
   v.m_type = pdk::as_integer<JsonValue::Type>((val.m_type == JsonValue::Type::Undefined ? JsonValue::Type::Null : val.m_type));
   v.m_latinOrIntValue = compressed;
   v.m_latinKey = false;
   v.m_value = jsonprivate::LocalValue::valueToStore(val, valueOffset);
   if (valueSize) {
      jsonprivate::LocalValue::copyData(val, (char *)m_array + valueOffset, compressed);
   } 
}

void JsonArray::replace(int i, const JsonValue &value)
{
   PDK_ASSERT (m_array && i >= 0 && i < (int)(m_array->m_length));
   JsonValue val = value;
   bool compressed;
   int valueSize = jsonprivate::LocalValue::requiredStorage(val, &compressed);
   if (!detach(valueSize)) {
      return;
   }
   if (!m_array->m_length) {
      m_array->m_tableOffset = sizeof(jsonprivate::LocalArray);
   }
   int valueOffset = m_array->reserveSpace(valueSize, i, 1, true);
   if (!valueOffset) {
      return;
   }      
   jsonprivate::LocalValue &v = (*m_array)[i];
   v.m_type = pdk::as_integer<JsonValue::Type>((val.m_type == JsonValue::Type::Undefined ? JsonValue::Type::Null : val.m_type));
   v.m_latinOrIntValue = compressed;
   v.m_latinKey = false;
   v.m_value = jsonprivate::LocalValue::valueToStore(val, valueOffset);
   if (valueSize) {
      jsonprivate::LocalValue::copyData(val, (char *)m_array + valueOffset, compressed);
   }
   ++m_data->m_compactionCounter;
   if (m_data->m_compactionCounter > 32u && m_data->m_compactionCounter >= unsigned(m_array->m_length) / 2u) {
      compact();
   }
}

bool JsonArray::contains(const JsonValue &value) const
{
   for (int i = 0; i < getSize(); i++) {
      if (at(i) == value) {
         return true;
      }
   }
   return false;
}

JsonValueRef JsonArray::operator [](int i)
{
   PDK_ASSERT(m_array && i >= 0 && i < (int)m_array->m_length);
   return JsonValueRef(this, i);
}

JsonValue JsonArray::operator[](int i) const
{
   return at(i);
}

bool JsonArray::operator==(const JsonArray &other) const
{
   if (m_array == other.m_array) {
      return true;
   }
   if (!m_array) {
      return !other.m_array->m_length;
   }
   if (!other.m_array) {
      return !m_array->m_length;
   }
   if (m_array->m_length != other.m_array->m_length) {
      return false;
   }
   for (int i = 0; i < (int)m_array->m_length; ++i) {
      if (JsonValue(m_data, m_array, m_array->at(i)) != JsonValue(other.m_data, other.m_array, other.m_array->at(i))) {
         return false;
      } 
   }
   return true;
}

bool JsonArray::operator!=(const JsonArray &other) const
{
   return !(*this == other);
}

bool JsonArray::detach(uint reserve)
{
   if (!m_data) {
      if (reserve >= jsonprivate::LocalValue::MaxSize) {
         warning_stream("Json: Document too large to store in data structure");
         return false;
      }
      m_data = new jsonprivate::Data(reserve, JsonValue::Type::Array);
      m_array = static_cast<jsonprivate::LocalArray *>(m_data->m_header->getRoot());
      m_data->m_ref.ref();
      return true;
   }
   if (reserve == 0 && m_data->m_ref.load() == 1)
      return true;
   
   jsonprivate::Data *x = m_data->clone(m_array, reserve);
   if (!x) {
      return false;
   }      
   x->m_ref.ref();
   if (!m_data->m_ref.deref()) {
      delete m_data;
   }
   m_data = x;
   m_array = static_cast<jsonprivate::LocalArray *>(m_data->m_header->getRoot());
   return true;
}

void JsonArray::compact()
{
   if (!m_data || !m_data->m_compactionCounter){
      return;      
   }
   detach();
   m_data->compact();
   m_array = static_cast<jsonprivate::LocalArray *>(m_data->m_header->getRoot());
}

#if !defined(QT_NO_DEBUG_STREAM) && !defined(QT_JSON_READONLY)
Debug operator<<(Debug dbg, const JsonArray &array)
{
   DebugStateSaver saver(dbg);
   if (!array.m_array) {
      dbg << "JsonArray()";
      return dbg;
   }
   ByteArray json;
   jsonprivate::Writer::arrayToJson(array.m_array, json, 0, true);
   dbg.nospace() << "JsonArray("
                 << json.getConstRawData() // print as utf-8 string without extra quotation marks
                 << ")";
   return dbg;
}
#endif

} // json
} // utils
} // pdk
