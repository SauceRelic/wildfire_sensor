/*
 * compass_subsystem.c
 *
 *  Created on: Oct 22, 2019
 *      Author: sanfordij
 */
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include "sdkconfig.h"

#define PIN_SDA 14
#define PIN_CLK 16
#define I2C_ADDRESS 0b0011110

static char tag[] = "LIS2MDLTR";

#undef ESP_ERROR_CHECK
#define ESP_ERROR_CHECK(x) do { esp_err_t rc = (x); if (rc != ESP_OK) { ESP_LOGE("err", "esp_err_t = %d", rc); assert(0 && #x);} } while(0);


void app_main(void){
	ESP_LOGD(tag, ">> LIS2MDLTR");
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = PIN_SDA;
	conf.scl_io_num = PIN_CLK;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 100000;
	ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
	ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

	uint8_t data[6];

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd, 0x60, 1); // Config A
	i2c_master_write_byte(cmd, 0x80, 1); // Enable temp compensation and setup configuration register A according to datasheet guidelines
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
	i2c_cmd_link_delete(cmd);

	//maybe delete config B if not necessary
	//Set value in "Configuration Register B"
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd, 0x61, 1); // "Configuration Register B"
	i2c_master_write_byte(cmd, 0x07, 1); // default setting check for later to know registers
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
	i2c_cmd_link_delete(cmd);

	//Set value in "Configuration Register C"
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd, 0x62, 1); // "Configuration Register C"
	i2c_master_write_byte(cmd, 0x01, 1); // Setup configuration register C according to datasheet guidelines
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
	i2c_cmd_link_delete(cmd);

	//Set active register to "Identification Register"
	cmd = i2c_cmd_link_create();
	ESP_ERROR_CHECK(i2c_master_start(cmd));
	ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1));
	ESP_ERROR_CHECK(i2c_master_write_byte(cmd, 0x4F, 1)); //"Identification Register"
	ESP_ERROR_CHECK(i2c_master_stop(cmd));
	ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100)));
	i2c_cmd_link_delete(cmd);

	//Get data from Identification Register A, B and C
	cmd = i2c_cmd_link_create();
	ESP_ERROR_CHECK(i2c_master_start(cmd));
	ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_READ, 1));
	i2c_master_read_byte(cmd, data,   0);
	i2c_master_read_byte(cmd, data+1, 0);
	i2c_master_read_byte(cmd, data+2, 1);
	ESP_ERROR_CHECK(i2c_master_stop(cmd));
	ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100)));
	i2c_cmd_link_delete(cmd);

	while(1) {
		//Set active register to "Data Output X MSB Register"
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1);
		i2c_master_write_byte(cmd, 0x68 + 0x80, 1); //0x69 = "Data Output X MSB Register" 0x80 = Auto increment
//		i2c_master_stop(cmd);
//		i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
//		i2c_cmd_link_delete(cmd);

		//Read values for X, Y and Z
//		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_READ, 1);
		i2c_master_read_byte(cmd, data,   0); //"Data Output X LSB Register"
		i2c_master_read_byte(cmd, data+1, 0); //"Data Output X MSB Register"
		i2c_master_read_byte(cmd, data+2, 0); //"Data Output Y LSB Register"
		i2c_master_read_byte(cmd, data+3, 0); //"Data Output Y MSB Register"
		i2c_master_read_byte(cmd, data+4, 0); //"Data Output Z LSB Register"
		i2c_master_read_byte(cmd, data+5, 1); //"Data Output Z MSB Register"
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
		i2c_cmd_link_delete(cmd);

		short x = data[1] << 8 | data[0];
		short y = data[3] << 8 | data[2];
		short z = data[5] << 8 | data[4];
		int angle = atan2((double)y,(double)x) * (180 / 3.14159265) + 180; // angle in degrees
		//ESP_LOGD(tag, "angle: %d, x: %d, y: %d, z: %d", angle, x, y, z);
		printf("angle: %d, x: %d, y: %d, z: %d \n", angle, x, y, z);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

	vTaskDelete(NULL);
}
