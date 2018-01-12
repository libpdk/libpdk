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
// Created by softboy on 2017/01/12.

#include "gtest/gtest.h"
#include "pdk/stdext/optional/Optional.h"
#include <type_traits>

namespace  {

struct PrivDefault
{
private: PrivDefault() {}
};

struct CustDefault
{
   CustDefault() {}
};

struct CustomizedTrivial
{
   CustomizedTrivial() {}
};

struct DeletedDefault
{
   DeletedDefault() = delete;
};

struct CustDtor
{
   ~CustDtor() {}
};

struct NoDefault
{
   explicit NoDefault(int) {}
};

struct Empty {};

template <typename T, typename U>
struct Aggregate { T t; U u; };

struct CustAssign
{
   CustAssign& operator=(CustAssign const&) { return *this; }
};

struct CustMove
{
   CustMove(CustMove &&) {}
};
}

namespace pdk { 
namespace stdext { 
namespace optional { 
namespace config {

template <> struct OptionalUsesDirectStorageFor<CustomizedTrivial> : std::true_type {};

}
}
}
}

using pdk::stdext::optional::config::OptionalUsesDirectStorageFor;

void test_type_traits()
{
   // this only tests if type traits are implemented correctly
   PDK_STATIC_ASSERT(( OptionalUsesDirectStorageFor<int>::value ));
   PDK_STATIC_ASSERT(( OptionalUsesDirectStorageFor<double>::value ));
   
   PDK_STATIC_ASSERT(( OptionalUsesDirectStorageFor<CustomizedTrivial>::value ));
   
   PDK_STATIC_ASSERT((! OptionalUsesDirectStorageFor<PrivDefault>::value ));
   PDK_STATIC_ASSERT((! OptionalUsesDirectStorageFor<NoDefault>::value ));
   PDK_STATIC_ASSERT((! OptionalUsesDirectStorageFor<CustDefault>::value ));
   PDK_STATIC_ASSERT((! OptionalUsesDirectStorageFor<Aggregate<int, CustDefault> >::value ));
   
   PDK_STATIC_ASSERT((! OptionalUsesDirectStorageFor<CustDtor>::value ));
   PDK_STATIC_ASSERT((! OptionalUsesDirectStorageFor<CustAssign>::value ));
   PDK_STATIC_ASSERT((! OptionalUsesDirectStorageFor<CustMove>::value ));
   PDK_STATIC_ASSERT((! OptionalUsesDirectStorageFor<Aggregate<int, CustMove> >::value ));
   
   //  PDK_STATIC_ASSERT(( pdk::stdext::optional::internal::Isis_type_trivially_copyable<int> ));
   //  PDK_STATIC_ASSERT(( Optional_detail::is_type_trivially_copyable<double> ));
   
   PDK_STATIC_ASSERT((! OptionalUsesDirectStorageFor<Empty>::value ));
   PDK_STATIC_ASSERT((! OptionalUsesDirectStorageFor<Aggregate<int, double> >::value ));
   PDK_STATIC_ASSERT((! OptionalUsesDirectStorageFor<Aggregate<Aggregate<Empty, int>, double> >::value ));
   
   //  PDK_STATIC_ASSERT(( Optional_detail::is_type_trivially_copyable<Empty> ));
   //  PDK_STATIC_ASSERT(( Optional_detail::is_type_trivially_copyable<Aggregate<int, double> > ));
   //  PDK_STATIC_ASSERT(( Optional_detail::is_type_trivially_copyable<Aggregate<Aggregate<Empty, int>, double> > ));
   
   
   PDK_STATIC_ASSERT((! OptionalUsesDirectStorageFor<DeletedDefault>::value ));
   PDK_STATIC_ASSERT((! OptionalUsesDirectStorageFor<Aggregate<int, DeletedDefault> >::value ));
   
   //  PDK_STATIC_ASSERT((! Optional_detail::is_type_trivially_copyable<CustDtor> ));
   //  PDK_STATIC_ASSERT((! Optional_detail::is_type_trivially_copyable<CustAssign> ));
   //  PDK_STATIC_ASSERT((! Optional_detail::is_type_trivially_copyable<CustMove> ));
   //  PDK_STATIC_ASSERT((! Optional_detail::is_type_trivially_copyable<Aggregate<int, CustMove> > ));
}

using pdk::stdext::optional::internal::TriviallyCopyableOptionalBase;
using pdk::stdext::optional::Optional;

void test_trivial_copyability()
{
   PDK_STATIC_ASSERT((std::is_base_of<TriviallyCopyableOptionalBase<int>, Optional<int> >::value));
   PDK_STATIC_ASSERT((std::is_base_of<TriviallyCopyableOptionalBase<double>, Optional<double> >::value ));
   PDK_STATIC_ASSERT((std::is_base_of<TriviallyCopyableOptionalBase<CustomizedTrivial>, Optional<CustomizedTrivial> >::value ));
   PDK_STATIC_ASSERT((!std::is_base_of<TriviallyCopyableOptionalBase<DeletedDefault>, Optional<DeletedDefault> >::value ));
   
   //  PDK_STATIC_ASSERT(( Optional_detail::is_type_trivially_copyable<Optional<int> > ));
   //  PDK_STATIC_ASSERT(( Optional_detail::is_type_trivially_copyable<Optional<double> > ));
   //  PDK_STATIC_ASSERT(( Optional_detail::is_type_trivially_copyable<Optional<CustomizedTrivial> > ));
   
   //  PDK_STATIC_ASSERT((! Optional_detail::is_type_trivially_copyable<Optional<DeletedDefault> > ));
   
   //  PDK_STATIC_ASSERT((! Optional_detail::is_type_trivially_copyable<Optional<Empty> > ));
   //  PDK_STATIC_ASSERT((! Optional_detail::is_type_trivially_copyable<Optional<Aggregate<int, double> > > ));
   //  PDK_STATIC_ASSERT((! Optional_detail::is_type_trivially_copyable<Optional<Aggregate<Aggregate<Empty, int>, double> > > ));
}

TEST(OptionalStaticPropertiesTest, testStaticsProperties)
{
   test_type_traits();
   test_trivial_copyability();
}

namespace {

struct NotDefaultConstructible
{
   NotDefaultConstructible() = delete;
};

void test_tc_base()
{
   Optional<NotDefaultConstructible> o;
   
   ASSERT_TRUE(pdk::stdext::none == o);
}

struct S
{
   
};

template<class T>
struct W
{
   T& t_;
   
   template<class... Args>
   W(Args&&... args)
      : t_(std::forward<Args>(args)...)
   {
   }
};

void test_value_init()
{
   {
      S s;
      W<S> w{s};
   }
   Optional<W<S&> > o;
   ASSERT_TRUE(pdk::stdext::none == o);
}


}


TEST(OptionalStaticPropertiesTest, testTcBase)
{
   test_tc_base();
   test_value_init();
}
