#ifndef Memory_h
#define Memory_h


#include "LCD.h"
#include "Display.h"
#include "Menu.h"

void params_save();
void params_load();
int memoryTest();

// How many characters across for the LCD (must be at least sixteen)
extern const byte LCD_width;
// Calculate the middle point of the LCD display width
extern const byte LCD_split;
//Calculate how many PIDs fit on a data screen (two per line)
extern const byte LCD_PID_count;

// parameters
// each trip contains fuel used and distance done
typedef struct
{
  unsigned long dist;   // in cm
  unsigned long fuel;   // in �L
  unsigned long waste;  // in �L
}
trip_t;

// each screen contains n PIDs (two per line)
typedef struct
{
  byte PID[16];
}
screen_t;


typedef struct
{
  byte contrast;       // we only use 0-100 value in step 20
  byte use_metric;     // 0=rods and hogshead, 1=SI
  boolean use_comma;   // When using metric, also use the comma decimal separator
  byte per_hour_speed; // speed from which we toggle to fuel/hour (km/h or mph)
  byte fuel_adjust;    // because of variation from car to car, temperature, etc
  byte speed_adjust;   // because of variation from car to car, tire size, etc
  byte eng_dis;        // engine displacement in dL
  unsigned int gas_price; // price per unit of fuel in 10th of cents. 905 = $0.905
  unsigned int  tank_size;   // tank size in dL or dgal depending of unit
  byte OutingStopOver; // Allowable stop over time (in tens of minutes). Exceeding time starts a new outing.
  byte TripStopOver;   // Allowable stop over time (in hours). Exceeding time starts a new outing.
  trip_t trip[3];        //NBTRIP trip0=tank, trip1=a trip HACK
  screen_t screen[NBSCREEN];  // screen
}
params_t;


params_t getParameters();

extern params_t params;

#endif
