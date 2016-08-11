/******************************************************************************
 * @file
 * @brief Simple GPS periodic fix application for the TDxxxx RF modules.
 * @author Telecom Design S.A.
 * @version 1.1.0
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2014-2015 Telecom Design S.A., http://www.telecomdesign.fr</b>
 ******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Telecom Design SA has no
 * obligation to support this Software. Telecom Design SA is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Telecom Design SA will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
  ******************************************************************************/

#include "config.h"

#include <stdint.h>
#include <stdbool.h>

#include <efm32.h>

#include <td_core.h>
#include <td_uart.h>
#include <td_printf.h>
#include <td_stream.h>
#include <td_flash.h>
#include <td_scheduler.h>
#include <td_watchdog.h>
#include <td_gpio.h>
#include <td_utils.h>
#include <td_measure.h>

#include <td_sensor.h>
#include <sensor_data_geoloc.h>

#include <td_accelero.h>
#include <td_geoloc.h>
#include <td_sigfox.h>

#include <td_config.h>

/*******************************************************************************
 ******************************  DEFINES ****************************
 ******************************************************************************/

/** Flash variable version ID */
#define VARIABLES_VERSION 0

// Interval at which position is reported (in seconds)
#define FIX_INTERVAL 1*3600 // Send a Sigfox message every 1 hour

/** Acceptable minimum horizontal accuracy, 800 to be very accurate */
#define FIX_HDOP 800

/** Boot monitoring, 1 to enable */
#define BOOT_MONITORING 0

/** Keepalive monitoring interval in hours, 0 to disable
 * If you wish to send a keepalive frame remember to add a scheduler as well in the setup function */
#define KEEPALIVE_INTERVAL 0

#define DEBUG_USE_PRINTF 0 //trace on serial
#if DEBUG_USE_PRINTF
	#define DEBUG_PRINTF(...) tfp_printf(__VA_ARGS__);
#else
	#define DEBUG_PRINTF(...)
#endif

TD_GEOLOC_Fix_t CurrentGPSPosition;
int timeout;
uint8_t fixIntervalSchedulerId;

//Configure the downlink request every maxCounter sigfox message
int downlinkCounter;
int downlinkMaxCounter;
bool encrypt = false;
int gpsSleepMode = TD_GEOLOC_OFF;
int precision = TD_GEOLOC_2D_FIX;

/*******************************************************************************
 ******************************  GLOBAL FUNCTIONS  ****************************
 ******************************************************************************/

void encryption(uint8_t * bytes, uint8_t * cryptMessage, int size){

	//Here you can insert your own encryption method - This function does nothing

	int i=0;

	for(i=0;i<size;i++){
		cryptMessage[i] = bytes[i];
	}
}

/***************************************************************************//**
 * @brief
 *  GPS fix callback
 *
 * @param[in] fix
 *   The GPS fix data structure.
 *
 * @param[in] timeout
 *   Flag that indicates whether a timeout occurred if set to true.
 ******************************************************************************/
