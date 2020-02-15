/*
 * Main Controller Testing Program
 * Author: Lin, Yushan
 * Date: Feb. 10th, 2020
 *
 * GPIO status:
 * GPIO16:  input, pulled up, connected to switch 1.
 * GPIO17:  input, pulled up, connected to switch 2.
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

_Bool voltDetect(_Bool flag);
unsigned int fireDetect(unsigned int angle);

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
