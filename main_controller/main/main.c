/***************************************************************************
 External UI Testing Program
 Wildfire Detection Team:
 Colin Wiese, Yushan Lin, Ian Sanford, Ali Alotaibi, Phillip Hiemenz
 Created 02/01/2020
 //========================================================================//
 This lab focuses on creating an External UI to display output and
 state logic, which the final design will incorporate

 The main loop will handle the next state logic and the current state
 logic, while the function external_update will display the state
 information to a laptop screen.

 The variables state and next_state are used to properly update the
 state logic, and 5 pushbuttons that will be read by the ESP32 will
 update these variables.
 //========================================================================//
 States and state variable value:
 ->Reset = 0x00
 -- This state is the initial state, and the state that the GPS value
 -- is chosen. This is due to the need for the GPS to be the state to
 -- update the display, while the initial two seconds this state is in
 -- is enough to update the GPS. All states except

 ->GPS = 0x01
 -- Updates the GPS on the display and then goes to the search state.
 -- The intermediary state between the reset and search state.

 ->Search = 0x02
 -- Search for fire state. The default state after initialization. If
 -- no buttons are pressed, this will be the state that the program will
 -- stay in. The display information for motor position, camera tilt,
 -- and the lines displaying the no error message are the outputs.
 -- The fire alert state and communication error state can be freely
 -- moved to and back from should the fire disappear or the communication
 -- error be resolved. Can also only move one-way to the reset state and
 -- the internal error state. The camera angle will add 45 degrees from an
 -- initial 45 degrees every two seconds, and reset at 360 degrees to zero.

 ->Fire Alert = 0x03
 -- This state will update the display on the fire alert, it functions
 -- similarly to the search state, except the camera angle is frozen.
 -- Otherwise it follows the same logic.

 ->Communication Error = 0x04
 -- This state will freeze the camera and motion information, such as the
 -- camera angle, the tilt angle, and the fire alert data. Until this is
 -- fixed, the last known information will be the only data on the screen.
 -- The program will wait as long as it takes to fix this, and go into the
 -- fire alert state or the search state based on what the inputs are.

 ->Internal Error = 0x05
 -- Once moved into this state, the error message and nothing else will
 -- be displayed. This program won't leave this state until the reset
 -- button is pressed.
 //========================================================================//
 Inputs:
 ->Pin 15 will be connected to the reset button.
 -- Priority #1

 ->Pin 4 will be connected to the internal error button.
 -- Priority #2

 ->Pin 17 will be connected to the communication error button.
 -- Priority #3

 ->Pin 2 will be connected to the fire alarm button
 -- Priority #4

 ->Pin 16 will be connected to the GPS button.
 -- Non-Priority (doesn't decide state logic)
 //========================================================================//
 Display:

 Line 1: -Tilt angle                     “(deg)”
 or                         "[N/A]"
 -And Motor angle           “(deg)”,
 or                        "[N/A]"

 Line 2: GPS location                    “[Option #1] deg”
 or                        “[Option #2] deg”

 Line 3: Error message                   “Good connection”
 or                        “Connection lost”

 Line 4: No error message                "[N/A]"
 or                        “Internal error detected”

 Line 5: No fire alert                   “nothing detected”
 or Fire alert             “fire alert”

 //========================================================================//
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"

#include "nvs_flash.h"
#include "driver/gpio.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

const char* MAIN_TAG = "main";

int state = 0;
int next_state = 0;

int clr = 0;
int GPS = 0;
int fire_alert = 0;
int comm_error = 0;
int intern_error = 0;

int clr_read = 0;
int GPS_read = 0;
int fire_alert_read = 0;
int comm_error_read = 0;
int intern_error_read = 0;

int cam_angle = 0;
int cam_print = 0;
int tilt_print = 21; //degrees out of 57 degrees

#define GPIO_INPUT_IO_0          15
#define GPIO_INPUT_IO_1          4
#define GPIO_INPUT_IO_2          17
#define GPIO_INPUT_IO_3          2
#define GPIO_INPUT_IO_4          16
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1) | (1ULL<<GPIO_INPUT_IO_2) |(1ULL<<GPIO_INPUT_IO_3) | (1ULL<<GPIO_INPUT_IO_4))

void external_update(void);
void init_gpio(void);

//========================================================================//

void app_main(void) {
	init_gpio();
	vTaskDelay(pdMS_TO_TICKS(2000));

	//====================================================================//
	//======================== Next State Logic ==========================//
	while (1) {
		//input reads
		clr_read = gpio_get_level(GPIO_INPUT_IO_0);
		intern_error_read = gpio_get_level(GPIO_INPUT_IO_1);
		comm_error_read = gpio_get_level(GPIO_INPUT_IO_2);
		fire_alert_read = gpio_get_level(GPIO_INPUT_IO_3);
		GPS_read = gpio_get_level(GPIO_INPUT_IO_4);

		if (clr_read == 0) {
			next_state = 0;
		} else {
			if (intern_error_read == 0) {
				next_state = 5;
			} else {
				if (comm_error_read == 0) {
					if (state == 0) {
						next_state = 1;
					} else if (state == 5) {
						next_state = 5;
					} else {
						next_state = 4;
					}
				} else {
					if (fire_alert_read == 0) {
						if (state == 0) {
							next_state = 1;
						} else if (state == 5) {
							next_state = 5;
						} else {
							next_state = 3;
						}
					} else {
						if (state == 0) {
							next_state = 1;
						} else if (state == 5) {
							next_state = 5;
						} else {
							next_state = 2;
						}
					}
				}
			}
		}

		//====================== End Next State Logic ========================//
		//====================================================================//
		//--------------------------------------------------------------------//
		//====================================================================//
		//======================= Current State Logic ========================//
		if (state == 0) {
			if (GPS_read == 0) { // GPS button press                  Non-Priority
				GPS = 1;
			} else {
				GPS = 0;
			}
			clr = 1;
			intern_error = 0;
			fire_alert = 0;
			comm_error = 0;
		} else if (state == 1) {
			clr = 0;
		} else if (state == 2) {
			clr = 0;
			fire_alert = 0;
			comm_error = 0;
			if (cam_angle <= 7) {
				cam_angle = cam_angle + 1;
			} else {
				cam_angle = 0;
			}
		} else if (state == 3) {             // fire alert state
			fire_alert = 1;
			comm_error = 0;
		} else if (state == 4) {             // communication error state
			comm_error = 1;
		} else {        // internal error state
			clr = 1;
			intern_error = 1;
			fire_alert = 0;
			comm_error = 0;
		}
		//===================== End Current State Logic ======================//
		//====================================================================//
		state = next_state;       //state update
		external_update();         //print update
	}
}

//========================================================================//
//=========================== Printing Logic =============================//

/* Uses current state logic outputs to print to the screen. This allows
 * the current state logic above to be simplified and easier to modify.
 *
 * The lines are printed in order of their lines. Also, the clear command
 * will clear the screen and keep cleared, if in reset state, or display
 * an error message on line 4.
 *
 * The other states will uses lines 1, 2, 3, and 5
 */

