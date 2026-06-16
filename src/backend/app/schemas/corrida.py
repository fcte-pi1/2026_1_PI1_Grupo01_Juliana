from pydantic import BaseModel, ConfigDict, field_validator
from datetime import datetime
from typing import Optional
from app.models.models import ResultadoCorrida


class CorridaResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    labirinto_id: int
    tamanho: str  # ex: "4x4", "8x8", "16x16"
    data: Optional[datetime]
    duracao: Optional[float]
    velocidade_media: Optional[float]
    resultado: ResultadoCorrida


class CorridaEncerrarIn(BaseModel):
    """Corpo do encerramento de corrida.

    Define o resultado final; os agregados (duração e velocidade média) são
    calculados pelo backend a partir dos eventos de telemetria.
    """

    resultado: ResultadoCorrida = ResultadoCorrida.concluida

    @field_validator("resultado")
    @classmethod
    def _nao_pode_encerrar_em_andamento(cls, valor: ResultadoCorrida) -> ResultadoCorrida:
        if valor == ResultadoCorrida.em_andamento:
            raise ValueError("resultado do encerramento deve ser 'concluida' ou 'falha'")
        return valor
