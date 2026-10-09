import os
import psycopg
DSN = os.getenv('DATABASE_URL', 'postgresql://market:market@localhost:5432/market')
SCHEMA = """
CREATE TABLE IF NOT EXISTS updates (
 symbol TEXT NOT NULL, sequence BIGINT NOT NULL, side CHAR(1) NOT NULL,
 price_ticks BIGINT NOT NULL, quantity BIGINT NOT NULL,
 best_bid_ticks BIGINT NOT NULL, best_ask_ticks BIGINT NOT NULL,
 ingested_at TIMESTAMPTZ NOT NULL DEFAULT now(),
 PRIMARY KEY(symbol, sequence)
);
CREATE INDEX IF NOT EXISTS updates_symbol_time ON updates(symbol, ingested_at DESC);
"""
def initialize():
    with psycopg.connect(DSN) as conn:
        conn.execute(SCHEMA)
