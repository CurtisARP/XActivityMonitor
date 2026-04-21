import sqlite3

from .constants import DB_PATH
from .helpers import maybe_cleanup


def get_db():
    conn = sqlite3.connect(DB_PATH)
    conn.execute("PRAGMA journal_mode=WAL")
    return conn


def init_db():
    conn = get_db()
    try:
        conn.executescript("""
            CREATE TABLE IF NOT EXISTS activity_summaries (
                id             INTEGER PRIMARY KEY AUTOINCREMENT,
                received_at_ms INTEGER NOT NULL,
                lc             INTEGER NOT NULL DEFAULT 0,
                rc             INTEGER NOT NULL DEFAULT 0,
                mc             INTEGER NOT NULL DEFAULT 0,
                kp             INTEGER NOT NULL DEFAULT 0,
                mm             INTEGER NOT NULL DEFAULT 0
            );
            CREATE INDEX IF NOT EXISTS idx_activity_summaries_received_at
                ON activity_summaries (received_at_ms);
        """)
        conn.commit()
    finally:
        conn.close()


def insert_activity(received, lc, rc, mc, kp, mm):
    conn = get_db()
    try:
        conn.execute(
            """
            INSERT INTO activity_summaries (received_at_ms, lc, rc, mc, kp, mm)
            VALUES (?, ?, ?, ?, ?, ?)
        """,
            (received, lc, rc, mc, kp, mm),
        )
        maybe_cleanup(conn)
        conn.commit()
    finally:
        conn.close()


def get_activity_since(cutoff):
    conn = get_db()
    try:
        rows = conn.execute(
            """
            SELECT received_at_ms, lc, rc, mc, kp, mm
            FROM activity_summaries
            WHERE received_at_ms >= ?
            ORDER BY received_at_ms ASC
        """,
            (cutoff,),
        ).fetchall()
    finally:
        conn.close()
    return rows
