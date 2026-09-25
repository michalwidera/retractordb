# Srodowisko budowy kol (wheels) modulu retractordb._core - faza J4
# (docs/jupyter-integration.md). Obraz bazowy to manylinux_2_28 z PyPA:
# AlmaLinux 8, glibc 2.28, gcc-toolset-14 (GCC 14, C++23 razem z <print>),
# CPython 3.10-3.15 w /opt/python i auditwheel. Kolo zbudowane tutaj laduje sie
# na kazdym Linuksie z glibc >= 2.28 - w tym w Google Colab (runtime 2026.07:
# Ubuntu 22.04, glibc 2.35, Python 3.12).
#
# Dlaczego nie manylinux_2_34: ten sam GCC 14, ale obraz ma status ALPHA, a kolo
# nie zaladuje sie na systemach z glibc 2.28-2.33 (RHEL 8, Debian 10/11).
#
# Digest przypiety do tego samego wydania, ktore przypina cibuildwheel 4.x
# (cibuildwheel/resources/pinned_docker_images.cfg, tag 2026.09.05-1): kolo z
# cibuildwheel i proba dymna tutaj powstaja na tym samym toolchainie. Obraz
# jest jednoarchitekturowy; aarch64 to ten sam plik z innym MANYLINUX_IMAGE:
#
#   x86_64   quay.io/pypa/manylinux_2_28_x86_64@sha256:53390351aeb4688114b02c36a23b3e6ce1166ee9b7afc5df1a4f776354fc764c
#   aarch64  quay.io/pypa/manylinux_2_28_aarch64@sha256:ad74e53b713f3b07d8c889c526dc0c6500da9827b45e38739570875fef52e28f
ARG MANYLINUX_IMAGE=quay.io/pypa/manylinux_2_28_x86_64@sha256:53390351aeb4688114b02c36a23b3e6ce1166ee9b7afc5df1a4f776354fc764c
FROM ${MANYLINUX_IMAGE}

# pipefail: potok w RUN ma zglaszac blad pierwszego polecenia, nie ostatniego.
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# Jak w docker/ci/Dockerfile: jedyne zrodlo wersji to plik VERSION w korzeniu
# repo, przekazywany przy budowie (--build-arg RDB_VERSION="$(cat VERSION)").
ARG RDB_VERSION
LABEL com.retractordb.version="${RDB_VERSION}"
LABEL com.retractordb.description="manylinux_2_28 wheel build environment for retractordb._core"
LABEL com.retractordb.url="https://retractordb.com"

# valgrind nie jest potrzebny kolu, tylko konfiguracji: CMakeLists.txt na
# Linuksie szuka go z REQUIRED, bo opakowuje nim kazdy test jednostkowy. Bez
# niego drzewo w obecnym stanie w ogole sie tu nie skonfiguruje.
# hadolint ignore=DL3041
RUN dnf -y install valgrind && dnf clean all

# Narzedzia budowy w osobnym venv, a nie w PATH obrazu: do PATH trafiaja tylko
# dowiazania z /opt/rdb-tools/bin, zeby python3 z venv nie przeslonil
# interpreterow manylinux. CMake przypiety do podlogi projektu, bo cmake obrazu
# bazowego z pipx nie ma takiej gwarancji - to kolejne miejsce z ta sama
# wersja, obok wymienionych w docker/ci/Dockerfile, i podnosi sie je razem.
# `--only-binary`: sdist cmake to kompilacja calego CMake ze zrodel.
RUN /opt/python/cp312-cp312/bin/python3 -m venv /opt/rdb-tools/venv \
    && /opt/rdb-tools/venv/bin/pip install --no-cache-dir --disable-pip-version-check conan ninja \
    && /opt/rdb-tools/venv/bin/pip install --no-cache-dir --disable-pip-version-check --only-binary=:all: "cmake>=4.4.2" \
    && mkdir -p /opt/rdb-tools/bin \
    && for tool in conan cmake ctest ninja; do ln -s "/opt/rdb-tools/venv/bin/$tool" /opt/rdb-tools/bin/; done
ENV PATH=/opt/rdb-tools/bin:${PATH}

# Cache Conana poza $HOME: cibuildwheel i proba dymna ustawiaja HOME po swojemu,
# a cache ma byc ten sam niezaleznie od tego, kto uruchamia kontener (jako root -
# domyslnie w docker run i w cibuildwheel).
ENV CONAN_HOME=/opt/conan

# Profil jak w docker/ci/Dockerfile (gnu23, Ninja) plus platform_tool_requires
# jak w scripts/release_package/package-portable.sh: tool_requires cmake z
# conanfile.py spelnia cmake z venv, zamiast budowac CMake ze zrodel - pod
# emulacja x86_64 na Apple silicon to dziesiatki minut.
RUN conan profile detect \
    && sed -i -e "s/^compiler.cppstd=.*/compiler.cppstd=gnu23/" "$CONAN_HOME/profiles/default" \
    && printf '\n[conf]\ntools.cmake.cmaketoolchain:generator=Ninja\n\n[platform_tool_requires]\ncmake/%s\n' \
       "$(cmake --version | head -n 1 | awk '{print $3}')" >> "$CONAN_HOME/profiles/default" \
    && cat "$CONAN_HOME/profiles/default"

