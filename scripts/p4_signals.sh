#!/usr/bin/env bash
# Project 4 -- compile monitor_service and demonstrate signal handling.
#
# Two scripted demonstration runs are captured:
#   demo_sigint.txt  : heartbeats + two SIGUSR1 status requests + SIGINT (graceful)
#   demo_sigterm.txt : heartbeat + SIGUSR1 + SIGTERM (emergency shutdown)
set -u
cd "$(dirname "$0")/../project4_signal_service"
OUT=outputs
mkdir -p "$OUT"

echo "[p4] compiling monitor_service ..."
gcc -O0 -g -o monitor_service monitor_service.c

# --- Demonstration 1: SIGUSR1 (status) x2, then SIGINT (graceful shutdown) ---
echo "[p4] demo 1: SIGUSR1 status requests, then SIGINT graceful shutdown ..."
./monitor_service > "$OUT/demo_sigint.txt" 2>&1 &
PID=$!
echo "      started monitor_service with PID $PID"
sleep 6                       # let one heartbeat print
echo "      -> kill -SIGUSR1 $PID"
kill -SIGUSR1 "$PID"
sleep 6                       # heartbeat + status report
echo "      -> kill -SIGUSR1 $PID"
kill -SIGUSR1 "$PID"
sleep 6
echo "      -> kill -SIGINT $PID  (graceful)"
kill -SIGINT "$PID"
wait "$PID"
echo "      demo 1 exit code: $?"

# --- Demonstration 2: SIGUSR1, then SIGTERM (emergency shutdown) ---
echo "[p4] demo 2: SIGUSR1, then SIGTERM emergency shutdown ..."
./monitor_service > "$OUT/demo_sigterm.txt" 2>&1 &
PID=$!
echo "      started monitor_service with PID $PID"
sleep 6
echo "      -> kill -SIGUSR1 $PID"
kill -SIGUSR1 "$PID"
sleep 3
echo "      -> kill -SIGTERM $PID  (emergency)"
kill -SIGTERM "$PID"
wait "$PID"
echo "      demo 2 exit code: $?"

echo ""
echo "[p4] ===== demo_sigint.txt ====="
cat "$OUT/demo_sigint.txt"
echo "[p4] ===== demo_sigterm.txt ====="
cat "$OUT/demo_sigterm.txt"
echo "[p4] DONE. Outputs in project4_signal_service/$OUT/"