static void GPSFix(TD_GEOLOC_Fix_t * fix, bool timeout)
{
	int i;
	uint8_t bytes[12];
	uint8_t cryptMessage[12];
	uint8_t timeoutMessage[1];

	unsigned long latitude, longitude;
	char latitude_direction, longitude_direction;
	int hdop, nbSat, acqTime, speed, size;

	//Message init - Set to zero not to get random unwanted values
	for(i=0;i<12;i++){
		bytes[i] = 0x00;
	}

	//
	if (fix->type >= precision && fix->hard.rtc_calibrated) {
		uint32_t mv = TD_MEASURE_VoltageTemperatureExtended(false);

		DEBUG_PRINTF("Voltage: %d\r\n", mv);
		DEBUG_PRINTF("Lat: %d - %08lX\r\n", fix->position.latitude,  fix->position.latitude);
		DEBUG_PRINTF("Long: %d - %08lX\r\n", fix->position.longitude, fix->position.longitude);
		DEBUG_PRINTF("Hdop: %d \r\n", fix->quality.hdop);
		DEBUG_PRINTF("nb sat: %d \r\n", fix->quality.sat);
		DEBUG_PRINTF("acquisition time: %d \r\n", fix->duration);
		DEBUG_PRINTF("speed: %d \r\n", fix->speed.speed_kmh);

		//Stop GPS
		TD_GEOLOC_StopFix(gpsSleepMode);

		//Now we add the received data in the bytes array:

		//Latitude
		//Hint: divid the return value by 10 will make it understandable by Sigfox backend
		if (fix->position.latitude < 0) {
			  latitude = (int32_t)(-1)* ((int32_t)fix->position.latitude / 10) ;
			  latitude_direction = 'S';
			  latitude = latitude| 0x80000000;
		} else {
			  latitude = fix->position.latitude / 10;
			  latitude_direction = 'N';
		}
		bytes[0] = (latitude >> 24) & 0xFF;
		bytes[1] = (latitude >> 16) & 0xFF;
		bytes[2] = (latitude >> 8) & 0xFF;
		bytes[3] = latitude & 0xFF;

		//Longitude
		if (fix->position.longitude < 0) {
			  longitude = (int32_t)(-1)* ((int32_t)fix->position.longitude / 10) ;
			  longitude_direction = 'W';
			  longitude = longitude| 0x80000000;
		} else {
			  longitude = fix->position.longitude / 10;
			  longitude_direction = 'E';
		}
		bytes[4] = (longitude >> 24) & 0xFF;
		bytes[5] = (longitude >> 16) & 0xFF;
		bytes[6] = (longitude >> 8) & 0xFF;
		bytes[7] = longitude & 0xFF;

		//Battery
		for(i=0; i<255; i++){
			if(mv>=i*15 && mv<=(i+1)*15){
				bytes[8] = bytes[8] |  i;
			}else if(mv>3825){
				bytes[8] = bytes[8] | 0xFF;
			}
		}

		//Hdop
		/*
		 * On the byte 9, we will encode both quality (hdop) and number of satellites
		 * 		  h h s s           (h for hdop and s for the number of satellites)
		 * Byte : _ _ _ _   _ _ _ _
		 */
		hdop = fix->quality.hdop / 100;
		if(hdop > 5){
			//Set the 2 first bits to 3
			bytes[9] = 0xC0;
		} else if(hdop >= 2 && hdop <=5){
			//Set the 2 first bits to 2
			bytes[9] = 0x80;
		} else if(hdop >= 1 && hdop <2){
			//Set the 2 first bits to 1
			bytes[9] = 0x04;
		}

		//Satellites number
		nbSat = fix->quality.sat;
		if(nbSat >=8){
			bytes[9] = bytes[9] | 0x30;
		}else if(nbSat >=6 && nbSat <= 8){
			bytes[9] = bytes[9] | 0x20;
		}else if(nbSat >=4 && nbSat <= 6){
			bytes[9] = bytes[9] | 0x10;
		}

		//Acquisition time
		/*
		 * On the byte 10, we will encode both acquisition (a) time and the speed (s)
		 * 		  a a a a   s s s s
		 * Byte : _ _ _ _   _ _ _ _
		 */
		uint8_t mask;
		acqTime = fix->duration;
		for(i=0; i<15; i++){
			if(acqTime>=i*5 && acqTime<=(i+1)*5){
				mask = i << 4 ;
				bytes[10] = bytes[10] | mask;
			}else if(acqTime>75){
				bytes[10] = bytes[10] | 0xF0;
			}
		}

		//Speed
		speed = fix->speed.speed_kmh;
		for(i=0; i<15; i++){
			if(speed>=i*5 && speed<=(i+1)*5){
				bytes[10] = bytes[10] |  i;
			}else if(speed>75){
				bytes[10] = bytes[10] | 0x0F;
			}
		}

		//DEBUG
		size = sizeof(bytes)/sizeof(bytes[0]);
		for(i=0;i<size;i++){
			DEBUG_PRINTF("bytes[%d]: %x\r\n", i, bytes[i]);
		}
		//TD_GEOLOC_PrintfFix(fix);
		//encryption(&bytes, &cryptMessage, size);
		for(i=0;i<size;i++){
			DEBUG_PRINTF("crypt[%d]: %x\r\n", i, cryptMessage[i]);
		}

		//Send data
		/*
		 * If downlinkMaxCounter is reached, send a downlink request with the sigfox message
		 */
		//DEBUG_PRINTF("Downlink counter: %d / %d\r\n", downlinkCounter, downlinkMaxCounter);
		if(downlinkCounter >= downlinkMaxCounter){
			TD_SIGFOX_SendV1(MODE_FRAME, false,encrypt?cryptMessage:bytes, size, 2, true, false);
			downlinkCounter = 0;
		}else{
			TD_SIGFOX_Send(encrypt?cryptMessage:bytes, size, 2);
			downlinkCounter++;
		}

	} else if (timeout) {
		//If no GPS fix has been found, we still send the voltage
		uint32_t mv = TD_MEASURE_VoltageTemperatureExtended(false);
		DEBUG_PRINTF("Voltage: %d\r\n", mv);
		for(i=0; i<255; i++){
			if(mv>=i*15 && mv<=(i+1)*15){
				timeoutMessage[0] = timeoutMessage[0] |  i;
			}else if(mv>3825){
				timeoutMessage[0] = timeoutMessage[0] | 0xFF;
			}
		}
		//Set the device in its sleep mode
		TD_GEOLOC_StopFix(gpsSleepMode);
		DEBUG_PRINTF("Fix Timeout\r\n");
		DEBUG_PRINTF("Time: %d \r\n", fix->duration);
		TD_GEOLOC_PrintfFix(fix);
		DEBUG_PRINTF("Downlink counter: %d / %d\r\n", downlinkCounter, downlinkMaxCounter);
		if(downlinkCounter >= downlinkMaxCounter){
			TD_SIGFOX_SendV1(MODE_FRAME, false, timeoutMessage, 1, 2, true, false);
			downlinkCounter = 0;
		}else{
			TD_SIGFOX_Send((uint8_t *) timeoutMessage, 1, 2);
			downlinkCounter++;
		}



	}
}

