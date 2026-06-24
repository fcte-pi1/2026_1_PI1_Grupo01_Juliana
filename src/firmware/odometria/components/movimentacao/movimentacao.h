#ifndef MOVIMENTACAO_H
#define MOVIMENTACAO_H

#include "m_driver.h"
#include "odometria.h"
#include "encoder.h"
#include "calibracao.h"

// Define os fatores de compensacao (trim) e ganhos usados pelo controle de
// retidao em malha fechada. Se nunca for chamada, a movimentacao usa valores
// neutros (trim 1.0, ganhos padrao), equivalente ao comportamento antigo
// porem ja com a correcao por encoder ligada.
void movimentacao_aplicar_calibracao(const calib_t *cal);

void mouse_movefwd(motor_t *motorR, motor_t *motorL);

void mouse_movebwd(motor_t *motorR, motor_t *motorL);

void mouse_spin(motor_t *motorR, motor_t *motorL, bool sentido);

void mouse_break(motor_t *motorR, motor_t *motorL);

void movimentacao_move_cell(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos);

void movimentacao_turn_clws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos);

void movimentacao_turn_ctclws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos);

#endif
