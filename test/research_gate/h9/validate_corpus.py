#!/usr/bin/env python3
"""Fail-closed validity gate for the complete K26 RQL corpus.

The gate runs before campaign start.  It proves that the exact set
of 21 generated plans compiles under every one of the four optimizer
profiles.  It also proves the opposite side of the language boundary: the
historical F9-X program that names constituents through ``#`` must fail.

The engine revision is deliberately not hard-coded before P6.  Evidence records
the clean revision it was generated with and ``--check`` requires that revision
to equal the current HEAD.  The binding campaign pin is created only at start.

The generated report contains no cost measurements.  It records compiler
plans, diagnostics, binary identities, and a checksum manifest only.
"""

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import gen_corpus

HERE = Path(__file__).resolve().parent
# Bramka zyje w drzewie silnika (test/research_gate/h9), wiec repozytorium kodu
# jest trzy poziomy wyzej. Nadpisywalne przez RDB_CODE_REPO.
CODE_REPO = Path(os.environ.get("RDB_CODE_REPO", HERE.parent.parent.parent)).resolve()
#: Katalog z buildami profili ablacji; domyslnie build/ repozytorium kodu.
BUILD_ROOT = Path(os.environ.get("K26V3_BUILD_ROOT", CODE_REPO / "build")).resolve()
PROFILES = {
    "DEFAULT": ("ON", "ON"),
    "NO_R2_CANON": ("OFF", "ON"),
    "NO_R1_FACTOR": ("ON", "OFF"),
    "NO_R1_NO_R2": ("OFF", "OFF"),
}
EXPECTED_BUILD_COMMON = {
    "RDB_OPT_DEDUP_SUBSTRATES": "ON",
    "RDB_OPT_SHARE_EQUIVALENT_SELECTS": "ON",
    "RDB_BENCH_PROBE": "ON",
    "RDB_OPT_SIMPLIFY_EXPRESSIONS": "ON",
}

HISTORICAL_INVALID_F9X = """STORAGE 'temp'
SUBSTRAT 'memory'
DECLARE v INTEGER STREAM A, 1/100 FILE 'a.txt'
DECLARE v INTEGER STREAM B, 1/50 FILE 'b.txt'
DECLARE v INTEGER STREAM C, 1/100 FILE 'c.txt'
DECLARE v INTEGER STREAM D, 1/50 FILE 'd.txt'
SELECT Sqrt(A[0]*C[0]+B[0]*D[0]) STREAM m
FROM ((A>2)#(B>1))+((C>2)#(D>1))
"""


class GateError(RuntimeError):
    pass


REWRITE_APPLIED = re.compile(r"^REWRITE_APPLIED r1=(\d+) r2=(\d+) r3=(\d+)$", re.M)


def require_main_r3_zero(profile, plan, diagnostics):
    """R3 jest wspolna flaga, ale nie moze zmieniac planow mierzonych dla H9."""
    matches = REWRITE_APPLIED.findall(diagnostics)
    if len(matches) != 1:
        raise GateError(f"{profile}/{plan}: expected exactly one complete REWRITE_APPLIED row")
    if not plan.endswith("_controls.rql") and int(matches[0][2]) != 0:
        raise GateError(f"{profile}/{plan}: R3 applied {matches[0][2]} rewrites in a main plan")


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def expected_plans():
    names = []
    for family in gen_corpus.FAMILIES:
        slug = family.replace("-", "_")
        names.extend(f"{slug}_Q{q}.rql" for q in gen_corpus.Q_GRID)
        names.append(f"{slug}_controls.rql")
    return sorted(names)


def require_exact_corpus():
    generated = gen_corpus.corpus()
    generated_rql = sorted(Path(name).name for name in generated if name.startswith("rql/"))
    wanted = expected_plans()
    if generated_rql != wanted:
        missing = sorted(set(wanted) - set(generated_rql))
        extra = sorted(set(generated_rql) - set(wanted))
        raise GateError(f"generator does not define the exact 21-plan corpus; missing={missing}, extra={extra}")
    disk = sorted(path.name for path in (HERE / "rql").glob("*.rql"))
    if disk != wanted:
        missing = sorted(set(wanted) - set(disk))
        extra = sorted(set(disk) - set(wanted))
        raise GateError(f"on-disk RQL set is incomplete or extended; missing={missing}, extra={extra}")
    for rel, content in generated.items():
        path = HERE / rel
        if not path.exists() or path.read_text() != content:
            raise GateError(f"generated corpus mismatch: {rel}")
    return wanted


