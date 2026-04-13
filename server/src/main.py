import os
import time

import flask
from flask_cors import CORS
from flask_limiter import Limiter
from flask_limiter.util import get_remote_address

from .constants import GET_RATE_LIMIT, FOURTEEN_DAYS_MS
from .db import init_db, insert_activity, get_activity_since
from .helpers import round_to_5min, is_authorized, safe_int

app = flask.Flask(__name__)
CORS(app)

limiter = Limiter(key_func=get_remote_address, app=app, default_limits=[])


@app.route("/health")
def health():
    return flask.jsonify({"status": "ok"}), 200


@app.route("/frames", methods=["POST"])
def frames():
    if not is_authorized():
        return flask.jsonify({"error": "unauthorized"}), 401

    data = flask.request.get_json(silent=True)
    if not isinstance(data, dict):
        return flask.jsonify({"error": "invalid_json"}), 400

    summary = data.get("summary", {})
    if not isinstance(summary, dict):
        summary = {}

    received = round_to_5min(int(time.time() * 1000))

    insert_activity(
        received,
        safe_int(summary.get("total_lc")),
        safe_int(summary.get("total_rc")),
        safe_int(summary.get("total_mc")),
        safe_int(summary.get("total_kp")),
        safe_int(summary.get("total_mm")),
    )

    return flask.jsonify({"message": "ok"}), 200


@app.route("/activity/summary")
@limiter.limit(GET_RATE_LIMIT)
def activity_summary():
    cutoff = int(time.time() * 1000) - FOURTEEN_DAYS_MS
    rows = get_activity_since(cutoff)

    return flask.jsonify(
        {
            "rows": [
                {"t": r[0], "lc": r[1], "rc": r[2], "mc": r[3], "kp": r[4], "mm": r[5]}
                for r in rows
            ]
        }
    )


if __name__ == "__main__":
    init_db()
    host = os.environ.get("HOST", "0.0.0.0")
    port = int(os.environ.get("PORT", "5000"))
    app.run(host=host, port=port)
