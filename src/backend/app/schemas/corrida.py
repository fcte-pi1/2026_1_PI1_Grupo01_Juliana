from pydantic import BaseModel, ConfigDict
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
