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

#ifndef PDK_M_BASE_JSON_JSON_OBJECT_H
#define PDK_M_BASE_JSON_JSON_OBJECT_H

#include "pdk/base/utils/json/JsonValue.h"
#include "pdk/utils/Iterator.h"
#include <utility>
#include <initializer_list>
#include <map>
#include <any>

namespace pdk {

// forward declare class with namespace
namespace io {
class Debug;
} // io

// forward declare class with namespace
namespace lang {
class String;
class StringList;
class Latin1String;
} // lang

namespace utils {
namespace json {

using pdk::io::Debug;
using AnyMap = std::map<String, std::any>;
using pdk::lang::StringList;

class PDK_CORE_EXPORT JsonObject
{
public:
   JsonObject();
   
   JsonObject(std::initializer_list<std::pair<String, JsonValue> > args)
   {
      initialize();
      for (std::initializer_list<std::pair<String, JsonValue> >::const_iterator iter = args.begin();
           iter != args.end(); ++iter) {
         insert(iter->first, iter->second);
      }
   }
   ~JsonObject();
   JsonObject(const JsonObject &other);
   JsonObject &operator =(const JsonObject &other);
   JsonObject(JsonObject &&other) noexcept
      : m_data(other.m_data),
        m_object(other.m_object)
   {
      other.m_data = nullptr;
      other.m_object = nullptr;
   }
   
   JsonObject &operator =(JsonObject &&other) noexcept
   {
      swap(other);
      return *this;
   }
   
   void swap(JsonObject &other) noexcept
   {
      std::swap(m_data, other.m_data);
      std::swap(m_object, other.m_object);
   }
   
   static JsonObject fromAnyMap(const AnyMap &map);
   AnyMap toAnyMap() const;
   
   StringList getKeys() const;
   int getSize() const;
   inline int count() const
   {
      return getSize();
   }
   
   inline int length() const
   {
      return getSize();
   }
   
   bool isEmpty() const;
   
   JsonValue getValue(const String &key) const;
   JsonValue getValue(Latin1String key) const;
   JsonValue operator[] (const String &key) const;
   JsonValue operator[] (Latin1String key) const
   {
      return getValue(key);
   }
   
   JsonValueRef operator[] (const String &key);
   JsonValueRef operator[] (Latin1String key);
   
   void remove(const String &key);
   JsonValue take(const String &key);
   bool contains(const String &key) const;
   bool contains(Latin1String key) const;
   
   bool operator==(const JsonObject &other) const;
   bool operator!=(const JsonObject &other) const;
   
   class const_iterator;
   
   class iterator
   {
      friend class const_iterator;
      friend class JsonObject;
      JsonObject *m_object;
      int m_index;
      
   public:
      using iterator_category = std::random_access_iterator_tag;
      using difference_type = int;
      using value_type = JsonValue;
      using reference = JsonValueRef;
      using pointer = JsonValuePtr;
      
      constexpr inline iterator()
         : m_object(nullptr),
           m_index(0)
      {}
      
      constexpr inline iterator(JsonObject *object, int index)
         : m_object(object),
           m_index(index)
      {}
      
      inline String getKey() const 
      {
         return m_object->keyAt(i);
      }
      
      inline JsonValueRef getValue() const
      {
         return JsonValueRef(m_object, m_index);
      }
      
      inline JsonValueRef operator*() const
      {
         return JsonValueRef(m_object, m_index);
      }
      
      inline JsonValueRefPtr operator->() const
      {
         return JsonValueRefPtr(m_object, m_index);
      }
      
      inline bool operator==(const iterator &other) const
      {
         return m_index == other.m_index;
      }
      
      inline bool operator!=(const iterator &other) const
      {
         return m_index != other.m_index;
      }
      
      inline iterator &operator++()
      {
         ++m_index;
         return *this;
      }
      
      inline iterator operator++(int)
      {
         iterator r = *this;
         ++m_index;
         return r;
      }
      
      inline iterator &operator--()
      {
         --m_index;
         return *this;
      }
      
      inline iterator operator--(int)
      {
         iterator r = *this;
         --m_index;
         return r;
      }
      inline iterator operator+(int j) const
      {
         iterator r = *this;
         r.m_index += j;
         return r;
      }
      
      inline iterator operator-(int j) const
      {
         return operator+(-j);
      }
      
      inline iterator &operator+=(int j)
      {
         m_index += j;
         return *this;
      }
      
      inline iterator &operator-=(int j)
      {
         m_index -= j;
         return *this;
      }
      
   public:
      inline bool operator==(const const_iterator &other) const
      {
         return m_index == other.m_index;
      }
      
      inline bool operator!=(const const_iterator &other) const
      {
         return m_index != other.m_index;
      }
   };
   friend class iterator;
   
