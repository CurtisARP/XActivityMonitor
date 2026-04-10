import json
import os
import sqlite3
import time

import flask
from flask_limiter import Limiter
from flask_limiter.util import get_remote_address

app = flask.Flask(__name__)

DB_PATH = os.getenv("ACTIVITY_DB_PATH", "activity_monitor.db")
API_KEY = os.getenv("SERVER_API_KEY", "secret")
GET_RATE_LIMIT_WINDOW_SEC = os.getenv("GET_RATE_LIMIT_WINDOW_SEC", "60")
GET_RATE_LIMIT_MAX_REQUESTS = os.getenv("GET_RATE_LIMIT_MAX_REQUESTS", "30")
GET_RATE_LIMIT = f"{GET_RATE_LIMIT_MAX_REQUESTS} per {GET_RATE_LIMIT_WINDOW_SEC} second"

limiter = Limiter(
    key_func=get_remote_address,
    app=app,
    default_limits=[],
)


def init_db():
    conn = sqlite3.connect(DB_PATH)
    try:
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS frames (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                received_at_ms INTEGER NOT NULL,
                timestamp_ms INTEGER,
                total_lc INTEGER,
                total_rc INTEGER,
                total_mc INTEGER,
                total_scroll INTEGER,
                total_kp INTEGER,
                total_mm INTEGER,
                payload_json TEXT NOT NULL
            )
            """
        )
        conn.commit()
    finally:
        conn.close()


def is_authorized():
    header = flask.request.headers.get("Authorization", "")
    if not header:
        return False
    if header == API_KEY:
        return True
    return header == f"Bearer {API_KEY}"


def insert_frame_payload(data):
    summary = data.get("summary", {}) if isinstance(data, dict) else {}
    received_at_ms = int(time.time() * 1000)
    timestamp_ms = data.get("timestamp_ms") if isinstance(data, dict) else None

    conn = sqlite3.connect(DB_PATH)
    try:
        conn.execute(
            """
            INSERT INTO frames (
                received_at_ms,
                timestamp_ms,
                total_lc,
                total_rc,
                total_mc,
                total_scroll,
                total_kp,
                total_mm,
                payload_json
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                received_at_ms,
                timestamp_ms,
                summary.get("total_lc"),
                summary.get("total_rc"),
                summary.get("total_mc"),
                summary.get("total_scroll"),
                summary.get("total_kp"),
                summary.get("total_mm"),
                json.dumps(data),
            ),
        )
        conn.commit()
    finally:
        conn.close()


@app.route('/')
@limiter.limit(GET_RATE_LIMIT)
def index():
    return 'Hello, World!'

@app.route('/frames', methods=['POST'])
def frames():
    if not is_authorized():
        return flask.jsonify({"error": "unauthorized"}), 401

    data = flask.request.get_json(silent=True)
    if not isinstance(data, dict):
        return flask.jsonify({"error": "invalid_json"}), 400

    insert_frame_payload(data)
    return flask.jsonify({'message': 'Frames received'}), 200

if __name__ == '__main__':
    init_db()
    app.run(host='0.0.0.0', port=5000)