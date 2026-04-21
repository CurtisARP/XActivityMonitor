import time
from unittest.mock import MagicMock, patch

import pytest

import src.helpers as helpers


@pytest.fixture(autouse=True)
def reset_last_cleanup():
    helpers._last_cleanup = 0
    yield
    helpers._last_cleanup = 0


def test_round_to_5min_aligned():
    boundary = 5 * 60 * 1000
    assert helpers.round_to_5min(boundary) == boundary
    assert helpers.round_to_5min(3 * boundary) == 3 * boundary


def test_round_to_5min_rounds_down():
    base = 1_000_000
    rounded = helpers.round_to_5min(base)
    assert rounded <= base
    assert rounded % (5 * 60 * 1000) == 0
    assert helpers.round_to_5min(base + 1) == rounded


@pytest.mark.parametrize(
    "val, expected",
    [
        (0, 0),
        (42, 42),
        (-3, -3),
        ("7", 7),
        ("0", 0),
        (None, 0),
        ("", 0),
        ("x", 0),
        (3.7, 3),
        ([], 0),
    ],
)
def test_safe_int(val, expected):
    assert helpers.safe_int(val) == expected


def test_is_authorized_raw_key(app, monkeypatch):
    monkeypatch.setattr(helpers, "API_KEY", "k9")
    with app.test_request_context("/", headers={"Authorization": "k9"}):
        assert helpers.is_authorized() is True
    with app.test_request_context("/", headers={"Authorization": "wrong"}):
        assert helpers.is_authorized() is False
    with app.test_request_context("/"):
        assert helpers.is_authorized() is False


def test_is_authorized_bearer(app, monkeypatch):
    monkeypatch.setattr(helpers, "API_KEY", "tok")
    with app.test_request_context("/", headers={"Authorization": "Bearer tok"}):
        assert helpers.is_authorized() is True
    with app.test_request_context("/", headers={"Authorization": "Bearer other"}):
        assert helpers.is_authorized() is False


def test_maybe_cleanup_skips_when_day_elapsed_since_marker():
    conn = MagicMock()
    now_ms = 10**12
    with patch.object(helpers, "_last_cleanup", 0):
        with patch("src.helpers.time.time", return_value=now_ms / 1000.0):
            helpers.maybe_cleanup(conn)
    conn.execute.assert_not_called()


def test_maybe_cleanup_runs_delete_when_within_day_of_marker():
    conn = MagicMock()
    now_ms = 10**12
    helpers._last_cleanup = now_ms
    cutoff = int(now_ms - helpers.THIRTY_DAYS_MS)
    with patch("src.helpers.time.time", return_value=now_ms / 1000.0):
        helpers.maybe_cleanup(conn)
    conn.execute.assert_called_once_with(
        "DELETE FROM activity_summaries WHERE received_at_ms < ?",
        (cutoff,),
    )
    assert helpers._last_cleanup == now_ms
