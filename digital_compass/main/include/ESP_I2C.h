/*
 * ESP_I2C.h
 *
 *  Created on: Feb 26, 2020
 *      Author: hiemenzpe
 */

#ifndef MAIN_INCLUDE_ESP_I2C_H_
#define MAIN_INCLUDE_ESP_I2C_H_

#include "ESP_I2C.c"
#include "driver/i2c.h"

i2c_config_t I2C_Master_Config(uint32_t clk, gpio_num_t sda, gpio_pullup_t sdares, gpio_num_t scl, gpio_pullup_t sclres);



#endif /* MAIN_INCLUDE_ESP_I2C_H_ */
