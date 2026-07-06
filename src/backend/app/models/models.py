from sqlalchemy import Column, Integer, String, Float, DateTime, ForeignKey, Enum
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func
import enum
from app.database import Base

class DimensaoLabirinto(str, enum.Enum):
    quatro_x_quatro    = "4x4"
    oito_x_oito        = "8x8"
    dezesseis_x_dezesseis = "16x16"

class ResultadoCorrida(str, enum.Enum):
    concluida = "concluida"
    falha = "falha"
    em_andamento = "em_andamento"

class Labirinto(Base):
    __tablename__ = "labirintos"

    id       = Column(Integer, primary_key=True, index=True)
    dimensao = Column(Enum(DimensaoLabirinto), nullable=False)

    corridas = relationship("Corrida", back_populates="labirinto")

class Corrida(Base):
    __tablename__ = "corridas"

    id = Column(Integer, primary_key=True, index=True)
    labirinto_id = Column(Integer, ForeignKey("labirintos.id"), nullable=False)
    data = Column(DateTime(timezone=True), server_default=func.now())
    duracao = Column(Float, nullable=True)
    velocidade_media = Column(Float, nullable=True)
    resultado = Column(Enum(ResultadoCorrida), nullable=False, default=ResultadoCorrida.em_andamento)

    labirinto = relationship("Labirinto", back_populates="corridas")
    telemetria = relationship("EventoTelemetria", back_populates="corrida", cascade="all, delete-orphan")

class EventoTelemetria(Base):
    __tablename__ = "eventos_telemetria"

    id = Column(Integer, primary_key=True, index=True)
    corrida_id = Column(Integer, ForeignKey("corridas.id"), nullable=False)
    timestamp = Column(DateTime(timezone=True), nullable=False)
    posicao_x = Column(Integer, nullable=False)
    posicao_y = Column(Integer, nullable=False)
    nivel_bateria = Column(Float, nullable=False)  # porcentagem 0-100
    velocidade = Column(Float, nullable=True)  # m/s
    orientacao = Column(String, nullable=True)
    mensagem = Column(String, nullable=True)

    corrida = relationship("Corrida", back_populates="telemetria")