def parse_build_info(text):
    result = {}
    for line in text.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            result[key] = value
    return result


def profile_binary(profile):
    binary = BUILD_ROOT / f"K26v3-{profile}" / "src" / "retractor" / "xretractor"
    if not binary.is_file() or not os.access(binary, os.X_OK):
        raise GateError(f"missing K26 binary for {profile}: {binary}")
    completed = subprocess.run([binary, "--build-info"], check=True, text=True, capture_output=True)
    info = parse_build_info(completed.stdout)
    for key, value in EXPECTED_BUILD_COMMON.items():
        if info.get(key) != value:
            raise GateError(f"{profile}: {key}={info.get(key)!r}, expected {value}")
    commutative, factor = PROFILES[profile]
    expected = {
        "RDB_OPT_COMMUTATIVE_ADD": commutative,
        "RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES": factor,
    }
    expected_keys = set(EXPECTED_BUILD_COMMON) | set(expected)
    if set(info) != expected_keys:
        raise GateError(f"{profile}: build-info keys={sorted(info)}, expected {sorted(expected_keys)}")
    for key, value in expected.items():
        if info.get(key) != value:
            raise GateError(f"{profile}: {key}={info.get(key)!r}, expected {value}")
    return binary, completed.stdout


#: Bramka regresyjna dziala na drzewie roboczym, kampania nie. Ustawiane z CLI.
ALLOW_DIRTY = False


def checked_code_sha(expected=None):
    completed = subprocess.run(
        ["git", "-C", CODE_REPO, "rev-parse", "HEAD"], check=True, text=True, capture_output=True
    )
    actual = completed.stdout.strip()
    if expected is not None and actual != expected:
        raise GateError(f"engine SHA {actual}, evidence/start pin {expected}")
    dirty = subprocess.run(
        ["git", "-C", CODE_REPO, "status", "--short"], check=True, text=True, capture_output=True
    ).stdout
    if dirty:
        # Dowod kampanii musi nazywac rewizje, wiec brudne drzewo jest bledem.
        # Bramka regresyjna pyta o co innego: czy 21 planow nadal kompiluje sie
        # w czterech profilach. Wolno ja uruchomic na drzewie roboczym, ale
        # wynik NIE jest dowodem proweniencji i jest tak oznaczany.
        if not ALLOW_DIRTY:
            raise GateError("engine worktree is dirty")
        return f"{actual}-dirty"
    return actual


def compile_one(binary, rql, work):
    env = os.environ.copy()
    env["RDB_BENCH_PLAN"] = "1"
    return subprocess.run(
        [binary, rql, "-c"], cwd=work, env=env, text=True, capture_output=True
    )


def write_manifest(root):
    manifest = root / "manifest.sha256"
    entries = []
    for path in sorted(root.rglob("*")):
        if path.is_file() and path != manifest:
            entries.append(f"{sha256(path)}  {path.relative_to(root)}")
    manifest.write_text("\n".join(entries) + "\n")
    return len(entries)


def expected_report_rows(plans, binary_hashes):
    expected = {}
    historical_hash = hashlib.sha256(HISTORICAL_INVALID_F9X.encode()).hexdigest()
    for profile in PROFILES:
        for plan in plans:
            expected[(profile, plan)] = ("PASS", sha256(HERE / "rql" / plan), binary_hashes[profile])
        expected[(profile, "historical_invalid_F9_X.rql")] = (
            "REJECTED", historical_hash, binary_hashes[profile]
        )
    return expected


def validate_report_rows(rows, plans, binary_hashes):
    expected = expected_report_rows(plans, binary_hashes)
    actual = {}
    for line in rows:
        fields = line.split("\t")
        if len(fields) != 5:
            raise GateError(f"invalid corpus-validation row: {line!r}")
        profile, plan, status, rql_hash, binary_hash = fields
        key = (profile, plan)
        if key in actual:
            raise GateError(f"duplicate corpus-validation row: {profile}/{plan}")
        actual[key] = (status, rql_hash, binary_hash)
    if set(actual) != set(expected):
        missing = sorted(set(expected) - set(actual))
        extra = sorted(set(actual) - set(expected))
        raise GateError(f"corpus-validation inventory mismatch; missing={missing}, extra={extra}")
    for key, wanted in expected.items():
        if actual[key] != wanted:
            raise GateError(f"corpus-validation mismatch for {key}: {actual[key]}, expected {wanted}")


