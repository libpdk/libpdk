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
// Created by softboy on 2018/01/20.

// Boost.Signals2 library

// Copyright Frank Mori Hess 2007-2008.
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef PDK_KERNEL_SIGNAL_INTERNAL_SLOT_GROUP_H
#define PDK_KERNEL_SIGNAL_INTERNAL_SLOT_GROUP_H

#include "pdk/kernel/signal/Connection.h"
#include <optional>
#include <list>
#include <map>
#include <utility>

namespace pdk {
namespace kernel {
namespace signal {
namespace internal {

enum class SlotMetaGroup
{
   FrontUngroupedSlots, 
   GroupedSlots, 
   BackUngroupedSlots
};

template<typename Group>
struct GroupKey
{
   using type = std::pair<SlotMetaGroup, std::optional<Group>>;
};

template<typename Group, typename GroupCompare>
class GroupKeyLess
{
public:
   GroupKeyLess()
   {}
   
   GroupKeyLess(const GroupCompare &groupCompare)
      : m_groupCompare(groupCompare)
   {}
   
   bool operator ()(const typename GroupKey<Group>::type &key1, 
                    const typename GroupKey<Group>::type &key2) const
   {
      if(key1.first != key2.first) {
         return key1.first < key2.first;
      }
      if(key1.first != SlotMetaGroup::GroupedSlots) {
         return false;
      }
      return m_groupCompare(key1.second.value(), key2.second.value());
   }
   
private:
   GroupCompare m_groupCompare;
};

template<typename Group, typename GroupCompare, typename ValueType>
class GroupedList
{
public:
   using GroupKeyCompareType = GroupKeyLess<Group, GroupCompare>;
private:
   using ListType = std::list<ValueType>;
   using MapType = std::map
   <
   typename GroupKey<Group>::type,
   typename ListType::iterator,
   GroupKeyCompareType
   >;
   using MapIterator = typename MapType::iterator;
   using ConstMapIterator = typename MapType::const_iterator;
public:
   using iterator = typename ListType::iterator;
   using const_iterator = typename ListType::const_iterator;
   using Iterator = iterator;
   using ConstIterator = const_iterator;
   using GroupKeyType = typename GroupKey<Group>::type;
   
   GroupedList(const GroupKeyCompareType &groupKeyCompare):
      m_groupKeyCompare(groupKeyCompare)
   {}
   
   GroupedList(const GroupedList &other)
      : m_list(other.m_list),
        m_groupMap(other.m_groupMap), 
        m_groupKeyCompare(other.m_groupKeyCompare)
   {
      // fix up m_groupMap
      typename MapType::const_iterator otherMapIter;
      typename ListType::iterator thisListIter = m_list.begin();
      typename MapType::iterator thisMapIter = m_groupMap.begin();
      for(otherMapIter = other.m_groupMap.begin();
          otherMapIter != other.m_groupMap.end();
          ++otherMapIter, ++thisMapIter) {
         PDK_ASSERT(thisMapIter != m_groupMap.end());
         thisMapIter->second = thisListIter;
         typename ListType::const_iterator otherListIter = other.getListIterator(otherMapIter);
         typename MapType::const_iterator otherNextMapIter = otherMapIter;
         ++otherNextMapIter;
         typename ListType::const_iterator otherNextListIter = other.getListIterator(otherNextMapIter);
         while(otherListIter != otherNextListIter)
         {
            ++otherListIter;
            ++thisListIter;
         }
      }
   }
   
   iterator begin()
   {
      return m_list.begin();
   }
   
   iterator end()
   {
      return m_list.end();
   }
   
   iterator lowerBound(const GroupKeyType &key)
   {
      MapIterator mapIter = m_groupMap.lower_bound(key);
      return getListIterator(mapIter);
   }
   
   iterator upperBound(const GroupKeyType &key)
   {
      MapIterator mapIter = m_groupMap.upper_bound(key);
      return getListIterator(mapIter);
   }
   
