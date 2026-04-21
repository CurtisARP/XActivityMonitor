def test_get_activity_since_empty(isolated_db):
    db = isolated_db
    assert db.get_activity_since(0) == []


def test_insert_and_get_activity_since(isolated_db):
    db = isolated_db
    t = 5_000_000
    db.insert_activity(t, 1, 2, 3, 4, 5)
    rows = db.get_activity_since(0)
    assert len(rows) == 1
    assert rows[0] == (t, 1, 2, 3, 4, 5)


def test_get_activity_since_respects_cutoff(isolated_db):
    db = isolated_db
    db.insert_activity(100, 1, 0, 0, 0, 0)
    db.insert_activity(500, 2, 0, 0, 0, 0)
    assert db.get_activity_since(200) == [(500, 2, 0, 0, 0, 0)]


def test_get_activity_since_ordered_asc(isolated_db):
    db = isolated_db
    db.insert_activity(300, 0, 0, 0, 0, 0)
    db.insert_activity(100, 1, 0, 0, 0, 0)
    db.insert_activity(200, 2, 0, 0, 0, 0)
    rows = db.get_activity_since(0)
    assert [r[0] for r in rows] == [100, 200, 300]


def test_init_db_idempotent(isolated_db):
    db = isolated_db
    db.init_db()
    db.insert_activity(1, 0, 0, 0, 0, 0)
    db.init_db()
    assert len(db.get_activity_since(0)) == 1
