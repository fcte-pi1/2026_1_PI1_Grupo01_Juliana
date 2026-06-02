"""Testes da rota POST /telemetria (Tarefa T6).

Cobre os critérios de conclusão:
1. Envio válido de telemetria (cria corrida quando não há corrida_id)
2. Envio com corrida_id existente e inexistente (404)
3. labirinto_id inexistente (404)
4. Payload inválido (erro de validação Pydantic / 422)
5. Evento persistido corretamente no banco de teste
"""

from app.models.models import Corrida, EventoTelemetria


# ---------- 1. Envio válido — cria corrida automaticamente ----------


def test_post_telemetria_cria_corrida_e_persiste_evento(client, labirintos, pacote_telemetria):
    resp = client.post("/telemetria", json=pacote_telemetria(labirinto_id=labirintos["4x4"]))

    assert resp.status_code == 201
    corpo = resp.json()
    assert corpo["posicao_x"] == 0
    assert corpo["nivel_bateria"] == 100.0
    assert corpo["corrida_id"] >= 1


def test_post_telemetria_dois_envios_criam_corridas_distintas(client, labirintos, pacote_telemetria):
    r1 = client.post("/telemetria", json=pacote_telemetria(labirinto_id=labirintos["4x4"]))
    r2 = client.post("/telemetria", json=pacote_telemetria(labirinto_id=labirintos["4x4"]))

    assert r1.status_code == 201
    assert r2.status_code == 201
    assert r1.json()["corrida_id"] != r2.json()["corrida_id"]


def test_post_telemetria_com_velocidade_opcional(client, labirintos, pacote_telemetria):
    payload = pacote_telemetria(labirinto_id=labirintos["8x8"], velocidade=1.5)
    resp = client.post("/telemetria", json=payload)

    assert resp.status_code == 201
    assert resp.json()["velocidade"] == 1.5


# ---------- 2. Envio com corrida_id existente e inexistente ----------


def test_post_telemetria_corrida_existente_reutiliza(client, labirintos, pacote_telemetria):
    r1 = client.post("/telemetria", json=pacote_telemetria(labirinto_id=labirintos["4x4"]))
    corrida_id = r1.json()["corrida_id"]

    r2 = client.post("/telemetria", json=pacote_telemetria(corrida_id=corrida_id))

    assert r2.status_code == 201
    assert r2.json()["corrida_id"] == corrida_id


def test_post_telemetria_corrida_inexistente_retorna_404(client, pacote_telemetria):
    resp = client.post("/telemetria", json=pacote_telemetria(corrida_id=9999))
    assert resp.status_code == 404


# ---------- 3. labirinto_id inexistente ----------


def test_post_telemetria_labirinto_inexistente_retorna_404(client, pacote_telemetria):
    resp = client.post("/telemetria", json=pacote_telemetria(labirinto_id=9999))
    assert resp.status_code == 404


# ---------- 4. Payload inválido — 422 ----------


def test_post_telemetria_sem_corrida_ou_labirinto_retorna_422(client, pacote_telemetria):
    resp = client.post("/telemetria", json=pacote_telemetria())
    assert resp.status_code == 422


def test_post_telemetria_corpo_vazio_retorna_422(client):
    resp = client.post("/telemetria", json={})
    assert resp.status_code == 422


def test_post_telemetria_sem_timestamp_retorna_422(client, labirintos, pacote_telemetria):
    payload = pacote_telemetria(labirinto_id=labirintos["4x4"])
    del payload["timestamp"]
    resp = client.post("/telemetria", json=payload)
    assert resp.status_code == 422


def test_post_telemetria_sem_posicao_retorna_422(client, labirintos, pacote_telemetria):
    payload = pacote_telemetria(labirinto_id=labirintos["4x4"])
    del payload["posicao_x"]
    resp = client.post("/telemetria", json=payload)
    assert resp.status_code == 422


def test_post_telemetria_bateria_acima_de_100_retorna_422(client, labirintos, pacote_telemetria):
    payload = pacote_telemetria(labirinto_id=labirintos["4x4"], nivel_bateria=150.0)
    resp = client.post("/telemetria", json=payload)
    assert resp.status_code == 422


def test_post_telemetria_bateria_negativa_retorna_422(client, labirintos, pacote_telemetria):
    payload = pacote_telemetria(labirinto_id=labirintos["4x4"], nivel_bateria=-1.0)
    resp = client.post("/telemetria", json=payload)
    assert resp.status_code == 422


def test_post_telemetria_posicao_negativa_retorna_422(client, labirintos, pacote_telemetria):
    payload = pacote_telemetria(labirinto_id=labirintos["4x4"], posicao_x=-1)
    resp = client.post("/telemetria", json=payload)
    assert resp.status_code == 422


# ---------- 5. Persistência correta no banco ----------


def test_post_telemetria_evento_persistido_no_banco(client, db_session, labirintos, pacote_telemetria):
    payload = pacote_telemetria(
        labirinto_id=labirintos["16x16"],
        posicao_x=3,
        posicao_y=7,
        nivel_bateria=85.5,
        velocidade=0.42,
    )
    resp = client.post("/telemetria", json=payload)
    assert resp.status_code == 201

    evento_id = resp.json()["id"]
    evento = db_session.get(EventoTelemetria, evento_id)

    assert evento is not None
    assert evento.posicao_x == 3
    assert evento.posicao_y == 7
    assert evento.nivel_bateria == 85.5
    assert evento.velocidade == 0.42


def test_post_telemetria_corrida_criada_em_andamento(client, db_session, labirintos, pacote_telemetria):
    resp = client.post("/telemetria", json=pacote_telemetria(labirinto_id=labirintos["4x4"]))
    corrida_id = resp.json()["corrida_id"]

    corrida = db_session.get(Corrida, corrida_id)

    assert corrida is not None
    assert corrida.resultado.value == "em_andamento"
    assert corrida.labirinto_id == labirintos["4x4"]
