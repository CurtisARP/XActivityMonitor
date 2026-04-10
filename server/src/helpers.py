import flask

from .constants import API_KEY, FIVE_MIN_MS


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
