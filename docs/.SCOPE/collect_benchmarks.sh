#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PY="$SCRIPT_DIR/collect_benchmarks.py"
OUT="${1:-$SCRIPT_DIR/bench.json}"
shift || true
if [ ! -x "$PY" ]; then
  chmod +x "$PY" || true
fi
python3 "$PY" --output "$OUT" "$@"
