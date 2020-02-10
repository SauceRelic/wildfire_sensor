#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/ledc.h"
#include "sdkconfig.h"
#include "esp_log.h"

#define NOP() asm volatile ("nop")

#define PIN_A 4
#define PIN_B 16
#define PIN_C 17
#define PIN_D 5

#define SPEED_RPM 100
#define REV_STEPS 200
#define STOP_TIME 500
const uint32_t step_delay = (60 * 1000* 1000) / (REV_STEPS * SPEED_RPM);
int step_number = 0;

unsigned long IRAM_ATTR micros();
void IRAM_ATTR delayMicroseconds(uint32_t us);
void stepMotor(int thisStep);
void step(int steps_to_move);

void app_main()
{
    gpio_pad_select_gpio(PIN_A);
    gpio_set_direction(PIN_A, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(PIN_B);
    gpio_set_direction(PIN_B, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(PIN_C);
    gpio_set_direction(PIN_C, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(PIN_D);
    gpio_set_direction(PIN_D, GPIO_MODE_OUTPUT);

    //int step_delay = 60L * 1000L / REV_STEPS / SPEED_RPM;

    while(1) {
    	for(int i=0; i<REV_STEPS; i+=25){
			step(25);
			vTaskDelay(STOP_TIME / portTICK_PERIOD_MS);
    	}
    	for(int i=REV_STEPS; i>0; i-=25){
			step(-25);
			vTaskDelay(STOP_TIME / portTICK_PERIOD_MS);
    	}
    }
}

void step(int steps_to_move)
{
	int direction = 0;
	int steps_left = abs(steps_to_move);
	if (steps_to_move > 0) { direction = 1; }
	if (steps_to_move < 0) { direction = 0; }
	while (steps_left > 0)
	{
		if (direction == 1)
		{
			step_number++;
			if (step_number == REV_STEPS) {
				step_number = 0;
			}
		}
		else
		{
			if (step_number == 0) {
				step_number = REV_STEPS;
			}
				step_number--;
		}
		// decrement the steps left:
		steps_left--;
		stepMotor(step_number % 4);
		//vTaskDelay(5 / portTICK_PERIOD_MS);
		delayMicroseconds(step_delay);
	}
}

void stepMotor(int thisStep)
{
    switch (thisStep) {
      case 0:  // 1010
        gpio_set_level(PIN_A, 1);
        gpio_set_level(PIN_B, 0);
        gpio_set_level(PIN_C, 1);
        gpio_set_level(PIN_D, 0);
      break;
      case 1:  // 0110
        gpio_set_level(PIN_A, 0);
        gpio_set_level(PIN_B, 1);
        gpio_set_level(PIN_C, 1);
        gpio_set_level(PIN_D, 0);
      break;
      case 2:  //0101
        gpio_set_level(PIN_A, 0);
        gpio_set_level(PIN_B, 1);
        gpio_set_level(PIN_C, 0);
        gpio_set_level(PIN_D, 1);
      break;
      case 3:  //1001
        gpio_set_level(PIN_A, 1);
        gpio_set_level(PIN_B, 0);
        gpio_set_level(PIN_C, 0);
        gpio_set_level(PIN_D, 1);
      break;
    }
}

unsigned long IRAM_ATTR micros()
{
    return (unsigned long) (esp_timer_get_time());
}
void IRAM_ATTR delayMicroseconds(uint32_t us)
{
    uint32_t m = micros();
    if(us){
        uint32_t e = (m + us);
        if(m > e){ //overflow
            while(micros() > e){
                NOP();
            }
        }
        while(micros() < e){
            NOP();
        }
    }
}
