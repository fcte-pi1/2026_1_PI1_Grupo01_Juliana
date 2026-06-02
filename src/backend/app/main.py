from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from app.routes.health import router as health_router
from app.routes.telemetria import router as telemetria_router
from app.routes.corridas import router as corridas_router

app = FastAPI(title="Micromouse Telemetria API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173"],
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(health_router)
app.include_router(telemetria_router)
app.include_router(corridas_router)