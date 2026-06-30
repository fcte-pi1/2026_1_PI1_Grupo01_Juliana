#!/usr/bin/env python3
"""Simulador de telemetria do micromouse (sem hardware).

Faz o papel que a ESP32 fara quando a Tarefa 3.3 (#15, transmissao Wi-Fi)
estiver pronta: envia ao backend uma corrida fake, celula a celula, do canto
(0,0) ate o centro do labirinto. Cada pacote vai por `POST /telemetria`, que o
backend persiste E retransmite pelo WebSocket -> o dashboard desenha o trajeto
em tempo real, exatamente como faria com o robo real.

Serve para:
  - validar o pipeline backend + frontend ponta a ponta sem ESP;
  - demonstrar a telemetria funcionando antes da #15 ficar pronta.

Pre-requisitos:
  - backend rodando (uvicorn) e migrations aplicadas;
  - `python seed.py` ja executado (cria os labirintos; o 4x4 costuma ser id=1);
  - httpx instalado (ja esta no requirements.txt do backend).

Uso tipico (com o venv do backend ativado, dentro de src/backend):
    python tools/simulador_telemetria.py
    python tools/simulador_telemetria.py --lado 8 --labirinto-id 2 --intervalo 0.3
"""

import argparse
import sys
import time
from datetime import datetime, timezone

try:
    import httpx
except ImportError:
    sys.exit(
        "httpx nao encontrado. Ative o venv do backend e rode "
        "`pip install -r requirements.txt`."
    )


def caminho_ate_centro(lado: int) -> list[tuple[int, int]]:
    """Trajeto simples (x, y) de (0,0) ate uma celula central do bloco 2x2.

    Anda pela borda inferior (y=0) ate a coluna central e depois sobe ate a
    linha central. O ultimo ponto cai no centro [lado/2-1 .. lado/2], que e
    onde o frontend marca "desafio cumprido".
    """
    meio = lado // 2
    caminho = [(x, 0) for x in range(0, meio + 1)]          # (0,0) -> (meio,0)
    caminho += [(meio, y) for y in range(1, meio + 1)]      # sobe ate (meio,meio)
    return caminho


def main() -> None:
    parser = argparse.ArgumentParser(description="Simulador de telemetria do micromouse.")
    parser.add_argument("--base-url", default="http://localhost:8000",
                        help="URL do backend (padrao: http://localhost:8000).")
    parser.add_argument("--labirinto-id", type=int, default=1,
                        help="ID do labirinto que abre a corrida (padrao: 1 = 4x4 do seed).")
    parser.add_argument("--lado", type=int, default=4,
                        help="Lado do labirinto, so para gerar o trajeto (padrao: 4).")
    parser.add_argument("--intervalo", type=float, default=0.5,
                        help="Segundos entre cada celula (padrao: 0.5).")
    parser.add_argument("--bateria-inicial", type=float, default=100.0,
                        help="Nivel de bateria inicial em %% (padrao: 100).")
    parser.add_argument("--encerrar", action="store_true",
                        help="Ao terminar, chama /corridas/{id}/encerrar (marca concluida).")
    args = parser.parse_args()

    base = args.base_url.rstrip("/")
    caminho = caminho_ate_centro(args.lado)
    queda_por_passo = 25.0 / max(1, len(caminho))  # gasta ~25%% de bateria na corrida

    corrida_id: int | None = None

    print(f"Simulando corrida no {args.lado}x{args.lado} -> {base}/telemetria")
    print(f"{len(caminho)} celulas, intervalo {args.intervalo}s\n")

    with httpx.Client(timeout=5.0) as client:
        for i, (x, y) in enumerate(caminho):
            bateria = max(0.0, args.bateria_inicial - queda_por_passo * i)
            pacote = {
                "timestamp": datetime.now(timezone.utc).isoformat(),
                "posicao_x": x,
                "posicao_y": y,
                "nivel_bateria": round(bateria, 1),
                "velocidade": round(0.40 + 0.05 * (i % 3), 2),
            }
            # O primeiro pacote abre a corrida via labirinto_id; os demais
            # reaproveitam o corrida_id devolvido pelo backend.
            if corrida_id is None:
                pacote["labirinto_id"] = args.labirinto_id
            else:
                pacote["corrida_id"] = corrida_id

            try:
                resp = client.post(f"{base}/telemetria", json=pacote)
            except httpx.ConnectError:
                sys.exit(f"\nNao conectou em {base}. O backend (uvicorn) esta rodando?")

            if resp.status_code != 201:
                sys.exit(f"\nBackend recusou o pacote ({resp.status_code}): {resp.text}")

            evento = resp.json()
            corrida_id = evento["corrida_id"]
            print(f"  celula ({x},{y})  bateria {pacote['nivel_bateria']}%  "
                  f"corrida {corrida_id}  evento {evento['id']}")
            time.sleep(args.intervalo)

    print(f"\nCorrida {corrida_id} enviada ({len(caminho)} eventos).")

    if args.encerrar and corrida_id is not None:
        with httpx.Client(timeout=5.0) as client:
            resp = client.post(f"{base}/corridas/{corrida_id}/encerrar",
                               json={"resultado": "concluida"})
        if resp.status_code == 200:
            print(f"Corrida {corrida_id} encerrada como concluida.")
        else:
            print(f"Falha ao encerrar ({resp.status_code}): {resp.text}")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nInterrompido.")
