# https://hub.docker.com/_/debian
# Multi-stage: builder uses CI image (conan cache pre-populated), runtime is minimal.

# ── Stage 0: build rsupervisor ───────────────────────────────────────────────
FROM golang:1.24-bookworm AS gobuilder

COPY docker/supervisor/ /src/
WORKDIR /src
RUN CGO_ENABLED=0 go build -o /rsupervisor .

# ── Stage 1: build ───────────────────────────────────────────────────────────
FROM micwide/buildenv-retractordb:latest AS builder

# Copy project source on top of existing workspace (preserves .venv from CI image)
COPY --chown=developer:developer . /home/developer/workspace/

WORKDIR /home/developer/workspace

RUN . .venv/bin/activate \
    && conan install . -s build_type=Release --build missing \
    && conan build . -s build_type=Release

# Collect non-system shared library dependencies of all three binaries
RUN mkdir /home/developer/libs && \
    for bin in \
        build/Release/src/retractor/xretractor \
        build/Release/src/qry/xqry; \
    do \
        ldd "$bin" \
            | awk '/=> \// { print $3 }' \
            | grep -v '^/lib\|^/usr/lib/x86_64-linux-gnu/libc\|libstdc++\|libm\|libgcc\|libpthread\|libdl\|librt' \
            | xargs -I{} cp -n "{}" /home/developer/libs/ 2>/dev/null || true; \
    done

# ── Stage 2: runtime ─────────────────────────────────────────────────────────
FROM debian:trixie-slim

LABEL com.retractordb.version="0.1.4"
LABEL vendor1="Michal Widera"
LABEL com.retractordb.release-date="2026-05-04"
LABEL com.retractordb.description="RetractorDB production runtime"
LABEL com.retractordb.url="https://retractordb.com"

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/bash retractor

# Binaries
COPY --from=builder \
    /home/developer/workspace/build/Release/src/retractor/xretractor \
    /home/developer/workspace/build/Release/src/qry/xqry \
    /usr/local/bin/

COPY --from=gobuilder /rsupervisor /usr/local/bin/rsupervisor

# Extra shared libs not in trixie-slim (antlr4-cppruntime etc.)
COPY --from=builder /home/developer/libs/ /usr/local/lib/retractordb/

RUN ldconfig /usr/local/lib/retractordb

RUN mkdir -p /home/retractor/temp && chown retractor:retractor /home/retractor/temp

WORKDIR /home/retractor
USER retractor

COPY --chown=retractor:retractor docker/supervisor/startup.rql /home/retractor/startup.rql

ENV RDB_XRETRACTOR=/usr/local/bin/xretractor
ENV RDB_XQRY=/usr/local/bin/xqry

EXPOSE 8080 50051 9090

CMD ["/usr/local/bin/rsupervisor"]

# How to build and push:
#   docker build -f docker/prod/Dockerfile -t retractordb:latest .
#   docker tag retractordb:latest micwide/retractordb:latest
#   docker push micwide/retractordb:latest

# How to run with a local directory mounted (queries + data):
#   docker run -d \
#     -v /path/to/local/data:/home/retractor/data \
#     -e RDB_QUERY_FILE=/home/retractor/data/queries.rql \
#     -p 8080:8080 -p 50051:50051 -p 9090:9090 \
#     retractordb:latest