   class const_iterator
   {
      friend class iterator;
      const JsonObject *m_object;
      int m_index;
      
   public:
      using iterator_category = std::random_access_iterator_tag;
      using difference_type = int;
      using value_type = JsonValue;
      using reference = JsonValue;
      using pointer = JsonValuePtr;
      
      constexpr inline const_iterator()
         : m_object(nullptr), 
           m_index(0)
      {}
      
      constexpr inline const_iterator(const JsonObject *object, int index)
         : m_object(object), 
           m_index(index)
      {}
      
      inline const_iterator(const iterator &other)
         : m_object(other.m_object),
           m_index(other.m_index)
      {}
      
      inline String getKey() const
      {
         return m_object->keyAt(m_index);
      }
      
      inline JsonValue getValue() const
      {
         return m_object->valueAt(m_index);
      }
      
      inline JsonValue operator*() const
      {
         return m_object->valueAt(m_index);
      }
      
      inline JsonValuePtr operator->() const
      {
         return JsonValuePtr(m_object->valueAt(m_index));
      }
      
      inline bool operator==(const const_iterator &other) const
      {
         return m_index == other.m_index;
      }
      
      inline bool operator!=(const const_iterator &other) const
      {
         return m_index != other.m_index;
      }
      
      inline const_iterator &operator++()
      {
         ++m_index;
         return *this;
      }
      
      inline const_iterator operator++(int)
      {
         const_iterator r = *this;
         ++m_index;
         return r;
      }
      
      inline const_iterator &operator--()
      {
         --m_index;
         return *this;
      }
      
      inline const_iterator operator--(int)
      {
         const_iterator r = *this;
         --m_index;
         return r;
      }
      
      inline const_iterator operator+(int j) const
      {
         const_iterator r = *this;
         r.m_index += j;
         return r;
      }
      
      inline const_iterator operator-(int j) const
      {
         return operator+(-j);
      }
      
      inline const_iterator &operator+=(int j)
      {
         m_index += j;
         return *this;
      }
      
      inline const_iterator &operator-=(int j)
      {
         m_index -= j;
         return *this;
      }
      
      inline bool operator==(const iterator &other) const
      {
         return m_index == other.m_index;
      }
      
      inline bool operator!=(const iterator &other) const
      {
         return m_index != other.m_index;
      }
   };
   friend class const_iterator;
   
   // STL style
   inline iterator begin()
   {
      detach();
      return iterator(this, 0);
   }
   
   inline const_iterator begin() const
   {
      return const_iterator(this, 0);
   }
   
   inline const_iterator cbegin() const
   {
      return const_iterator(this, 0);
   }
   
   inline iterator end()
   {
      detach();
      return iterator(this, getSize());
   }
   
   inline const_iterator end() const
   {
      return const_iterator(this, getSize());
   }
   
   inline const_iterator cend() const
   {
      return const_iterator(this, getSize());
   }
   
   iterator erase(iterator iter);
   
   typedef iterator Iterator;
   typedef const_iterator ConstIterator;
   iterator find(const String &key);
   iterator find(Latin1String key);
   const_iterator find(const String &key) const
   {
      return constFind(key);
   }
   
   const_iterator find(Latin1String key) const
   {
      return constFind(key);
   }
   
   const_iterator constFind(const String &key) const;
   const_iterator constFind(Latin1String key) const;
   iterator insert(const String &key, const JsonValue &value);
   
   // STL compatibility
   using mapped_type = JsonValue;
   using key_type = String;
   using size_type = int;
   
   inline bool empty() const
   {
      return isEmpty();
   }
   
private:
   friend class jsonprivate::Data;
   friend class JsonValue;
   friend class JsonDocument;
   friend class JsonValueRef;
   
   friend PDK_CORE_EXPORT Debug operator<<(Debug, const JsonObject &);
   
   JsonObject(jsonprivate::Data *data, jsonprivate::LocalObject *object);
   void initialize();
   void detach(uint reserve = 0);
   void compact();
   
   String keyAt(int i) const;
   JsonValue valueAt(int i) const;
   void setValueAt(int i, const JsonValue &val);
   
   jsonprivate::Data *m_data;
   jsonprivate::LocalObject *m_object;
};

#if !defined(PDK_NO_DEBUG_STREAM) && !defined(PDK_JSON_READONLY)
PDK_CORE_EXPORT Debug operator<<(Debug, const JsonObject &);
#endif

} // json
} // utils
} // pdk

PDK_DECLARE_TYPEINFO(pdk::utils::json::JsonObject, PDK_MOVABLE_TYPE);

#endif // PDK_M_BASE_JSON_JSON_OBJECT_H
