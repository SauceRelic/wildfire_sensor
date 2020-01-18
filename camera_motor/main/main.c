/*
  Servo Working code

  This code first configures timer 0 to generated 50Hz signal of repeat after 20ms.
  Then we used LEDC channel 0 to work with timer 0 and PIn 18 (Servo PIN).
  The code then keeps on generating pulse correspond to duty cycle for angles 0, 45, 90, 135 and 180 degree
  after delay of 1.5 seconds. Which  positions the servo at these angles.
  The process keeps on repeating and servo keeps on changing its position after every 1.5 seconds.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/ledc.h"
#include "sdkconfig.h"
#include "esp_log.h"

//Variables to be used by Servo Motor
// Delay to wait wach Servo Stop (milliseconds)
#define SERVO_DELAY 1500
// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33
#define SERVO_PIN 18
// Servo Stop Angles
#define SERVO_ANGLE_COUNT 8
const int servo_angles[SERVO_ANGLE_COUNT] = { 0, 45, 90, 135, 180, 135, 90, 45 };
// create servo object to control a servo

static char tag[] = "servo1";

void moveServo_task(void *ignore) {
	int bitSize = 15;
	int minValue = 600;  // microseconds (uS)
	int maxValue = 3500; // microseconds (uS)
	int duty = (1 << bitSize) * minValue / 20000;
	int steps = 180;	// Total Degrees

	//Setup 50Hz Wave using Timer 0
	ledc_timer_config_t timer_conf;
	timer_conf.duty_resolution = LEDC_TIMER_15_BIT;
	timer_conf.freq_hz = 50;
	timer_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
	timer_conf.timer_num = LEDC_TIMER_0;
	ledc_timer_config(&timer_conf);

	//Setup LECD Channel 0 to work with Timer 0 on GPIO 16
	ledc_channel_config_t ledc_conf;
	ledc_conf.channel = LEDC_CHANNEL_0;
	ledc_conf.duty = duty;
	ledc_conf.gpio_num = SERVO_PIN;
	ledc_conf.intr_type = LEDC_INTR_DISABLE;
	ledc_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
	ledc_conf.timer_sel = LEDC_TIMER_0;
	ledc_channel_config(&ledc_conf);
	int current_angle_index = 0;

	while (1) {
		//calculate Change in Duty Cycle with respect to original
		int degree = servo_angles[current_angle_index];
		int delta = degree * (maxValue - minValue) / steps;
		//Update New Duty Cycle to be used at Channel 0
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty + delta);
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
		//Increment Angle Index
		current_angle_index++;
		//If Angle Index exceed maximum count reset to index 0
		if (current_angle_index >= SERVO_ANGLE_COUNT)
			current_angle_index = 0;
		//Display at Debug Port
		ESP_LOGD(tag, "Moving Servo at Angle: %d", degree);
		ESP_LOGD(tag, "Duty Cycle is: %d", duty + delta);
		//Delay for required Time (SERVO_DELAY)
		vTaskDelay(SERVO_DELAY / portTICK_PERIOD_MS);
	} // End loop forever

	vTaskDelete(NULL);
}

void app_main() {
	xTaskCreate(&moveServo_task, "sweepServo_task", 2048, NULL, 5, NULL);
	printf("servo sweep task  started\n");
}
