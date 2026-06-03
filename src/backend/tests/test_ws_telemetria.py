"""Testes do WebSocket de telemetria e do ConnectionManager — Tarefa T7 (#124).

Cobre os critérios de conclusão:
1. Conexão e desconexão de clientes no ``ConnectionManager``;
2. ``broadcast`` para múltiplos clientes e exclusão via ``exceto``;
3. Remoção de conexões mortas durante o broadcast;
4. Repasse, via ``TestClient``/WebSocket, da telemetria recebida aos conectados;
5. Cenário de cliente que cai durante o envio.

As partes unitárias do ``ConnectionManager`` (assíncrono) são exercitadas com
um dublê de WebSocket e ``asyncio.run`` — sem depender de ``pytest-asyncio``.
A parte de integração usa o ``TestClient`` (fixtures de ``conftest.py`` / T3).
"""

import asyncio

import pytest

from app.routes.telemetria import ConnectionManager, manager


# --- Dublê de WebSocket -----------------------------------------------------
class FakeWS:
    """WebSocket assíncrono falso para testar o ConnectionManager.

    ``falha=True`` faz ``send_json`` levantar exceção, simulando uma conexão
    morta / cliente que caiu durante o envio.
    """

    def __init__(self, falha: bool = False) -> None:
        self.aceito = False
        self.enviados: list = []
        self.falha = falha

    async def accept(self) -> None:
        self.aceito = True

    async def send_json(self, data) -> None:
        if self.falha:
            raise RuntimeError("conexão morta")
        self.enviados.append(data)


@pytest.fixture(autouse=True)
def _limpa_manager_global():
    """Garante que o manager singleton começa e termina vazio em cada teste."""
    manager._conexoes.clear()
    yield
    manager._conexoes.clear()


# ===========================================================================
# 1. Conexão e desconexão
# ===========================================================================


def test_conectar_aceita_e_registra():
    cm = ConnectionManager()
    ws = FakeWS()

    asyncio.run(cm.conectar(ws))

    assert ws.aceito is True
    assert ws in cm._conexoes


def test_desconectar_remove():
    cm = ConnectionManager()
    ws = FakeWS()
    asyncio.run(cm.conectar(ws))

    cm.desconectar(ws)

    assert ws not in cm._conexoes


def test_desconectar_conexao_inexistente_e_seguro():
    cm = ConnectionManager()
    # Não deve levantar mesmo que o ws nunca tenha sido registrado.
    cm.desconectar(FakeWS())
    assert cm._conexoes == []


# ===========================================================================
# 2. Broadcast para múltiplos clientes e exclusão via `exceto`
# ===========================================================================


def test_broadcast_envia_para_todos_os_conectados():
    cm = ConnectionManager()
    a, b, c = FakeWS(), FakeWS(), FakeWS()
    for ws in (a, b, c):
        asyncio.run(cm.conectar(ws))

    payload = {"id": 1, "posicao_x": 0, "posicao_y": 0}
    asyncio.run(cm.broadcast(payload))

    assert a.enviados == [payload]
    assert b.enviados == [payload]
    assert c.enviados == [payload]


def test_broadcast_exceto_pula_o_remetente():
    cm = ConnectionManager()
    remetente, outro = FakeWS(), FakeWS()
    asyncio.run(cm.conectar(remetente))
    asyncio.run(cm.conectar(outro))

    payload = {"id": 9}
    asyncio.run(cm.broadcast(payload, exceto=remetente))

    assert remetente.enviados == []  # não recebe o próprio envio
    assert outro.enviados == [payload]


# ===========================================================================
# 3. Remoção de conexões mortas durante o broadcast
# ===========================================================================


def test_broadcast_remove_conexao_morta():
    cm = ConnectionManager()
    vivo, morto = FakeWS(), FakeWS(falha=True)
    asyncio.run(cm.conectar(vivo))
    asyncio.run(cm.conectar(morto))

    asyncio.run(cm.broadcast({"id": 1}))

    # O cliente que falhou é removido; o saudável permanece e recebe os dados.
    assert morto not in cm._conexoes
    assert vivo in cm._conexoes
    assert vivo.enviados == [{"id": 1}]


# ===========================================================================
# 5. Cliente que cai durante o envio (não derruba os demais)
# ===========================================================================


def test_broadcast_cliente_cai_no_meio_nao_afeta_os_outros():
    cm = ConnectionManager()
    a, caido, c = FakeWS(), FakeWS(falha=True), FakeWS()
    for ws in (a, caido, c):
        asyncio.run(cm.conectar(ws))

    asyncio.run(cm.broadcast({"ok": True}))

    # a e c recebem normalmente; o que caiu é descartado.
    assert a.enviados == [{"ok": True}]
    assert c.enviados == [{"ok": True}]
    assert caido not in cm._conexoes
    assert set(cm._conexoes) == {a, c}


def test_broadcast_sem_conexoes_nao_quebra():
    cm = ConnectionManager()
    # Não deve levantar exceção ao transmitir sem ninguém conectado.
    asyncio.run(cm.broadcast({"x": 1}))
    assert cm._conexoes == []


# ===========================================================================
# 4. Integração via TestClient/WebSocket
# ===========================================================================


def test_ws_repassa_telemetria_para_conectados(client, labirintos, pacote_telemetria):
    payload = pacote_telemetria(labirinto_id=labirintos["4x4"], posicao_x=2, posicao_y=3)

    with client.websocket_connect("/ws/telemetria") as ws_a, \
         client.websocket_connect("/ws/telemetria") as ws_b:
        ws_a.send_json(payload)

        recebido_a = ws_a.receive_json()
        recebido_b = ws_b.receive_json()

    # Ambos os clientes recebem o mesmo evento serializado (TelemetriaOut).
    assert recebido_a["posicao_x"] == 2
    assert recebido_a["posicao_y"] == 3
    assert recebido_a["corrida_id"] >= 1
    assert recebido_b == recebido_a


def test_ws_payload_invalido_retorna_erro_sem_derrubar(client):
    with client.websocket_connect("/ws/telemetria") as ws:
        ws.send_json({})  # faltam campos obrigatórios -> ValidationError
        resposta = ws.receive_json()

    assert resposta["erro"] == "payload inválido"
    assert "detalhes" in resposta


def test_ws_labirinto_inexistente_retorna_404(client, pacote_telemetria):
    with client.websocket_connect("/ws/telemetria") as ws:
        ws.send_json(pacote_telemetria(labirinto_id=9999))
        resposta = ws.receive_json()

    assert resposta["status"] == 404
    assert "erro" in resposta


def test_ws_desconecta_remove_do_manager(client, labirintos, pacote_telemetria):
    with client.websocket_connect("/ws/telemetria") as ws:
        ws.send_json(pacote_telemetria(labirinto_id=labirintos["8x8"]))
        ws.receive_json()
        assert len(manager._conexoes) == 1

    # Ao sair do contexto, o cliente desconecta e é removido do manager.
    assert manager._conexoes == []
