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

#ifndef PDK_M_BASE_IO_FS_INTERNAL_RESOURCE_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_RESOURCE_PRIVATE_H

#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::io::IoDevice;
using pdk::time::DateTime;

class ResourceFileEnginePrivate;
class ResourceFileEngine : public AbstractFileEngine
{
private:
   PDK_DECLARE_PRIVATE(ResourceFileEngine);
public:
   explicit ResourceFileEngine(const String &path);
   ~ResourceFileEngine();
   
   virtual void setFileName(const String &file) override;
   
   virtual bool open(IoDevice::OpenMode flags) override ;
   virtual bool close() override;
   virtual bool flush() override;
   virtual pdk::pint64 getSize() const override;
   virtual pdk::pint64 getPosition() const override;
   virtual bool atEnd() const;
   virtual bool seek(pdk::pint64) override;
   virtual pdk::pint64 read(char *data, pdk::pint64 maxlen) override;
   virtual pdk::pint64 write(const char *data, pdk::pint64 len) override;
   
   virtual bool remove() override;
   virtual bool copy(const String &newName) override;
   virtual bool rename(const String &newName) override;
   virtual bool link(const String &newName) override;
   
   virtual bool isSequential() const override;
   
   virtual bool isRelativePath() const override;
   
   virtual bool mkdir(const String &dirName, bool createParentDirectories) const override;
   virtual bool rmdir(const String &dirName, bool recurseParentDirectories) const override;
   
   virtual bool setSize(pdk::pint64 size) override;
   
   virtual StringList entryList(Dir::Filters filters, const StringList &filterNames) const override;
   
   virtual bool caseSensitive() const override;
   
   virtual FileFlags fileFlags(FileFlags type) const override;
   
   virtual bool setPermissions(uint perms) override;
   
   virtual String fileName(QAbstractFileEngine::FileName file) const override;
   
   virtual uint ownerId(FileOwner) const override;
   virtual String owner(FileOwner) const override;
   
   virtual DateTime fileTime(FileTime time) const override;
   
   virtual Iterator *beginEntryList(Dir::Filters filters, const StringList &filterNames) override;
   virtual Iterator *endEntryList() override;
   
   bool extension(Extension extension, const ExtensionOption *option = 0, ExtensionReturn *output = 0) override;
   bool supportsExtension(Extension extension) const override;
};

} // internal
} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_INTERNAL_RESOURCE_PRIVATE_H
