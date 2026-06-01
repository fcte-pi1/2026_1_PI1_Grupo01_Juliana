"""Teste-modelo do endpoint de health (base para a Tarefa T9 / #126)."""


def test_health_retorna_ok(client):
    resp = client.get("/health")
    assert resp.status_code == 200
    assert resp.json() == {"status": "OK"}
