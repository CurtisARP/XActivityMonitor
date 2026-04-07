# Activity Monitor

The **client** (C) captures keyboard and mouse activity via XRecord and periodically POSTs a JSON payload with a wall-clock timestamp and lifetime summary counters. The **server** (Python) is a small Flask app that accepts those POSTs for development and testing.

## Project structure

- `client/`: C source, headers, and `CMakeLists.txt`
- `server/`: Flask app (`api.py`), `pyproject.toml`, and `uv` lockfile

## Client

### Dependencies (Arch Linux)

```bash
sudo pacman -S cmake gcc pkgconf libx11 libxtst libxi curl
```

### Build

```bash
cd client
cmake -S . -B build
cmake --build build
```

### Run

```bash
./build/activity_monitor
```

Requires a running X session (`DISPLAY` set). Point the client at your API with `API_URL` and optional `Authorization` header via `API_KEY` in `client/include/config.h`.

## Server

The server is a minimal [Flask](https://flask.palletsprojects.com/) application in `server/api.py`. It listens on **all interfaces** (`0.0.0.0`) on port **5000** and exposes:

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Simple health-style response |
| POST | `/frames` | Accepts JSON from the client; the handler prints the body and returns `200` with a short JSON message |

Dependencies are managed with **[uv](https://github.com/astral-sh/uv)** (`pyproject.toml` pins Python ≥ 3.11 and Flask). From the `server` directory:

```bash
uv sync
uv run python api.py
```

The default client `API_URL` is `http://localhost:5000/frames`, matching this app.

### Request body shape

The client sends JSON like:

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

`timestamp_ms` is Unix time in milliseconds (real-time clock). The `total_*` fields are cumulative lifetime totals since the client started. You can replace `print(data)` in `api.py` with persistence, validation, or forwarding as needed.

## Configuration (client)

Edit `client/include/config.h` for:

- `FRAME_INTERVAL_MS` — sampling interval for aggregating input
- `FLUSH_INTERVAL_SEC` — how often to POST (when there was activity since the last successful send)
- `API_URL` — e.g. `http://localhost:5000/frames`
- `API_KEY` — sent as the `Authorization` header value
