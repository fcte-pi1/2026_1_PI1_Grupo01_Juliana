#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "infrared.h"
#include "m_driver.h"
#include "encoder.h"
#include "odometria.h"

extern QueueHandle_t navigation_queue;

void navigation_init(
    motor_t *mR,
    motor_t *mL,
    encoder_t *eR,
    encoder_t *eL,
    pose_t *pose
);