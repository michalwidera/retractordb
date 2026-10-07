# C++ API

Requires Linux and C++23. Building from source also needs CMake and Boost headers with Boost.JSON, provided by the repository's Conan setup. The public header uses only the standard library.

The API is optional in the repository build. Build `rdb_monitor` explicitly to produce the example and its `rdb_client` and `rdb_api_json` dependencies. Use `ninja install-withapi` from the build directory to install the engine together with both static libraries, the public header, and CMake package configuration. Plain `ninja` and `ninja install` omit the API; `ninja test-api` builds and runs its tests. Configure with `-DRDB_WITH_API=ON` to include the API in CPack packages and its tests in the ordinary `test` target.

```sh
cmake --build build/Debug --target rdb_monitor
build/Debug/api/cpp/rdb_monitor laboratory temperature 10 /path/to/xqry
```

For an application built alongside the source:

```cmake
add_subdirectory(/path/to/retractordb/api/cpp rdb-api)
target_link_libraries(my_monitor PRIVATE RetractorDB::client)
```

For an installed SDK (no Boost headers needed by the consumer):

```cmake
find_package(RetractorDBClient CONFIG REQUIRED)
target_link_libraries(my_monitor PRIVATE RetractorDB::client)
```

The server must already be running. Select it explicitly by name:

```cpp
#include <iostream>
#include "retractordb/client.hpp"

int main() {
  retractordb::Client db("laboratory");
  auto samples = db.subscribe("temperature", {.limit = 10});
  while (auto record = samples.next()) {
    const auto &value = record->values.at("v").front();
    if (const auto *number = std::get_if<double>(&value))
      std::cout << *number << '\n';
  }
}
```

Use `Options{.xqry="/path/to/xqry", .timeout=...}` for the binary and command/schema timeout. `SubscribeOptions` exposes `limit`, `idleTimeout` and `capacity`. Timeouts use `std::chrono::milliseconds`.

`next(timeout)` returns `std::optional<Record>`; empty means normal completion or explicit closure. Errors throw `retractordb::Error` with a string `code`. A `read_timeout` keeps the subscription open. `schema()` returns the schema; `endReason()` identifies normal completion and `pid()` identifies the child. Explicit `close()` and destructors reap that child. Subscriptions are move-only. Destroying the client closes all its subscriptions, including surviving handles.

`Client` is also move-only, with a `noexcept` move constructor and move assignment. Moving transfers its subscriptions; move assignment first closes the destination's previous subscriptions. A moved-from client can be destroyed, closed, or assigned another client. Calling `ping()`, `streams()`, `describe()`, or `subscribe()` on a moved-from client throws `Error("closed")`, as it does on an explicitly closed client when called with valid arguments. Methods returning data or subscriptions are `[[nodiscard]]`.

`ping()` returns `void` on a valid response and throws `retractordb::Error` on failure. The Python equivalent preserves its historical `True` result on success and raises `Error` on failure; it never returns `False`. Existing Python conditions such as `if db.ping():` keep their success branch.

`Record.values` maps names to vectors of `Value`: scalars have one element, arrays have their full length. Use `std::get_if` or `std::visit` for the alternatives in [the shared contract](../README.md). NULL is `std::monostate`.

Two subscriptions own different processes. Serialize calls on each handle. The example handles SIGINT/SIGTERM with a stop flag and timed reads; the library does not change the application's signal handlers.