# Proba toolchainu PRZED budowa zaleznosci: jesli C++23 silnika nie miesci sie
# w polityce manylinux_2_28, obraz ma pasc po minucie, a nie po godzinie Boosta.
# Szczegoly w naglowku toolchain-probe.cpp.
COPY docker/wheel/audit-so.py docker/wheel/toolchain-probe.cpp /opt/rdb-tools/
RUN mkdir /tmp/rdb-probe \
    && g++ -std=c++23 -O2 -Wall -Wextra -Werror -o /tmp/rdb-probe/probe /opt/rdb-tools/toolchain-probe.cpp \
    && /tmp/rdb-probe/probe \
    && g++ -std=c++23 -O2 -Wall -Wextra -Werror -fPIC -shared -Wl,--no-undefined -DRDB_PROBE_LIBRARY \
       -o /tmp/rdb-probe/rdbprobe.so /opt/rdb-tools/toolchain-probe.cpp \
    && /opt/rdb-tools/venv/bin/python3 /opt/rdb-tools/audit-so.py /tmp/rdb-probe/rdbprobe.so \
    && rm -rf /tmp/rdb-probe

# Cache zaleznosci z conanfile.py, wylacznie Release - kola buduja sie tylko w
# Release. Liste generuje ten sam gen_dockerconan.sh co w obrazie CI; plik
# DockerConanExtra.txt (additional_docker_requires dla starszych galezi) jest
# tu pomijany, bo kola buduje sie tylko z biezacego drzewa. python3 z venv, bo
# obraz manylinux nie gwarantuje python3 w PATH.
#
# --build "*", a nie --build missing: binaria z Conan Center dla Linuksa powstaja
# na glibc nowszej niz 2.28 i w tym obrazie sie nie uruchamiaja. Pierwsza budowa
# x86_64 padla na b2 (narzedzie budowy Boosta), ktore Conan Center ma gotowe dla
# x86_64 i ktore wymaga GLIBC_2.34; na aarch64 gotowego nie ma, wiec tam
# zbudowalo sie ze zrodel i problem sie nie ujawnil. Biblioteki i tak buduja sie
# ze zrodel (profil GCC 14 / gnu23 nie pasuje do binariow Conan Center), wiec
# koszt to tylko b2 - a zadne binarium spoza tego obrazu nie trafi ani do kola,
# ani do jego budowy.
COPY conanfile.py docker/ci/gen_dockerconan.sh /tmp/rdb-conan/
RUN PATH=/opt/rdb-tools/venv/bin:$PATH bash /tmp/rdb-conan/gen_dockerconan.sh \
       /tmp/rdb-conan/conanfile.py /tmp/rdb-conan/conanfile.txt \
    && conan install /tmp/rdb-conan/conanfile.txt -of /tmp/rdb-conan/out -s build_type=Release --build "*" \
    && conan cache clean "*" \
    && rm -rf /tmp/rdb-conan

# Bez mold: CMakeLists.txt wlacza go na Linuksie domyslnie (RDB_MOLD_DEFAULT),
# niezaleznie od tego, czy jest zainstalowany, a w tym obrazie go nie ma. Z
# -fuse-ld=mold w CMAKE_CXX_FLAGS kazda proba kompilacyjna po PlatformChecks
# pada na linkowaniu - api/cpp konczy wtedy konfiguracje bledem o pipe2, a
# RDB_BOOST_POSIX_SHM po cichu wychodzi 0. Zmienna srodowiska, a nie -D, bo
# CMakeLists.txt czyta ja z FORCE i obejmuje kazda konfiguracje w obrazie, takze
# te z cibuildwheel (tak samo robi scripts/release_package/package-portable.sh).
# Na koncu pliku, bo dotyczy tylko drzewa retractordb, nie zaleznosci Conana.
ENV RDB_USE_MOLD=OFF

# --- Budowa, weryfikacja i publikacja ---------------------------------------
#
# 1. Budowa - z KORZENIA repo (kontekst musi objac conanfile.py i docker/).
#    Przebudowac po kazdej zmianie zaleznosci w conanfile.py, inaczej proba
#    dymna (i cibuildwheel) zatrzyma sie na `--build never`.
#
#    x86_64 - cel Colab. Na Apple silicon budowa idzie pod emulacja, Boost i
#    runtime ANTLR kompiluja sie wtedy kilka razy dluzej:
#
#      docker build --platform linux/amd64 --build-arg RDB_VERSION="$(cat VERSION)" \
#        -f docker/wheel/Dockerfile -t buildenv-retractordb-wheel:manylinux_2_28_x86_64 .
#
#    aarch64 - na Apple silicon natywnie, wiec najszybsza pierwsza proba pliku:
#
#      docker build --platform linux/arm64 --build-arg RDB_VERSION="$(cat VERSION)" \
#        --build-arg MANYLINUX_IMAGE=quay.io/pypa/manylinux_2_28_aarch64@sha256:ad74e53b713f3b07d8c889c526dc0c6500da9827b45e38739570875fef52e28f \
#        -f docker/wheel/Dockerfile -t buildenv-retractordb-wheel:manylinux_2_28_aarch64 .
#
# 2. Weryfikacja - proba dymna buduje _core z biezacego drzewa, sprawdza go
#    auditwheelem i uruchamia api/python/tests (szczegoly w smoke.sh):
#
#      docker run --rm --platform linux/amd64 -v "$PWD:/src:ro" \
#        buildenv-retractordb-wheel:manylinux_2_28_x86_64 bash /src/docker/wheel/smoke.sh
#
#    Zawartosc cache Conana:
#
#      docker run --rm buildenv-retractordb-wheel:manylinux_2_28_x86_64 conan list "*"
#
# 3. Publikacja - osobne repozytorium obok obrazu CI, jeden tag na architekture.
#    Wypycha czlowiek; nic w CI tego obrazu jeszcze nie uzywa (to J4):
#
#      docker login
#      docker tag buildenv-retractordb-wheel:manylinux_2_28_x86_64 micwide/buildenv-retractordb-wheel:manylinux_2_28_x86_64
#      docker push micwide/buildenv-retractordb-wheel:manylinux_2_28_x86_64
#      (i analogicznie :manylinux_2_28_aarch64)
