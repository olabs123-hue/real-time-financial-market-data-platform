FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends g++ cmake make librdkafka-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DENABLE_KAFKA=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build -j2 && ctest --test-dir build --output-on-failure
FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends librdkafka1 && rm -rf /var/lib/apt/lists/*
COPY --from=build /src/build/feed_producer /usr/local/bin/feed_producer
ENTRYPOINT ["feed_producer"]
CMD ["--count","10000"]
