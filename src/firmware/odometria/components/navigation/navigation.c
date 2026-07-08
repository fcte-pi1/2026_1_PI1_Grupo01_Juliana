#include "navigation.h"

#include "movimentacao.h"

static motor_t *motorR;
static motor_t *motorL;

static encoder_t *encoderR;
static encoder_t *encoderL;

static pose_t *robot_pose;

QueueHandle_t navigation_queue;

static void navigation_task(void *arg)
{
    ir_event_t event;

    while (1)
    {
        if (xQueueReceive(
                navigation_queue,
                &event,
                portMAX_DELAY))
        {
            mouse_break(motorR, motorL);

            switch (event)
            {
                case IR_EVENT_FRONT:

                    if (gpio_get_level(IR_R))
                    {
                        movimentacao_turn_clws(
                            motorR,
                            motorL,
                            encoderR,
                            encoderL,
                            robot_pose
                        );
                    }
                    else if (gpio_get_level(IR_L))
                    {
                        movimentacao_turn_ctclws(
                            motorR,
                            motorL,
                            encoderR,
                            encoderL,
                            robot_pose
                        );
                    }
                    else
                    {
                        mouse_movebwd(
                            motorR,
                            motorL
                        );

                        vTaskDelay(pdMS_TO_TICKS(300));

                        mouse_break(
                            motorR,
                            motorL
                        );

                        movimentacao_turn_clws(
                            motorR,
                            motorL,
                            encoderR,
                            encoderL,
                            robot_pose
                        );

                        vTaskDelay(pdMS_TO_TICKS(50));

                        movimentacao_turn_clws(
                            motorR,
                            motorL,
                            encoderR,
                            encoderL,
                            robot_pose
                        );
                    }

                    break;

                case IR_EVENT_FRONT_RIGHT:

                    mouse_movebwd(
                        motorR,
                        motorL
                    );

                    vTaskDelay(pdMS_TO_TICKS(300));

                    mouse_break(
                        motorR,
                        motorL
                    );

                    mouse_spin(
                        motorR,
                        motorL,
                        0
                    );

                    vTaskDelay(pdMS_TO_TICKS(300));

                    mouse_break(
                        motorR,
                        motorL
                    );

                    break;

                case IR_EVENT_FRONT_LEFT:

                    mouse_movebwd(
                        motorR,
                        motorL
                    );

                    vTaskDelay(pdMS_TO_TICKS(300));

                    mouse_break(
                        motorR,
                        motorL
                    );

                    mouse_spin(
                        motorR,
                        motorL,
                        1
                    );

                    vTaskDelay(pdMS_TO_TICKS(300));

                    mouse_break(
                        motorR,
                        motorL
                    );

                    break;

                default:
                    break;
            }
        }
    }
}

void navigation_init(
    motor_t *mR,
    motor_t *mL,
    encoder_t *eR,
    encoder_t *eL,
    pose_t *pose)
{
    motorR = mR;
    motorL = mL;

    encoderR = eR;
    encoderL = eL;

    robot_pose = pose;

    navigation_queue =
        xQueueCreate(
            10,
            sizeof(ir_event_t)
        );

    xTaskCreate(
        navigation_task,
        "navigation",
        4096,
        NULL,
        5,
        NULL
    );
}