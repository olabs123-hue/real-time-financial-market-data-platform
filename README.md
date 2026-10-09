# Real-Time Financial Market Data Platform

A reproducible **portfolio / learning implementation** of a C++17 market-data feed producer, Kafka-compatible event bus (Redpanda), PostgreSQL history store, and Python FastAPI/WebSocket read service. Uses **synthetic ACME prices only**; no live exchange connectivity or trading execution.

## Architecture

```text
C++17 synthetic feed + in-memory L1 book
             | JSON messages keyed by symbol
             v
      Redpanda / Kafka
             |
      Python consumer (manual commits)
             |
       PostgreSQL (unique symbol, sequence)
             |
     FastAPI REST + WebSocket polling
```

## Features
- Deterministic, sequenced synthetic bid/ask price-level updates, encoded as integer price ticks.
- C++ in-memory best-bid/best-ask reconstruction; sequence gaps rejected.
- Kafka producer with delivery queue backpressure and flush on completion.
- Python consumer validates incoming JSON, inserts idempotently with `(symbol, sequence)` uniqueness, then commits Kafka offsets after the database transaction succeeds.
- REST `/updates/{symbol}?limit=100`, `/health`, and `/ws/{symbol}` WebSocket for live polling of stored updates.
- CMake/CTest and Python unittest checks, GitHub Actions CI, reproducible local C++ microbenchmark.

## Quick start (Docker required)

```bash
docker compose up --build -d redpanda postgres consumer api
docker compose run --rm producer --count 10000
curl http://localhost:8000/health
curl 'http://localhost:8000/updates/ACME?limit=3'
docker compose logs consumer
```

For a clean dataset, run `docker compose down -v` before starting again. The producer resets its sequence to 1 on every run, so replaying it into an existing database intentionally produces idempotent conflicts rather than new rows.

Connect a WebSocket client to `ws://localhost:8000/ws/ACME` to receive newly persisted updates. The WebSocket endpoint polls PostgreSQL every 250 ms; **it is not a low-latency push feed**.

## Build and test without Docker

```bash
cmake -S cpp -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
build/feed_producer --benchmark --count 1000000
PYTHONPATH=python python3 -m unittest discover -s python/tests -v
```

A host build defaults to benchmark-only mode. To publish to Kafka on the host, install `librdkafka-dev` and configure with `-DENABLE_KAFKA=ON`.

## Benchmark methodology

Run `bash scripts/benchmark.sh`. It measures local C++ generation, in-memory book updates and JSON serialization **only** (no Kafka, PostgreSQL or network). Report `operations_per_second` alongside CPU, OS, compiler, optimization flags, iteration count and repetitions. Do **not** describe this as end-to-end throughput. Measured local runs and environment details are recorded in `docs/validation-report.md`.

For end-to-end measurements, compare the number of unique rows ingested with wall-clock time after producer completion and consumer catch-up, then report hardware, Kafka partition count, database state and load parameters. The example does not yet automate this.

## Design decisions and limitations

- **Synthetic-only**: no authenticated exchange APIs, financial advice or order placement.
- **Sequence scope**: single producer and one symbol `ACME`; sequence monotonicity is enforced in C++ only. The consumer deduplicates on `(symbol,sequence)` but does not yet detect missing sequences in persistent storage.
- **At-least-once processing**: manual Kafka offset commits follow committed PostgreSQL transactions. A crash between DB commit and offset commit can cause replay; the unique key makes replays idempotent.
- **Malformed messages**: currently logged and skipped with committed offsets, not routed to a dead-letter topic.
- **Best quotes**: crossing updates are rejected in C++; the synthetic producer uses separate bid/ask price bands. This is not an exchange matching engine.
- **API**: polls Postgres and supports bounded history; no authentication, rate limits, retention policy, TLS, or horizontal scaling configuration.
- **Security**: default local development passwords; never expose this compose stack directly to the internet.

## Roadmap
1. Multi-symbol feed handlers with partition-aware sequence tracking and persisted gap detection.
2. Kafka integration tests and failure injection (database outage, duplicate delivery, consumer restart).
3. Event-time OHLCV/VWAP analytics and snapshots/replay.
4. Prometheus metrics, Grafana dashboards and end-to-end benchmark harness.
5. Authenticated, backpressured WebSocket fan-out.

## Validation evidence
See `docs/validation-report.md` for independently rerun local tests, benchmark repetitions, and unverified integration checks.

## CV-safe description

> Built a synthetic market-data pipeline using a C++17 feed generator, Kafka-compatible streaming, a Python consumer with post-transaction offset commits, PostgreSQL idempotent storage, and REST/WebSocket query endpoints; added unit tests, CI and a reproducible local C++ microbenchmark.

Only use this claim once you have run and validated the full Docker stack yourself.

## License
MIT

## Reproduce the local validation
Run `bash scripts/verify_local.sh` and compare with `docs/validation-report.md`. Docker integration remains a separate step; the report clearly marks checks that were not run.
