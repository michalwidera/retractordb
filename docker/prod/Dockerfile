# https://hub.docker.com/_/debian
# Multi-stage: builder uses CI image (conan cache pre-populated), runtime is minimal.

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
        build/Release/src/qry/xqry \
        build/Release/src/rdb/xtrdb; \
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
    /home/developer/workspace/build/Release/src/rdb/xtrdb \
    /usr/local/bin/

# Extra shared libs not in trixie-slim (antlr4-cppruntime etc.)
COPY --from=builder /home/developer/libs/ /usr/local/lib/retractordb/

RUN ldconfig /usr/local/lib/retractordb

WORKDIR /home/retractor
USER retractor

CMD ["/usr/local/bin/xretractor"]

# How to build and push:
#   docker build -f docker/prod/Dockerfile -t retractordb:latest .
#   docker tag retractordb:latest micwide/retractordb:latest
#   docker push micwide/retractordb:latest
