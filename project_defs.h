
// Compilation modifiers:
// The following will cause the compiler to add or remove features from the
// OBDuino build this keeps the build size down, will not allow 'on the fly'
// changes. Some features are dependent on other features.

// Comment for normal build
// Uncomment for a debug build
//#define DEBUG

// Comment to build for a Duemilanove or compatible (ATmega328P)
// Uncomment to build for a Mega (ATmega1280)
//#define MEGA

// Comment for normal build
// Uncomment to enable GPS. NOTE: requires that MEGA be set as well
//#define ENABLE_GPS

// Comment for normal build
// Uncomment to enable VDIP1 flash storage module. NOTE: requires that MEGA be set as well
//#define ENABLE_VDIP

// Comment for normal build
// Uncomment to enable power-fail detection (currently only useful if VDIP is enabled)
//#define ENABLE_PWRFAILDETECT

// Comment to use MC33290 ISO K line chip
// Uncomment to use ELM327
//#define ELM

// Comment out to use only the ISO 9141 K line
// Uncomment to also use the ISO 9141 L line
// This option requires additional wiring to function!
// Most newer cars do NOT require this
//#define useL_Line

// Uncomment only one of the below init sequences if using ISO
//#define ISO_9141
//#define ISO_14230_fast
//#define ISO_14230_slow

// Comment out to just try the PIDs without need to find ECU 
// Uncomment to use ECU polling to see if car is On or Off 
//#define useECUState

// Comment out if ISO 9141 does not need to reinit
// Uncomment define below to force reinitialization of ISO 9141 after no ECU communication
// this requires ECU polling
//#define do_ISO_Reinit 

// Comment out to use the PID screen when the car is off (This will interfere with ISO reinit process)
// Uncomment to use the Car Alarm Screen when the car is off
//#define carAlarmScreen
