#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
echo "Date (UTC): $(date -u +%Y-%m-%dT%H:%M:%SZ)"
uname -a
g++ --version | head -1
echo "Workload: Current repository feed_producer synthetic in-memory generation/book updates/JSON serialization, excluding Kafka, PostgreSQL, API and network."
for i in 1 2 3 4 5; do
  echo "========== RUN $i / 5 =========="
  cmake -S cpp -B "$tmp/build" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$tmp/build" -j2
  "$tmp/build/feed_producer" --benchmark --count 100000
done
