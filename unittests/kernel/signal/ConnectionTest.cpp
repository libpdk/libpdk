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
// Created by softboy on 2018/01/21.

// Boost.Signals library

// Copyright (C) Douglas Gregor 2001-2006. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include "gtest/gtest.h"
#include "pdk/kernel/signal/Signal.h"
#include <optional>

namespace Signals = pdk::kernel::signal;

namespace {

typedef Signals::Signal<void ()> SignalType;

void my_slot()
{}

}

TEST(ConnectionTest, testConnectSwap)
{
   SignalType signal1;
   {
      Signals::Connection conn1 = signal1.connect(&my_slot);
      Signals::Connection conn2;
      ASSERT_TRUE(conn1.connected());
      ASSERT_FALSE(conn2.connected());
      conn1.swap(conn2);
      ASSERT_FALSE(conn1.connected());
      ASSERT_TRUE(conn2.connected());
      conn1.swap(conn2);
      ASSERT_TRUE(conn1.connected());
      ASSERT_FALSE(conn2.connected());
   }
   {
      Signals::ScopedConnection conn1;
      Signals::ScopedConnection conn2;
      conn1 = signal1.connect(&my_slot);
      ASSERT_TRUE(conn1.connected());
      ASSERT_FALSE(conn2.connected());
      conn1.swap(conn2);
      ASSERT_FALSE(conn1.connected());
      ASSERT_TRUE(conn2.connected());
      conn1.swap(conn2);
      ASSERT_TRUE(conn1.connected());
      ASSERT_FALSE(conn2.connected());
   }
}

TEST(ConnectionTest, testRelease)
{
   SignalType signal;
   Signals::Connection conn;
   {
      Signals::ScopedConnection scoped(signal.connect(&my_slot));
      ASSERT_TRUE(scoped.connected());
      conn = scoped.release();
   }
   ASSERT_TRUE(conn.connected());
   Signals::Connection conn2;
   {
      Signals::ScopedConnection scoped(conn);
      ASSERT_TRUE(scoped.connected());
      conn = scoped.release();
      ASSERT_TRUE(conn.connected());
      ASSERT_FALSE(scoped.connected());
      conn.disconnect();
      
      // earlier release shouldn't affect new connection
      conn2 = signal.connect(&my_slot);
      scoped = conn2;
   }
   ASSERT_FALSE(conn2.connected());
}

TEST(ConnectionTest, testMove)
{
   SignalType signal;
   Signals::Connection conn;
   // test move assignment from scoped_connection to connection
   {
      Signals::ScopedConnection scoped(signal.connect(&my_slot));
      ASSERT_TRUE(scoped.connected());
      conn = std::move(scoped);
      ASSERT_FALSE(scoped.connected());
   }
   // test move construction from scoped to scoped
   ASSERT_TRUE(conn.connected());
   {
      Signals::ScopedConnection scoped2(conn);
      ASSERT_TRUE(scoped2.connected());
      Signals::ScopedConnection scoped3(std::move(scoped2));
      ASSERT_TRUE(scoped3.connected());
      ASSERT_FALSE(scoped2.connected());
      ASSERT_TRUE(conn.connected());
   }
   ASSERT_FALSE(conn.connected());
   // test move assignment from scoped to scoped
   conn = signal.connect(&my_slot);
   {
      Signals::ScopedConnection scoped3;
      Signals::ScopedConnection scoped2(conn);
      ASSERT_TRUE(scoped2.connected());
      scoped3 = std::move(scoped2);
      ASSERT_TRUE(scoped3.connected());
      ASSERT_FALSE(scoped2.connected());
      ASSERT_TRUE(conn.connected());
   }
   ASSERT_FALSE(conn.connected());
}
