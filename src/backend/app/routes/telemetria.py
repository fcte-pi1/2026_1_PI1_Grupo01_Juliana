from typing import List, Optional

from fastapi import APIRouter, Depends, HTTPException, WebSocket, WebSocketDisconnect, status
from pydantic import ValidationError
from sqlalchemy.orm import Session

from app.database import SessionLocal, get_db
from app.models.models import Corrida, EventoTelemetria, Labirinto, ResultadoCorrida
from app.schemas.telemetria import TelemetriaIn, TelemetriaOut

router = APIRouter(tags=["telemetria"])


class ConnectionManager:
    def __init__(self) -> None:
        self._conexoes: List[WebSocket] = []

    async def conectar(self, ws: WebSocket) -> None:
        await ws.accept()
        self._conexoes.append(ws)

    def desconectar(self, ws: WebSocket) -> None:
        if ws in self._conexoes:
            self._conexoes.remove(ws)

    async def broadcast(self, payload: dict, *, exceto: Optional[WebSocket] = None) -> None:
        mortos: List[WebSocket] = []
        for ws in self._conexoes:
            if ws is exceto:
                continue
            try:
                await ws.send_json(payload)
            except Exception:
                mortos.append(ws)
        for ws in mortos:
            self.desconectar(ws)


manager = ConnectionManager()


def _resolver_corrida(db: Session, payload: TelemetriaIn) -> Corrida:
    if payload.corrida_id is not None:
        corrida = db.query(Corrida).filter(Corrida.id == payload.corrida_id).first()
        if corrida is None:
            raise HTTPException(
                status_code=status.HTTP_404_NOT_FOUND,
                detail=f"corrida_id {payload.corrida_id} não encontrada",
            )
        return corrida

    labirinto = db.query(Labirinto).filter(Labirinto.id == payload.labirinto_id).first()
    if labirinto is None:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"labirinto_id {payload.labirinto_id} não encontrado",
        )
    corrida = Corrida(labirinto_id=labirinto.id, resultado=ResultadoCorrida.em_andamento)
    db.add(corrida)
    db.flush()  # garante corrida.id sem encerrar a transação
    return corrida


def _persistir(db: Session, payload: TelemetriaIn) -> EventoTelemetria:
    corrida = _resolver_corrida(db, payload)
    evento = EventoTelemetria(
        corrida_id=corrida.id,
        timestamp=payload.timestamp,
        posicao_x=payload.posicao_x,
        posicao_y=payload.posicao_y,
        nivel_bateria=payload.nivel_bateria,
        velocidade=payload.velocidade,
    )
    db.add(evento)
    db.commit()
    db.refresh(evento)
    return evento


@router.post(
    "/telemetria",
    response_model=TelemetriaOut,
    status_code=status.HTTP_201_CREATED,
)
async def receber_telemetria(payload: TelemetriaIn, db: Session = Depends(get_db)) -> EventoTelemetria:
    evento = _persistir(db, payload)
    await manager.broadcast(TelemetriaOut.model_validate(evento).model_dump(mode="json"))
    return evento


@router.websocket("/ws/telemetria")
async def ws_telemetria(websocket: WebSocket) -> None:
    await manager.conectar(websocket)
    try:
        while True:
            mensagem = await websocket.receive_json()
            try:
                payload = TelemetriaIn.model_validate(mensagem)
            except ValidationError as exc:
                await websocket.send_json({"erro": "payload inválido", "detalhes": exc.errors()})
                continue

            db = SessionLocal()
            try:
                evento = _persistir(db, payload)
                serializado = TelemetriaOut.model_validate(evento).model_dump(mode="json")
            except HTTPException as exc:
                await websocket.send_json({"erro": exc.detail, "status": exc.status_code})
                continue
            finally:
                db.close()

            await manager.broadcast(serializado)
    except WebSocketDisconnect:
        manager.desconectar(websocket)