def run_gate(destination):
    plans = require_exact_corpus()
    code_sha = checked_code_sha()
    if destination.exists():
        raise GateError(f"refusing to overwrite existing evidence directory: {destination}")

    destination.parent.mkdir(parents=True, exist_ok=True)
    staging = Path(tempfile.mkdtemp(prefix="k26-corpus-", dir=destination.parent))
    rows = ["profile\tplan\tstatus\trql_sha256\tbinary_sha256"]
    try:
        (staging / "plans").mkdir()
        (staging / "profiles").mkdir()
        with tempfile.TemporaryDirectory(prefix="k26-compile-") as temp_name:
            work = Path(temp_name)
            for profile in PROFILES:
                binary, build_info = profile_binary(profile)
                (staging / "profiles" / f"build-info-{profile}.txt").write_text(build_info)
                binary_hash = sha256(binary)
                profile_dir = staging / "plans" / profile
                profile_dir.mkdir()
                for plan in plans:
                    rql = HERE / "rql" / plan
                    completed = compile_one(binary, rql, work)
                    stem = Path(plan).stem
                    (profile_dir / f"{stem}.plan").write_text(completed.stdout)
                    (profile_dir / f"{stem}.stderr").write_text(completed.stderr)
                    if completed.returncode != 0:
                        raise GateError(f"valid corpus plan rejected: {profile}/{plan}")
                    require_main_r3_zero(profile, plan, completed.stderr)
                    rows.append(f"{profile}\t{plan}\tPASS\t{sha256(rql)}\t{binary_hash}")

                invalid = work / "historical_invalid_F9_X.rql"
                invalid.write_text(HISTORICAL_INVALID_F9X)
                rejected = compile_one(binary, invalid, work)
                (profile_dir / "historical_invalid_F9_X.stdout").write_text(rejected.stdout)
                (profile_dir / "historical_invalid_F9_X.stderr").write_text(rejected.stderr)
                if rejected.returncode == 0:
                    raise GateError(f"historical illegal F9-X unexpectedly compiled in {profile}")
                rows.append(f"{profile}\thistorical_invalid_F9_X.rql\tREJECTED\t"
                            f"{hashlib.sha256(HISTORICAL_INVALID_F9X.encode()).hexdigest()}\t{binary_hash}")

        (staging / "corpus-validation.tsv").write_text("\n".join(rows) + "\n")
        (staging / "provenance.tsv").write_text(
            "key\tvalue\n"
            f"engine_sha\t{code_sha}\n"
            f"valid_plans\t{len(plans)}\n"
            f"profiles\t{len(PROFILES)}\n"
            f"valid_compilations\t{len(plans) * len(PROFILES)}\n"
            f"invalid_controls_rejected\t{len(PROFILES)}\n"
        )
        count = write_manifest(staging)
        staging.rename(destination)
        return len(plans) * len(PROFILES), len(PROFILES), count
    except Exception:
        shutil.rmtree(staging, ignore_errors=True)
        raise


def check_evidence(destination):
    plans = require_exact_corpus()
    manifest = destination / "manifest.sha256"
    if not manifest.is_file():
        raise GateError(f"missing evidence manifest: {manifest}")
    entries = manifest.read_text().splitlines()
    listed = set()
    for line in entries:
        try:
            digest, rel = line.split("  ", 1)
        except ValueError as exc:
            raise GateError(f"invalid evidence manifest row: {line!r}") from exc
        if rel in listed:
            raise GateError(f"duplicate evidence manifest entry: {rel}")
        listed.add(rel)
        path = destination / rel
        if not path.is_file() or sha256(path) != digest:
            raise GateError(f"evidence checksum mismatch: {rel}")
    actual_files = {
        str(path.relative_to(destination))
        for path in destination.rglob("*")
        if path.is_file() and path != manifest
    }
    if actual_files != listed:
        missing = sorted(listed - actual_files)
        extra = sorted(actual_files - listed)
        raise GateError(f"evidence inventory mismatch; missing={missing}, extra={extra}")

    report_lines = (destination / "corpus-validation.tsv").read_text().splitlines()
    expected_header = "profile\tplan\tstatus\trql_sha256\tbinary_sha256"
    if not report_lines or report_lines[0] != expected_header:
        raise GateError("corpus-validation.tsv has an invalid header")
    binary_hashes = {}
    for profile in PROFILES:
        binary, _ = profile_binary(profile)
        binary_hashes[profile] = sha256(binary)
    validate_report_rows(report_lines[1:], plans, binary_hashes)
    for profile in PROFILES:
        for plan in plans:
            require_main_r3_zero(
                profile, plan,
                (destination / "plans" / profile / f"{Path(plan).stem}.stderr").read_text(),
            )

    provenance_lines = (destination / "provenance.tsv").read_text().splitlines()
    provenance = {}
    for line in provenance_lines[1:]:
        fields = line.split("\t")
        if len(fields) == 2 and fields[0] not in provenance:
            provenance[fields[0]] = fields[1]
    evidence_sha = provenance.get("engine_sha")
    if not evidence_sha:
        raise GateError("corpus-validation provenance has no engine_sha")
    checked_code_sha(evidence_sha)
    expected_provenance = [
        "key\tvalue",
        f"engine_sha\t{evidence_sha}",
        f"valid_plans\t{len(plans)}",
        f"profiles\t{len(PROFILES)}",
        f"valid_compilations\t{len(plans) * len(PROFILES)}",
        f"invalid_controls_rejected\t{len(PROFILES)}",
    ]
    if provenance_lines != expected_provenance:
        raise GateError("corpus-validation provenance is incomplete or inconsistent")
    return len(entries)


