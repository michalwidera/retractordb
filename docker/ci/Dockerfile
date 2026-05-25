# https://hub.docker.com/_/debian
FROM debian:trixie-slim

LABEL com.retractordb.version="0.0.6"
LABEL vendor1="Michal Widera"
LABEL com.retractordb.release-date="2026-05-25"
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

# Copy any local files if needed (uncomment and modify as needed)
COPY DockerConan.txt /home/developer/workspace/

RUN python3 -m venv .venv \
    && . .venv/bin/activate \
    && pip install --upgrade pip \
    && pip install cmakelang conan \
    && conan profile detect \
    && sed -i -e "s/gnu17/gnu23/g" ~/.conan2/profiles/default \
    && echo '[conf]' >> ~/.conan2/profiles/default \
    && echo 'tools.cmake.cmaketoolchain:generator=Ninja' >> ~/.conan2/profiles/default \
    && conan install DockerConan.txt -s build_type=Release --build missing \
    && conan install DockerConan.txt -s build_type=Debug --build missing \
    && conan cache clean "*" \
    && pip cache purge

RUN echo "source .venv/bin/activate" >> ~/.bashrc
# RUN echo "cat /etc/lsb-release" >> ~/.bashrc

# Set default command
CMD ["/bin/bash"]

# Optional: Expose ports if your application needs them
# EXPOSE 8080

# How to check docker layers size - install https://github.com/wagoodman/dive
# First: start docker desktop app on windows
# docker image ls
# dive <IMAGE ID>

# How to deploy to DockerHub (run from docker/ci/)
# docker build -t buildenv-retractordb .
# docker run -it buildenv-retractordb
# docker login
# docker tag buildenv-retractordb micwide/buildenv-retractordb:latest
# docker push micwide/buildenv-retractordb:latest

# clean up all dangling images
# https://stackoverflow.com/questions/44785585/how-can-i-delete-all-local-docker-images
