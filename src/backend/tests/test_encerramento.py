"""Testes do encerramento de corrida (fim de corrida) — issue #16.

Cobre o gatilho/endpoint `POST /corridas/{id}/encerrar`:
1. Encerramento com sucesso e transição de `resultado` (concluida/falha)
2. Cálculo de `duracao` (último menos primeiro timestamp)
3. Cálculo de `velocidade_media` (média dos eventos; nula sem velocidades)
4. Erros: corrida inexistente (404), corrida já encerrada (409), resultado inválido (422)
5. `GET /corridas` reflete os agregados após o encerramento

Estes testes validam a implementação da #16; a suíte ampla de aceitação é a
Tarefa T15 (#145).
"""

from datetime import datetime, timedelta, timezone


BASE = datetime(2026, 5, 10, 12, 0, 0, tzinfo=timezone.utc)


def _criar_corrida_com_eventos(client, pacote_telemetria, labirinto_id, eventos):
    """Abre uma corrida via telemetria e injeta os eventos pedidos.

    `eventos` é uma lista de dicts com overrides (timestamp, velocidade, ...).
    Devolve o `corrida_id` gerado.
    """
    corrida_id = None
    for ev in eventos:
        if corrida_id is None:
            payload = pacote_telemetria(labirinto_id=labirinto_id, **ev)
        else:
            payload = pacote_telemetria(corrida_id=corrida_id, **ev)
        resp = client.post("/telemetria", json=payload)
        assert resp.status_code == 201, resp.text
        corrida_id = resp.json()["corrida_id"]
    return corrida_id


# ---------------------------------------------------------------------------
# Sucesso + agregados
# ---------------------------------------------------------------------------

def test_encerrar_calcula_duracao_velocidade_media_e_resultado(client, labirintos, pacote_telemetria):
    corrida_id = _criar_corrida_com_eventos(
        client, pacote_telemetria, labirintos["4x4"],
        [
            {"timestamp": BASE.isoformat(), "velocidade": 0.4},
            {"timestamp": (BASE + timedelta(seconds=10)).isoformat(), "velocidade": 0.6},
        ],
    )

    resp = client.post(f"/corridas/{corrida_id}/encerrar", json={"resultado": "concluida"})
    assert resp.status_code == 200, resp.text

    dados = resp.json()
    assert dados["resultado"] == "concluida"
    assert dados["duracao"] == 10.0
    assert dados["velocidade_media"] == 0.5


def test_encerrar_com_falha_define_resultado_falha(client, labirintos, pacote_telemetria):
    corrida_id = _criar_corrida_com_eventos(
        client, pacote_telemetria, labirintos["8x8"],
        [{"timestamp": BASE.isoformat(), "velocidade": 1.0}],
    )

    resp = client.post(f"/corridas/{corrida_id}/encerrar", json={"resultado": "falha"})
    assert resp.status_code == 200, resp.text
    assert resp.json()["resultado"] == "falha"


def test_encerrar_sem_corpo_usa_concluida_por_padrao(client, labirintos, pacote_telemetria):
    corrida_id = _criar_corrida_com_eventos(
        client, pacote_telemetria, labirintos["4x4"],
        [{"timestamp": BASE.isoformat()}],
    )

    resp = client.post(f"/corridas/{corrida_id}/encerrar")
    assert resp.status_code == 200, resp.text
    assert resp.json()["resultado"] == "concluida"


def test_encerrar_sem_velocidades_resulta_em_media_nula(client, labirintos, pacote_telemetria):
    corrida_id = _criar_corrida_com_eventos(
        client, pacote_telemetria, labirintos["4x4"],
        [
            {"timestamp": BASE.isoformat()},
            {"timestamp": (BASE + timedelta(seconds=5)).isoformat()},
        ],
    )

    resp = client.post(f"/corridas/{corrida_id}/encerrar", json={"resultado": "falha"})
    assert resp.status_code == 200, resp.text

    dados = resp.json()
    assert dados["velocidade_media"] is None
    assert dados["duracao"] == 5.0


def test_encerrar_corrida_com_evento_unico_tem_duracao_zero(client, labirintos, pacote_telemetria):
    corrida_id = _criar_corrida_com_eventos(
        client, pacote_telemetria, labirintos["4x4"],
        [{"timestamp": BASE.isoformat(), "velocidade": 0.9}],
    )

    resp = client.post(f"/corridas/{corrida_id}/encerrar", json={"resultado": "concluida"})
    assert resp.status_code == 200, resp.text

    dados = resp.json()
    assert dados["duracao"] == 0.0
    assert dados["velocidade_media"] == 0.9


# ---------------------------------------------------------------------------
# Erros
# ---------------------------------------------------------------------------

def test_encerrar_corrida_inexistente_retorna_404(client):
    resp = client.post("/corridas/999/encerrar", json={"resultado": "concluida"})
    assert resp.status_code == 404


def test_encerrar_corrida_ja_encerrada_retorna_409(client, labirintos, pacote_telemetria):
    corrida_id = _criar_corrida_com_eventos(
        client, pacote_telemetria, labirintos["4x4"],
        [{"timestamp": BASE.isoformat(), "velocidade": 0.5}],
    )

    primeira = client.post(f"/corridas/{corrida_id}/encerrar", json={"resultado": "concluida"})
    assert primeira.status_code == 200

    segunda = client.post(f"/corridas/{corrida_id}/encerrar", json={"resultado": "concluida"})
    assert segunda.status_code == 409


def test_encerrar_com_resultado_em_andamento_retorna_422(client, labirintos, pacote_telemetria):
    corrida_id = _criar_corrida_com_eventos(
        client, pacote_telemetria, labirintos["4x4"],
        [{"timestamp": BASE.isoformat()}],
    )

    resp = client.post(f"/corridas/{corrida_id}/encerrar", json={"resultado": "em_andamento"})
    assert resp.status_code == 422


# ---------------------------------------------------------------------------
# GET /corridas reflete os agregados após o encerramento
# ---------------------------------------------------------------------------

def test_get_corridas_reflete_agregados_apos_encerramento(client, labirintos, pacote_telemetria):
    corrida_id = _criar_corrida_com_eventos(
        client, pacote_telemetria, labirintos["8x8"],
        [
            {"timestamp": BASE.isoformat(), "velocidade": 1.0},
            {"timestamp": (BASE + timedelta(seconds=20)).isoformat(), "velocidade": 2.0},
        ],
    )

    antes = next(c for c in client.get("/corridas").json() if c["id"] == corrida_id)
    assert antes["resultado"] == "em_andamento"
    assert antes["duracao"] is None
    assert antes["velocidade_media"] is None

    client.post(f"/corridas/{corrida_id}/encerrar", json={"resultado": "concluida"})

    depois = next(c for c in client.get("/corridas").json() if c["id"] == corrida_id)
    assert depois["resultado"] == "concluida"
    assert depois["duracao"] == 20.0
    assert depois["velocidade_media"] == 1.5
