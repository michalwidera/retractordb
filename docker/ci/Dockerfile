# https://hub.docker.com/_/debian
FROM debian:trixie-slim

# Wersja bez wartości domyślnej — jedyne źródło prawdy to plik VERSION w korzeniu
# repo. Przekazywana przy buildzie: --build-arg RDB_VERSION="$(cat VERSION)".
ARG RDB_VERSION

LABEL com.retractordb.version="${RDB_VERSION}"
LABEL vendor1="Michal Widera"
LABEL com.retractordb.release-date="2026-08-19"
LABEL com.retractordb.description="Development environment for RetractorDB project"
LABEL com.retractordb.url="https://retractordb.com"

# Set environment variables to avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Update package list and install essential packages
RUN apt-get update && apt-get upgrade -y && apt-get install -y --no-install-recommends \
    # GCC toolchain and build essentials
    openssh-client \
    build-essential \
    gcc \
    g++ \
    gdb \
    make \
    cmake \
    ninja-build \
    python3 \
    python3-pip \
    python3-venv \
    valgrind \
    cppcheck \
    mold \
    ccache \
    # Additional development tools
    git \
    wget \
    curl \
    vim \
    nano \
    clang-format \
    # Library development packages
    libc6-dev \
    linux-libc-dev \
    # Package management tools
    apt-utils \
    pkg-config \
    sudo \
    graphviz \
    gcovr \
    lcov \
    bsdmainutils \
    graphviz \
    feh \
    tmux \
    gnuplot \
    ripgrep \
    # Clean up to reduce image size
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Verify C++23 support.
# Tests std::ranges::fold_left (<algorithm>) and the uz size_t literal — both C++23,
# both used in the RetractorDB codebase.  Build fails here if the compiler is too old.
RUN echo '#include <algorithm>'                                                           > /tmp/cxx23check.cpp \
    && echo '#include <cstddef>'                                                         >> /tmp/cxx23check.cpp \
    && echo '#include <vector>'                                                          >> /tmp/cxx23check.cpp \
    && echo 'int main() {'                                                               >> /tmp/cxx23check.cpp \
    && echo '  std::vector<int> v{1, 2, 3};'                                            >> /tmp/cxx23check.cpp \
    && echo '  std::size_t s = std::ranges::fold_left(v, 0uz,'                          >> /tmp/cxx23check.cpp \
    && echo '      [](std::size_t a, int x){ return a + std::size_t(x); });'            >> /tmp/cxx23check.cpp \
    && echo '  return s == 6 ? 0 : 1;'                                                  >> /tmp/cxx23check.cpp \
    && echo '}'                                                                          >> /tmp/cxx23check.cpp \
    && g++ -std=c++23 -o /tmp/cxx23check /tmp/cxx23check.cpp \
    && /tmp/cxx23check \
    && rm /tmp/cxx23check.cpp /tmp/cxx23check \
    && echo "C++23 OK — g++ $(g++ -dumpversion)"

# Create a non-root user for development (optional but recommended)
RUN useradd -m -s /bin/bash developer && \
    usermod -aG sudo developer

# Allow passwordless sudo for the developer user
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# Set working directory
WORKDIR /home/developer/workspace

# Change ownership of workspace to developer user
RUN chown -R developer:developer /home/developer/workspace

# Switch to non-root user
USER developer

# Generuj DockerConan.txt wewnątrz kontenera na podstawie conanfile.py.
# Gdy conanfile.py ma additional_docker_requires, powstaje też DockerConanExtra.txt
# ze starszymi wersjami: conan nie przyjmuje dwóch wersji pakietu w jednym [requires],
# więc grzeją cache osobnym przebiegiem instalacji poniżej.
# docker build wywołuj z katalogu głównego repo: docker build -f docker/ci/Dockerfile .
COPY --chown=developer:developer conanfile.py /tmp/conanfile.py
COPY --chown=developer:developer docker/ci/gen_dockerconan.sh /tmp/gen_dockerconan.sh
RUN bash /tmp/gen_dockerconan.sh /tmp/conanfile.py /home/developer/workspace/DockerConan.txt \
    && rm /tmp/conanfile.py /tmp/gen_dockerconan.sh

