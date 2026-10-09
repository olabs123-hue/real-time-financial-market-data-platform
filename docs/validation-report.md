# Local Validation Report — 2026-10-09

## Scope
This report records **actual execution in the current Linux environment**. It does **not** claim a complete Kafka → PostgreSQL → API Docker integration test. Docker, Podman, PostgreSQL server, and required Python database/Kafka client libraries were unavailable in this execution environment.

## Fixes made before testing
1. The synthetic feed now generates non-crossing bid and ask price bands (bid 9900–9949 ticks; ask 10000–10049 ticks).
2. The C++ book rejects an update that would cross the opposite best quote, without advancing the sequence.
3. The Python Kafka consumer reuses a PostgreSQL connection and reconnects after transient database failures instead of opening one connection per message.
4. Added a regression assertion for crossed-book rejection.

## Tests performed
- `cmake -S cpp -B build -DCMAKE_BUILD_TYPE=Release` — success
- `cmake --build build -j2` — success
- `ctest --test-dir build --output-on-failure` — 1/1 passed
- `PYTHONPATH=python python3 -m unittest discover -s python/tests -v` — 4/4 passed
- `python3 -m compileall -q python` — success

## C++ benchmark methodology
Executed `build/feed_producer --benchmark --count N` five times per N on Linux x86_64, AMD EPYC 9V74 CPU, GCC 14.2.0, CMake Release build. This is a **single-process microbenchmark** of update generation, in-memory book operations and JSON serialization. The JSON is constructed but not published. Throughput can vary with system load and compiler/hardware.

| Messages/run | Run 1 | Run 2 | Run 3 | Run 4 | Run 5 | Median |
|---|---:|---:|---:|---:|---:|---:|
| 100,000 | 2.831M/s | 2.665M/s | 2.686M/s | 2.312M/s | 2.672M/s | 2.672M/s |
| 1,000,000 | 3.286M/s | 2.759M/s | 3.095M/s | 3.076M/s | 3.059M/s | **3.076M/s** |

Final quote in each benchmark: best bid 9949 ticks, best ask 10000 ticks (non-crossed).

## NOT independently verified
- Docker Compose startup and container health
- Redpanda topic publication/consumption and offset commits
- PostgreSQL inserted row count, duplicate replay, persistence and reconnection
- FastAPI `/health`, `/updates/ACME`, and `/ws/ACME` responses backed by a running database
- End-to-end throughput, end-to-end latency, crash recovery and live WebSocket fan-out

**Do not describe the 3.076M/s median as Kafka, PostgreSQL, API, or end-to-end throughput.**

## How to verify the remaining integration checks
On a machine with Docker Engine + Compose installed:

```bash
docker compose down -v
docker compose up --build -d redpanda postgres consumer api
docker compose run --rm producer --count 10000
docker compose logs --tail=100 consumer
curl -fsS http://localhost:8000/health
curl -fsS 'http://localhost:8000/updates/ACME?limit=3'
docker compose exec -T postgres psql -U market -d market -c 'SELECT count(*),min(sequence),max(sequence) FROM updates WHERE symbol = '\''ACME'\'';'
# Repeat producer and verify row count stays at 10000.
docker compose run --rm producer --count 10000
docker compose exec -T postgres psql -U market -d market -c 'SELECT count(*) FROM updates WHERE symbol = '\''ACME'\'';'
```

Expected outcomes are **hypotheses** until observed. Use `docker compose down -v` to reset volumes between independent runs.
