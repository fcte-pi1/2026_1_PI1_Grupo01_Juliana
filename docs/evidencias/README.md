# Evidências — Integração E2E do lado software (4x4)

Validação **do lado software** da cadeia telemetria → persistência → histórico,
relacionada às issues **#22** (integração E2E no 4x4) e **#27** (critérios de
aceitação). Não substitui a validação com o robô físico — cobre tudo o que é
verificável sem hardware.

## Como foi feito

Um navegador real (Chromium, via Playwright) carrega o frontend e conecta no
WebSocket do backend. Um script simula o ESP32 enviando telemetria por `POST
/telemetria` (abre a corrida, anda do canto `(0,0)` até o centro `(1,1)` do 4x4)
e depois encerra a corrida por `POST /corridas/{id}/encerrar`.

- Backend: FastAPI (`uvicorn`) em `:8000`, SQLite migrado + `seed.py`.
- Frontend: Vite em `:5173`.

## Resultado — PASS ✅

| Verificação | Esperado | Resultado |
|---|---|---|
| Conexão WebSocket | indicador "Conectado" | ✅ |
| Telemetria ao vivo | trajeto e posição no mapa; painel atualizando | ✅ |
| Bateria | 85% | ✅ |
| Desafio cumprido (chegou ao centro) | Sim | ✅ |
| Posição atual | (1, 1) | ✅ |
| Velocidade média | 0.42 m/s | ✅ |
| Encerramento (#16) | HTTP 200, `duracao=6s`, `resultado=concluida` | ✅ |
| Persistência + histórico | corrida aparece como "4x4 · 00:00:06 · Concluída" | ✅ |

## Arquivos

- `01_conectado.png` — tela conectada ao WebSocket.
- `02_telemetria_ao_vivo.png` — telemetria ao vivo (mapa + painel de métricas).
- `03_historico.png` — histórico refletindo a corrida finalizada.
- `log.txt` — saída do script com os `CHECK ... PASS`.
