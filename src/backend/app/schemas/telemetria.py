from datetime import datetime
from typing import Optional

from pydantic import BaseModel, ConfigDict, Field, model_validator


class TelemetriaIn(BaseModel):
    """Pacote de telemetria enviado pelo micromouse.

    Deve referenciar uma corrida existente via ``corrida_id`` OU informar
    ``labirinto_id`` para que uma nova corrida em andamento seja aberta
    automaticamente.
    """

    corrida_id: Optional[int] = Field(default=None, ge=1)
    labirinto_id: Optional[int] = Field(default=None, ge=1)

    timestamp: datetime
    posicao_x: int = Field(ge=0)
    posicao_y: int = Field(ge=0)
    nivel_bateria: float = Field(ge=0.0, le=100.0)
    velocidade: Optional[float] = Field(default=None, ge=0.0)
    orientacao: Optional[str] = Field(default=None, max_length=32)
    mensagem: Optional[str] = Field(default=None, max_length=256)

    @model_validator(mode="after")
    def _exige_corrida_ou_labirinto(self) -> "TelemetriaIn":
        if self.corrida_id is None and self.labirinto_id is None:
            raise ValueError("informe corrida_id ou labirinto_id")
        return self


class TelemetriaOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    corrida_id: int
    timestamp: datetime
    posicao_x: int
    posicao_y: int
    nivel_bateria: float
    velocidade: Optional[float] = None
    orientacao: Optional[str] = None
    mensagem: Optional[str] = None
