#include <fcntl.h>
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
    template <class T>
    posixBinaryFileAccessor<T>::posixBinaryFileAccessor(std::string fileName) : fileNameStr(fileName){};

    template <class T>
    std::string posixBinaryFileAccessor<T>::FileName()
    {
        return fileNameStr;
    }

    template <class T>
    void posixBinaryFileAccessor<T>::Write(const T *ptrData, const size_t size, const size_t position)
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

    template <class T>
    void posixBinaryFileAccessor<T>::Read(T *ptrData, const size_t size, const size_t position)
    {
        int fd = -1;
        fd = ::open(fileNameStr.c_str(), O_RDONLY | kOpenBaseFlags);
        assert(fd >= 0);
        ssize_t read_size = ::pread(fd, ptrData, size, static_cast<off_t>(position));
        assert(read_size >= 0 );
        ::close(fd);
    };

    template class posixBinaryFileAccessor<std::byte>;
    template class posixBinaryFileAccessor<char>;
    template class posixBinaryFileAccessor<unsigned char>;

} // namespace rdb