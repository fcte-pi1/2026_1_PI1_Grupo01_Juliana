from fastapi import APIRouter, Depends, HTTPException, Query, status
from sqlalchemy.orm import Session, joinedload
from typing import Optional, List

from app.database import get_db
from app.models.models import Corrida, EventoTelemetria, Labirinto, DimensaoLabirinto, ResultadoCorrida
from app.schemas.corrida import CorridaResponse, CorridaEncerrarIn

router = APIRouter(prefix="/corridas", tags=["corridas"])


def _corrida_para_resposta(corrida: Corrida) -> CorridaResponse:
    return CorridaResponse(
        id=corrida.id,
        labirinto_id=corrida.labirinto_id,
        tamanho=corrida.labirinto.dimensao.value,
        data=corrida.data,
        duracao=corrida.duracao,
        velocidade_media=corrida.velocidade_media,
        resultado=corrida.resultado,
    )


def _calcular_duracao(eventos: List[EventoTelemetria]) -> float:
    """Duração da corrida em segundos: último menos primeiro timestamp."""
    if len(eventos) < 2:
        return 0.0
    return (eventos[-1].timestamp - eventos[0].timestamp).total_seconds()


def _calcular_velocidade_media(eventos: List[EventoTelemetria]) -> Optional[float]:
    """Média das velocidades informadas nos eventos (ignora os nulos)."""
    velocidades = [e.velocidade for e in eventos if e.velocidade is not None]
    if not velocidades:
        return None
    return sum(velocidades) / len(velocidades)


@router.get("", response_model=List[CorridaResponse])
def listar_corridas(
    tamanho: Optional[DimensaoLabirinto] = Query(
        default=None,
        description="Filtrar por dimensão do labirinto: 4x4, 8x8 ou 16x16",
    ),
    db: Session = Depends(get_db),
):
    """
    Retorna o histórico de corridas.
    Use o parâmetro `tamanho` para filtrar por dimensão do labirinto (ex: ?tamanho=4x4).
    Sem filtro, retorna todas as corridas.
    """
    query = db.query(Corrida).options(joinedload(Corrida.labirinto))

    if tamanho is not None:
        query = query.join(Labirinto).filter(Labirinto.dimensao == tamanho)

    corridas = query.order_by(Corrida.data.desc()).all()

    return [_corrida_para_resposta(c) for c in corridas]


@router.post("/{corrida_id}/encerrar", response_model=CorridaResponse)
def encerrar_corrida(
    corrida_id: int,
    payload: CorridaEncerrarIn = CorridaEncerrarIn(),
    db: Session = Depends(get_db),
):
    """Encerra uma corrida em andamento (gatilho de fim de corrida).

    Calcula e persiste os dados agregados a partir dos eventos de telemetria
    (`duracao` = último menos primeiro timestamp; `velocidade_media` = média das
    velocidades) e define o `resultado` final (`concluida` ou `falha`).
    """
    corrida = (
        db.query(Corrida)
        .options(joinedload(Corrida.labirinto))
        .filter(Corrida.id == corrida_id)
        .first()
    )
    if corrida is None:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"corrida_id {corrida_id} não encontrada",
        )
    if corrida.resultado != ResultadoCorrida.em_andamento:
        raise HTTPException(
            status_code=status.HTTP_409_CONFLICT,
            detail=f"corrida {corrida_id} já está encerrada",
        )

    eventos = (
        db.query(EventoTelemetria)
        .filter(EventoTelemetria.corrida_id == corrida_id)
        .order_by(EventoTelemetria.timestamp)
        .all()
    )

    corrida.duracao = _calcular_duracao(eventos)
    corrida.velocidade_media = _calcular_velocidade_media(eventos)
    corrida.resultado = payload.resultado

    db.commit()
    db.refresh(corrida)

    return _corrida_para_resposta(corrida)
