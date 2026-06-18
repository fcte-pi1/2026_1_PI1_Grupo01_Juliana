#ifndef MOVIMENTACAO_H
#define MOVIMENTACAO_H

#include "m_driver.h"
#include "odometria.h"
#include "encoder.h"

void mouse_movefwd(motor_t *motorR, motor_t *motorL);

void mouse_movebwd(motor_t *motorR, motor_t *motorL);

void mouse_spin(motor_t *motorR, motor_t *motorL, bool sentido);

void mouse_coast(motor_t *motorR, motor_t *motorL);

void mouse_break(motor_t *motorR, motor_t *motorL);

void movimentacao_move_cell(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos);

void movimentacao_turn_clws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos);

void movimentacao_turn_ctclws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos);

#endif
