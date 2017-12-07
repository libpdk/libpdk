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
// Created by softboy on 2017/12/07.

#include "gtest/gtest.h"
#include "pdk/utils/MemoryHelper.h"
#include <list>
#include <utility>
#include <tuple>
#include <cstdlib>

namespace
{

bool check_size(size_t value, uint min)
{
   return value >= min && value <= INT_MAX;
}

}

TEST(MemoryHelperTest, testBlockSizeCalculations)
{
   ASSERT_EQ(pdk::utils::calculate_block_size(0, 1), static_cast<size_t>(0));
   ASSERT_TRUE(pdk::utils::calculate_growing_block_size(0, 1).m_size <= INT_MAX);
   ASSERT_TRUE(pdk::utils::calculate_growing_block_size(0, 1).m_elementCount <= INT_MAX);
   
   ASSERT_EQ(pdk::utils::calculate_block_size(INT_MAX, 1), static_cast<size_t>(INT_MAX));
   ASSERT_EQ(pdk::utils::calculate_block_size(INT_MAX / 2, 2), static_cast<size_t>(INT_MAX) - 1);
   ASSERT_EQ(pdk::utils::calculate_block_size(INT_MAX / 2, 2, 1), static_cast<size_t>(INT_MAX));
   
   ASSERT_EQ(pdk::utils::calculate_growing_block_size(INT_MAX, 1).m_size, static_cast<size_t>(INT_MAX));
   ASSERT_EQ(pdk::utils::calculate_growing_block_size(INT_MAX, 1).m_elementCount, static_cast<size_t>(INT_MAX));
   ASSERT_EQ(pdk::utils::calculate_growing_block_size(INT_MAX / 2, 2, 1).m_size, static_cast<size_t>(INT_MAX));
   ASSERT_EQ(pdk::utils::calculate_growing_block_size(INT_MAX / 2, 2, 1).m_elementCount, static_cast<size_t>(INT_MAX / 2));
   
   ASSERT_EQ(pdk::utils::calculate_block_size(static_cast<uint>(INT_MAX) + 1, 1), static_cast<size_t>(~0));
   ASSERT_EQ(pdk::utils::calculate_block_size(static_cast<size_t>(-1), 1), static_cast<size_t>(~0));
   ASSERT_EQ(pdk::utils::calculate_block_size(INT_MAX, 1, 1), static_cast<size_t>(~0));
   ASSERT_EQ(pdk::utils::calculate_block_size(INT_MAX / 2 + 1, 2), static_cast<size_t>(~0));
   
   ASSERT_EQ(pdk::utils::calculate_growing_block_size(static_cast<uint>(INT_MAX) + 1, 1).m_size, static_cast<size_t>(~0));
   ASSERT_EQ(pdk::utils::calculate_growing_block_size(INT_MAX / 2 + 1, 2).m_size, static_cast<size_t>(~0));
   
   // overflow conditions
   // on 32-bit platforms, (1 << 16) * (1 << 16) = (1 << 32) which is zero
   ASSERT_EQ(pdk::utils::calculate_block_size(1 << 16, 1 << 16), static_cast<size_t>(~0));
   ASSERT_EQ(pdk::utils::calculate_block_size(INT_MAX / 4, 16), static_cast<size_t>(~0));
   // on 32-bit platforms, (1 << 30) * 3 + (1 << 30) would overflow to zero
   ASSERT_EQ(pdk::utils::calculate_block_size(1U << 30, 3, 1U << 30), static_cast<size_t>(~0));
   
   // exact block sizes
   for (int i = 1; i < 1 << 31; i <<= 1) {
      ASSERT_EQ(pdk::utils::calculate_block_size(0, 1, i), static_cast<size_t>(i));
      ASSERT_EQ(pdk::utils::calculate_block_size(i, 1), static_cast<size_t>(i));
      ASSERT_EQ(pdk::utils::calculate_block_size(i + i/2, 1), static_cast<size_t>(i + i/2));
   }
   for (int i = 1; i < 1 << 30; i <<= 1) {
      ASSERT_EQ(pdk::utils::calculate_block_size(i, 2), 2 * static_cast<size_t>(i));
      ASSERT_EQ(pdk::utils::calculate_block_size(i, 2, 1), 2 * static_cast<size_t>(i) + 1);
      ASSERT_EQ(pdk::utils::calculate_block_size(i, 2, 16), 2 * static_cast<size_t>(i) + 16);
   }
   
   // growing sizes
   for (int i = 1; i < 1 << 30; i <<= 1) {
      ASSERT_TRUE(check_size(pdk::utils::calculate_growing_block_size(i, 1).m_size, i));
      ASSERT_TRUE(check_size(pdk::utils::calculate_growing_block_size(i, 1).m_elementCount, i));
      ASSERT_TRUE(check_size(pdk::utils::calculate_growing_block_size(i, 1, 16).m_size, i));
      ASSERT_TRUE(check_size(pdk::utils::calculate_growing_block_size(i, 1, 16).m_elementCount, i));
      ASSERT_TRUE(check_size(pdk::utils::calculate_growing_block_size(i, 1, 24).m_size, i));
      ASSERT_TRUE(check_size(pdk::utils::calculate_growing_block_size(i, 1, 24).m_elementCount, i));
   }
   
   // growth should be limited
   for (int elementSize = 1; elementSize < (1 << 8); elementSize <<= 1) {
      size_t alloc = 1;
      while (true) {
         ASSERT_TRUE(check_size(pdk::utils::calculate_growing_block_size(alloc, elementSize).m_size, alloc * elementSize));
         size_t newAlloc = pdk::utils::calculate_growing_block_size(alloc, elementSize).m_elementCount;
         ASSERT_TRUE(check_size(newAlloc, alloc));
         if (newAlloc == alloc) {
            break;
         }
         alloc = newAlloc;
      }
      ASSERT_TRUE(check_size(alloc, static_cast<size_t>(INT_MAX) / elementSize));
      // the next allocation should be invalid
      ASSERT_EQ(pdk::utils::calculate_growing_block_size(alloc + 1, elementSize).m_size, static_cast<size_t>(~0));
   }
}
