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
// Created by softboy on 2018/02/22.

#include "pdk/base/io/fs/Resource.h"
#include "pdk/base/io/fs/internal/ResourceIteratorPrivate.h"
#include <variant>

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::io::fs::Resource;

ResourceFileEngineIterator::ResourceFileEngineIterator(Dir::Filters filters, 
                                                       const StringList &filterNames)
   : AbstractFileEngineIterator(filters, filterNames),
     m_index(-1)
{}

ResourceFileEngineIterator::~ResourceFileEngineIterator()
{}

String ResourceFileEngineIterator::next()
{
   if (!hasNext()) {
      return String();
   }
   ++m_index;
   return getCurrentFilePath();
}

bool ResourceFileEngineIterator::hasNext() const
{
   if (m_index == -1) {
      Resource resource(getPath());
      if (!resource.isValid()) {
         return false;
      }
      // Initialize and move to the next entry.
      m_entries = resource.getChildren();
      m_index = 0;
   }
   return static_cast<size_t>(m_index) < m_entries.size();
}

String ResourceFileEngineIterator::getCurrentFileName() const
{
   if (m_index <= 0 || static_cast<size_t>(m_index) > m_entries.size()) {
      return String();
   }
   return m_entries.at(m_index - 1);
}

} // internal
} // fs
} // io
} // pdk
