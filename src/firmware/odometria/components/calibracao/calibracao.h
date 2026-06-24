#ifndef CALIBRACAO_H
#define CALIBRACAO_H

// Componente de verificacao/validacao dos encoders e de compensacao do
// desvio de trajetoria do carrinho.
//
// Por que isto existe:
// O carrinho tem duas rodas motrizes (com encoder) e uma roda boba dianteira
// desbalanceada que "empina" o chassi. Esse desbalanceamento desloca peso,
// muda o atrito/escorregamento das rodas e faz o carrinho desviar mesmo
// quando os dois motores recebem o mesmo PWM. Em vez de refazer a mecanica,
// compensamos por software:
//   1. validamos que os dois encoders estao realmente respondendo;
//   2. medimos qual roda anda mais (trim) para um mesmo PWM;
//   3. fechamos a malha usando a diferenca entre os encoders (ver movimentacao).

#include <stdbool.h>

#include "m_driver.h"
#include "encoder.h"

// --------------------------------------------------------------------------
// Parametros ajustaveis (tune conforme o seu carrinho)
// --------------------------------------------------------------------------
// 0 = no boot NAO aciona os motores (evita brownout/reset por pico de corrente).
//     O controle de rumo por encoder continua ativo com ganhos padrao.
// 1 = roda validacao + trim no boot (carrinho suspenso, fonte estável).
#define CALIBRACAO_MOTORES_NO_BOOT  0

#define CALIB_PWM_TESTE      50    // PWM usado nos testes de acionamento
#define CALIB_TEMPO_MS       500   // duracao de cada teste de acionamento [ms]
#define CALIB_MIN_PULSOS     5     // minimo de pulsos p/ considerar o encoder "vivo"
#define CALIB_RUIDO_MAX      2     // pulsos toleraveis com o carrinho parado
#define CALIB_ASSIMETRIA_MAX 1.6f  // razao max aceitavel entre roda rapida/lenta

// Ganhos padrao do controlador de retidao (usado em movimentacao).
// kp atua sobre a diferenca ACUMULADA de pulsos (erro de rumo);
// ki fica disponivel para ajuste fino e por padrao vem desligado.
#define CALIB_KP_PADRAO      1.2f
#define CALIB_KI_PADRAO      0.0f

// Resultado da validacao dos encoders.
typedef enum {
    CALIB_OK = 0,        // os dois encoders responderam de forma coerente
    CALIB_ERRO_ENC_DIR,  // encoder direito nao gerou pulsos suficientes
    CALIB_ERRO_ENC_ESQ,  // encoder esquerdo nao gerou pulsos suficientes
    CALIB_ERRO_AMBOS,    // nenhum dos encoders respondeu
    CALIB_ERRO_RUIDO,    // contagem subindo com o carrinho parado (ruido/glitch)
    CALIB_ASSIMETRIA,    // ambos contam, porem muito diferentes entre si
} calib_status_t;

// Fatores de compensacao aplicados pela movimentacao.
typedef struct {
    float trim_dir;  // multiplicador do PWM da roda direita  (~1.0)
    float trim_esq;  // multiplicador do PWM da roda esquerda (~1.0)
    float kp;        // ganho proporcional do controle de retidao
    float ki;        // ganho integral (opcional)
    bool  valido;    // true depois de uma estimativa bem-sucedida
} calib_t;

// Preenche 'c' com valores neutros/seguros (trims = 1.0, ganhos padrao).
void calibracao_padrao(calib_t *c);

// Texto legivel para um status (para logs/relatorio).
const char *calibracao_status_str(calib_status_t s);

// Verifica e valida os encoders:
//   - confere que nao ha contagem espuria com o carrinho parado;
//   - aciona os dois motores por um curto intervalo e confere que cada
//     encoder gerou pulsos suficientes;
//   - sinaliza assimetria grosseira entre as rodas.
// Deixa os motores parados ao final. Rode com o carrinho com as rodas livres
// (suspenso) ou em superficie segura.
calib_status_t calibracao_validar_encoders(motor_t *mR, motor_t *mL,
                                           encoder_t *eR, encoder_t *eL);

// Estima os fatores de trim acionando os dois motores no mesmo PWM e
// medindo qual roda andou mais. A roda mais rapida recebe trim < 1.0 para
// igualar a mais lenta. Em caso de sucesso preenche 'cal' (com kp/ki padrao)
// e marca cal->valido = true. Retorna o status da medicao.
calib_status_t calibracao_estimar_trim(motor_t *mR, motor_t *mL,
                                       encoder_t *eR, encoder_t *eL,
                                       calib_t *cal);

#endif
