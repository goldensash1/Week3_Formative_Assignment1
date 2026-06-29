#!/usr/bin/env bash
# Project 2 -- compile backup_tool and trace its system calls with strace.
set -u
cd "$(dirname "$0")/../project2_syscall_monitor"
OUT=outputs
mkdir -p "$OUT"

echo "[p2] compiling backup_tool ..."
gcc -O0 -g -o backup_tool backup_tool.c

# Clean any artefacts from a previous run so the trace is reproducible.
rm -f backup_data.txt backup.log

echo "[p2] running under strace (full timestamped trace) ........"
# -f follow forks, -tt absolute timestamps (us), -T time spent in each call.
strace -f -tt -T -o "$OUT/strace_output.txt" ./backup_tool > "$OUT/program_stdout.txt" 2>&1

echo "[p2] running under strace (-c summary table) .............."
rm -f backup_data.txt backup.log
strace -f -c -o "$OUT/strace_summary.txt" ./backup_tool >/dev/null 2>&1

echo "[p2] file-related syscalls only (filtered view) ..........."
strace -f -e trace=file,desc -tt -o "$OUT/strace_file_calls.txt" ./backup_tool >/dev/null 2>&1

echo "[p2] ---- strace -c summary ----"
cat "$OUT/strace_summary.txt"

echo "[p2] DONE. Outputs in project2_syscall_monitor/$OUT/"
