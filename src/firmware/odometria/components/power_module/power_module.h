#ifndef POWER_MODULE_H
#define POWER_MODULE_H

#define I2C_PORT          0
#define I2C_SDA GPIO_NUM_22
#define I2C_SCL GPIO_NUM_21

#define SHUNT       0.100f
#define MAX_CURRENT 0.820f
#define INA_ADDRESS 0x44

#define INA226_REG_CONFIG       0x00
#define INA226_REG_SHUNT_VOLT   0x01
#define INA226_REG_BUS_VOLT     0x02
#define INA226_REG_POWER        0x03
#define INA226_REG_CURRENT      0x04
#define INA226_REG_CALIBRATION  0x05

#include "driver/i2c_master.h"
#include "esp_err.h"

typedef struct
{
    i2c_master_dev_handle_t dev;

    float shunt_resistance;
    float current_lsb;

} ina226_t;

void scan_devices(i2c_master_bus_handle_t bus);

esp_err_t ina226_init(
    ina226_t *ina,
    i2c_master_bus_handle_t bus,
    uint8_t address,
    float shunt_resistance,
    float max_current);

esp_err_t ina226_get_bus_voltage(
    ina226_t *ina,
    float *voltage);

esp_err_t ina226_get_current(
    ina226_t *ina,
    float *current);

esp_err_t ina226_get_power(
    ina226_t *ina,
    float *power);

#endif