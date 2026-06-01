"""Configuração compartilhada dos testes do backend.

Cria um banco SQLite isolado em memória (não toca em ``micromouse.db``),
redireciona as sessões da aplicação para esse banco e oferece fixtures
reutilizáveis pelas demais suítes (Tarefas T6–T9):

- ``client``: ``TestClient`` da API com o banco de teste já conectado;
- ``db_session``: sessão para montar dados diretamente no banco;
- ``labirintos``: semeia os três labirintos padrão (4x4, 8x8, 16x16);
- ``pacote_telemetria``: monta um payload de telemetria válido.
"""
from datetime import datetime, timezone

import pytest
from fastapi.testclient import TestClient
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from sqlalchemy.pool import StaticPool

from app import database
from app.database import Base, get_db
from app.main import app
from app.models.models import DimensaoLabirinto, Labirinto


@pytest.fixture()
def engine():
    """Engine SQLite em memória, isolado por teste.

    ``StaticPool`` mantém a mesma conexão, garantindo que o banco em memória
    persista entre as várias sessões abertas durante um teste.
    """
    eng = create_engine(
        "sqlite://",
        connect_args={"check_same_thread": False},
        poolclass=StaticPool,
    )
    Base.metadata.create_all(bind=eng)
    yield eng
    Base.metadata.drop_all(bind=eng)
    eng.dispose()


@pytest.fixture()
def session_factory(engine):
    return sessionmaker(autocommit=False, autoflush=False, bind=engine)


@pytest.fixture()
def db_session(session_factory):
    session = session_factory()
    try:
        yield session
    finally:
        session.close()


@pytest.fixture()
def client(session_factory, monkeypatch):
    """``TestClient`` com as sessões da aplicação apontando para o banco de teste.

    Sobrescreve ``get_db`` (usado pelas rotas HTTP) e o ``SessionLocal``
    (instanciado diretamente pelo endpoint WebSocket) para o banco isolado.
    """
    def _get_db_override():
        db = session_factory()
        try:
            yield db
        finally:
            db.close()

    app.dependency_overrides[get_db] = _get_db_override
    monkeypatch.setattr(database, "SessionLocal", session_factory)
    monkeypatch.setattr("app.routes.telemetria.SessionLocal", session_factory)

    with TestClient(app) as c:
        yield c

    app.dependency_overrides.clear()


@pytest.fixture()
def labirintos(db_session):
    """Semeia os três labirintos padrão e retorna ``{dimensao: id}``."""
    labs = {
        "4x4": Labirinto(dimensao=DimensaoLabirinto.quatro_x_quatro),
        "8x8": Labirinto(dimensao=DimensaoLabirinto.oito_x_oito),
        "16x16": Labirinto(dimensao=DimensaoLabirinto.dezesseis_x_dezesseis),
    }
    db_session.add_all(list(labs.values()))
    db_session.commit()
    return {dim: lab.id for dim, lab in labs.items()}


@pytest.fixture()
def pacote_telemetria():
    """Devolve um construtor de payload de telemetria válido para os testes."""
    def _build(labirinto_id=None, corrida_id=None, **overrides):
        payload = {
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "posicao_x": 0,
            "posicao_y": 0,
            "nivel_bateria": 100.0,
        }
        if labirinto_id is not None:
            payload["labirinto_id"] = labirinto_id
        if corrida_id is not None:
            payload["corrida_id"] = corrida_id
        payload.update(overrides)
        return payload

    return _build
