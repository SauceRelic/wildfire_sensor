#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include <math.h>

// simulation variables
//
// offset to north from motor angle
static float compass_sim_offset = 69.420;


// log tags
static const char* MOTOR_TAG = "det_motor";
static const char* THERMAL_TAG = "det_thermal";
static const char* COMPASS_TAG = "det_compass";
static const char* MAIN_TAG = "det_main";

// TODO: remove after integration
void init_GPIO(void);

bool error_check(bool*);
void motor_move(int);
bool thermal_snapshot(float*);
void compass_read(float*, int);

void app_main(void)
{
	// variable declarations
	//
	// motor angle, in degrees
	static int motor_ang = 0;
	// fire angle, in degrees
	static float fire_ang = 0;
	// fire detected flag
	static bool fire_flag = false;
	// error flag
	static bool err_flag = false;

	// TODO: remove after integration
	init_GPIO();

	while(true)
	{
		fire_flag = false;
		fire_ang = 0;
		err_flag = 0;
		for(motor_ang=0; motor_ang<360; motor_ang+=45){
			if(error_check(&err_flag))
				break;

			// command motor to move to motor_ang
			motor_move(motor_ang);

			if(error_check(&err_flag))
				break;

			// command camera to take image, waits for result
			fire_flag = thermal_snapshot(&fire_ang);

			if(error_check(&err_flag))
				break;

			if(fire_flag){
				// read compass to determine bearing to north
				// TODO: remove motor dependency
				compass_read(&fire_ang, motor_ang);

				if(error_check(&err_flag))
					break;

				break;
			}
		}

		ESP_LOGI(MAIN_TAG, "returning data:\nerror = %s\nfire_flag = %s\nfire_ang = %f",
				err_flag ? "true" : "false",
				fire_flag ? "true" : "false",
				fire_ang);

		motor_move(0);
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
}

// TODO: remove after integration
// setup pins 5, 26, and 34 as pullup inputs
void init_GPIO(void)
{
	int pins[3] = {25, 26, 34};

	// default: GPIO function, pullup enabled, mode disabled
	// desired: GPIO function, pullup enabled, mode input
	for(int i=0; i<3; i++){
		ESP_ERROR_CHECK(gpio_set_direction(pins[i], GPIO_MODE_INPUT));
	}
}

bool error_check(bool* flag)
{
	bool err = gpio_get_level(34);
	*flag = err;
	return err;
}

void motor_move(int angle)
{
	ESP_LOGI(MOTOR_TAG, "running motor");
	// TODO: integrate motor controls
	vTaskDelay(pdMS_TO_TICKS(2000));
	ESP_LOGI(MOTOR_TAG, "motor position changed to %d degrees", angle);
}

bool thermal_snapshot(float* angle_ptr)
{
	float angle = 0;
	// TODO: integrate purethermal module commands, camera controls
	ESP_LOGI(THERMAL_TAG, "command sent to PureThermal");
	// send command to purethermal module to perform radiometric capture/analysis
	// wait for response from purethermal module
	vTaskDelay(pdMS_TO_TICKS(500));
	// extract flag and angle data received from purethermal module
	// TODO: flag will be 1 bit, using as simulated location with io
	int fire_flag = (gpio_get_level(25) << 1)
			     + gpio_get_level(26);

	// TODO: selecting angle based on "flag", will be extracted from received data
	switch(fire_flag){
	// no buttons, no fire
	case 0:
		angle = 0;
		break;
	// IO26, fire on far right
	case 1:
		angle = 23.5;
		break;
	// IO5, fire on far left
	case 2:
		angle = -23.5;
		break;
	// IO5+IO26, fire in center
	case 3:
		angle = 0;
		break;
	default:
		angle = 0;
	}

	*angle_ptr = angle;

	// log result
	ESP_LOGI(THERMAL_TAG, "response received: %s at angle %.2f",
			fire_flag ? "fire detected" : "no fire detected", angle);

	// TODO: can directly return after integration
	if(fire_flag != 0)
		return true;
	else
		return false;
}

// TODO: remove motor dependency
void compass_read(float* angle_ptr, int motor)
{
	// TODO: integrate digital compass
	vTaskDelay(pdMS_TO_TICKS(500));
	float angle_ref = fmod((float)motor + compass_sim_offset, 360);

	ESP_LOGI(COMPASS_TAG, "camera currently facing %.2f", angle_ref);
	float angle_therm = *angle_ptr;

	float angle = fmod(angle_ref + angle_therm, 360);
	ESP_LOGI(COMPASS_TAG, "fire at %.2f clockwise from north", angle);

	*angle_ptr = angle;
}
