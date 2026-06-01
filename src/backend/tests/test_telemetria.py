"""Testes-modelo da rota POST /telemetria.

Servem de base para a Tarefa T6 (#123). Mostram o uso das fixtures
``client``, ``labirintos`` e ``pacote_telemetria`` do ``conftest.py``.
A suíte completa (mais casos de borda) é responsabilidade da T6.
"""


def test_post_telemetria_cria_corrida_e_persiste_evento(client, labirintos, pacote_telemetria):
    resp = client.post("/telemetria", json=pacote_telemetria(labirinto_id=labirintos["4x4"]))

    assert resp.status_code == 201
    corpo = resp.json()
    assert corpo["posicao_x"] == 0
    assert corpo["nivel_bateria"] == 100.0
    assert corpo["corrida_id"] >= 1


def test_post_telemetria_sem_corrida_ou_labirinto_retorna_422(client, pacote_telemetria):
    # Sem corrida_id nem labirinto_id: o validador do schema deve rejeitar.
    resp = client.post("/telemetria", json=pacote_telemetria())
    assert resp.status_code == 422


def test_post_telemetria_labirinto_inexistente_retorna_404(client, pacote_telemetria):
    resp = client.post("/telemetria", json=pacote_telemetria(labirinto_id=9999))
    assert resp.status_code == 404
