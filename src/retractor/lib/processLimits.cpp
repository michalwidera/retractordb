#include "processLimits.hpp"

#include <algorithm>
#include <cerrno>

namespace proclimits {

OpenFilesLimit raiseOpenFilesLimit(const rlim_t target) {
  struct rlimit limit{};
  if (::getrlimit(RLIMIT_NOFILE, &limit) != 0) return {.err = errno};
  OpenFilesLimit result{.before = limit.rlim_cur, .after = limit.rlim_cur, .hard = limit.rlim_max};
  const rlim_t wanted = std::min(target, limit.rlim_max);
  if (limit.rlim_cur >= wanted) return result;
  limit.rlim_cur = wanted;
  if (::setrlimit(RLIMIT_NOFILE, &limit) != 0) {
    result.err = errno;
    return result;
  }
  result.after = wanted;
  return result;
}

}  // namespace proclimits
