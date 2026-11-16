---
name: local-ci
description: Lokalne odtworzenie zadań CircleCI w Dockerze przez scripts/test-ci.sh - profile i cele ninja test-ci*, limity zasobów, ccache w wolumenie, wariant szybki z zachowanym build/, czego lokalny przebieg nie dowodzi o CI. Wywołaj przed uruchomieniem lokalnego CI i przy czytaniu jego wyników lub czasów.
---

# Lokalne CI - przepis

Reguła (profile odwzorowują `.circleci/config.yml` ręcznie; push i CI tylko na prośbę człowieka) jest w `CLAUDE.md`, sekcje *Build* i *Commits, push and CI*. Ten plik zawiera resztę.

**CI locally, before pushing** (`scripts/test-ci.sh`, needs a running Docker):
```bash
ninja test-ci        # = test-ci-commit: the only job CircleCI runs after a push
ninja test-ci-nightly-debug # Debug job of the manual-nightly-full workflow, including Valgrind
ninja test-ci-fast   # same job, build/ kept between runs - quick, NOT a faithful CI run
scripts/test-ci.sh --list   # every profile with the CircleCI job it mirrors
```
Each profile copies the working tree into a container built from the CI image (`micwide/buildenv-retractordb`; the exception is `make-build`, which mirrors a job on stock `cimg/base:current`) capped at the CI executor's resources (4 vCPU / 8 GiB) and runs that job's steps. Outside `all` and outside `ninja test`. Profiles mirror `.circleci/config.yml` by hand: change that file, change `scripts/test-ci.sh`.

By default, local CI profiles start with a fresh build directory and an empty ccache. CircleCI's `build-debug-ninja-mydocker` and `build-release-ninja-mydocker` jobs instead restore ccache by build type, branch and revision, falling back to the branch and then master; before saving they evict entries older than 7200 seconds. The other jobs do not use those restore/save steps. A fresh checkout therefore does not imply a cold compiler cache.

`scripts/test-ci.sh --profile <name> --ccache-volume <volume>` keeps ccache in a named Docker volume across local runs. This models cache reuse, but its contents are independent of CircleCI's cache, so local build times do not establish CI build times. Separately, `test-ci-fast` keeps `build/` in a per-profile Docker volume; use it to check a change before committing, not as evidence of a fresh CI build. `scripts/test-ci.sh --profile <name> --reset-build` drops that kept build directory without clearing an explicitly selected ccache volume.
