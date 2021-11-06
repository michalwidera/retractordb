#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#ifndef __Fuchsia__
#include <sys/resource.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <assert.h>

#include "faccposix.h"

// Common flags defined for all posix open operations
#if defined(HAVE_O_CLOEXEC)
constexpr const int kOpenBaseFlags = O_CLOEXEC;
#else
constexpr const int kOpenBaseFlags = 0;
#endif // defined(HAVE_O_CLOEXEC)

namespace rdb
{
    posixBinaryFileAccessor::posixBinaryFileAccessor(std::string fileName) : fileNameStr(fileName){};

    std::string posixBinaryFileAccessor::FileName()
    {
        return fileNameStr;
    }

    void posixBinaryFileAccessor::Write(const std::byte *ptrData, const size_t size, const size_t position)
    {
        int fd;

        if (position == std::numeric_limits<size_t>::max())
        {
            fd = ::open(fileNameStr.c_str(), O_APPEND | O_RDWR | O_CREAT | kOpenBaseFlags, 0644);
            assert(fd > 0);
            auto result = ::lseek(fd, 0, SEEK_END);
            assert( result != static_cast<off_t>(-1) );
        }
        else
        {
            fd = ::open(fileNameStr.c_str(), O_RDWR | O_CREAT | kOpenBaseFlags, 0644);
            assert(fd > 0);
            auto result = ::lseek(fd, position, SEEK_SET);
            assert( result != static_cast<off_t>(-1) );
        }

        size_t sizesh(size);

        while (sizesh > 0)
        {
            ssize_t write_result = ::write(fd, ptrData, sizesh);
            if (write_result < 0)
            {
                if (errno == EINTR)
                {
                    continue; // Retry
                }
                assert(errno);
            }
            ptrData += write_result;
            sizesh -= write_result;
        }
        ::close(fd);
    };

    void posixBinaryFileAccessor::Read(std::byte *ptrData, const size_t size, const size_t position)
    {
        int fd = -1;
        fd = ::open(fileNameStr.c_str(), O_RDONLY | kOpenBaseFlags);
        assert(fd >= 0);
        ssize_t read_size = ::pread(fd, ptrData, size, static_cast<off_t>(position));
        assert(read_size >= 0 );
        ::close(fd);
    };

} // namespace rdb