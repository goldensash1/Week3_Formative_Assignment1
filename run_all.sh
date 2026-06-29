#!/usr/bin/env bash
# Regenerate every command output in this repository from scratch.
#
#   docker build -t week3-linux .
#   docker run --rm -v "$PWD":/work -w /work week3-linux ./run_all.sh
#
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"

echo "############################################################"
echo "# Week 3 Formative Assignment 1 -- full regeneration"
echo "# $(uname -a)"
echo "############################################################"

bash "$HERE/scripts/p1_analyze.sh"
echo
bash "$HERE/scripts/p2_trace.sh"
echo
bash "$HERE/scripts/p3_benchmark.sh"
echo
bash "$HERE/scripts/p4_signals.sh"

echo
echo "All four projects regenerated."
