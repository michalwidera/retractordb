#include "serviceControl.hpp"

#include <sys/wait.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include <spdlog/spdlog.h>

namespace {

// Kod wyjścia dziecka, gdy execvp() się nie powiedzie — konwencja powłoki "command not found".
constexpr int kExecFailedExitCode{127};

// Domyślny wykonawca: fork + execvp("systemctl", argv). Bez shella (brak ryzyka interpretacji argv).
// Zwraca kod wyjścia systemctl albo -1 przy błędzie fork/wait.
int execSystemctl(const std::vector<std::string> &argv) {
  const pid_t pid = fork();
  if (pid < 0) {
    SPDLOG_ERROR("restartService: fork() failed before systemctl exec");
    return -1;
  }
  if (pid == 0) {
    std::vector<char *> cargv;
    cargv.reserve(argv.size() + 1);
    for (const auto &a : argv)
      cargv.push_back(const_cast<char *>(a.c_str()));
    cargv.push_back(nullptr);
    execvp(cargv[0], cargv.data());
    _exit(kExecFailedExitCode);  // exec się nie powiódł (np. brak systemctl w PATH)
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) return -1;
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

}  // namespace

namespace servicecontrol {

int restartService(bool userScope, const std::string &unit, const SystemctlRunner &runner) {
  std::vector<std::string> argv{"systemctl"};
  if (userScope) argv.emplace_back("--user");
  argv.emplace_back("restart");
  argv.push_back(unit);

  return runner ? runner(argv) : execSystemctl(argv);
}

bool deliverQueryFile(const std::string &source, const std::string &target) {
  std::ifstream src(source, std::ios::binary);
  if (!src.is_open()) {
    SPDLOG_ERROR("deliverQueryFile: cannot open source {}", source);
    return false;
  }
  std::ostringstream buf;
  buf << src.rdbuf();

  const std::filesystem::path targetPath(target);
  const std::filesystem::path tmpPath =
      targetPath.parent_path() / (targetPath.filename().string() + ".tmp." + std::to_string(::getpid()));

  {
    std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
      SPDLOG_ERROR("deliverQueryFile: cannot open temp {}", tmpPath.string());
      return false;
    }
    out << buf.str();
    out.flush();
    if (!out) {
      SPDLOG_ERROR("deliverQueryFile: write failed to temp {}", tmpPath.string());
      return false;
    }
  }

  std::error_code ec;
  std::filesystem::rename(tmpPath, targetPath, ec);
  if (ec) {
    SPDLOG_ERROR("deliverQueryFile: rename {} -> {} failed: {}", tmpPath.string(), targetPath.string(), ec.message());
    std::filesystem::remove(tmpPath, ec);
    return false;
  }
  return true;
}

}  // namespace servicecontrol
