
#ifdef ENABLE_VDIP
// Vinculum setup
#define VDIP_RESET      12  // Pin for reset of VDIP module (active low)
#define VDIP_STATUS_LED 11  // LED to show whether a file is open
#define VDIP_WRITE_LED  10  // LED to show when write is in progress
#define VDIP_RTS_PIN     9  // Check if the VDIP is ready to receive. Active low
#define LOG_LED 4           // Pin connected to the "log" status LED (active high)
#define LOG_BUTTON 3
#define LOG_BUTTON_INT 1
#define POWER_SENSE_PIN 2
#define POWER_SENSE_INT 0
byte logPid[] = {
  LOAD_VALUE,
  COOLANT_TEMP,
  ENGINE_RPM,
  VEHICLE_SPEED,
  TIMING_ADV,
  INT_AIR_TEMP,
  MAF_AIR_FLOW,
  THROTTLE_POS,
  FUEL_RAIL_P,
  FUEL_LEVEL,
  BARO_PRESSURE,
  AMBIENT_TEMP,
  FUEL_CONS,
  BATT_VOLTAGE
  };
byte logPidCount = sizeof(logPid) / sizeof(logPid[0]);


#define LOG_INTERVAL   1000      // Milliseconds between log entries on the memory stick
// We need the PString library to create a log buffer
#include <PString.h>
volatile unsigned long logButtonTimestamp = 0;
#endif


void powerFail();
void initVdip();
void processVdipBuffer();
void modeButton();
