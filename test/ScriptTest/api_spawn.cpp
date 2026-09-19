#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
std::promise<void> opened;
std::promise<void> resume;
thread_local bool pausePipe = false;

void pauseOnce() {
  if (!pausePipe) return;
  pausePipe = false;
  opened.set_value();
  resume.get_future().wait();
}
int testPipe(int *fd) {
  const int result = ::pipe(fd);
  pauseOnce();
  return result;
}
}  // namespace

#include "apiPlatform.h"
#if RDB_API_HAS_PIPE2
namespace {
int testPipe2(int *fd, int flags) {
  const int result = ::pipe2(fd, flags);
  pauseOnce();
  return result;
}
}  // namespace
#endif

// Przeplatamy wywolania prawdziwego Process bez zmiany kodu produkcyjnego.
// Pauza jest PO utworzeniu potoku, ale PRZED ewentualnym fcntl(FD_CLOEXEC).
#define pipe  testPipe
#define pipe2 testPipe2
#include "../../api/cpp/src/client.cpp"
#undef pipe2
#undef pipe

int main() {
  using namespace std::chrono_literals;
  try {
    auto shortTask = std::async(std::launch::async, [] {
      pausePipe = true;
      // `/bin/sleep 0`, a nie `/bin/true`: na macOS /bin/true NIE ISTNIEJE (true i false
      // stoja w /usr/bin), wiec posix_spawnp oddawalo ENOENT i test padal na "No such file
      // or directory", zanim zdazyl sprawdzic cokolwiek o deskryptorach. Biorac ten sam
      // program, co zadanie dlugie, test zalezy od JEDNEJ bezwzglednej sciezki zamiast
      // dwoch - a rozni sie od niej dokladnie tym, o co w nim chodzi: czasem zycia dziecka.
      retractordb::Process quick({"/bin/sleep", "0"}, 10);
      try {
        quick.read(500ms);
      } catch (const retractordb::Error &error) {
        if (error.code == "process_exit") return;
        throw;
      }
      throw std::runtime_error("missing EOF");
    });
    opened.get_future().wait();
    retractordb::Process longTask({"/bin/sleep", "5"}, 10);
    resume.set_value();
    shortTask.get();
    longTask.close();
    std::cout << "PASS concurrent spawn descriptor isolation\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
