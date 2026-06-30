#!/usr/bin/env python3
"""Ponte serial -> backend (telemetria a partir do log da ESP32).

Solucao-ponte enquanto a Tarefa 3.3 (#15, Wi-Fi no firmware) nao fica pronta:
em vez da ESP transmitir sozinha, este script roda no PC onde a ESP esta
plugada por USB, le o log serial, extrai a posicao impressa pelo flood fill e
faz `POST /telemetria` no backend. Resultado: a posicao REAL do robo aparece no
dashboard, sem precisar mexer no firmware.

Como o firmware ja loga, a cada celula, algo como:
    passo 3 | celula (1,2) | orientacao NORTE | dist 4
este script captura "celula (linha,coluna)" e envia
posicao_y=linha, posicao_x=coluna.

Dependencias:
  - pyserial:  pip install pyserial
  (o POST usa urllib da biblioteca padrao, entao nao precisa de httpx aqui)

Uso (no PC do Noboru, com a ESP conectada):
    python tools/ponte_serial.py --porta /dev/ttyUSB0
    python tools/ponte_serial.py --porta COM3 --base-url http://localhost:8000
Descubra a porta: Linux `ls /dev/ttyUSB* /dev/ttyACM*`, Windows = COMx.
Saia com Ctrl+C.
"""

import argparse
import json
import re
import sys
import urllib.error
import urllib.request
from datetime import datetime, timezone

try:
    import serial  # pyserial
except ImportError:
    sys.exit("pyserial nao encontrado. Rode `pip install pyserial`.")

# Captura "celula (linha,coluna)" no log do firmware.
RE_CELULA = re.compile(r"celula\s*\(\s*(\d+)\s*,\s*(\d+)\s*\)")
# Captura a velocidade media, se a linha trouxer (opcional).
RE_VEL = re.compile(r"velocidade media \(m/s\):\s*([0-9.]+)")


def postar(base_url: str, pacote: dict) -> dict | None:
    dados = json.dumps(pacote).encode("utf-8")
    req = urllib.request.Request(
        f"{base_url}/telemetria",
        data=dados,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=5) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        print(f"  [backend recusou {e.code}] {e.read().decode('utf-8', 'ignore')}")
    except urllib.error.URLError as e:
        print(f"  [sem conexao com {base_url}] {e.reason}")
    return None


def main() -> None:
    parser = argparse.ArgumentParser(description="Ponte serial -> backend de telemetria.")
    parser.add_argument("--porta", required=True,
                        help="Porta serial da ESP (ex.: /dev/ttyUSB0, COM3).")
    parser.add_argument("--baud", type=int, default=115200,
                        help="Baud rate (padrao: 115200, igual ao log do firmware).")
    parser.add_argument("--base-url", default="http://localhost:8000",
                        help="URL do backend (padrao: http://localhost:8000).")
    parser.add_argument("--labirinto-id", type=int, default=1,
                        help="ID do labirinto que abre a corrida (padrao: 1 = 4x4 do seed).")
    parser.add_argument("--bateria", type=float, default=100.0,
                        help="Bateria a reportar enquanto o firmware nao mede (padrao: 100).")
    args = parser.parse_args()

    base = args.base_url.rstrip("/")
    corrida_id: int | None = None
    ultima_vel: float | None = None

    print(f"Abrindo {args.porta} @ {args.baud}... (Ctrl+C para sair)")
    try:
        ser = serial.Serial(args.porta, args.baud, timeout=1)
    except serial.SerialException as e:
        sys.exit(f"Nao abriu a porta {args.porta}: {e}")

    print(f"Lendo log e enviando para {base}/telemetria\n")

    try:
        while True:
            linha = ser.readline().decode("utf-8", errors="ignore").strip()
            if not linha:
                continue

            mvel = RE_VEL.search(linha)
            if mvel:
                ultima_vel = float(mvel.group(1))

            m = RE_CELULA.search(linha)
            if not m:
                continue

            linha_idx, coluna_idx = int(m.group(1)), int(m.group(2))
            pacote = {
                "timestamp": datetime.now(timezone.utc).isoformat(),
                "posicao_x": coluna_idx,
                "posicao_y": linha_idx,
                "nivel_bateria": args.bateria,
            }
            if ultima_vel is not None:
                pacote["velocidade"] = ultima_vel
            if corrida_id is None:
                pacote["labirinto_id"] = args.labirinto_id
            else:
                pacote["corrida_id"] = corrida_id

            evento = postar(base, pacote)
            if evento:
                corrida_id = evento["corrida_id"]
                print(f"  celula ({linha_idx},{coluna_idx}) -> corrida {corrida_id} "
                      f"evento {evento['id']}")
    except KeyboardInterrupt:
        print("\nEncerrando ponte.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
