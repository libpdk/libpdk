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

#ifndef PDK_M_BASE_JSON_JSON_ARRAY_H
#define PDK_M_BASE_JSON_JSON_ARRAY_H

#include "pdk/base/utils/json/JsonValue.h"
#include "pdk/utils/Iterator.h"
#include <initializer_list>
#include <list>
#include <any>

namespace pdk {

// forward declare class with namespace
namespace io {
class Debug;
} // io

// forward declare class with namespace
namespace lang {
class StringList;
} // lang

namespace utils {
namespace json {

using pdk::io::Debug;
using pdk::lang::StringList;

using AnyList = std::list<std::any>;

class PDK_CORE_EXPORT JsonArray
{
public:
   JsonArray();
   
   JsonArray(std::initializer_list<JsonValue> args)
   {
      initialize();
      for (std::initializer_list<JsonValue>::const_iterator iter = args.begin(); iter != args.end(); ++iter) {
         append(*iter);
      }
   }
   
   ~JsonArray();
   
   JsonArray(const JsonArray &other);
   JsonArray &operator =(const JsonArray &other);
   
   JsonArray(JsonArray &&other) noexcept
      : m_data(other.m_data),
        m_array(other.m_array)
   {
      other.m_data = nullptr;
      other.m_array = nullptr;
   }
   
   JsonArray &operator =(JsonArray &&other) noexcept
   {
      swap(other);
      return *this;
   }
   
   static JsonArray fromStringList(const StringList &list);
   static JsonArray fromAnyList(const AnyList &list);
   AnyList toAnyList() const;
   
   int getSize() const;
   inline int count() const
   {
      return getSize();
   }
   
   bool isEmpty() const;
   JsonValue at(int i) const;
   JsonValue first() const;
   JsonValue last() const;
   
   void prepend(const JsonValue &value);
   void append(const JsonValue &value);
   void removeAt(int i);
   JsonValue takeAt(int i);
   inline void removeFirst()
   {
      removeAt(0);
   }
   
   inline void removeLast()
   {
      removeAt(getSize() - 1);
   }
   
   void insert(int i, const JsonValue &value);
   void replace(int i, const JsonValue &value);
   
   bool contains(const JsonValue &element) const;
   JsonValueRef operator[](int i);
   JsonValue operator[](int i) const;
   
   bool operator==(const JsonArray &other) const;
   bool operator!=(const JsonArray &other) const;
   
   void swap(JsonArray &other) noexcept
   {
      std::swap(m_data, other.m_data);
      std::swap(m_array, other.m_array);
   }
   
   class const_iterator;
   
   class iterator {
   public:
      JsonArray *m_array;
      int m_index;
      using iterator_category = std::random_access_iterator_tag;
      using difference_type = int;
      using value_type = JsonValue;
      using reference = JsonValueRef;
      using pointer = JsonValueRefPtr;
      
      inline iterator()
         : m_array(nullptr),
           m_index(0)
      {}
      
      explicit inline iterator(JsonArray *array, int index)
         : m_array(array),
           m_index(index)
      {}
      
      inline JsonValueRef operator*() const
      {
         return JsonValueRef(m_array, m_index);
      }
      
      inline JsonValueRefPtr operator->() const
      {
         return JsonValueRefPtr(m_array, m_index);
      }
      
      inline JsonValueRef operator[](int j) const
      {
         return JsonValueRef(m_array, m_index + j);
      }
      
      inline bool operator==(const iterator &other) const
      {
         return m_index == other.m_index;
      }
      
      inline bool operator!=(const iterator &other) const
      {
         return m_index != other.m_index;
      }
      
      inline bool operator<(const iterator& other) const
      {
         return m_index < other.m_index;
      }
      
      inline bool operator<=(const iterator& other) const
      {
         return m_index <= other.m_index;
      }
      
      inline bool operator>(const iterator& other) const
      {
         return m_index > other.m_index;
      }
      
      inline bool operator>=(const iterator& other) const
      {
         return m_index >= other.m_index;
      }
      
      inline bool operator==(const const_iterator &other) const
      {
         return m_index == other.m_index;
      }
      inline bool operator!=(const const_iterator &other) const
      {
         return m_index != other.m_index;
      }
      
      inline bool operator<(const const_iterator& other) const
      {
         return m_index < other.m_index;
      }
      
      inline bool operator<=(const const_iterator& other) const
      {
         return m_index <= other.m_index;
      }
      
      inline bool operator>(const const_iterator& other) const
      {
         return m_index > other.m_index;
      }
      
      inline bool operator>=(const const_iterator& other) const
      {
         return m_index >= other.m_index;
      }
      
      inline iterator &operator++()
      {
         ++m_index;
         return *this;
      }
      
      inline iterator operator++(int)
      {
         iterator n = *this;
         ++m_index;
         return n;
      }
      
      inline iterator &operator--()
      {
         m_index--;
         return *this;
      }
      
