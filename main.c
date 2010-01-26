/**
 Copyright (C) 2008-2009
 * OBDuinoMega  (Requres ATmega1280 for some features, can
 * build for Atmega328 if features are turned off)
 Main coding/ISO/ELM: Fr�d�ric (aka Magister on ecomodder.com)
 LCD part: Dave (aka dcb on ecomodder.com), optimized by Fr�d�ric
 ISO Communication Protocol: Russ, Antony, Mike
 Features: Mike, Antony
 Bugs & Fixes: Antony, Fr�d�ric, Mike
 Mega support added by Jonathan Oxer <jon@oxer.com.au>
 
 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc.,
 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 */

#undef int
#include "Arduino/WProgram.h"
#include "project_defs.h"
#include <stdio.h>
#include <limits.h>
#include "main.h"
#include <avr/pgmspace.h>
#include "Common.h"
#include "Comms.h"
#include "ELMComms.h"
#include "Memory.h"
#include "Menu.h"
#include "Display.h"
#include "LCD.h"
#include "Host.h"
//#include "Menu2.h"



byte logActive = 0;  // Flag used in logging state machine


// Use the TinyGPS library to parse GPS data, but only if GPS is enabled
#ifdef ENABLE_GPS
#include <TinyGPS.h>
TinyGPS gps;
float gpsFLat, gpsFLon;
unsigned long gpsAge, gpsDate, gpsTime, gpsChars;
int gpsYear;
byte gpsMonth, gpsDay, gpsHour, gpsMinute, gpsSecond, gpsHundredths;
// Include the floatToString helper
#include "floatToString.h"
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

#ifdef useECUState
boolean oldECUconnection;  // Used to test for change in ECU connection state
#endif


__attribute__ ((OS_main)) int main(void)
{
	init();

	setup();
    
	for (;;)
		loop();
        
	return 0;
}

/**
 * Initialization
 */
void setup()                    // run once, when the sketch starts
{
#ifdef HOST
  HOST.begin(HOST_BAUD_RATE);
  //pinMode(buttonGnd, OUTPUT);
  //digitalWrite(buttonGnd, LOW);
#endif

//  hostPrintLn("OBDuinoMega starting up");
#ifdef DEBUG
//  hostPrintLn("*********** DEBUG MODE *************");
#endif

#ifdef ENABLE_VDIP
//  initVdip();
#endif

#ifdef ENABLE_GPS
  initGps();
#endif

  // buttons init
//  hostPrint(" * Initialising buttons         ");
  menu_init_buttons();
  hostPrintLn("[OK]");

  // load parameters
  params_load();  // if something is wrong, default parms are used
  delay(100);
	calc_init();

  // LCD pin init
  hostPrint(" * Initializing LCD             ");
  lcd_setBrightness();
  params_t params = getParameters();

  lcd_init(params.contrast);
  lcd_print_P(PSTR("OBDuinoMega"));
  hostPrintLn("[OK]");
#ifdef ELM
elm_init();
#endif

//#ifdef carAlarmScreen
//  hostPrint(" * Car-alarm screen setup       ");
//   refreshAlarmScreen = true;
//   hostPrintLn("[OK]");
//#endif      

  // check supported PIDs
  //hostPrint(" * Checking supported PIDs      ");

  check_supported_pids(&tempLong);
  hostPrintLn("[OK]");

  lcd_cls();
  setOldTime();
  
  #ifdef ENABLE_VDIP
  //hostPrint(" * Attaching logging ISR        ");
  // Interrupt triggered by pressing "log on/off" button
  attachInterrupt(1, modeButton, FALLING);
  hostPrintLn("[OK]");
  #endif
  
  #ifdef ENABLE_PWRFAILDETECT
  // Interrupt triggered by falling voltage on power supply input
  hostPrint(" * Attaching powerfail ISR      ");
  attachInterrupt(0, powerFail, FALLING);
  hostPrintLn("[OK]");
  #endif
  //hostPrintLn("******** Startup completed *********");
}


/**
 * Main loop
 */
void loop()
{
  hostPrintLn("main");
  
  #ifdef MEGA
  // Process any commands from the host
  processHostCommands();
  #endif
  
  #ifdef ENABLE_VDIP
  // Echo data from the VDIP back to the host
  processVdipBuffer();

  char vdipBuffer[160];
  PString logEntry( vdipBuffer, sizeof( vdipBuffer ) ); // Create a PString object called logEntry
  char valBuffer[15];  // Buffer for converting numbers to strings before appending to vdipBuffer
  #endif
  
  #ifdef ENABLE_GPS
  processGpsBuffer();
  if( logActive == 1 )
  {
    gps.f_get_position( &gpsFLat, &gpsFLon, &gpsAge );
    //gps.get_datetime( &gpsDate, &gpsTime, &gpsAge );
    //gps.crack_datetime( &gpsYear, &gpsMonth, &gpsDay, &gpsHour, &gpsMinute, &gpsSecond, &gpsHundredths, &gpsAge );

    //char valBuffer[15];

    // Latitude
    floatToString(valBuffer, gpsFLat, 5);
    logEntry += valBuffer;
    logEntry += ",";
    // Longitude
    floatToString(valBuffer, gpsFLon, 5);
    logEntry += valBuffer;
    logEntry += ",";
    // Altitude (meters)
    floatToString(valBuffer, gps.f_altitude(), 0);
    logEntry += valBuffer;
    logEntry += ",";
    // Speed (km/h)
    floatToString(valBuffer, gps.f_speed_kmph(), 0);
    logEntry += valBuffer;
  }
  #endif

  #ifdef useECUState
    #ifdef DEBUG
      ECUconnection = true;
      has_rpm = true;  
    #else
      ECUconnection = verifyECUAlive();
    #endif

  if (oldECUconnection != ECUconnection)
  {
    if (ECUconnection)
    {
      do_engine_on();
    }
    else  // Car is off
    {
      #ifdef do_ISO_Reinit
        ISO_InitStep = 0;
      #endif

      do_engine_off();
    }  
    oldECUconnection = ECUconnection;
  } 
  if (ECUconnection)
  {
    // this read and assign vss and maf and accumulate trip data
    accu_trip();
  
    // display on LCD
	displayPids();
  }
  else
  {
    #ifdef carAlarmScreen
      // ECU is off so print ready screen instead of PIDS while we wait for ECU action
      displayAlarmScreen();
    #else
	displayPids();
    #endif

  }    
#else


  has_engine_started_or_stopped();

  #ifdef carAlarmScreen
    displayAlarmScreen();
  #else
  // this read and assign vss and maf and accumulate trip data
  	accu_trip();
	displayPids();
  #endif
#endif

  // test buttons
  test_buttons();

  #ifdef ENABLE_VDIP
    char str[STRLEN];
  // Get PIDs we want to store on the memory stick
  if( logActive == 1 )
  {
    for(byte i=0; i < logPidCount; i++) {
      logEntry += ",";
      if (get_pid( logPid[i], str, &tempLong))
        logEntry += tempLong;
    }
  }

  writeLogToFlash();
  #endif
}