void external_update() {
	if (clr == 1) {
		//clear screen                            lines[1-5]
		if (state != 1) {
			ESP_LOGI(MAIN_TAG, "\n\n\n\n\n");
		} else if (state == 1) {  //display GPS         lines[1-5]
			ESP_LOGI(MAIN_TAG, " ");
			if (GPS == 1) {
				//display "[GPS Option #1] degrees"     line [2]
				ESP_LOGI(MAIN_TAG, "12.3456");
			} else {
				//display "[GPS Option #2] degrees"     line [2]
				ESP_LOGI(MAIN_TAG, "13.5791");
			}
			ESP_LOGI(MAIN_TAG, "\n\n\n");
		}
		if (intern_error == 1) {
			//display "internal error"              line [4]
			ESP_LOGI(MAIN_TAG, "\n\n\n");
			ESP_LOGI(MAIN_TAG, "internal error");
		}
	} else {
		if (cam_angle < 7) {
			cam_print = cam_angle * 45;
			//display "[cam_print] degrees"         line [1]
			ESP_LOGI(MAIN_TAG, "%d", cam_print);
		} else {
			//display "0 degrees"                   line [1]
			ESP_LOGI(MAIN_TAG, "315 degrees");
		}
		//display "[tilt_print] degrees"              line [1]
		ESP_LOGI(MAIN_TAG, "0 degrees");
		if (GPS == 1) {
			//display "[GPS Option #1] degrees"     line [2]
			ESP_LOGI(MAIN_TAG, "12.3456");
		} else {
			//display "[GPS Option #2] degrees"     line [2]
			ESP_LOGI(MAIN_TAG, "13.5791");
		}
		if (comm_error == 1) {
			//display "connection lost"             line [3]
			ESP_LOGI(MAIN_TAG, "connection lost");
		} else {
			//display "good connection"             line [3]
			ESP_LOGI(MAIN_TAG, "good connection");
		}
		//display "              "              line [4]
		ESP_LOGI(MAIN_TAG, "\n");
		if (fire_alert == 1) {
			//display "fire_alert"                  line [5]
			ESP_LOGI(MAIN_TAG, "fire_alert");
		} else {
			//display "nothing detected"            line [5]
			ESP_LOGI(MAIN_TAG, "nothing detected");
		}
	}
	vTaskDelay(pdMS_TO_TICKS(2000));
}

//========================= End Printing Logic ===========================//
//========================================================================//

void init_gpio(void) {
	// pin [15,2,0,4,16] setup
	// good low power settings for the other pins
	gpio_config_t io_conf;
	//bit mask of the pins, use GPIO4/5 here
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	//set as input mode
	io_conf.mode = GPIO_MODE_INPUT;
	//enable pull-up mode
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);
}

//========================================================================//
