#!/usr/bin/env bash

set -euo pipefail

BUILD_DIR=${1:-build}

echo "[*] Looking for benchmarks in $BUILD_DIR"

BENCHES=$(find "$BUILD_DIR" -maxdepth 1 -type f -executable -name '*benchmark*')

if [[ -z "$BENCHES" ]]; then
  echo "[!] No benchmark executables found."
  exit 1
fi

FAILED=0

for bench in $BENCHES; do
  echo "[+] Running benchmark: $bench"
  if ! "$bench"; then
    echo "[!] Benchmark $bench failed."
    FAILED=1
  fi
done

if [[ "$FAILED" -eq 1 ]]; then
  echo "[!] Some benchmarks failed."
  exit 1
fi

echo "[✓] All benchmarks passed."
