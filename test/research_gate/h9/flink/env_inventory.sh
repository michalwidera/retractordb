#!/bin/bash
# Krok A kampanii K26 - inwentarz srodowiska HOSTA (strona Flinka), zapisany maszynowo.
#
# To ma byc ODCZYT, nie przepisanie z planu: wszystkie wersje i sumy SHA-256 pochodzia
# z uruchomienia narzedzi i z plikow, nie z dokumentu. Wynik wchodzi do predeklaracji
# (D-2: Flink biegnie na hoscie, wiec predeklaracja zamraza dwa srodowiska, nie jedno).
#
# Wyjscie: results/flink_environment.tsv
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="$HERE/results/flink_environment.tsv"
mkdir -p "$HERE/results"

# Przypiecie JDK jest jawne i celowe: domyslne `java` na hoscie to inna wersja glowna,
# wiec kampania nie moze polegac na PATH. Ten sam wzorzec, co K22v5/freeze_check.sh.
JAVA_HOME_PINNED="${JAVA_HOME_PINNED:-/usr/lib/jvm/java-17-openjdk-amd64}"
FLINK_HOME="${FLINK_HOME:-/home/michal/opt/flink-2.3.0}"

JAVA="$JAVA_HOME_PINNED/bin/java"
JAVAC="$JAVA_HOME_PINNED/bin/javac"
FLINK_JAR="$FLINK_HOME/lib/flink-dist-2.3.0.jar"

# --- warstwa przenosnosci powloki -------------------------------------------
# Kopie funkcji z test/IntegrationTest/portable.sh (tam oryginal i pelne
# uzasadnienie). Bramka badawcza jest z zalozenia SAMODZIELNA wzgledem kopii
# test/ robionej przez test/CMakeLists.txt, wiec nie siega po tamten plik.

# Suma SHA-256 pliku, sam skrot. GNU ma sha256sum, BSD/macOS `shasum -a 256`.
sha256_of() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | awk '{print $1}'
  else
    shasum -a 256 "$1" | awk '{print $1}'
  fi
}

# Rozmiar pliku w bajtach. Zastepuje `stat -c %s` (GNU) i `stat -f %z` (BSD).
file_size() {
  [ -f "$1" ] || { echo 0; return 0; }
  wc -c <"$1" | tr -d ' '
}

# Sciezka bezwzgledna. Zastepuje `readlink -f`, ktorego BSD `readlink` nie zna.
abs_path() {
  ( cd "$(dirname "$1")" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$1")" )
}

# Liczba rdzeni dostepnych online. `nproc` jest z GNU coreutils.
cpu_online() {
  getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1
}

# Nazwa systemu. /etc/os-release to konwencja Linuksa; na Darwinie tego pliku nie
# ma i podstawienie wychodzilo PUSTE, czyli inwentarz milczal o systemie zamiast go
# zapisac.
os_pretty_name() {
  if [ -r /etc/os-release ]; then
    ( . /etc/os-release && echo "$PRETTY_NAME" )
  elif command -v sw_vers >/dev/null 2>&1; then
    printf '%s %s (%s)\n' "$(sw_vers -productName)" "$(sw_vers -productVersion)" \
      "$(sw_vers -buildVersion)"
  else
    uname -sr
  fi
}

# Nazwa procesora. /proc nie istnieje na Darwinie, wiec tam pytamy sysctl.
# Inwentarz ma byc ODCZYTEM, wiec brak odczytu jest nazwany, a nie pominiety.
cpu_model() {
  if [ -r /proc/cpuinfo ]; then
    awk -F': ' '/^model name/{print $2; exit}' /proc/cpuinfo
  elif command -v sysctl >/dev/null 2>&1; then
    sysctl -n machdep.cpu.brand_string 2>/dev/null || echo "nieustalony"
  else
    echo "nieustalony"
  fi
}

