#!/usr/bin/env bash
# Simple load test for simple-sftpd: multiple concurrent FTP sessions.
# Usage: ./scripts/load-test.sh [HOST] [PORT] [NUM_CLIENTS] [USER] [PASS]
# Requires: nc (optional), or a working 'ftp' client for your OS.

set -e

HOST="${1:-localhost}"
PORT="${2:-21}"
NUM_CLIENTS="${3:-10}"
USER="${4:-test}"
PASS="${5:-test}"

echo "Load test: host=$HOST port=$PORT clients=$NUM_CLIENTS user=$USER"
echo "Each client will: connect, login, LIST, QUIT."
echo ""

if ! command -v nc &>/dev/null; then
  echo "nc (netcat) required. Install netcat-openbsd or equivalent."
  exit 1
fi

run_one_ftp() {
  local id="$1"
  (
    printf "USER %s\r\nPASS %s\r\nPWD\r\nQUIT\r\n" "$USER" "$PASS" | timeout 10 nc "$HOST" "$PORT" 2>/dev/null || true
  ) &
}

for i in $(seq 1 "$NUM_CLIENTS"); do
  run_one_ftp "$i"
done
wait
echo "Done: $NUM_CLIENTS concurrent control-channel sessions attempted."
echo "For transfer load testing, use a dedicated FTP load tool (e.g. JMeter FTP, custom script with RETR/STOR)."
