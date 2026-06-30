# tools/ — utilitários de telemetria (integração)

Scripts para colocar a telemetria funcionando **antes** da Tarefa 3.3
(#15, transmissão Wi-Fi no firmware) estar pronta. Eles mandam pacotes para o
`POST /telemetria`, que o backend persiste e retransmite pelo WebSocket — o
dashboard desenha o trajeto em tempo real.

Antes de tudo, suba o backend e popule os labirintos:

```bash
cd src/backend
source venv/bin/activate          # Windows: venv\Scripts\activate
pip install -r requirements.txt
PYTHONPATH=. alembic upgrade head
PYTHONPATH=. python seed.py        # cria 4x4 (id=1), 8x8 (id=2), 16x16 (id=3)
uvicorn app.main:app --reload
```

E o frontend em outro terminal (`cd src/frontend && npm install && npm run dev`).

## `simulador_telemetria.py` — sem hardware

Faz uma corrida fake do canto até o centro. Não precisa de ESP nem do robô.

```bash
python tools/simulador_telemetria.py                  # 4x4, labirinto id=1
python tools/simulador_telemetria.py --encerrar       # ao fim, marca a corrida concluida
python tools/simulador_telemetria.py --lado 8 --labirinto-id 2 --intervalo 0.3
```
