import asyncio
import json
import os
from fastapi import FastAPI, HTTPException, WebSocket, WebSocketDisconnect
from psycopg.rows import dict_row
from psycopg_pool import AsyncConnectionPool
from .db import DSN
app = FastAPI(title='Market Data Platform', version='0.1.0')
pool = None
@app.on_event('startup')
async def start():
    global pool
    pool = AsyncConnectionPool(DSN, min_size=1, max_size=5, open=False)
    await pool.open(wait=True)
@app.on_event('shutdown')
async def stop():
    if pool: await pool.close()
@app.get('/health')
async def health():
    async with pool.connection() as conn:
        await conn.execute('SELECT 1')
    return {'status':'ok'}
@app.get('/updates/{symbol}')
async def updates(symbol: str, limit: int = 100):
    if not symbol.isalnum() or len(symbol)>20 or not 1<=limit<=1000:
        raise HTTPException(400, 'invalid symbol or limit')
    async with pool.connection() as conn:
        async with conn.cursor(row_factory=dict_row) as cur:
            await cur.execute('SELECT symbol,sequence,side,price_ticks,quantity,best_bid_ticks,best_ask_ticks FROM updates WHERE symbol=%s ORDER BY sequence DESC LIMIT %s', (symbol,limit))
            return await cur.fetchall()
@app.websocket('/ws/{symbol}')
async def stream(websocket: WebSocket, symbol: str):
    await websocket.accept()
    if not symbol.isalnum() or len(symbol)>20:
        await websocket.close(code=1008)
        return
    last=0
    try:
        while True:
            async with pool.connection() as conn:
                async with conn.cursor(row_factory=dict_row) as cur:
                    await cur.execute('SELECT symbol,sequence,side,price_ticks,quantity,best_bid_ticks,best_ask_ticks FROM updates WHERE symbol=%s AND sequence>%s ORDER BY sequence LIMIT 200',(symbol,last))
                    rows=await cur.fetchall()
            for row in rows:
                await websocket.send_json(row)
                last=row['sequence']
            await asyncio.sleep(0.25)
    except WebSocketDisconnect:
        pass
