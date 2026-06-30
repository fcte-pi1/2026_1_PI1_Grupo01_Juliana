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

## `ponte_serial.py` — com o robô real (roda no PC onde a ESP está plugada)

Lê o log serial da ESP (`celula (linha,coluna)` impresso pelo flood fill) e
envia ao backend. Solução-ponte de bancada até o firmware transmitir por Wi-Fi
sozinho (#15).

```bash
pip install pyserial
python tools/ponte_serial.py --porta /dev/ttyUSB0     # Linux
python tools/ponte_serial.py --porta COM3             # Windows
```

> Importante: o `idf.py monitor` e esta ponte **disputam a mesma porta serial**.
> Rode um de cada vez (ou abra o monitor só para conferir o boot e feche antes
> de iniciar a ponte).

## Backend em outra máquina (acesso pela rede)

Se o backend não estiver no mesmo PC, passe `--base-url http://<IP>:8000` e, no
backend, suba com `--host 0.0.0.0` e inclua a origem do frontend em
`CORS_ORIGINS` (ver README do backend).
