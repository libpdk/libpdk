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
// Created by softboy on 2018/02/08.

#ifndef PDK_M_BASE_IO_FS_RESOURCE_H
#define PDK_M_BASE_IO_FS_RESOURCE_H

#include "pdk/base/lang/String.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/utils/Locale.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdk/base/ds/StringList.h"

namespace pdk {
namespace io {
namespace fs {

// forward declare class with namespace
namespace internal {
class ResourcePrivate;
class ResourceFileEngine;
class ResourceFileEngineIterator;
} // internal

using pdk::lang::String;
using pdk::utils::Locale;
using pdk::time::DateTime;
using pdk::ds::StringList;
using internal::ResourcePrivate;
using internal::ResourceFileEngine;
using internal::ResourceFileEngineIterator;

class PDK_CORE_EXPORT Resource
{
public:
    Resource(const String &file= String(), const Locale &locale = Locale());
    ~Resource();

    void setFileName(const String &file);
    String getFileName() const;
    String getAbsoluteFilePath() const;

    void setLocale(const Locale &locale);
    Locale getLocale() const;

    bool isValid() const;

    bool isCompressed() const;
    pdk::pint64 getSize() const;
    const uchar *getData() const;
    DateTime lastModified() const;

    static void addSearchPath(const String &path);
    static StringList getSearchPaths();

    static bool registerResource(const String &rccFilename, const String &resourceRoot = String());
    static bool unregisterResource(const String &rccFilename, const String &resourceRoot = String());

    static bool registerResource(const uchar *rccData, const String &resourceRoot = String());
    static bool unregisterResource(const uchar *rccData, const String &resourceRoot = String());

protected:
    friend class ResourceFileEngine;
    friend class ResourceFileEngineIterator;
    bool isDir() const;
    inline bool isFile() const
    {
       return !isDir();
    }
    
    StringList children() const;

protected:
    pdk::utils::ScopedPointer<ResourcePrivate> m_implPtr;

private:
    PDK_DECLARE_PRIVATE(Resource);
};

} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_RESOURCE_H
