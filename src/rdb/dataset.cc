#include <cstddef>

#include "rdb/dataset.h"
#include "rdb/dsacc.h"
#include "rdb/desc.h"

namespace rdb
{
        dataSet::~dataSet()
        {
            for (auto const& [_, val] : data)
            {
                auto statusRemove1 = remove(val.get()->pAccessor->FileName().c_str());
                auto descFilename( val.get()->pAccessor->FileName() + ".desc" );
                auto statusRemove2 = remove(descFilename.c_str());
            }
        }

        long dataSet::streamStoredSize(std::string filename)
        {
            return data[filename]->recordsCount ;
        }

        long dataSet::GetLen(std::string filename)
        {
            return data[filename]->recordsCount * recordSize;
        }

        void dataSet::DefBlock(std::string filename, int frameSize)
        {
            recordSize = frameSize;
            auto desc{rdb::Descriptor("payload", frameSize, rdb::Bytearray)};
            data[filename] = std::unique_ptr<rdb::DataStorageAccessor<std::byte>>(new rdb::DataStorageAccessor(desc, filename, false));
            recordSize = desc.GetSize();
        }

        void dataSet::PutBlock(std::string filename, char *pRawData)
        {
            data[filename]->Put(reinterpret_cast<std::byte *>(pRawData));
        }

        int dataSet::GetBlock(std::string filename, int offset, char *pRawData)
        {
            bool success = data[filename]->Get(reinterpret_cast<std::byte *>(pRawData),offset);
            return success ? 1 : 0 ;
        }
}