# Venv w $HOME, nie w WORKDIR: scripts/buildrdb.sh wykrywa conana wyłącznie po
# ~/.venv/bin/conan (tool_installed). Przy venv w katalogu roboczym CI go nie
# widzi i przy każdym jobie stawia drugi venv z pip install conan od zera —
# CircleCI i tak pracuje w ~/retractordb, nie w WORKDIR obrazu.
#
# CMake przypięty w tym samym venv, bo apt-owy z trixie (3.31) jest poniżej
# podłogi projektu. Bez tego `scripts/buildrdb.sh toolchain` dociąga koło cmake
# w KAŻDYM jobie CI — siedem razy na przebieg nocny; instalacja tutaj kasuje ten
# koszt raz na obraz. `--only-binary`: PyPI ma dla cmake także sdist, a jego
# budowa to kompilacja całego CMake ze źródeł.
#
# To CZWARTE miejsce z tą samą podłogą wersji. Pozostałe trzy: RDB_CMAKE_MIN_VERSION
# w scripts/buildrdb.sh, tool_requires w conanfile.py i górna granica zakresu
# cmake_minimum_required w CMakeLists.txt. Podnosi się je razem, w jednym commicie.
RUN python3 -m venv ~/.venv \
    && . ~/.venv/bin/activate \
    && pip install --upgrade pip \
    && pip install cmakelang conan \
    && pip install --only-binary=:all: "cmake>=4.4.2" \
    && conan profile detect \
    && sed -i -e "s/gnu17/gnu23/g" ~/.conan2/profiles/default \
    && echo '[conf]' >> ~/.conan2/profiles/default \
    && echo 'tools.cmake.cmaketoolchain:generator=Ninja' >> ~/.conan2/profiles/default \
    && conan install DockerConan.txt -s build_type=Release --build missing \
    && conan install DockerConan.txt -s build_type=Debug --build missing \
    && if [ -f DockerConanExtra.txt ]; then \
         conan install DockerConanExtra.txt -s build_type=Release --build missing \
         && conan install DockerConanExtra.txt -s build_type=Debug --build missing; \
       fi \
    && conan cache clean "*" \
    && pip cache purge

RUN echo "source ~/.venv/bin/activate" >> ~/.bashrc
# RUN echo "cat /etc/lsb-release" >> ~/.bashrc

# Set default command
CMD ["/bin/bash"]

# Optional: Expose ports if your application needs them
# EXPOSE 8080

# Podgląd rozmiaru warstw — dive: https://github.com/wagoodman/dive
# (na Windows najpierw uruchom Docker Desktop)
#   docker image ls
#   dive <IMAGE ID>

# ── Budowa i publikacja obrazu ───────────────────────────────────────────────
#
# 1. Budowa. Wywoływać z KORZENIA repo: kontekst musi objąć conanfile.py oraz
#    docker/ci/gen_dockerconan.sh, oba kopiowane niżej. Buduj po każdej zmianie
#    zależności w conanfile.py — inaczej joby -mydocker dobudowują je w locie.
#
#      docker build --build-arg RDB_VERSION="$(cat VERSION)" -f docker/ci/Dockerfile -t buildenv-retractordb .
#
#    Uwaga: docker/ci/DockerConan.txt z repo NIE trafia do obrazu — jest tylko
#    lokalnym podglądem (i jest w .gitignore). Listę zależności generuje
#    gen_dockerconan.sh wewnątrz kontenera z conanfile.py, więc ręczne zmiany
#    w tym pliku nie mają na obraz żadnego wpływu.
#
# 2. Weryfikacja przed pushem — czy cache ma wersje z conanfile.py (i te
#    z additional_docker_requires, jeśli krotka istnieje):
#
#      docker run --rm buildenv-retractordb bash -c 'PATH=~/.venv/bin:$PATH conan list "*"'
#
#    Kontrola venv — musi być w $HOME, bo tam i tylko tam szuka conana
#    scripts/buildrdb.sh; przy złej ścieżce CI stawia drugi venv przy każdym jobie:
#
#      docker run --rm buildenv-retractordb ls -d /home/developer/.venv
#
#    Shell w obrazie, gdy trzeba pogrzebać ręcznie:
#
#      docker run -it buildenv-retractordb
#
# 3. Publikacja. Po pushu joby -mydocker biorą nowy :latest przy następnym
#    uruchomieniu pipeline'u — CircleCI nie buduje tego obrazu samo.
#
#      docker login
#      docker tag buildenv-retractordb micwide/buildenv-retractordb:latest
#      docker push micwide/buildenv-retractordb:latest

# Sprzątanie po budowach (przebudowa tego obrazu zostawia sporo śmieci):
#   docker system df               # co ile zajmuje
#   docker image prune -f          # obrazy osierocone (dangling)
#   docker builder prune -f        # cache BuildKita — rośnie do dziesiątek GB
# https://stackoverflow.com/questions/44785585/how-can-i-delete-all-local-docker-images
