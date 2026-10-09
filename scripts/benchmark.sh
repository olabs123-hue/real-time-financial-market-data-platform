#!/usr/bin/env bash
set -euo pipefail
cmake -S cpp -B /tmp/market-benchmark-build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/market-benchmark-build -j2
ctest --test-dir /tmp/market-benchmark-build --output-on-failure
for n in 100000 1000000; do /tmp/market-benchmark-build/feed_producer --benchmark --count "$n"; done
