import json
import logging
import os
import time
from confluent_kafka import Consumer, KafkaException
import psycopg
from .db import DSN, initialize
from .models import validate
logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)
SQL = """INSERT INTO updates(symbol,sequence,side,price_ticks,quantity,best_bid_ticks,best_ask_ticks)
 VALUES (%s,%s,%s,%s,%s,%s,%s) ON CONFLICT (symbol,sequence) DO NOTHING"""

def process(conn, payload):
    update = validate(json.loads(payload))
    with conn.transaction():
        conn.execute(SQL, tuple(getattr(update, field) for field in update.__dataclass_fields__))
    return update

def run():
    while True:
        try:
            initialize()
            break
        except psycopg.OperationalError:
            log.warning('Database not ready; retrying')
            time.sleep(2)
    c = Consumer({'bootstrap.servers': os.getenv('KAFKA_BROKERS','redpanda:9092'),
                  'group.id': 'market-storage-v1', 'auto.offset.reset':'earliest',
                  'enable.auto.commit':False})
    c.subscribe([os.getenv('KAFKA_TOPIC','market.updates')])
    conn = None
    try:
        while True:
            if conn is None or conn.closed:
                try:
                    conn = psycopg.connect(DSN)
                except psycopg.OperationalError:
                    log.exception('Database connection unavailable; retrying')
                    time.sleep(2)
                    continue
            msg = c.poll(1)
            if msg is None: continue
            if msg.error():
                log.error('Kafka error: %s', msg.error())
                continue
            try:
                process(conn, msg.value())
                c.commit(message=msg, asynchronous=False)
            except (ValueError, json.JSONDecodeError) as exc:
                log.error('Invalid payload at offset %s: %s; skipping', msg.offset(), exc)
                c.commit(message=msg, asynchronous=False)
            except (psycopg.Error, KafkaException):
                log.exception('Transient processing error; retrying without offset commit')
                if conn is not None:
                    conn.close()
                    conn = None
                time.sleep(2)
    finally:
        if conn is not None: conn.close()
        c.close()
if __name__=='__main__':run()
