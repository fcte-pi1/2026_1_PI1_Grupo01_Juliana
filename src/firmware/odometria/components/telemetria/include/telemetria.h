#ifndef TELEMETRIA_H
#define TELEMETRIA_H

#include <stdbool.h>

// Modulo de transmissao de telemetria do micromouse (Tarefa 3.3, issue #15).
//
// Sobe o Wi-Fi (station) e um cliente WebSocket que fala com o backend
// (FastAPI) no endpoint /ws/telemetria. A cada celula percorrida, o flood fill
// chama telemetria_envia(...) e um pacote JSON e transmitido; o backend
// persiste e retransmite para o dashboard.
//
// TUDO E BEST-EFFORT: se o Wi-Fi ou o WebSocket nao conectarem, as funcoes
// apenas registram um aviso e retornam. A navegacao do robo NUNCA e bloqueada
// por falha de telemetria.
//
// Configuracao de rede (SSID, senha, IP do backend) fica no topo de
// telemetria.c, no bloco CONFIGURACAO.

// Sobe Wi-Fi + WebSocket. `labirinto_id` abre a corrida no backend
// (1 = 4x4, 2 = 8x8, conforme o seed). Bloqueia ate conectar ou estourar o
// timeout interno. Retorna true se o WebSocket conectou.
bool telemetria_init(int labirinto_id);

// Envia um pacote de telemetria referente a uma celula.
//   linha, coluna : posicao na grade (mapeadas para posicao_y, posicao_x).
//   nivel_bateria : porcentagem 0-100.
//   velocidade    : m/s (use valor negativo para omitir).
// No-op se a telemetria nao estiver conectada.
void telemetria_envia(int linha, int coluna, float nivel_bateria, float velocidade);

// Encerra o cliente WebSocket (opcional).
void telemetria_stop(void);

#endif // TELEMETRIA_H
