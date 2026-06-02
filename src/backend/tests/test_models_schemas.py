"""Testes unitários de models (SQLAlchemy) e schemas (Pydantic) — Tarefa T8.

Cobre os critérios de conclusão:
1. Criação e relacionamento de ``Corrida``, ``EventoTelemetria`` e ``Labirinto``;
2. Enum ``ResultadoCorrida`` e valores padrão;
3. Validação do schema ``TelemetriaIn`` (campos obrigatórios, tipos, limites);
4. Serialização do ``TelemetriaOut``;
5. Casos de borda (campos opcionais, valores inválidos).

Usa as fixtures de ``conftest.py`` (Tarefa T3): ``db_session`` para o banco
SQLite em memória e ``pacote_telemetria`` para montar payloads válidos.
"""

from datetime import datetime, timezone

import pytest
from pydantic import ValidationError

from app.models.models import (
    Corrida,
    DimensaoLabirinto,
    EventoTelemetria,
    Labirinto,
    ResultadoCorrida,
)
from app.schemas.telemetria import TelemetriaIn, TelemetriaOut


# =========================================================================
# 1. Models — criação e relacionamento
# =========================================================================


def test_cria_labirinto(db_session):
    lab = Labirinto(dimensao=DimensaoLabirinto.oito_x_oito)
    db_session.add(lab)
    db_session.commit()

    assert lab.id is not None
    assert lab.dimensao == DimensaoLabirinto.oito_x_oito
    assert lab.corridas == []


def test_cria_corrida_ligada_ao_labirinto(db_session):
    lab = Labirinto(dimensao=DimensaoLabirinto.quatro_x_quatro)
    corrida = Corrida(labirinto=lab)
    db_session.add(corrida)
    db_session.commit()

    assert corrida.id is not None
    assert corrida.labirinto_id == lab.id
    # relacionamento bidirecional
    assert corrida.labirinto is lab
    assert corrida in lab.corridas


def test_cria_evento_telemetria_ligado_a_corrida(db_session):
    lab = Labirinto(dimensao=DimensaoLabirinto.dezesseis_x_dezesseis)
    corrida = Corrida(labirinto=lab)
    evento = EventoTelemetria(
        corrida=corrida,
        timestamp=datetime.now(timezone.utc),
        posicao_x=2,
        posicao_y=5,
        nivel_bateria=90.0,
    )
    db_session.add(evento)
    db_session.commit()

    assert evento.id is not None
    assert evento.corrida_id == corrida.id
    assert evento in corrida.telemetria


def test_remover_corrida_remove_eventos_em_cascata(db_session):
    """``cascade="all, delete-orphan"`` deve apagar a telemetria junto."""
    lab = Labirinto(dimensao=DimensaoLabirinto.quatro_x_quatro)
    corrida = Corrida(labirinto=lab)
    corrida.telemetria.append(
        EventoTelemetria(
            timestamp=datetime.now(timezone.utc),
            posicao_x=0,
            posicao_y=0,
            nivel_bateria=100.0,
        )
    )
    db_session.add(corrida)
    db_session.commit()

    db_session.delete(corrida)
    db_session.commit()

    assert db_session.query(EventoTelemetria).count() == 0


# =========================================================================
# 2. Enum ResultadoCorrida e valores padrão
# =========================================================================


def test_resultado_corrida_valores_do_enum():
    assert ResultadoCorrida.concluida.value == "concluida"
    assert ResultadoCorrida.falha.value == "falha"
    assert ResultadoCorrida.em_andamento.value == "em_andamento"


def test_dimensao_labirinto_valores_do_enum():
    assert DimensaoLabirinto.quatro_x_quatro.value == "4x4"
    assert DimensaoLabirinto.oito_x_oito.value == "8x8"
    assert DimensaoLabirinto.dezesseis_x_dezesseis.value == "16x16"


def test_corrida_resultado_padrao_em_andamento(db_session):
    lab = Labirinto(dimensao=DimensaoLabirinto.quatro_x_quatro)
    corrida = Corrida(labirinto=lab)
    db_session.add(corrida)
    db_session.commit()

    assert corrida.resultado == ResultadoCorrida.em_andamento


def test_corrida_data_preenchida_por_padrao(db_session):
    lab = Labirinto(dimensao=DimensaoLabirinto.quatro_x_quatro)
    corrida = Corrida(labirinto=lab)
    db_session.add(corrida)
    db_session.commit()

    assert corrida.data is not None


def test_corrida_aceita_outro_resultado(db_session):
    lab = Labirinto(dimensao=DimensaoLabirinto.oito_x_oito)
    corrida = Corrida(labirinto=lab, resultado=ResultadoCorrida.concluida)
    db_session.add(corrida)
    db_session.commit()

    assert corrida.resultado == ResultadoCorrida.concluida


def test_campos_opcionais_da_corrida_iniciam_nulos(db_session):
    lab = Labirinto(dimensao=DimensaoLabirinto.quatro_x_quatro)
    corrida = Corrida(labirinto=lab)
    db_session.add(corrida)
    db_session.commit()

    assert corrida.duracao is None
    assert corrida.velocidade_media is None


# =========================================================================
# 3. Schema TelemetriaIn — validação
# =========================================================================


def test_telemetria_in_valido_com_labirinto():
    pacote = TelemetriaIn(
        labirinto_id=1,
        timestamp=datetime.now(timezone.utc),
        posicao_x=0,
        posicao_y=0,
        nivel_bateria=100.0,
    )
    assert pacote.labirinto_id == 1
    assert pacote.corrida_id is None
    assert pacote.velocidade is None  # campo opcional


