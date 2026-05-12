from fastapi import FastAPI
from app.routes.health import router as health_router
from app.routes.telemetria import router as telemetria_router

app = FastAPI(title="Micromouse Telemetria API")

app.include_router(health_router)
app.include_router(telemetria_router)