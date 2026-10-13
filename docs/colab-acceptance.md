# J5 acceptance: container and Google Colab

Issue #448 accepts a ready C4 wheel (#492) from a file or direct HTTPS URL. PyPI
publication, DeviceWindowLoader and MPS acceptance are separate work. A container
run is evidence for the installed wheel; acceptance in real Colab requires its own
CPU and GPU runs by a human. No Colab result is claimed by this protocol.

## Prepare the artifact

1. Build the Linux x86_64 wheel from the intended source revision using
   `docker/wheel/build-wheels.sh x86_64`. Keep its full source SHA and build log.
   Record any uncommitted changes that affect the wheel; rebuild from the reviewed
   commit before distribution. Use the repaired cp312-abi3 wheel for Python 3.12+.
2. Record its filename and `sha256sum build/wheelhouse/<wheel-name>.whl`. Use that
   same wheel for the container and Colab acceptance, not an older file with the
   same version and filename.
3. Run both installed-wheel container profiles from
   [the manual release check](jupyter-integration.md#manual-release-check-for-j5).
   Preserve each `acceptance.json` and the three executed notebooks. Require
   `notebook-smoke: OK - 3/3 notebooks`; the profile without torch must record
   `training.status = skipped` and successful NumPy assertions. The default profile
   must record successful CPU training. Compare wheel SHA-256 with step 2.

## Real Colab: CPU, then GPU

Perform each run in a fresh runtime. A local notebook execution or a container
named after Colab does not complete this step.

1. Upload `api/python/notebooks/colab_quickstart.ipynb` through Colab's notebook
   upload dialog. Use the notebook from the reviewed change; the GitHub copy can
   differ before the pull request is merged.
2. Select a CPU runtime for the first run. Upload the wheel through the Files panel
   to `/content/`, or set `WHEEL` in the first code cell to a direct HTTPS URL. The
   URL must contain the original wheel filename in its path; signed query strings
   are allowed. Set `SOURCE_COMMIT` to the full SHA recorded by the build.
3. Run all cells without installing/replacing NumPy or torch manually. The notebook
   installs only `retractordb` with `--no-deps`. Save the first cell's before/after
   versions, separate download/install/import times and `_core` path. Compare the
   reported wheel SHA-256 with the prepared artifact.
4. Require both NumPy assertions and the training cell to complete. A real Colab
   runtime with torch must train; an unexpected ML skip is not CPU/GPU acceptance.
   The notebook's optional no-torch path is covered separately by the container.
5. Download `rdb-acceptance.json` and the executed notebook. Record the Colab runtime
   version/date, CPU or GPU selection and whether Colab actually required a restart.
   `restart_performed=false` describes the uninterrupted notebook execution;
   `restart_required=null` intentionally needs human observation. Do not convert
   that unknown field to a proven no-restart result without observing the run.
   If a restart is required, preserve the pre-restart output and its reason, then
   record the completed run after restarting separately.
6. Repeat in a fresh GPU runtime (for example T4). Require
   `training.device = cuda`, the actual GPU name, epoch throughput/step times and
   CPU-reference loss agreement. A CPU fallback is not GPU acceptance. T4 should
   report `torch.float16` with GradScaler enabled; L4/A100 should report native
   `torch.bfloat16` without fp16 scaling. Record what the assigned runtime actually
   provides, including torch and CUDA versions.

The model receives copied CPU windows from `StreamDataset`, with `num_workers=0`.
CUDA is a separate device transfer. The implementation follows
[PyTorch's AMP API](https://docs.pytorch.org/docs/stable/amp.html), with fp16 gradient
scaling and bf16 when native support is reported. Epoch timing uses
[CUDA synchronization](https://docs.pytorch.org/docs/stable/generated/torch.cuda.synchronize.html).

Windows/s = processed windows / training-loop seconds. Mean step time = that
elapsed time / batch count. Both include loading, transfers and optimization;
engine advancement and CPU-reference training are reported separately. The first
epoch includes warm-up. Record all epochs; do not report the fastest alone as the
result. This small model is a demonstration, not a general engine benchmark.
CUDA and CPU loss comparison uses the same initialization, data order and SGD
parameters, with `rtol=0.05, atol=0.002`. An assertion failure remains a failed run.

Approximately 20 seconds is a roadmap reference, not a closure threshold. Record
conditions: upload versus URL, download time, cold/warm runtime, Python version,
CPU/GPU type and whether package caches were already populated. Compare times only
under comparable conditions; container installation does not measure Colab.

## Result to record in #448

Keep container and Colab results separate. Attach the build log and local result
artifacts, then fill this template with observed values. Posting the comment,
commit, push and opening a pull request require separate human authorization.

| Field | Container with torch | Container without torch | Colab CPU | Colab GPU |
|---|---|---|---|---|
| Status (not run / passed / failed) | not run | not run | not run | not run |
| Date and runtime version | | | | |
| Wheel filename / source commit / SHA-256 | | | | |
| Python / NumPy / torch / CUDA | | | | |
| Wheel source (file or URL; omit signed URL secrets) | | | | |
| Download / install / import seconds | | | | |
| NumPy/torch before = after | | | | |
| NumPy assertions | | | | |
| Training device / precision / GradScaler | | skipped | | |
| Epoch windows/s and mean step ms | | skipped | | |
| CPU-reference loss agreement (GPU only) | n/a | n/a | n/a | |
| Restart required / performed (human observation) | n/a | n/a | | |
| Report and executed notebook location | | | | |

Do not close #448 until the required container and real Colab results are recorded,
and the reviewed change reaches `dev/445-jupyter-epic` through a pull request. The
protocol itself and a green local build-module smoke do not meet those criteria.
