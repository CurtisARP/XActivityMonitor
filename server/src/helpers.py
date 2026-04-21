import time
import flask

from .constants import API_KEY, FIVE_MIN_MS, THIRTY_DAYS_MS, ONE_DAY_MS


def round_to_5min(ms: int) -> int:
    return (ms // FIVE_MIN_MS) * FIVE_MIN_MS


def is_authorized() -> bool:
    header = flask.request.headers.get("Authorization", "")
    return header in (API_KEY, f"Bearer {API_KEY}")


def safe_int(val) -> int:
    try:
        return int(val)
    except (TypeError, ValueError):
        return 0

_last_cleanup = 0

def maybe_cleanup(conn):
    global _last_cleanup
    now = int(time.time() * 1000)
    if now - _last_cleanup >= ONE_DAY_MS:
        return
    cutoff = int(now - THIRTY_DAYS_MS)
    conn.execute("DELETE FROM activity_summaries WHERE received_at_ms < ?", (cutoff,))
    _last_cleanup = now