   void pushFront(const GroupKeyType &key, const ValueType &value)
   {
      MapIterator mapIter;
      if(key.first == SlotMetaGroup::FrontUngroupedSlots) {
         // optimization
         mapIter = m_groupMap.begin();
      } else {
         mapIter = m_groupMap.lower_bound(key);
      }
      mapInsert(mapIter, key, value);
   }
   
   void pushBack(const GroupKeyType &key, const ValueType &value)
   {
      MapIterator mapIter;
      if(key.first == SlotMetaGroup::BackUngroupedSlots) {
         // optimization
         mapIter = m_groupMap.end();
      }else {
         mapIter = m_groupMap.upper_bound(key);
      }
      mapInsert(mapIter, key, value);
   }
   
   void erase(const GroupKeyType &key)
   {
      MapIterator mapIter = m_groupMap.lower_bound(key);
      iterator beginListIter = getListIterator(mapIter);
      iterator endListIter = upperBound(key);
      if(beginListIter != endListIter)
      {
         m_list.erase(beginListIter, endListIter);
         m_groupMap.erase(mapIter);
      }
   }
   
   iterator erase(const GroupKeyType &key, const Iterator &iter)
   {
      PDK_ASSERT(iter != m_list.end());
      MapIterator mapIter = m_groupMap.lower_bound(key);
      PDK_ASSERT(mapIter != m_groupMap.end());
      PDK_ASSERT(weaklyEquivalent(mapIter->first, key));
      if(mapIter->second == iter) {
         iterator next = iter;
         ++next;
         // if next is in same group
         if(next != upperBound(key)) {
            m_groupMap[key] = next;
         } else {
            m_groupMap.erase(mapIter);
         }
      }
      return m_list.erase(iter);
   }
   
   void clear()
   {
      m_list.clear();
      m_groupMap.clear();
   }
   
private:
   /* Suppress default assignment operator, since it has the wrong semantics. */
   GroupedList& operator=(const GroupedList &other);
   
   bool weaklyEquivalent(const GroupKeyType &arg1, const GroupKeyType &arg2)
   {
      if(m_groupKeyCompare(arg1, arg2)) {
         return false;
      }
      if(m_groupKeyCompare(arg2, arg1)) {
         return false;
      }
      return true;
   }
   
   void mapInsert(const MapIterator &mapIter, const GroupKeyType &key, const ValueType &value)
   {
      iterator listIter = getListIterator(mapIter);
      iterator newIter = m_list.insert(listIter, value);
      if(mapIter != m_groupMap.end() && weaklyEquivalent(key, mapIter->first)) {
         m_groupMap.erase(mapIter);
      }
      MapIterator lowerBoundIter = m_groupMap.lower_bound(key);
      if(lowerBoundIter == m_groupMap.end() ||
         weaklyEquivalent(lowerBoundIter->first, key) == false) {
         // doing the following instead of just
         // m_groupMap[key] = newIter;
         // to avoid bogus error when enabling checked iterators with g++
         m_groupMap.insert(typename MapType::value_type(key, newIter));
      }
   }
   
   iterator getListIterator(const ConstMapIterator &mapIter)
   {
      iterator listIter;
      if(mapIter == m_groupMap.end()) {
         listIter = m_list.end();
      } else {
         listIter = mapIter->second;
      }
      return listIter;
   }
   
   const_iterator getListIterator(const ConstMapIterator &mapIter) const
   {
      const_iterator listIter;
      if(mapIter == m_groupMap.end()) {
         listIter = m_list.end();
      } else {
         listIter = mapIter->second;
      }
      return listIter;
   }
   
   ListType m_list;
   // holds iterators to first list item in each group
   MapType m_groupMap;
   GroupKeyCompareType m_groupKeyCompare;
};

} // internal

enum ConnectPosition
{ 
   AtBack, 
   AtFront
};

} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_INTERNAL_SLOT_GROUP_H
