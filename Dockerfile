# docker build -t buildenv-retractordb .
# docker run -it buildenv-retractordb
# docker login
# docker tag buildenv-retractordb micwide/buildenv-retractordb:latest
# docker push micwide/buildenv-retractordb:latest

# clean up all dangling images
# https://stackoverflow.com/questions/44785585/how-can-i-delete-all-local-docker-images

# https://hub.docker.com/_/debian
# Use Ubuntu 24.04 LTS as base image, but we need modern compiler for C++20 - gcc 14 in ubuntu 25.04
# neee experimental: FROM ubuntu:25.04
# stable: FROM ubuntu:24.04
FROM debian:trixie-slim

# Set environment variables to avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

RUN apt-get update && apt-get install -y apt-utils && apt-get upgrade -y && apt-get clean

# Update package list and install essential packages
RUN apt-get update && apt-get install -y \
    # GCC toolchain and build essentials
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
    pkg-config \
    sudo \
    # Clean up to reduce image size
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user for development (optional but recommended)
RUN useradd -m -s /bin/bash developer && \
    usermod -aG sudo developer

# Allow passwordless sudo for the developer user
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# Set working directory
WORKDIR /home/developer/workspace

# Copy any local files if needed (uncomment and modify as needed)
COPY DockerConan.txt /home/developer/workspace/

# Change ownership of workspace to developer user
RUN chown -R developer:developer /home/developer/workspace

# Switch to non-root user
USER developer

RUN python3 -m venv .venv \
    && . .venv/bin/activate \ 
    && pip install --upgrade pip \
    && pip install cmakelang \
    && pip install conan \
    && conan profile detect \
    && sed -i -e "s/gnu17/gnu20/g" ~/.conan2/profiles/default \
    && echo '[conf]' >> ~/.conan2/profiles/default \
    && echo 'tools.cmake.cmaketoolchain:generator=Ninja' >> ~/.conan2/profiles/default 

RUN . .venv/bin/activate \
    && conan install DockerConan.txt -s build_type=Release --build missing \
    && conan cache clean "*"

RUN echo "source .venv/bin/activate" >> ~/.bashrc
# RUN echo "cat /etc/lsb-release" >> ~/.bashrc

# Set default command
CMD ["/bin/bash"]

# Optional: Expose ports if your application needs them
# EXPOSE 8080

