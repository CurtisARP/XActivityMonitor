# Activity Monitor

The **client** (C) records keyboard and mouse activity on X11 via XRecord, aggregates counts on a fixed sampling interval, and periodically POSTs a JSON snapshot to an HTTP API. The **server** (Python) is a Flask app that authenticates those POSTs, rate-limits the public GET route, and stores each payload in SQLite.

## Project structure

- `client/` — C sources, `include/`, `CMakeLists.txt`
- `server/` — `src/` (Flask app), `pyproject.toml`, `uv.lock`

## Client

### Behavior

- Input events are counted in **atomic** counters from the XRecord thread (no per-frame ring buffer, no mutex on the hot path).
- On each **sample tick** (`FRAME_INTERVAL_MS`), the main thread reads and zeroes those counters and adds the deltas into in-memory **summary totals** used for the next upload.
- On each **flush tick** (`FLUSH_INTERVAL_SEC`), the client POSTs **only if there was activity** since the last **successful** send. If the request fails, totals are **not** cleared so the next flush can retry.
- After a **successful** POST, summary totals are **reset to zero** so the next payload reflects only new activity since that upload.
- On shutdown, a **forced** final POST is attempted (even if there was no new activity flag), then HTTP resources are torn down.

### Dependencies (Arch Linux)

```bash
sudo pacman -S cmake gcc pkgconf libx11 libxtst libxi curl
```

### Build and run

```bash
cd client
cmake -S . -B build
cmake --build build
./build/activity_monitor
```

Requires a running X session (`DISPLAY` set).

### Configuration (`client/include/config.h`)

| Symbol | Role |
|--------|------|
| `FRAME_INTERVAL_MS` | How often the main loop snapshots atomic counters into summary totals |
| `FLUSH_INTERVAL_SEC` | How often a POST is considered (still gated on activity / last success) |
| `API_URL` | POST target, e.g. `http://localhost:5000/frames` |
| `API_KEY` | Sent as the `Authorization` header; must match the server’s `SERVER_API_KEY` |

### JSON payload (client → server)

```json
{
  "timestamp_ms": 1775600941542,
  "summary": {
    "total_lc": 0,
    "total_rc": 0,
    "total_mc": 0,
    "total_scroll": 0,
    "total_kp": 0,
    "total_mm": 0
  }
}
```

- `timestamp_ms` — wall-clock Unix time in milliseconds when the payload is built.
- `summary.total_*` — **counts since the last successful upload** (not lifetime). After each successful POST they are cleared on the client.

Wheel motion is folded into `total_scroll` (X11 buttons 4 and 5).

## Server

Flask app in `server/src/main.py`, bound to `0.0.0.0:5000`.

| Method | Path | Description |
|--------|------|-------------|
| POST | `/frames` | **Requires auth**; validates JSON; appends a row to SQLite and returns `200` |
| GET | `/activity/summary` | Returns aggregated activity data; **rate-limited** with [Flask-Limiter](https://flask-limiter.readthedocs.io/) |

### Dependencies and run

Uses **[uv](https://github.com/astral-sh/uv)** (`requires-python >= 3.11` in `pyproject.toml`).

```bash
cd server
uv sync
uv run python -m src.main
```

### Environment variables

| Variable | Default | Purpose |
|----------|---------|---------|
| `SERVER_API_KEY` | `secret` | Shared secret; client must send the same value in `Authorization` (plain or `Bearer …`) |
| `ACTIVITY_DB_PATH` | `activity_monitor.db` | SQLite file path (created next to the process cwd unless you set an absolute path) |
| `GET_RATE_LIMIT_MAX_REQUESTS` | `30` | Max GET `/` requests per client IP per window |
| `GET_RATE_LIMIT_WINDOW_SEC` | `60` | Window length in seconds for that limit |

### SQLite

Each accepted POST inserts into table `activity_summaries` with 5-minute rounded timestamps and the activity counts. The `/activity/summary` endpoint queries this data.

## End-to-end check

1. Start the server (`uv run python -m src.main` in `server/`).
2. Set `client/include/config.h` so `API_URL` points at `http://localhost:5000/frames` and `API_KEY` matches `SERVER_API_KEY`.
3. Run `./build/activity_monitor` from `client/` on an X11 session.
