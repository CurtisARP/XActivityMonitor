# Activity Monitor

The **client** (C) records keyboard and mouse activity on X11 via XRecord, aggregates counts on a fixed sampling interval, and periodically POSTs a JSON snapshot to an HTTP API. The **server** (Python) is a Flask app that authenticates those POSTs, rate-limits the public GET route, and stores each payload in SQLite.

## Project structure

- `client/` — C sources, `include/`, `CMakeLists.txt`
- `server/` — `src/` (Flask app), `Dockerfile`, `pyproject.toml`, `tests/` (unit tests for helpers and DB), `.env.example`

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
| `API_URL` | POST target, e.g. `http://localhost:5000/frames` when the server runs locally with default `PORT` |
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

- `timestamp_ms` — wall-clock Unix time in milliseconds when the payload is built (the server still records **received** time rounded to 5 minutes for storage).
- `summary.total_*` — **counts since the last successful upload** (not lifetime). After each successful POST they are cleared on the client.

Wheel motion is folded into `total_scroll` (X11 buttons 4 and 5). The server persists **left/right/middle click**, **keypress**, and **mouse-move** totals (`total_lc`, `total_rc`, `total_mc`, `total_kp`, `total_mm`); it does not store `total_scroll` in SQLite.

## Server

Flask app in `server/src/main.py`: CORS enabled, [Flask-Limiter](https://flask-limiter.readthedocs.io/) on the public summary route, SQLite via `server/src/db.py`.

### HTTP API

| Method | Path | Auth / limits | Description |
|--------|------|-----------------|-------------|
| GET | `/health` | none | JSON `{"status":"ok"}` — intended for load balancers and Docker health checks |
| POST | `/frames` | `Authorization: <SERVER_API_KEY>` or `Authorization: Bearer <SERVER_API_KEY>` | Validates JSON; inserts one row (5-minute rounded receive time and summary ints) |
| GET | `/activity/summary` | rate-limited per client IP (`GET_RATE_LIMIT_*`) | JSON `rows` for the last **14 days** from `activity_summaries` |

### Dependencies and run (local)

Uses **[uv](https://github.com/astral-sh/uv)** (`requires-python >= 3.11` in `pyproject.toml`).

```bash
cd server
uv sync
uv run python -m src.main
```

By default the dev server listens on **`0.0.0.0:5000`** unless you set `PORT` (and optionally `HOST`). `init_db()` runs on startup.

### Environment variables

Names and placeholders also appear in `server/.env.example` for copying into Coolify or a local `.env`.

| Variable | Default | Purpose |
|----------|---------|---------|
| `SERVER_API_KEY` | `secret` | Shared secret; client must send the same value in `Authorization` (plain or `Bearer …`) |
| `ACTIVITY_DB_PATH` | `activity_monitor.db` | SQLite file path (relative paths are resolved from the process working directory) |
| `GET_RATE_LIMIT_MAX_REQUESTS` | `30` | Max GET `/activity/summary` requests per client IP per window |
| `GET_RATE_LIMIT_WINDOW_SEC` | `60` | Window length in seconds for that limit |
| `PORT` | `5000` (when not set) | Listen port (`python -m src.main`); Docker/Coolify typically set this (see below) |
| `HOST` | `0.0.0.0` | Bind address |

### Docker and Coolify

Build context is **`server/`** (the directory that contains the `Dockerfile`).

```bash
docker build -t activity-monitor-server ./server
docker run --rm -p 3000:3000 \
  -e SERVER_API_KEY=your-key \
  -e ACTIVITY_DB_PATH=/data/activity_monitor.db \
  -v activity-data:/data \
  activity-monitor-server
```

The image defaults to **`PORT=3000`** and **`EXPOSE 3000`** (aligned with Coolify’s default application port). Set **`ACTIVITY_DB_PATH`** to a path under a **mounted volume** (e.g. `/data/...`) if you want the database to survive container replacement.

For **Coolify**: choose the Dockerfile build pack, set **base directory** to `server`, keep the exposed port in sync with `PORT` (default **3000**), add runtime variables from the table above (at minimum **`SERVER_API_KEY`**), and mount persistent storage where `ACTIVITY_DB_PATH` points if you use a custom path.

### SQLite

Table **`activity_summaries`**: `received_at_ms` (5-minute bucket from server receive time), and integer columns `lc`, `rc`, `mc`, `kp`, `mm`. Inserts may trigger **cleanup** of rows older than 30 days (see `maybe_cleanup` in `server/src/helpers.py`).

### Tests

Unit tests cover **helpers** (`round_to_5min`, `safe_int`, `is_authorized`, `maybe_cleanup`) and **DB** helpers (`init_db`, `insert_activity`, `get_activity_since`), not HTTP routes.

```bash
cd server
uv run --with pytest pytest
```

Dev dependency group: `uv sync --extra dev` then `pytest`.

## End-to-end check

1. Start the server (`uv run python -m src.main` in `server/`).
2. Set `client/include/config.h` so `API_URL` points at `http://localhost:5000/frames` (or your deployed URL) and `API_KEY` matches `SERVER_API_KEY`.
3. Run `./build/activity_monitor` from `client/` on an X11 session.
