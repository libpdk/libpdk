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
// Created by softboy on 2017/01/10.

#include "pdk/stdext/optional/Optional.h"

using pdk::stdext::optional::Optional;

#ifdef PDK_UNITTEST_ENABLE_TRACE
#define TRACE(msg) std::cout << msg << std::endl ;
#else
#define TRACE(msg)
#endif

#define ARG(T) (static_cast<const T *>(0))

class X
{
public :
    X (int av) : m_v(av)
    {
        ++ m_count ;
        TRACE ("X::X(" << av << "). this=" << this) ;
    }
    
    X (X const& rhs) : m_v(rhs.m_v)
    {
        m_pendingCopy = false ;
        TRACE ("X::X(X const& rhs). this=" << this << " rhs.v=" << rhs.v) ;
        if (m_throwOnCopy)
        {
            TRACE ("throwing exception in X's copy ctor") ;
            throw 0 ;
        }
        ++ m_count ;
    }
    
    ~X()
    {
        m_pendingDtor = false ;
        -- m_count ;
        TRACE ("X::~X(). v=" << v << "  this=" << this);
    }
    
    X& operator= (X const& rhs)
    {
        m_pendingAssign = false ;
        
        if (m_throwOnAssign)
        {
            TRACE ("throwing exception in X's assignment") ;
            m_v = -1 ;
            throw 0 ;
        }
        else
        {
            m_v = rhs.m_v ;
            
            TRACE ("X::operator =(X const& rhs). this=" << this << " rhs.v=" << rhs.m_v) ;
        }
        return *this ;
    }
    
    friend bool operator == (X const& a, X const& b)
    { return a.m_v == b.m_v ; }
    
    friend bool operator != (X const& a, X const& b)
    { return a.m_v != b.m_v ; }
    
    friend bool operator < (X const& a, X const& b)
    { return a.m_v < b.m_v ; }
    
    int  V() const { return m_v ; }
    int& V()       { return m_v ; }
    
    static int  m_count;
    static bool m_pendingCopy;
    static bool m_pendingDtor;
    static bool m_pendingAssign;
    static bool m_throwOnCopy;
    static bool m_throwOnAssign;
    
private :
    int  m_v ;
private :
    
    X() ;
};

int X::m_count = 0;
bool X::m_pendingCopy = false;
bool X::m_pendingAssign = false;
bool X::m_pendingDtor = false;
bool X::m_throwOnAssign = false;
bool X::m_throwOnCopy = false;

inline void set_pending_copy(const X *)
{ 
    X::m_pendingCopy = true;
}

inline void set_pending_dtor(const X *)
{ 
    X::m_pendingDtor = true;
}
inline void set_pending_assign(const X *)
{ 
    X::m_pendingAssign = true;
}

inline void set_throw_on_copy(const X *)
{ 
    X::m_throwOnCopy = true;
}

inline void set_throw_on_assign(const X *)
{ 
    X::m_throwOnAssign = true;
}

inline void reset_throw_on_copy(const X *)
{ 
    X::m_throwOnCopy = false;
}

inline void reset_throw_on_assign(const X *) 
{
    X::m_throwOnAssign = false;
}

inline void check_is_pending_copy(const X *)
{ 
    ASSERT_TRUE( X::m_pendingCopy );
}

inline void check_is_pending_dtor(const X *)
{ 
    ASSERT_TRUE( X::m_pendingDtor );
}

inline void check_is_pending_assign(const X *) 
{ 
    ASSERT_TRUE( X::m_pendingAssign );
}

inline void check_is_not_pending_copy(const X *)
{ 
    ASSERT_TRUE( !X::m_pendingCopy );
}

inline void check_is_not_pending_dtor(const X *)
{ 
    ASSERT_TRUE( !X::m_pendingDtor );
}

inline void check_is_not_pending_assign(const X *)
{ 
    ASSERT_TRUE( !X::m_pendingAssign );
}

inline void check_instance_count( int c,const X *)
{ 
    ASSERT_TRUE( X::m_count == c );
}

inline int  get_instance_count(const X *)
{
    return X::m_count;
}

inline void set_pending_copy           (...) {}
inline void set_pending_dtor           (...) {}
inline void set_pending_assign         (...) {}
inline void set_throw_on_copy          (...) {}
inline void set_throw_on_assign        (...) {}
inline void reset_throw_on_copy        (...) {}
inline void reset_throw_on_assign      (...) {}
inline void check_is_pending_copy      (...) {}
inline void check_is_pending_dtor      (...) {}
inline void check_is_pending_assign    (...) {}
inline void check_is_not_pending_copy  (...) {}
inline void check_is_not_pending_dtor  (...) {}
inline void check_is_not_pending_assign(...) {}
inline void check_instance_count       (...) {}
inline int  get_instance_count         (...) { return 0 ; }

template <typename T>
inline void check_uninitialized_const(const Optional<T> &opt)
{
    ASSERT_EQ(opt, 0);
    ASSERT_FALSE(opt);
    ASSERT_FALSE(opt.getPtr());
}

template <typename T>
inline void check_uninitialized(Optional<T> &opt)
{
    ASSERT_EQ(opt, 0);
    ASSERT_FALSE(opt);
    ASSERT_FALSE(opt.getPtr());
    check_uninitialized_const(opt);
}

template<class T>
inline void check_initialized_const (Optional<T> const& opt)
{
    ASSERT_TRUE(opt) ;
    ASSERT_TRUE(opt != 0) ;
    ASSERT_TRUE(!!opt) ;
    ASSERT_TRUE(opt.getPtr());
}

template<class T>
inline void check_initialized (Optional<T>& opt)
{
    ASSERT_TRUE(opt) ;
    ASSERT_TRUE(opt != 0) ;
    ASSERT_TRUE(!!opt) ;
    ASSERT_TRUE(opt.getPtr());
    check_initialized_const(opt);
}

template <typename T>
inline void check_value_const(const Optional<T>& opt, const T &v, const T &z)
{
    ASSERT_EQ(*opt, v);
    ASSERT_NE(*opt, z);
    ASSERT_EQ(opt.get(), v);
    ASSERT_NE(opt.get(), z);
    ASSERT_EQ(*(opt.operator ->()), v);
    ASSERT_NE(*(opt.operator ->()), z);
}

template <typename T>
inline void check_value(Optional<T>& opt, const T &v, const T &z)
{
    ASSERT_EQ(*opt, v);
    ASSERT_NE(*opt, z);
    ASSERT_EQ(opt.get(), v);
    ASSERT_NE(opt.get(), z);
    ASSERT_EQ(*(opt.operator ->()), v);
    ASSERT_NE(*(opt.operator ->()), z);
}
