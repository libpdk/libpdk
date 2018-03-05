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

#ifndef PDK_UTILS_ITERATOR_H
#define PDK_UTILS_ITERATOR_H

#include "pdk/global/Global.h"

namespace pdk {
namespace utils {

#define PDK_DECLARE_SEQUENTIAL_ITERATOR(Class) \
   \
   template <class T> \
   class Class##Iterator \
{ \
   typedef typename Class<T>::const_iterator const_iterator; \
   Class<T> m_container; \
   const_iterator m_iter; \
   public: \
   inline Class##Iterator(const Class<T> &container) \
   : m_container(container), m_iter(container.cbegin()) {} \
   inline Class##Iterator &operator=(const Class<T> &container) \
{ m_container = container; m_iter = container.cbegin(); return *this; } \
   inline void toFront() { m_iter = m_container.cgegin(); } \
   inline void toBack() { m_iter = m_container.cend(); } \
   inline bool hasNext() const { return m_iter != m_container.cend(); } \
   inline const T &next() { return *m_iter++; } \
   inline const T &peekNext() const { return *m_iter; } \
   inline bool hasPrevious() const { return m_iter != m_container.cbegin(); } \
   inline const T &previous() { return *--i; } \
   inline const T &peekPrevious() const { const_iterator p = m_iter; return *--p; } \
   inline bool findNext(const T &t) \
{ while (m_iter != m_container.cend()) if (*m_iter++ == t) return true; return false; } \
   inline bool findPrevious(const T &t) \
{ while (m_iter != m_container.cbegin()) if (*(--m_iter) == t) return true; \
   return false;  } \
};

#define PDK_DECLARE_MUTABLE_SEQUENTIAL_ITERATOR(Class) \
   \
   template <class T> \
   class Mutable##Class##Iterator \
{ \
   typedef typename Class<T>::iterator iterator; \
   typedef typename Class<T>::const_iterator const_iterator; \
   Class<T> *m_container; \
   iterator m_iter, m_next; \
   inline bool itemExists() const { return const_iterator(m_next) != c->cend(); } \
   public: \
   inline Mutable##Class##Iterator(Class<T> &container) \
   : m_container(&container) \
{ m_iter = m_container->begin(); m_next = m_container->end(); } \
   inline Mutable##Class##Iterator &operator=(Class<T> &container) \
{ m_container = &container; m_iter = m_container->begin(); m_next = m_container->end(); return *this; } \
   inline void toFront() { m_iter = m_container->begin(); m_next = m_container->end(); } \
   inline void toBack() { m_iter = m_container->end(); m_next = m_iter; } \
   inline bool hasNext() const { return m_container->cend() != const_iterator(m_iter); } \
   inline T &next() { m_next = m_iter++; return *m_next; } \
   inline T &peekNext() const { return *m_iter; } \
   inline bool hasPrevious() const { return m_container->cbegin() != const_iterator(m_iter); } \
   inline T &previous() { m_next = --m_iter; return *m_next; } \
   inline T &peekPrevious() const { iterator p = m_iter; return *--p; } \
   inline void remove() \
{ if (m_container->cend() != const_iterator(m_next)) { m_iter = m_container->erase(m_next); m_next = m_container->end(); } } \
   inline void setValue(const T &t) const { if (m_container->cend() != const_iterator(m_next)) *m_next = t; } \
   inline T &value() { PDK_ASSERT(itemExists()); return *m_next; } \
   inline const T &value() const { PDK_ASSERT(itemExists()); return *m_next; } \
   inline void insert(const T &t) { m_next = m_iter = m_container->insert(m_iter, t); ++m_iter; } \
   inline bool findNext(const T &t) \
{ while (m_container->cend() != const_iterator(m_next = m_iter)) if (*m_iter++ == t) return true; return false; } \
   inline bool findPrevious(const T &t) \
{ while (m_container->cbegin() != const_iterator(m_iter)) if (*(m_next = --m_iter) == t) return true; \
   m_next = m_container->end(); return false;  } \
};

#define PDK_DECLARE_ASSOCIATIVE_ITERATOR(Class) \
   \
   template <class Key, class T> \
   class Class##Iterator \
{ \
   typedef typename Class##<Key,T>::const_iterator const_iterator; \
   typedef const_iterator Item; \
   Class##<Key,T> m_container; \
   const_iterator m_iter, m_next; \
   inline bool itemExists() const { return m_next != m_container.cend(); } \
   public: \
   inline Class##Iterator(const Class##<Key,T> &container) \
   : m_container(container), m_iter(m_container.cbegin()), n(m_container.cend()) {} \
   inline Class##Iterator &operator=(const Class##<Key,T> &container) \
{ m_container = container; m_iter = m_container.cbegin(); m_next = m_container.cend(); return *this; } \
   inline void toFront() { m_iter = m_container.cbegin(); m_next = m_container.cend(); } \
   inline void toBack() { m_iter = m_container.cend(); m_next = m_container.cend(); } \
   inline bool hasNext() const { return m_iter != m_container.cend(); } \
   inline Item next() { m_next = m_iter++; return m_next; } \
   inline Item peekNext() const { return m_iter; } \
   inline bool hasPrevious() const { return m_iter != m_container.cbegin(); } \
   inline Item previous() { m_next = --m_iter; return m_next; } \
   inline Item peekPrevious() const { const_iterator p = m_iter; return --p; } \
   inline const T &value() const { PDK_ASSERT(itemExists()); return *m_next; } \
   inline const Key &key() const { PDK_ASSERT(itemExists()); return m_next.key(); } \
   inline bool findNext(const T &t) \
{ while ((m_next = m_iter) != m_container.cend()) if (*m_iter++ == t) return true; return false; } \
   inline bool findPrevious(const T &t) \
{ while (m_iter != m_container.cbegin()) if (*(m_next = --m_iter) == t) return true; \
   m_next = m_container.cend(); return false; } \
};

#define PDK_DECLARE_MUTABLE_ASSOCIATIVE_ITERATOR(Class) \
   \
   template <class Key, class T> \
   class Mutable##Class##Iterator \
{ \
   typedef typename Class##<Key,T>::iterator iterator; \
   typedef typename Class##<Key,T>::const_iterator const_iterator; \
   typedef iterator Item; \
   Class##<Key,T> *m_container; \
   iterator m_iter, m_next; \
   inline bool itemExists() const { return const_iterator(m_next) != m_container->cend(); } \
   public: \
   inline Mutable##Class##Iterator(Class##<Key,T> &container) \
   : m_container(&container) \
{ m_iter = m_container->begin(); m_next = m_container->end(); } \
   inline Mutable##Class##Iterator &operator=(Class##<Key,T> &container) \
{ m_container = &container; m_iter = m_container->begin(); m_next = m_container->end(); return *this; } \
   inline void toFront() { m_iter = m_container->begin(); m_next = m_container->end(); } \
   inline void toBack() { m_iter = m_container->end(); m_next = m_container->end(); } \
   inline bool hasNext() const { return const_iterator(m_iter) != m_container->cend(); } \
   inline Item next() { m_next = m_iter++; return m_next; } \
   inline Item peekNext() const { return m_iter; } \
   inline bool hasPrevious() const { return const_iterator(m_iter) != m_container->cbegin(); } \
   inline Item previous() { m_next = --m_iter; return m_next; } \
   inline Item peekPrevious() const { iterator p = m_iter; return --p; } \
   inline void remove() \
{ if (const_iterator(m_next) != m_container->cend()) { m_iter = m_container->erase(m_next); m_next = m_container->end(); } } \
   inline void setValue(const T &t) { if (const_iterator(m_next) != m_container->cend()) *m_next = t; } \
   inline T &value() { PDK_ASSERT(itemExists()); return *m_next; } \
   inline const T &value() const { PDK_ASSERT(itemExists()); return *m_next; } \
   inline const Key &key() const { PDK_ASSERT(itemExists()); return m_next.key(); } \
   inline bool findNext(const T &t) \
{ while (const_iterator(m_next = m_iter) != m_container->cend()) if (*m_iter++ == t) return true; return false; } \
   inline bool findPrevious(const T &t) \
{ while (const_iterator(m_iter) != m_container->cbegin()) if (*(m_next = --m_iter) == t) return true; \
   m_next = m_container->end(); return false; } \
};

template<typename Key, typename T, class Iterator>
class KeyValueIterator
{
public:
    typedef typename Iterator::iterator_category iterator_category;
    typedef typename Iterator::difference_type difference_type;
    typedef std::pair<Key, T> value_type;
    typedef const value_type *pointer;
    typedef const value_type &reference;

    KeyValueIterator() = default;
    constexpr explicit KeyValueIterator(Iterator other) noexcept(std::is_nothrow_move_constructible<Iterator>::value)
        : m_iter(std::move(other))
    {}

    std::pair<Key, T> operator*() const
    {
        return std::pair<Key, T>(m_iter.key(), m_iter.value());
    }

    friend bool operator==(KeyValueIterator lhs, KeyValueIterator rhs) noexcept
    {
       return lhs.m_iter == rhs.m_iter;
    }
    
    friend bool operator!=(KeyValueIterator lhs, KeyValueIterator rhs) noexcept
    {
       return lhs.m_iter != rhs.m_iter;
    }

    inline KeyValueIterator &operator++()
    {
       ++m_iter;
       return *this;
    }
    
    inline KeyValueIterator operator++(int)
    {
       return KeyValueIterator(m_iter++);
    }
    
    inline KeyValueIterator &operator--()
    {
       --m_iter;
       return *this;
    }
    
    inline KeyValueIterator operator--(int)
    {
       return KeyValueIterator(m_iter--);
    }
    
    Iterator base() const
    {
       return m_iter;
    }

private:
    Iterator m_iter;
};

} // utils
} // pdk

#endif // PDK_UTILS_ITERATOR_H
