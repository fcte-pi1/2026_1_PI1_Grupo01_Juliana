//codigo feito para testar integração odometria+sensores

#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
 
#include "m_driver.h"
#include "encoder.h"
#include "odometria.h"
#include "movimentacao.h"
#include "power_module.h"
#include "infrared.h"
#include "seletor_modo.h"

#include "telemetry_data_nvs.h"

static const char *TAG = "teste_navegacao";

extern SemaphoreHandle_t motor_mutex;

#define A3_FREQ         440
#define E4_FREQ         659
#define A4_FREQ         880

#define TEMPO           300

pose_t pose;

encoder_t encoderR;
encoder_t encoderL;

ina226_t ina;

static i2c_master_bus_handle_t bus_handle;

motor_t motorR = { .pwm_gpio1 = PWM_R1, .pwm_gpio2 = PWM_R2};
motor_t motorL = { .pwm_gpio1 = PWM_L1, .pwm_gpio2 = PWM_L2};

// task que reage a interrupcao do botao
static TaskHandle_t supervisor_handle = NULL;

void telemetry_task(void *arg){
    float voltage_v = 0.0;
    float current_a = 0.0;

    while (1)
    {
        // 1. Le os dados reais do sensor passando o endereço das variáveis (&)
        ina226_get_bus_voltage(&ina, &voltage_v);
        ina226_get_current(&ina, &current_a);

        // 2. Converte os floats (Volts/Amperes) para inteiros (Milivolts/Miliamperes)
        // O "cast" (uint16_t) garante que o ESP32 descarte as casas decimais corretamente
        uint16_t voltage_mv = (uint16_t)(voltage_v * 1000.0f);
        int16_t current_ma = (int16_t)(current_a * 1000.0f);

        // 3. Salva na memória NVS usando a função do Canvas
        telemetry_save_sample(voltage_mv, current_ma);

        // 4. Aguarda 500ms antes de ler novamente (2 amostras por segundo)
        // Isso evita encher a memória muito rápido
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void supervisor_task(void *arg)
{
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "botao pressionado: interrompendo e trocando de modo");

        mouse_break(&motorR, &motorL);

        modo_lab_t novo = seletor_modo_toggle();
        seletor_modo_sinalizar(novo);
    }
}

void app_main(void){

    ESP_LOGW(TAG, "motivo do ultimo reset: %d (1=PANIC 12=BROWNOUT)", esp_reset_reason());

    gpio_install_isr_service(0);

    // botao no GPIO 25 (interrupcao) + buzzer no GPIO 33
    seletor_modo_init(supervisor_handle);    

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT
    };
    
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    
    play_tone(A4_FREQ, TEMPO);

    encoder_init(&encoderR, GPIO_ENC_R);
    encoder_init(&encoderL, GPIO_ENC_L);

    play_tone(A3_FREQ, 50);

    vTaskDelay(pdMS_TO_TICKS(100));

    play_tone(A3_FREQ, 50);

    esp_err_t err = ina226_init(
        &ina, 
        bus_handle,
        INA_ADDRESS, 
        SHUNT, 
        MAX_CURRENT
    );
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "INA226 inicializado com sucesso!");
    } else {
        ESP_LOGE(TAG, "Falha ao inicializar INA226");
    }   

    play_tone(E4_FREQ, TEMPO);

    vTaskDelay(pdMS_TO_TICKS(1000));

    play_tone(E4_FREQ, 50);

    vTaskDelay(pdMS_TO_TICKS(100));

    play_tone(E4_FREQ, 50);

    ESP_ERROR_CHECK(telemetry_init());

    play_tone(A4_FREQ, TEMPO);

    driver_init();

    motor_init(&motorR);
    motor_init(&motorL);

    IR_init(&motorR, &motorL, &encoderR, &encoderL, &pose);
    
    odometria_pos_init(&pose, 0, 0, NORTE);

    motorR = (motor_t){
        PWM_R1,
        PWM_R2,
        motorR.comp1,
        motorR.comp2,
        motorR.gen1,
        motorR.gen2
    };

    motorL = (motor_t){
        PWM_L1,
        PWM_L2,
        motorL.comp1,
        motorL.comp2,
        motorL.gen1,
        motorL.gen2
    };

    // o supervisor precisa existir antes de ligar a interrupcao do botao,
    // pois e ele quem recebe a notificacao da ISR
    xTaskCreate(supervisor_task, "supervisor_task", 4096, NULL, 4, &supervisor_handle);

    // sinaliza o modo inicial e parte a primeira missao
    seletor_modo_sinalizar(seletor_modo_get());

    // criacao da task que vai ficar lendo o INA226 e salvando em segundo plano
    xTaskCreate(
        telemetry_task,
        "telemetry_task",
        4096,
        NULL,
        2, // Prioridade baixa, para não atrapalhar os motores e sensores
        NULL
    );

    telemetry_print_all();

    ESP_ERROR_CHECK(telemetry_clear());

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1500));
        play_tone(A3_FREQ, TEMPO);

        // Tenta pegar a chave dos motores
        if (motor_mutex != NULL && xSemaphoreTake(motor_mutex, portMAX_DELAY) == pdTRUE) {
            movimentacao_move_cell(&motorR, &motorL, &encoderR, &encoderL, &pose);
            
            // Devolve a chave após terminar de mover a célula
            xSemaphoreGive(motor_mutex);
        }
    }
}