      inline iterator operator--(int)
      {
         iterator n = *this;
         m_index--;
         return n;
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
      
      inline iterator operator+(int j) const
      {
         return iterator(a, m_index + j);
      }
      
      inline iterator operator-(int j) const
      {
         return iterator(a, m_index - j);
      }
      
      inline int operator-(iterator j) const
      {
         return m_index - j.m_index;
      }
   };
   friend class iterator;
   
   class const_iterator {
   public:
      const JsonArray *m_array;
      int m_index;
      using iterator_category = std::random_access_iterator_tag;
      using difference_type = pdk::ptrdiff;
      using value_type = JsonValue;
      using reference = JsonValue;
      using pointer = JsonValuePtr;
      
      inline const_iterator()
         : m_array(nullptr),
           m_index(0)
      {}
      
      explicit inline const_iterator(const JsonArray *array, int index) 
         : m_array(array),
           m_index(index)
      {}
      
      inline const_iterator(const iterator &other)
         : m_array(other.m_array),
           m_index(other.m_index) {}
      
      inline JsonValue operator*() const
      {
         return m_array->at(m_index);
      }
      
      inline JsonValuePtr operator->() const
      {
         return JsonValuePtr(m_array->at(m_index));
      }
      
      inline JsonValue operator[](int j) const
      {
         return m_array->at(m_index + j);
      }
      
      inline bool operator==(const const_iterator &other) const
      {
         return m_index == other.m_index;
      }
      
      inline bool operator!=(const const_iterator &other) const
      {
         return m_index != other.m_index;
      }
      
      inline bool operator<(const const_iterator& other) const
      {
         return m_index < other.m_index;
      }
      
      inline bool operator<=(const const_iterator& other) const
      {
         return m_index <= other.m_index;
      }
      
      inline bool operator>(const const_iterator& other) const
      {
         return m_index > other.m_index;
      }
      
      inline bool operator>=(const const_iterator& other) const
      {
         return m_index >= other.m_index;
      }
      
      inline const_iterator &operator++()
      {
         ++m_index;
         return *this;
      }
      
      inline const_iterator operator++(int)
      {
         const_iterator n = *this;
         ++m_index;
         return n;
      }
      
      inline const_iterator &operator--()
      {
         m_index--;
         return *this;
      }
      
      inline const_iterator operator--(int)
      {
         const_iterator n = *this;
         m_index--;
         return n;
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
      
      inline const_iterator operator+(int j) const
      {
         return const_iterator(a, m_index + j);
      }
      
      inline const_iterator operator-(int j) const
      {
         return const_iterator(a, m_index - j);
      }
      
      inline int operator-(const_iterator j) const
      {
         return m_index - j.m_index;
      }
   };
   friend class const_iterator;
   
   // stl style
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
   
   iterator insert(iterator before, const JsonValue &value)
   {
      insert(before.m_index, value);
      return before;
   }
   
   iterator erase(iterator iter)
   {
      removeAt(iter.m_index);
      return iter;
   }
   
   using Iterator = iterator;
   using ConstIterator = const_iterator;
   
   // convenience
   inline JsonArray operator+(const JsonValue &value) const
   {
      JsonArray n = *this;
      n += value;
      return n;
   }
   
   inline JsonArray &operator+=(const JsonValue &value)
   {
      append(value);
      return *this;
   }
   
   inline JsonArray &operator<< (const JsonValue &value)
   {
      append(value);
      return *this;
   }
   
   // stl compatibility
   inline void push_back(const JsonValue &value)
   {
      append(value);
   }
   
   inline void push_front(const JsonValue &value)
   {
      prepend(value);
   }
   
   inline void pop_front()
   {
      removeFirst();
   }
   
   inline void pop_back()
   {
      removeLast();
   }
   
   inline bool empty() const
   {
      return isEmpty();
   }
   
   using size_type = int;
   using value_type = JsonValue;
   using pointer = value_type*;
   using const_pointer = const value_type*;
   using reference = JsonValueRef;
   using const_reference = JsonValue;
   using difference_type = int;
   
private:
   friend class jsonprivate::Data;
   friend class JsonValue;
   friend class JsonDocument;
   friend PDK_CORE_EXPORT Debug operator<<(Debug, const JsonArray &);
   
   JsonArray(jsonprivate::Data *data, jsonprivate::LocalArray *array);
   void initialize();
   void compact();
   void detach(uint reserve = 0);
   
   jsonprivate::Data *m_data;
   jsonprivate::LocalArray *m_array;
};

#if !defined(PDK_NO_DEBUG_STREAM) && !defined(PDK_JSON_READONLY)
PDK_CORE_EXPORT Debug operator<<(Debug, const JsonArray &);
#endif

} // json
} // utils
} // pdk

PDK_DECLARE_TYPEINFO(pdk::utils::json::JsonArray, PDK_MOVABLE_TYPE);

#endif // PDK_M_BASE_JSON_JSON_ARRAY_H
