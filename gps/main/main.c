/* GPS Working Code
 * Name: Alotaibi, Aali
 * Date: 11/11/19
 * Purpose: GPS module for EE 408
 Latitude / Longitude Data Format
 $GNRMC,054404.00,A,4302.56296,N,08754.35012,W,0.020,,181219,,,D*78

 This code scans the incoming serial data from GPS device byte by byte. It keeps on
 looking the $GNRMC or $GPRMC string as shown in above string. after detecting correct start of
 sentence, code gets the latitude and longitude strings from after 4 the and 6the comma occurring in sentence.
 Then these strings are converted into float numbers and direction is corrected as +/- depending on E/W and S/N
 information obtained. Finally the obtained coordinates are displayed on serial terminal.

 */

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "sdkconfig.h"

#define GPS_PIN_TXD (17)
#define GPS_PIN_RXD (16)

#define BUF_SIZE (256)

static char* GPS_TAG = "gps";

//GPS Related Variables
int Gpsdata;             // for incoming serial data
unsigned int finish =0;  // indicate end of message
unsigned int pos_cnt=0;  // position counter
unsigned int lat_cnt=0;  // latitude data counter
unsigned int log_cnt=0;  // longitude data counter
unsigned int flg    =0;  // GPS flag
unsigned int com_cnt=0;  // comma counter
char lat[20];            // latitude array
char lg[20];             // longitude array
float nlat, nlng;
//Memory allocation for GPS data buffer
uint8_t data[BUF_SIZE];

//Parse GPS Data
void Parse_GPS_Data(int data_in)
{
	if(finish==0){
		Gpsdata = data_in;
		flg = 1;
		if( Gpsdata=='$' && pos_cnt == 0)   // finding GPRMC header
			pos_cnt=1;
		if( Gpsdata=='G' && pos_cnt == 1)
			pos_cnt=2;
		if( Gpsdata=='P' && pos_cnt == 2)
			pos_cnt=3;
		if( Gpsdata=='N' && pos_cnt == 2)
			pos_cnt=3;
		if( Gpsdata=='R' && pos_cnt == 3)
			pos_cnt=4;
		if( Gpsdata=='M' && pos_cnt == 4)
			pos_cnt=5;
		if( Gpsdata=='C' && pos_cnt==5 )
			pos_cnt=6;
		if(pos_cnt==6 &&  Gpsdata ==','){   // count commas in message
			com_cnt++;
			flg=0;
		}
		if(com_cnt==3 && flg==1){
			lat[lat_cnt++] =  Gpsdata;         // latitude
			flg=0;
		}
		//Check for Negative Latitude
		if(com_cnt==4 && flg==1){
			nlat = atof(lat)/100.0;
			if(Gpsdata == 'S'){
				nlat *= -1;
			}
			lat[lat_cnt++] =  Gpsdata;         // Negative latitude
			flg=0;
		}
		if(com_cnt==5 && flg==1){
			lg[log_cnt++] =  Gpsdata;         // Longitude
			flg=0;
		}
		//Check for Negative Longitude
		if(com_cnt==6 && flg==1){
			nlng = atof(lg) / 100.0;
			if(Gpsdata == 'W'){
				nlng *= -1;
			}
			lg[lat_cnt++] =  Gpsdata;         // Negative Longitude
			flg=0;
		}

		if( Gpsdata == '*' && com_cnt >= 6){
			com_cnt = 0;                      // end of GPRMC message
			lat_cnt = 0;
			log_cnt = 0;
			flg     = 0;
			finish  = 1;
			pos_cnt = 0;
		}
	}
}

//Read GPS data and feed for parsing
void feed_gps_data(){
	//uint8_t *data = (uint8_t*) malloc(BUF_SIZE);
	//uint8_t data[BUF_SIZE];
	int len = uart_read_bytes(UART_NUM_2, data, BUF_SIZE, 1000 / portTICK_RATE_MS);
	for(int i=0; i<len; i++){
		Parse_GPS_Data(data[i]);
	}
	//printf("%s",data);
	//free (data);
}

//Configure the the serial ports
void configure_uart(uart_port_t port,  int baud_rate, int PIN_RX, int PIN_TX){
	uart_config_t uart_config = {.baud_rate = baud_rate,
			.data_bits = UART_DATA_8_BITS,
			.parity = UART_PARITY_DISABLE,
			.stop_bits = UART_STOP_BITS_1,};

	uart_param_config(port, &uart_config);
	uart_set_pin(UART_NUM_2, PIN_TX, PIN_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	uart_driver_install(port, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);
}

//Main ESP32 Function
void app_main() {
	//Initialize Serial Ports
	configure_uart(UART_NUM_0,  115200, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	configure_uart(UART_NUM_2,  9600, GPS_PIN_RXD, GPS_PIN_TXD);



	//Read Data from GPS and write to USB Serial Port
	ESP_LOGI(GPS_TAG, "Starting ....");
	while (1) {
		feed_gps_data();
		//printf("Latitude  : %s \n", lat);
		ESP_LOGI(GPS_TAG, "Numeric Latitude  : %f", nlat);
		//printf("Longitude : %s \n", lg);
		ESP_LOGI(GPS_TAG, "Numeric Longitude : %f\n", nlng);
		finish = 0;
	}
}
