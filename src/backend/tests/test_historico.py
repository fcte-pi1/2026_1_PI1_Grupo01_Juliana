"""Testes de integração das rotas de consulta/histórico (Tarefa T9).

Cobre os critérios de conclusão da issue:
1. Listagem de corridas com dados semeados
2. Filtro por labirinto (?tamanho=4x4, 8x8, 16x16)
3. Filtro "todos" (sem parâmetro)
4. Resposta para histórico vazio
5. Valor inválido no filtro (422)
6. Resultados variados (concluida, falha, em_andamento)

Nota: o teste de /health já está em test_health.py — não duplicado aqui.
"""

from app.models.models import Corrida, ResultadoCorrida


# ---------------------------------------------------------------------------
# Helper
# ---------------------------------------------------------------------------

def _semear_corrida(db_session, labirinto_id, resultado=ResultadoCorrida.concluida,
                    duracao=30.0, velocidade_media=0.8):
    """Insere uma corrida diretamente no banco e devolve o objeto persistido."""
    corrida = Corrida(
        labirinto_id=labirinto_id,
        resultado=resultado,
        duracao=duracao,
        velocidade_media=velocidade_media,
    )
    db_session.add(corrida)
    db_session.commit()
    db_session.refresh(corrida)
    return corrida


# ---------------------------------------------------------------------------
# Histórico vazio
# ---------------------------------------------------------------------------

def test_listar_corridas_sem_dados_retorna_lista_vazia(client):
    resp = client.get("/corridas")
    assert resp.status_code == 200
    assert resp.json() == []


# ---------------------------------------------------------------------------
# Listagem com dados semeados
# ---------------------------------------------------------------------------

def test_listar_corridas_retorna_todas(client, db_session, labirintos):
    _semear_corrida(db_session, labirintos["4x4"])
    _semear_corrida(db_session, labirintos["8x8"])
    _semear_corrida(db_session, labirintos["16x16"])

    resp = client.get("/corridas")
    assert resp.status_code == 200
    assert len(resp.json()) == 3


def test_listar_corridas_contem_campos_do_schema(client, db_session, labirintos):
    _semear_corrida(db_session, labirintos["4x4"], duracao=42.5, velocidade_media=1.2)

    corrida = client.get("/corridas").json()[0]

    assert "id" in corrida
    assert "labirinto_id" in corrida
    assert "tamanho" in corrida
    assert "data" in corrida
    assert "duracao" in corrida
    assert "velocidade_media" in corrida
    assert "resultado" in corrida


def test_listar_corridas_valores_persistidos_corretos(client, db_session, labirintos):
    _semear_corrida(db_session, labirintos["4x4"], duracao=42.5, velocidade_media=1.2)

    corrida = client.get("/corridas").json()[0]

    assert corrida["tamanho"] == "4x4"
    assert corrida["duracao"] == 42.5
    assert corrida["velocidade_media"] == 1.2
    assert corrida["resultado"] == "concluida"


def test_listar_corridas_ordenadas_da_mais_recente(client, db_session, labirintos):
    """A rota ordena por data DESC — corrida com data mais recente vem primeiro."""
    from datetime import datetime, timezone, timedelta

    agora = datetime.now(timezone.utc)

    c1 = Corrida(labirinto_id=labirintos["4x4"], resultado=ResultadoCorrida.concluida,
                 data=agora - timedelta(seconds=10))
    c2 = Corrida(labirinto_id=labirintos["8x8"], resultado=ResultadoCorrida.concluida,
                 data=agora)
    db_session.add_all([c1, c2])
    db_session.commit()
    db_session.refresh(c1)
    db_session.refresh(c2)

    ids = [c["id"] for c in client.get("/corridas").json()]

    assert ids.index(c2.id) < ids.index(c1.id)


# ---------------------------------------------------------------------------
# Filtro por tamanho
# ---------------------------------------------------------------------------

def test_filtro_tamanho_retorna_apenas_corridas_do_labirinto(client, db_session, labirintos):
    _semear_corrida(db_session, labirintos["4x4"])
    _semear_corrida(db_session, labirintos["4x4"])
    _semear_corrida(db_session, labirintos["8x8"])

    resp = client.get("/corridas?tamanho=4x4")
    assert resp.status_code == 200

    dados = resp.json()
    assert len(dados) == 2
    assert all(c["tamanho"] == "4x4" for c in dados)


def test_filtro_tamanho_8x8_exclui_outros(client, db_session, labirintos):
    _semear_corrida(db_session, labirintos["4x4"])
    _semear_corrida(db_session, labirintos["8x8"])
    _semear_corrida(db_session, labirintos["16x16"])

    resp = client.get("/corridas?tamanho=8x8")
    assert resp.status_code == 200

    dados = resp.json()
    assert len(dados) == 1
    assert dados[0]["tamanho"] == "8x8"


def test_filtro_tamanho_sem_correspondencia_retorna_vazio(client, db_session, labirintos):
    _semear_corrida(db_session, labirintos["4x4"])

    resp = client.get("/corridas?tamanho=16x16")
    assert resp.status_code == 200
    assert resp.json() == []


def test_sem_filtro_retorna_todos_os_tamanhos(client, db_session, labirintos):
    _semear_corrida(db_session, labirintos["4x4"])
    _semear_corrida(db_session, labirintos["8x8"])
    _semear_corrida(db_session, labirintos["16x16"])

    tamanhos = {c["tamanho"] for c in client.get("/corridas").json()}

    assert tamanhos == {"4x4", "8x8", "16x16"}


def test_filtro_tamanho_invalido_retorna_422(client):
    resp = client.get("/corridas?tamanho=32x32")
    assert resp.status_code == 422


# ---------------------------------------------------------------------------
# Resultados variados
# ---------------------------------------------------------------------------

def test_corrida_em_andamento_aparece_no_historico(client, db_session, labirintos):
    _semear_corrida(db_session, labirintos["4x4"], resultado=ResultadoCorrida.em_andamento)

    corrida = client.get("/corridas").json()[0]

    assert corrida["resultado"] == "em_andamento"


def test_corrida_falha_aparece_no_historico(client, db_session, labirintos):
    _semear_corrida(db_session, labirintos["8x8"], resultado=ResultadoCorrida.falha)

    corrida = client.get("/corridas").json()[0]

    assert corrida["resultado"] == "falha"

def test_corrida_criada_via_telemetria_aparece_no_historico(client, labirintos, pacote_telemetria):
    resp_post = client.post("/telemetria", json=pacote_telemetria(labirinto_id=labirintos["4x4"]))
    assert resp_post.status_code == 201
    corrida_id = resp_post.json()["corrida_id"]

    resp_get = client.get("/corridas")
    assert resp_get.status_code == 200

    ids = [c["id"] for c in resp_get.json()]
    assert corrida_id in ids