#!/bin/bash
# Kompilacja strony Flinka K26 przypietym JDK 17 wobec przypietego Flinka 2.3.0.
# Bez Mavena — ten sam wzorzec, co aparatura K22 (javac + classpath z lib/).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JAVA_HOME_PINNED="${JAVA_HOME_PINNED:-/usr/lib/jvm/java-17-openjdk-amd64}"
FLINK_HOME="${FLINK_HOME:-/home/michal/opt/flink-2.3.0}"

CP="$(find "$FLINK_HOME/lib" -maxdepth 1 -name '*.jar' | sort | paste -sd:)"

rm -rf "$HERE/build"
mkdir -p "$HERE/build"
"$JAVA_HOME_PINNED/bin/javac" -Xlint:-deprecation -nowarn -cp "$CP" -d "$HERE/build" \
  "$HERE/java/Canon.java" "$HERE/java/CanonTest.java" "$HERE/java/K26Ops.java" \
  "$HERE/java/PlanDump.java" "$HERE/java/F9R2Job.java" "$HERE/java/F9R1Job.java" "$HERE/java/F9XJob.java"

echo "OK: $HERE/build"
