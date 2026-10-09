#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S cpp -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
PYTHONPATH=python python3 -m unittest discover -s python/tests -v
python3 -m compileall -q python
for n in 100000 1000000; do
  for r in 1 2 3 4 5; do
    echo "size=$n run=$r"
    build/feed_producer --benchmark --count "$n"
  done
done
