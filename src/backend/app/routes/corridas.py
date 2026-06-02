from fastapi import APIRouter, Depends, Query
from sqlalchemy.orm import Session, joinedload
from typing import Optional, List

from app.database import get_db
from app.models.models import Corrida, Labirinto, DimensaoLabirinto
from app.schemas.corrida import CorridaResponse

router = APIRouter(prefix="/corridas", tags=["corridas"])


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

    return [
        CorridaResponse(
            id=c.id,
            labirinto_id=c.labirinto_id,
            tamanho=c.labirinto.dimensao.value,
            data=c.data,
            duracao=c.duracao,
            velocidade_media=c.velocidade_media,
            resultado=c.resultado,
        )
        for c in corridas
    ]