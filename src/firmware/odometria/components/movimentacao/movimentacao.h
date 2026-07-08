#ifndef MOVIMENTACAO_H
#define MOVIMENTACAO_H

#include "m_driver.h"
#include "odometria.h"
#include "encoder.h"

#include <stdbool.h>

extern volatile bool motion_abort;

void mouse_movefwd(motor_t *motorR, motor_t *motorL);

void mouse_movebwd(motor_t *motorR, motor_t *motorL);

void mouse_spin(motor_t *motorR, motor_t *motorL, bool sentido);

void mouse_coast(motor_t *motorR, motor_t *motorL);

void mouse_break(motor_t *motorR, motor_t *motorL);

typedef enum {
    MOV_OK,
    MOV_AJUSTE_PAREDE,
    MOV_TRAVADO,
    MOV_ABORTADO,
} movimentacao_status_t;

bool movimentacao_move_cell(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL,
                            pose_t *pos, movimentacao_status_t *status);

bool movimentacao_re_curta(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL);

void movimentacao_turn_clws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos);

void movimentacao_turn_ctclws(motor_t *mtrR, motor_t *mtrL, encoder_t *encR, encoder_t *encL, pose_t *pos);

#endif
