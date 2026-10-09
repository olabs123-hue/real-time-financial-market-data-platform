from dataclasses import dataclass
@dataclass(frozen=True)
class Update:
    symbol: str
    sequence: int
    side: str
    price_ticks: int
    quantity: int
    best_bid_ticks: int
    best_ask_ticks: int

def validate(raw: dict) -> Update:
    fields = Update.__dataclass_fields__
    if set(raw) != set(fields):
        raise ValueError("unexpected or missing fields")
    if not isinstance(raw['symbol'], str) or not raw['symbol'].isalnum() or len(raw['symbol']) > 20:
        raise ValueError("invalid symbol")
    for key in ('sequence','price_ticks','quantity','best_bid_ticks','best_ask_ticks'):
        if type(raw[key]) is not int or raw[key] < 0 or raw[key] > 2**63-1:
            raise ValueError(f"invalid {key}")
    if raw['sequence'] == 0 or raw['price_ticks'] == 0 or raw['side'] not in ('A','B'):
        raise ValueError("invalid update")
    return Update(**raw)
