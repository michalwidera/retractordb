# Release packaging

Scripts that check whether a release can be made, build all RetractorDB release assets in Docker and verify a release after it is published on GitHub. Nothing here tags, pushes or publishes - those steps stay with the maintainer.

| Script | Role |
|--------|------|
| `release.sh [--check]` | Preflight checks, then both Docker builds; collects the assets and prints the publishing commands |
| `release-verify.sh [VERSION]` | Checks a published GitHub release the way the web installer sees it |
| `package-portable-x86.sh [--fresh]` | x86-64 build in Docker: portable archive and `.deb`, output in `build/X86-Packages` |
| `package-portable-arm64.sh [--fresh]` | AArch64 build in Docker under qemu emulation: portable archive, output in `build/ARM64-Packages` (also `ninja package-portable-arm64`) |
| `package-portable.sh arm64\|x86_64 [--fresh]` | Common implementation of the two scripts above |

## Requirements

- Docker, able to run `linux/arm64` containers (qemu binfmt emulation). Check with `docker run --rm --platform linux/arm64 debian:trixie-slim uname -m`, which must print `aarch64`; on a plain Linux host it can be registered with `docker run --privileged --rm tonistiigi/binfmt --install arm64`.
- `gh`, logged in (`gh auth login`).
- `git`, `sha256sum`, `python3`, and `dpkg-deb` for `release-verify.sh`.

The host toolchain (Conan, GCC, CMake) is not used: both builds run in the `debian:trixie-slim` image defined in `packaging/portable/Dockerfile`.

## Releasing a new version

1. Bump `VERSION`, commit it, push `master` and wait for CI to pass.
2. Run the preflight and fix every `[FAIL]`:

   ```bash
   scripts/release_package/release.sh --check
   ```

3. Build the assets:

   ```bash
   scripts/release_package/release.sh
   ```

   The script repeats the preflight, builds x86-64 and then AArch64 (one after another - two builds at once do not fit in RAM), and copies the three assets to `build/Release-Assets/v<VERSION>/`:

   - `retractordb-<VERSION>-Linux.deb`
   - `retractordb-<VERSION>-linux-x86_64-portable.tar.gz`
   - `retractordb-<VERSION>-linux-aarch64-portable.tar.gz`

4. Publish with the commands the script prints at the end, for example:

   ```bash
   git tag v0.1.11                  # only if the tag does not exist yet
   git push origin v0.1.11
   gh release create v0.1.11 --verify-tag --title v0.1.11 --generate-notes build/Release-Assets/v0.1.11/*
   ```

   Edit the title and notes on GitHub if needed. Do not mark the release as a draft or prerelease: the web installer ignores both.

5. Verify the published release:

   ```bash
   scripts/release_package/release-verify.sh           # version from VERSION
   scripts/release_package/release-verify.sh 0.1.10    # any other release
   ```

## Preflight checks (`release.sh --check`)

| Check | Fails when |
|-------|------------|
| Docker | the daemon does not respond, or a `linux/arm64` container does not report `aarch64` |
| `gh` | not authenticated |
| `VERSION` | not in `x.y.z` form, or not newer than the highest `v*` tag - the version was not bumped |
| Branch and tree | not on `master`, or the working tree has changes or untracked files |
| `origin/master` | `HEAD` differs from `origin/master` after `git fetch` - the release commit is not pushed |
| Tag `v<VERSION>` | it exists, locally or on `origin`, but points at another commit |
| GitHub release | `v<VERSION>` is already published |
| CI | the combined status of `HEAD` is `failure` or `error`; `pending` or no status is only a warning |
| Disk | less than 10 GB free is only a warning |

All checks run even after a failure, so one run lists everything to fix.

## What the builds check

Inside the container, before a package leaves it:

- all three programs in the portable archive are ELF binaries for the target architecture;
- `xretractor --build-info` reports the production optimizer switches (all `RDB_OPT_*` `ON`, `RDB_BENCH_PROBE=OFF`), as `scripts/buildrdb.sh release` requires;
- `xqry -h` and `xtrdb -h` run;
- the `.deb` (x86-64 only) has the architecture of the build image (`amd64`).

Package files are picked by the name built from `VERSION`, not by a wildcard, so a package of an older version left in the work directory is never taken instead of the new one.

## Build directories

Each architecture keeps three directories under `build/`: `<ARCH>-Conan-Cache` (Conan packages, reused between runs), `<ARCH>-Work` (the source copy and CMake build tree of the container) and `<ARCH>-Packages` (finished packages); `<ARCH>` is `X86` or `ARM64`. `--fresh` deletes `<ARCH>-Work` before building, so files removed from the repository cannot survive in the build; `release.sh` always uses it. Without `--fresh` the build is incremental, which is quicker for local tries.

The first x86-64 run builds the Conan dependencies from source. The AArch64 build runs under emulation and is several times slower than the x86-64 one. To reclaim the space, delete `build/X86-*` and `build/ARM64-*`.

## Checksums and the web installer

The web installer (`install.sh` on retractordb.com) does not download separate `.sha256` files. It reads the SHA-256 of each archive from the `digest` field that GitHub Releases computes on upload, and refuses an asset without it. That is why no checksum files are published. `release-verify.sh` checks the same thing the installer relies on:

- the release is published and is not a draft or prerelease;
- the tag on `origin` matches the local tag;
- all three assets are present, fully uploaded and carry a `sha256:` digest;
- every asset downloaded from GitHub matches its digest and, if `build/Release-Assets/v<VERSION>/` exists, the local build;
- each portable archive holds exactly the files and directories the installer accepts, with binaries for the right architecture;
- the `.deb` has the expected package name, version and architecture;
- which release the installer picks by default for each architecture (a warning, not a failure, when it is not the one being verified).
