#pragma once

#include <string>

namespace rdb {

// Przemianowanie archiwum i utrwalenie wpisow obu katalogow (jednego, gdy jest wspolny).
// false oznacza blad z diagnostyka ERROR, takze gdy rename sie udal, ale fsync/close nie.
// Nie jest to transakcja rodziny plikow ani utrwalenie zawartosci samego pliku.
[[nodiscard]] bool rotateStorageFile(const std::string &source, const std::string &archive);

}  // namespace rdb
