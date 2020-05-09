/*
 * Main Controller Testing Program
 * Author: Lin, Yushan
 * Date: Feb. 10th, 2020
 *
 * GPIO status:
 * GPIO16:  input, pulled up, connected to switch 1.
 * GPIO17:  input, pulled up, connected to switch 2.
 *
 * Update: Integrate voltage monitoring into main controller.
 * Date: Mar. 26th, 2020
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

_Bool voltDetect(_Bool flag);
unsigned int fireDetect(unsigned int angle);

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   8192        //Multisampling

static const char* VM_TAG = "vm";

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6;     //GPIO34
static const adc_atten_t atten = ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;

static const float SSF = 23.14;

static void check_efuse(void)
{
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        ESP_LOGI(VM_TAG, "eFuse Two Point: Supported");
    } else {
        ESP_LOGI(VM_TAG, "eFuse Two Point: NOT supported");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        ESP_LOGI(VM_TAG, "eFuse Vref: Supported\n");
    } else {
        ESP_LOGI(VM_TAG, "eFuse Vref: NOT supported");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGI(VM_TAG, "Characterized using Two Point Value");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGI(VM_TAG, "Characterized using eFuse Vref");
    } else {
        ESP_LOGI(VM_TAG, "Characterized using Default Vref");
    }
}

void app_main(void){
	gpio_set_direction(16, GPIO_MODE_INPUT);
	gpio_set_direction(17, GPIO_MODE_INPUT);

    printf("Start GPS fix\n");
    // Generate a random waiting time for GPS to fix
    int delayTime = rand() % 30;
    vTaskDelay(pdMS_TO_TICKS(delayTime * 1000));
    printf("GPS locked to the satellite\nWaiting time = %d seconds\n", delayTime);


    unsigned int gpsLat;
    unsigned int gpsLong;
    unsigned int status = 0;

    // Generate random latitude and longitude values
	gpsLat = rand() % (180 + 1 - 0) + 0;
	gpsLong = rand() % (360 + 1 - 0) + 0;
    printf("Latitude: %d\n", gpsLat);
    printf("Longitude: %d\n", gpsLong);

    _Bool voltLevel = false;
    _Bool voltFlag;
    unsigned int fireAngle = 0;
    unsigned int fireFlag;

    //Check if Two Point or Vref are burned into eFuse
    check_efuse();

    //Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(channel, atten);

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);

    while(1){
        // Get battery voltage level
        voltFlag = voltDetect(voltLevel);
        printf("Check voltage level\n");
    	// Get fire detection info
        fireFlag = fireDetect(fireAngle);
        printf("Check fire detection\n");
    	// Both switches are off
    	if((voltFlag == false) && (fireFlag == 0)){
    		printf("Nothing wrong\n");
    		status = 0;
    	}
    	// Low voltage level switch on
    	if(voltFlag == true){
    		// Set low power flag
    		printf("Low voltage\n");
    		status = 1;
    	}

    	// Fire detection switch on, fire detected
    	if(fireFlag != 0){
    		printf("Fire detected\nFire direction: %d degrees\n", fireFlag);
    		status = 2;
    	}
    	// Voltage is low and fire detected
    	if((voltFlag == true) && (fireFlag != 0)){
    		status = 3;
    	}
    	printf("Event created. 0x%.4X %.4X %.2X %.4X\n\n", gpsLat, gpsLong, status, fireFlag);
    	// Wait 6 seconds to restart the detection cycle
    	vTaskDelay(pdMS_TO_TICKS(6000));
    }
}

_Bool voltDetect(_Bool flag){
	if(gpio_get_level(16) == 0){
		flag = false;
	}
	else{
		flag = true;
	}
	return flag;

    uint32_t adc_reading = 0;
    //Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        if (unit == ADC_UNIT_1)
            adc_reading += adc1_get_raw((adc1_channel_t)channel);
    }
    adc_reading /= NO_OF_SAMPLES;
    //Convert adc_reading to voltage in mV
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    //Upscale to pre-divided voltage
    uint32_t upscaled = voltage * SSF;
    ESP_LOGI(VM_TAG, "Raw: %d\tVoltage: %d mV\tUpscaled: %d mV", adc_reading, voltage, upscaled);
    if(upscaled < 10000)
    	ESP_LOGI(VM_TAG, "LOW VOLTAGE");

    vTaskDelay(pdMS_TO_TICKS(1000));
}

unsigned int fireDetect(unsigned int angle){
	if(gpio_get_level(17) == 0){
		angle = 0;
	}
	else{
		angle = rand() % (360 + 1 - 0) + 0;
	}
	return angle;
}