/***************************************************************************//**
 * @brief
 *   Call-back function for SIGFOX downlink.
 *
 * @param[in] rx_frame
 *   Pointer to the received frame or null if timeout occurred.
 *
 * @param[in] length
 *   Length in bytes of the received frame.
 *
 * @return
 *   Returns the buffer length if OK, -1 if an error occurred.
 ******************************************************************************/
static int DownlinkCallback(uint8_t *rx_frame, uint8_t length)
{
	int i;
	int fixInterval;
	DEBUG_PRINTF("Downlink...\r\n");
	if (rx_frame == 0) {
		// Finished receiving
		//TD_SIGFOX_DOWNLINK_SetUserCallback(0);
		DEBUG_PRINTF("RX END\r\n");
		// Done
		return 1;
	} else {
		if (length == 0) {
			// Start receiving
			DEBUG_PRINTF("RX BEGIN\r\n");
			// Done
			return 1;
		}
		// Received one good frame
		tfp_dump("RX=", rx_frame, length);
		DEBUG_PRINTF("rx_frame: %d - %x - %s\r\n", rx_frame, rx_frame, rx_frame);
		for(i=0;i<8;i++){
			DEBUG_PRINTF("rx_frame[%d]: %x - %d\r\n", i, rx_frame[i], rx_frame[i]);

		}

		//Here is the GPS timeout value meaning the maximum time you will try to get a GPS fix
		timeout = rx_frame[0];
		DEBUG_PRINTF("timeout: %d (in seconds)\r\n", timeout);

		//The fixInterval (in hours) is the time interval to send a message
		fixInterval = rx_frame[1];
		if(fixInterval>3 && fixInterval<48){
			DEBUG_PRINTF("fixInterval: %d (in hours)\r\n", fixInterval);
			TD_SCHEDULER_SetInterval(fixIntervalSchedulerId, fixInterval * 3600,0,0);
			gpsSleepMode = TD_GEOLOC_OFF;
		}
		if(fixInterval>0 && fixInterval<=3){
			DEBUG_PRINTF("fixInterval: %d (in hours)\r\n", fixInterval);
			TD_SCHEDULER_SetInterval(fixIntervalSchedulerId, fixInterval * 3600,0,0);
			gpsSleepMode = TD_GEOLOC_HW_BCKP;
		}

		//you can add restriction for the max number of uplink to occur before requesting a downlink here
		//When 255, it will always be true
		//You can easily calculate the interval time for downlinks doing '(fixInterval * downlinkMaxCounter) + 1'
		if(rx_frame[2]<48){
			downlinkMaxCounter = rx_frame[2];
			DEBUG_PRINTF("downlinkMaxCounter: %d (in hours)\r\n", downlinkMaxCounter);
		}

		// Done
		return 1;

	}
}