def test_telemetria_in_valido_com_corrida():
    pacote = TelemetriaIn(
        corrida_id=5,
        timestamp=datetime.now(timezone.utc),
        posicao_x=1,
        posicao_y=2,
        nivel_bateria=50.0,
        velocidade=1.5,
    )
    assert pacote.corrida_id == 5
    assert pacote.velocidade == 1.5


def test_telemetria_in_exige_corrida_ou_labirinto():
    with pytest.raises(ValidationError, match="corrida_id ou labirinto_id"):
        TelemetriaIn(
            timestamp=datetime.now(timezone.utc),
            posicao_x=0,
            posicao_y=0,
            nivel_bateria=100.0,
        )


@pytest.mark.parametrize("campo", ["timestamp", "posicao_x", "posicao_y", "nivel_bateria"])
def test_telemetria_in_campos_obrigatorios(campo):
    dados = {
        "labirinto_id": 1,
        "timestamp": datetime.now(timezone.utc),
        "posicao_x": 0,
        "posicao_y": 0,
        "nivel_bateria": 100.0,
    }
    del dados[campo]
    with pytest.raises(ValidationError):
        TelemetriaIn(**dados)


def test_telemetria_in_converte_timestamp_de_string():
    """Pydantic deve parsear ISO 8601 para ``datetime``."""
    pacote = TelemetriaIn(
        labirinto_id=1,
        timestamp="2026-06-02T12:00:00+00:00",
        posicao_x=0,
        posicao_y=0,
        nivel_bateria=100.0,
    )
    assert isinstance(pacote.timestamp, datetime)


# ---------- Limites e tipos inválidos ----------


@pytest.mark.parametrize("bateria", [-0.1, 100.1, 150.0])
def test_telemetria_in_bateria_fora_do_limite(bateria):
    with pytest.raises(ValidationError):
        TelemetriaIn(
            labirinto_id=1,
            timestamp=datetime.now(timezone.utc),
            posicao_x=0,
            posicao_y=0,
            nivel_bateria=bateria,
        )


@pytest.mark.parametrize("bateria", [0.0, 50.0, 100.0])
def test_telemetria_in_bateria_nos_limites_validos(bateria):
    pacote = TelemetriaIn(
        labirinto_id=1,
        timestamp=datetime.now(timezone.utc),
        posicao_x=0,
        posicao_y=0,
        nivel_bateria=bateria,
    )
    assert pacote.nivel_bateria == bateria


@pytest.mark.parametrize("campo", ["posicao_x", "posicao_y"])
def test_telemetria_in_posicao_negativa_invalida(campo):
    dados = {
        "labirinto_id": 1,
        "timestamp": datetime.now(timezone.utc),
        "posicao_x": 0,
        "posicao_y": 0,
        "nivel_bateria": 100.0,
        campo: -1,
    }
    with pytest.raises(ValidationError):
        TelemetriaIn(**dados)


def test_telemetria_in_velocidade_negativa_invalida():
    with pytest.raises(ValidationError):
        TelemetriaIn(
            labirinto_id=1,
            timestamp=datetime.now(timezone.utc),
            posicao_x=0,
            posicao_y=0,
            nivel_bateria=100.0,
            velocidade=-0.5,
        )


@pytest.mark.parametrize("campo", ["corrida_id", "labirinto_id"])
def test_telemetria_in_ids_devem_ser_positivos(campo):
    with pytest.raises(ValidationError):
        TelemetriaIn(
            **{campo: 0},
            timestamp=datetime.now(timezone.utc),
            posicao_x=0,
            posicao_y=0,
            nivel_bateria=100.0,
        )


def test_telemetria_in_tipo_invalido_em_posicao():
    with pytest.raises(ValidationError):
        TelemetriaIn(
            labirinto_id=1,
            timestamp=datetime.now(timezone.utc),
            posicao_x="abc",
            posicao_y=0,
            nivel_bateria=100.0,
        )


# =========================================================================
# 4. Schema TelemetriaOut — serialização
# =========================================================================


def test_telemetria_out_serializa_a_partir_do_model(db_session):
    """``from_attributes`` deve montar o schema direto do objeto ORM."""
    lab = Labirinto(dimensao=DimensaoLabirinto.quatro_x_quatro)
    corrida = Corrida(labirinto=lab)
    evento = EventoTelemetria(
        corrida=corrida,
        timestamp=datetime.now(timezone.utc),
        posicao_x=3,
        posicao_y=4,
        nivel_bateria=75.5,
        velocidade=0.9,
    )
    db_session.add(evento)
    db_session.commit()

    saida = TelemetriaOut.model_validate(evento)

    assert saida.id == evento.id
    assert saida.corrida_id == corrida.id
    assert saida.posicao_x == 3
    assert saida.posicao_y == 4
    assert saida.nivel_bateria == 75.5
    assert saida.velocidade == 0.9


def test_telemetria_out_velocidade_opcional_ausente():
    saida = TelemetriaOut(
        id=1,
        corrida_id=1,
        timestamp=datetime.now(timezone.utc),
        posicao_x=0,
        posicao_y=0,
        nivel_bateria=100.0,
    )
    assert saida.velocidade is None


def test_telemetria_out_gera_dict_com_todos_os_campos():
    saida = TelemetriaOut(
        id=7,
        corrida_id=2,
        timestamp=datetime(2026, 6, 2, 12, 0, tzinfo=timezone.utc),
        posicao_x=1,
        posicao_y=1,
        nivel_bateria=80.0,
        velocidade=2.0,
    )
    dados = saida.model_dump()

    assert dados == {
        "id": 7,
        "corrida_id": 2,
        "timestamp": datetime(2026, 6, 2, 12, 0, tzinfo=timezone.utc),
        "posicao_x": 1,
        "posicao_y": 1,
        "nivel_bateria": 80.0,
        "velocidade": 2.0,
    }
