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
// Created by softboy on 2018/02/23.

#ifndef PDK_UTILS_CACHE_H
#define PDK_UTILS_CACHE_H

#include "pdk/global/Global.h"
#include <map>
#include <list>

namespace pdk {
namespace utils {

template <typename Key, typename T>
class Cache
{
   struct Node
   {
      inline Node()
         : m_keyPtr(nullptr)
      {}
      
      inline Node(T *data, int cost)
         : m_keyPtr(nullptr),
           m_cost(cost),
           m_data(data),
           m_prevNode(nullptr),
           m_nextNode(nullptr)
      {}
      const Key *m_keyPtr;
      int m_cost;
      T *m_data;
      Node *m_prevNode;
      Node *m_nextNode;
   };
   
   Node *m_forward;
   Node *m_last;
   std::map<Key, Node> m_map;
   int m_mx;
   int m_total;
   
   inline void unlink(Node &node)
   {
      if (node.m_prevNode) {
         node.m_prevNode->m_nextNode = node.m_nextNode;
      }
      if (node.m_nextNode) {
         node.m_nextNode->m_prevNode = node.m_prevNode;
      }
      if (m_last == &node) {
         m_last = node.m_prevNode;
      }
      if (m_forward == &node) {
         m_forward = node.m_nextNode;
      }
      T *obj = node.m_data;
      m_map.erase(*node.m_keyPtr);
      delete obj;
   }
   
   inline T *relink(const Key &key)
   {
      typename std::map<Key, Node>::iterator iter = m_map.find(key);
      if (typename std::map<Key, Node>::const_iterator(iter) == m_map.cend()) {
         return nullptr;
      }
      Node &node = *iter;
      if (m_forward != &node) {
         if (node.m_prevNode) {
            node.m_prevNode->m_nextNode = node.m_nextNode;
         }
         if (node.m_nextNode) {
            node.m_nextNode->m_prevNode = node.m_prevNode;
         }
         if (m_last == &node) {
            m_last = node.m_prevNode;
         }
         node.m_prevNode = nullptr;
         node.m_nextNode = m_forward;
         m_forward->m_prevNode = &node;
         m_forward = &node;
      }
      return node.m_data;
   }
   
   PDK_DISABLE_COPY(Cache);
   
public:
   inline explicit Cache(int maxCost = 100) noexcept;
   inline ~Cache()
   {
      clear();
   }
   
   inline int maxCost() const
   {
      return m_mx;
   }
   void setMaxCost(int m);
   inline int getTotalCost() const
   {
      return m_total;
   }
   
   inline int getSize() const
   {
      return m_map.size();
   }
   
   inline int getCount() const
   {
      return m_map.size();
   }
   
   inline bool isEmpty() const
   {
      return m_map.empty();
   }
   
   inline std::list<Key> keys() const
   {
      std::list<Key> keys;
      auto iter = m_map.cbegin();
      auto end = m_map.cend();
      while (iter != end) {
         keys.push_back(iter->first);
         ++iter;
      }
   }
   
   void clear();
   
   bool insert(const Key &key, T *object, int cost = 1);
   T *getData(const Key &key) const;
   inline bool contains(const Key &key) const
   {
      return m_map.find(key) != m_map.end();
   }
   
   T *operator[](const Key &key) const;
   
   bool remove(const Key &key);
   T *take(const Key &key);
   
private:
   void trim(int m);
};

template <typename Key, typename T>
inline Cache<Key, T>::Cache(int amaxCost) noexcept
   : m_forward(nullptr),
     m_last(nullptr),
     m_mx(amaxCost),
     m_total(0)
{}

template <typename Key, typename T>
inline void Cache<Key, T>::clear()
{
   while (m_forward) {
      delete m_forward->m_data;
      m_forward = m_forward->m_nextNode;
   }
   m_map.clear();
   m_last = nullptr;
   m_total = 0;
}

template <typename Key, typename T>
inline void Cache<Key, T>::setMaxCost(int maxCost)
{
   m_mx = maxCost;
   trim(m_mx);
}

template <typename Key, typename T>
inline T *Cache<Key, T>::getData(const Key &key) const
{
   return const_cast<Cache<Key, T> *>(this)->relink(key);
}

template <typename Key, typename T>
inline T *Cache<Key, T>::operator[](const Key &key) const
{
   return getData(key);
}

template <typename Key, typename T>
inline bool Cache<Key, T>::remove(const Key &key)
{
   typename std::map<Key, Node>::iterator iter = m_map.find(key);
   if (typename std::map<Key, Node>::const_iterator(iter) == m_map.cend()) {
      return false;
   } else {
      unlink(iter->second);
      return true;
   }
}

template <typename Key, typename T>
inline T *Cache<Key, T>::take(const Key &key)
{
   typename std::map<Key, Node>::iterator iter = m_map.find(key);
   if (iter == m_map.end()) {
      return nullptr;
   }
   Node &node = iter->second;
   T *data = node.m_data;
   node.m_data = nullptr;
   unlink(node);
   return data;
}


template <typename Key, typename T>
inline bool Cache<Key, T>::insert(const Key &key, T *data, int cost)
{
   remove(key);
   if (cost > m_mx) {
      delete data;
      return false;
   }
   trim(m_mx - cost);
   Node sn(data, cost);
   m_map[key] = sn;
   m_total += cost;
   typename std::map<Key, Node>::iterator iter = m_map.find(key);
   Node *node = &iter->second;
   node->m_keyPtr = &iter->first;
   if (m_forward) {
      m_forward->m_prevNode = node;
   }
   node->m_nextNode = m_forward;
   m_forward = node;
   if (!m_last) {
      m_last = m_forward;
   }
   return true;
}

template <class Key, class T>
void Cache<Key,T>::trim(int m)
{
   Node *node = m_last;
   while (node && m_total > m) {
      Node *u = node;
      node = node->m_prevNode;
      unlink(*u);
   }
}

} // utils
} // pdk

#endif // PDK_UTILS_CACHE_H