/***************************************************************************//**
 * @brief
 *  Start fixing periodically.
 *
 * @param[in] arg
 *  Generic argument set by the user that is passed along to the callback
 *  function.
 *
 * @param[in] repeat_count
 *  Updated repeat count, decremented at each timer trigger, unless it is an
 *  infinite timer.
 ******************************************************************************/
static void StartFixing(uint32_t arg, uint8_t repeat_count)
{
	DEBUG_PRINTF("Start fixing\r\n");
	TD_GEOLOC_TryToFix(TD_GEOLOC_NAVIGATION, timeout, GPSFix);
}

/***************************************************************************//**
 * @brief
 *  User Setup function.
 ******************************************************************************/
void TD_USER_Setup(void)
{
	TD_UART_Options_t options = {LEUART_DEVICE, LEUART_LOCATION, 9600, 8, 'N',
		1, false};

	// Open an I/O stream using LEUART0
	TD_UART_Open(&options, TD_STREAM_RDWR);

	// Use a 64 s automatic watchdog
	TD_WATCHDOG_Init(64);
	TD_WATCHDOG_Enable(true, true);
	TD_SENSOR_Init(SENSOR_TRANSMITTER, 0, 0);

	// Geoloc and accelerometer initialization
	TD_GEOLOC_Init();
	TD_ACCELERO_Init();

	//Fix timeout
	timeout = 60;

	//Init downlink counter
	downlinkCounter = 0;
	//Once every x uplink
	downlinkMaxCounter = 23; //One downlink every day

#if BOOT_MONITORING

	// Will only send a boot monitor frame on NEXT reboot
	TD_SENSOR_MonitorBoot(true, 0);
#endif

#if KEEPALIVE_INTERVAL > 0

	// Send a keep-alive frame immediately, then at given interval
	TD_SENSOR_MonitorKeepAlive(true, KEEPALIVE_INTERVAL);
#endif

	// Start fixing right now
	StartFixing(0, 0);

	// Start the fix infinite timer
	fixIntervalSchedulerId = TD_SCHEDULER_Append(FIX_INTERVAL, 0, 0, TD_SCHEDULER_INFINITE, StartFixing, 0);

	TD_SIGFOX_DOWNLINK_SetUserCallback(DownlinkCallback);
}

/***************************************************************************//**
 * @brief
 *   User loop function.
 ******************************************************************************/
void TD_USER_Loop(void)
{
	// Process Sensor events
	TD_SENSOR_Process();

	// Process geoloc events
	TD_GEOLOC_Process();

	// Process downlink events
	TD_SIGFOX_DOWNLINK_Process();
}