def selftest():
    wanted = expected_plans()
    if len(wanted) != 21 or len(set(wanted)) != 21:
        raise GateError("selftest: expected plan inventory is not exactly 21 unique names")
    shortened = wanted[:-1]
    if shortened == wanted or len(shortened) != 20:
        raise GateError("selftest: omitted-plan mutant was not constructed")
    extended = wanted + ["unexpected.rql"]
    if set(extended) == set(wanted):
        raise GateError("selftest: extra-plan mutant was not constructed")
    if "A[0]" not in HISTORICAL_INVALID_F9X or "#" not in HISTORICAL_INVALID_F9X:
        raise GateError("selftest: historical language-boundary mutant is missing")
    binary_hashes = {profile: f"binary-{profile}" for profile in PROFILES}
    expected = expected_report_rows(wanted, binary_hashes)
    rows = ["\t".join((*key, *value)) for key, value in sorted(expected.items())]
    validate_report_rows(rows, wanted, binary_hashes)

    def must_reject(mutant, name):
        try:
            validate_report_rows(mutant, wanted, binary_hashes)
        except GateError:
            return
        raise GateError(f"selftest: {name} report mutant was accepted")

    must_reject(rows[:-1], "omitted-row")
    must_reject(rows + [rows[0]], "duplicate-row")
    fields = rows[0].split("\t")
    fields[1] = "unexpected.rql"
    must_reject(rows + ["\t".join(fields)], "extra-row")
    fields = rows[0].split("\t")
    fields[2] = "REJECTED"
    must_reject(["\t".join(fields)] + rows[1:], "wrong-status")
    require_main_r3_zero("DEFAULT", "F9_R2_Q8.rql", "REWRITE_APPLIED r1=0 r2=4 r3=0\n")
    try:
        require_main_r3_zero("DEFAULT", "F9_R2_Q8.rql", "REWRITE_APPLIED r1=0 r2=4 r3=1\n")
    except GateError:
        pass
    else:
        raise GateError("selftest: nonzero-R3 main-plan mutant was accepted")
    print("OK: selftest rejects omitted, duplicate, extra, wrong-status, and historical-invalid corpus mutants")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=HERE / "corpus_validation")
    parser.add_argument("--check", action="store_true", help="verify already recorded evidence")
    parser.add_argument("--selftest", action="store_true")
    parser.add_argument(
        "--allow-dirty",
        action="store_true",
        help="pozwol na brudne drzewo silnika; dowod dostaje SHA z sufiksem -dirty "
             "i nie jest dowodem proweniencji (tryb bramki regresyjnej)",
    )
    args = parser.parse_args()
    global ALLOW_DIRTY
    ALLOW_DIRTY = args.allow_dirty
    try:
        if args.selftest:
            selftest()
        elif args.check:
            count = check_evidence(args.out.resolve())
            print(f"OK: corpus-validity evidence complete and immutable ({count} checksums)")
        else:
            valid, rejected, count = run_gate(args.out.resolve())
            print(f"OK: {valid} valid compilations; {rejected} illegal controls rejected; {count} checksums")
        return 0
    except (GateError, subprocess.CalledProcessError, OSError) as exc:
        print(f"BLAD: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