fail() { echo "BLAD INWENTARZA: $*" >&2; exit 2; }
[[ -x "$JAVA" && -x "$JAVAC" ]] || fail "brak przypietego JDK: $JAVA_HOME_PINNED"
[[ -r "$FLINK_JAR" ]] || fail "brak jara Flinka: $FLINK_JAR"

{
  printf 'key\tvalue\n'
  printf 'captured_utc\t%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'role\thost (Flink) - worker mierzy wylacznie RetractorDB (D-2)\n'
  printf 'hostname\t%s\n' "$(hostname)"
  printf 'kernel\t%s\n' "$(uname -srm)"
  printf 'os\t%s\n' "$(os_pretty_name)"
  printf 'cpu_model\t%s\n' "$(cpu_model)"
  printf 'cpu_online\t%s\n' "$(cpu_online)"

  printf 'jdk_pinned_home\t%s\n' "$JAVA_HOME_PINNED"
  printf 'jdk_pinned_java_version\t%s\n' "$("$JAVA" -version 2>&1 | head -n1)"
  printf 'jdk_pinned_runtime\t%s\n' "$("$JAVA" -version 2>&1 | sed -n 2p)"
  printf 'jdk_pinned_javac_version\t%s\n' "$("$JAVAC" -version 2>&1)"
  printf 'jdk_pinned_java_sha256\t%s\n' "$(sha256_of "$JAVA")"
  printf 'jdk_pinned_javac_sha256\t%s\n' "$(sha256_of "$JAVAC")"
  # Domyslne `java` z PATH zapisane WYLACZNIE jako dowod, ze przypiecie jest konieczne.
  printf 'jdk_path_default_version\t%s\n' "$(java -version 2>&1 | head -n1)"
  printf 'jdk_path_default_binary\t%s\n' "$(abs_path "$(command -v java)")"

  printf 'flink_home\t%s\n' "$FLINK_HOME"
  printf 'flink_dist_jar\t%s\n' "$FLINK_JAR"
  printf 'flink_dist_jar_sha256\t%s\n' "$(sha256_of "$FLINK_JAR")"
  printf 'flink_dist_jar_bytes\t%s\n' "$(file_size "$FLINK_JAR")"
  printf 'flink_version_from_jar\t%s\n' \
    "$(unzip -p "$FLINK_JAR" META-INF/MANIFEST.MF 2>/dev/null | tr -d '\r' | awk -F': ' '/^Implementation-Version/{print $2; exit}')"
  printf 'flink_dist_tarball_sha512_file\t%s\n' "$FLINK_HOME-bin-scala_2.12.tgz"
  if [[ -r "$FLINK_HOME.sha512" ]]; then
    printf 'flink_tarball_sha512_declared\t%s\n' "$(awk '{print $1}' "$FLINK_HOME.sha512")"
  fi
  # Wszystkie jary z lib/ - job kompiluje sie wobec calego lib, wiec caly lib jest przypieciem.
  while IFS= read -r jar; do
    printf 'flink_lib_sha256\t%s  %s\n' "$(sha256_of "$jar")" "$(basename "$jar")"
  done < <(find "$FLINK_HOME/lib" -maxdepth 1 -name '*.jar' | sort)

  printf 'oracle_cpp_binary\t%s\n' "$HERE/oracle/canonical_oracle"
  if [[ -x "$HERE/oracle/canonical_oracle" ]]; then
    printf 'oracle_cpp_sha256\t%s\n' "$(sha256_of "$HERE/oracle/canonical_oracle")"
  fi
  printf 'canonical_vectors_sha256\t%s\n' "$(sha256_of "$HERE/canonical_vectors.tsv")"

  printf 'retractordb_head\t%s\n' "$(git -C "${CODE_REPO:-/home/michal/github/retractordb}" rev-parse HEAD)"
  printf 'experiment_head\t%s\n' "$(git -C "$HERE/../.." rev-parse HEAD)"
  printf 'experiment_branch\t%s\n' "$(git -C "$HERE/../.." branch --show-current)"
} >"$OUT"

echo "OK: $OUT"
