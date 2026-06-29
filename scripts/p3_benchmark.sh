#!/usr/bin/env bash
# Project 3 -- build the C extension and benchmark it against pure Python.
set -u
cd "$(dirname "$0")/../project3_python_extension"
OUT=outputs
mkdir -p "$OUT"

N=${1:-5000000}
REPEATS=${2:-5}

echo "[p3] building the statsext C extension (build_ext --inplace) ..."
python3 setup.py build_ext --inplace > "$OUT/build_log.txt" 2>&1
tail -3 "$OUT/build_log.txt"
ls -1 statsext*.so | tee "$OUT/built_module.txt"

echo "[p3] running benchmark (N=$N, repeats=$REPEATS) ..."
python3 benchmark.py "$N" "$REPEATS" | tee "$OUT/benchmark_results.txt"

echo "[p3] DONE. Outputs in project3_python_extension/$OUT/"
