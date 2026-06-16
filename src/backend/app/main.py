import os

from dotenv import load_dotenv

# Carrega variáveis de um arquivo .env (se existir) antes de qualquer import que
# leia configuração de ambiente. Em produção/CI as variáveis costumam vir do
# próprio ambiente, e o .env é apenas uma conveniência para desenvolvimento.
load_dotenv()

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from app.routes.health import router as health_router
from app.routes.telemetria import router as telemetria_router
from app.routes.corridas import router as corridas_router

# Origens permitidas para CORS, configuráveis por ambiente (lista separada por
# vírgula). O default cobre o frontend local (Vite em :5173); para acessar o
# backend pela rede, inclua o endereço do frontend em CORS_ORIGINS, por exemplo:
# CORS_ORIGINS=http://localhost:5173,http://192.168.0.10:5173
CORS_ORIGINS_PADRAO = "http://localhost:5173,http://127.0.0.1:5173"
cors_origins = [
    origem.strip()
    for origem in os.getenv("CORS_ORIGINS", CORS_ORIGINS_PADRAO).split(",")
    if origem.strip()
]

app = FastAPI(title="Micromouse Telemetria API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=cors_origins,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(health_router)
app.include_router(telemetria_router)
app.include_router(corridas_router)
