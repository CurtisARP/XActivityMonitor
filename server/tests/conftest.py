import importlib

import flask
import pytest


@pytest.fixture
def app():
    return flask.Flask(__name__)


@pytest.fixture
def isolated_db(monkeypatch, tmp_path):
    db_file = tmp_path / "test.db"
    monkeypatch.setenv("ACTIVITY_DB_PATH", str(db_file))
    import src.constants as constants
    import src.db as db
    import src.helpers as helpers

    importlib.reload(constants)
    importlib.reload(helpers)
    importlib.reload(db)
    db.init_db()
    yield db
