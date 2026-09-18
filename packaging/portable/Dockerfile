FROM debian:trixie-slim

ENV DEBIAN_FRONTEND=noninteractive
ENV PATH=/opt/retractordb-venv/bin:$PATH

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential g++ git ninja-build python3 python3-venv valgrind \
    pkg-config ca-certificates curl file \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/retractordb-venv \
    && pip install --no-cache-dir conan 'cmake>=4.4.2'

WORKDIR /tmp/retractordb
