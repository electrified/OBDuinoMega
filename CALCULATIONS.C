
#include "WProgram.h"
#include "Common.h"
#include "Comms.h"
#include "Calculations.h"
#include "Memory.c"
#include "LCD.h"
#include "Utilities.h"
#include "Display.h"
#include <avr/pgmspace.h>

// Time information
#define MILLIS_PER_HOUR    3600000L
#define MILLIS_PER_MINUTE    60000L
#define MILLIS_PER_SECOND     1000L
#define MINUTES_GRANULARITY 10


bool has_rpm = false;
bool engine_started = false;
long vss = 0;  // speed
long maf = 0;  // MAF


// some globals, for trip calculation and others
unsigned long engine_on, engine_off; //used to track time of trip.

void calc_init() {
  engine_off = engine_on = millis();
}
/*
 * accumulate data for trip, called every loop()
 */
void accu_trip()
{
  long tempLong;
  static byte min_throttle_pos=255;   // idle throttle position, start high
  byte throttle_pos;   // current throttle position
  byte open_load;      // to detect open loop
  char str[STRLEN];
  unsigned long delta_dist;
  unsigned long delta_time;

  // if we return early set MAF to 0
  maf = 0;
  
  // time elapsed
  getElapsedTime(&delta_time);

  // distance in cm
  // 3km/h = 83cm/s and we can sample n times per second or so with CAN
  // so having the value in cm is not too large, not too weak.
  // ulong so max value is 4'294'967'295 cm or 42'949 km or 26'671 miles
  if (!get_pid(VEHICLE_SPEED, str, &vss))
  {
    return; // not valid, exit
  }
  
  if(vss > 0)
  {
    delta_dist = (vss * delta_time) /36;
    // accumulate for all trips
    for(byte i = 0; i < NBTRIP; i++)
      params.trip[i].dist += delta_dist;
  }

  // if engine is stopped, we can get out now
  if(!has_rpm)
  {
    return;
  }

  // accumulate fuel only if not in DFCO
  // if throttle position is close to idle and we are in open loop -> DFCO

  // detect idle pos
  if (get_pid(THROTTLE_POS, str, &tempLong))
  {
    throttle_pos = (byte)tempLong;
  
    if(throttle_pos<min_throttle_pos && throttle_pos != 0) //And make sure its not '0' returned by no response in read byte function 
      min_throttle_pos=throttle_pos;
  }
  else
  {
    return;
  }
  
  // get fuel status
  if(get_pid(FUEL_STATUS, str, &tempLong))
  {
    open_load = (tempLong & 0x0400) ? 1 : 0;
  }
  else
  {
    return;
  }
 
  if(throttle_pos<(min_throttle_pos+4) && open_load)
  {
    maf = 0;  // decellerate fuel cut-off, fake the MAF as 0 :)
  }
  else
  {
    // check if MAF is supported
    if(is_pid_supported(MAF_AIR_FLOW, 0))
    {
      // yes, just request it
      maf = (get_pid(MAF_AIR_FLOW, str, &tempLong)) ? tempLong : 0;
    }
    else
    {
      getMafFromMap(str, &tempLong);
    }
    // add MAF result to trip
    // we want fuel used in �L
    // maf gives grams of air/s
    // divide by 100 because our MAF return is not divided!
    // divide by 14.7 (a/f ratio) to have grams of fuel/s
    // divide by 730 to have L/s
    // mul by 1000000 to have �L/s
    // divide by 1000 because delta_time is in ms

    // at idle MAF output is about 2.25 g of air /s on my car
    // so about 0.15g of fuel or 0.210 mL
    // or about 210 �L of fuel/s so �L is not too weak nor too large
    // as we sample about 4 times per second at 9600 bauds
    // ulong so max value is 4'294'967'295 �L or 4'294 L (about 1136 gallon)
    // also, adjust maf with fuel param, will be used to display instant cons
    maf = (maf * params.fuel_adjust)/100;
    updateFuelUsed(&delta_time);
  }
}

void getMafFromMap(char *str, long *tempLong) {
/*
      I just hope if you don't have a MAF, you have a MAP!!

       No MAF (Uses MAP and Absolute Temp to approximate MAF):
       IMAP = RPM * MAP / IAT
       MAF = (IMAP/120)*(VE/100)*(ED)*(MM)/(R)
       MAP - Manifold Absolute Pressure in kPa
       IAT - Intake Air Temperature in Kelvin
       R - Specific Gas Constant (8.314472 J/(mol.K)
       MM - Average molecular mass of air (28.9644 g/mol)
       VE - volumetric efficiency measured in percent, let's say 80%
       ED - Engine Displacement in liters
       This method requires tweaking of the VE for accuracy.
       */
      long imap, rpm, manp, iat;

      // get_pid successful, assign variable, otherwise quit
      if (get_pid(ENGINE_RPM, str, tempLong)) rpm = *tempLong;
      else return;
      if (get_pid(MAN_PRESSURE, str, tempLong)) manp = *tempLong;
      else return;
      if (get_pid(INT_AIR_TEMP, str, tempLong)) iat = *tempLong;
      else return;
      
      imap=(rpm*manp)/(iat+273);

      // does not divide by 100 because we use (MAF*100) in formula
      // but divide by 10 because engine displacement is in dL
      // 28.9644*100/(80*120*8.314472*10)= about 0.0036 or 36/10000
      // ex: VSS=80km/h, MAP=64kPa, RPM=1800, IAT=21C
      //     engine=2.2L, efficiency=80%
      // maf = ( (1800*64)/(21+273) * 80 * 22 * 29 ) / 10000
      // maf = 1995 or 19.95 g/s which is about right at 80km/h
      maf = (imap * params.eng_dis * 36) / 100;  //only need to divide by 100 because no longer multiplying by V.E.
}

void updateFuelUsed(unsigned long *delta_time) {
  unsigned long delta_fuel;
  delta_fuel = (maf * *delta_time)/1073;
    for(byte i=0; i<NBTRIP; i++) {
      params.trip[i].fuel+=delta_fuel;
      //code to accumlate fuel wasted while idling
      if ( vss == 0 )  {//car not moving
        params.trip[i].waste+=delta_fuel;
      }
    }
}


void do_engine_on() {
    unsigned long nowOn = millis();
    unsigned long engineOffPeriod = calcTimeDiff(engine_off, nowOn);

//    lcd_setBrightness(brightnessIdx);
	params_t params = getParameters();
    if (engineOffPeriod > (params.OutingStopOver * MINUTES_GRANULARITY * MILLIS_PER_MINUTE))
    {
      //Reset the current outing trip from last trip
      trip_reset(OUTING, false);
      engine_on = nowOn; //Reset the time at which the car starts at
    }
    else
    {
       // combine last trip time to this one! Not including the stop over time
       engine_on = nowOn - calcTimeDiff(engine_on, engine_off);
    }  
    
    if (engineOffPeriod > (params.TripStopOver * MILLIS_PER_HOUR))
    {
      trip_reset(TRIP, false);
    }
}

void do_engine_off() {
    engine_off = millis();  //record the time the engine was shut off
    params_save();

    lcd_cls_print_P(PSTR("TRIPS SAVED!"));
    //Lets Display how much fuel for the tank we wasted.
    display_wasted();
    delay(2000);
    //Turn the Backlight off
    lcd_setBrightness(0);

    #ifdef carAlarmScreen
      refreshAlarmScreen = true;
    #endif
}


// instant fuel consumption
void get_instant_fuel_cons(char *retbuf)
{
	params_t params = getParameters();
  long cons;
  char decs[16];
  long toggle_speed = params.use_metric ? params.per_hour_speed : (params.per_hour_speed*1609)/1000;

  // divide MAF by 100 because our function return MAF*100
  // but multiply by 100 for double digits precision
  // divide MAF by 14.7 air/fuel ratio to have g of fuel/s
  // divide by 730 (g/L at 15?C) according to Canadian Gov to have L/s
  // multiply by 3600 to get litre per hour
  // formula: (3600 * MAF) / (14.7 * 730 * VSS)
  // = maf*0.3355/vss L/km
  // mul by 100 to have L/100km

  // if maf is 0 it will just output 0
  if(vss < toggle_speed)
    cons=(maf*3355)/10000;  // L/h, do not use float so mul first then divide
  else
    cons=(maf*3355)/(vss*100); // L/100kmh, 100 comes from the /10000*100

  if(params.use_metric)
  {
    long_to_dec_str(cons, decs, 2);
    sprintf_P(retbuf, pctspcts, decs, (vss<toggle_speed)?"L\004":"\001\002" );
  }
  else
  {
    // MPG
    // 6.17 pounds per gallon
    // 454 g in a pound
    // 14.7 * 6.17 * 454 * (VSS * 0.621371) / (3600 * MAF / 100)
    // multipled by 10 for single digit precision

    // new comment: convert from L/100 to MPG

    if(vss<toggle_speed)
        cons=(cons*10)/378;   // convert to gallon, can be 0 G/h
    else
    {
      if(cons==0)             // if cons is 0 (DFCO?) display 999.9MPG
        cons=9999;
      else
        cons=235214/cons;     // convert to MPG
    }

    long_to_dec_str(cons, decs, 1);
    sprintf_P(retbuf, pctspcts, decs, (vss<toggle_speed)?"G\004":"\006\007" );
  }
}


// trip 0 is tank
// trip 1 is trip
// trip 2 is outing
void get_fuel_cons(char *retbuf, byte ctrip)
{
  unsigned long cfuel;
  unsigned long cdist;
  long trip_cons;
  char decs[16];

  cfuel=params.trip[ctrip].fuel;
  cdist=params.trip[ctrip].dist;

  // the car has not moved yet or no fuel used
  if(cdist<1000 || cfuel==0)
  {
    // will display 0.00L/100 or 999.9mpg
    trip_cons=params.use_metric?0:9999;
  }
  else  // the car has moved and fuel used
  {
    // from ?L/cm to L/100 so div by 1000000 for L and mul by 10000000 for 100km
    // multiply by 100 to have 2 digits precision
    // we can not mul fuel by 1000 else it can go higher than ULONG_MAX
    // so divide distance by 1000 instead (resolution of 10 metres)

    trip_cons=cfuel/(cdist/1000); // div by 0 avoided by previous test

    if(params.use_metric)
    {
      if(trip_cons>9999)    // SI
        trip_cons=9999;     // display 99.99 L/100 maximum
    }
    else
    {
      // it's imperial, convert.
      // from m/mL to MPG so * by 3.78541178 to have gallon and * by 0.621371 for mile
      // multiply by 10 to have a digit precision

      // new comment: convert L/100 to MPG
      trip_cons=235214/trip_cons;
      if(trip_cons<10)
        trip_cons=10;  // display 1.0 MPG min
    }
  }

#if 1
  long_to_dec_str(trip_cons, decs, 1+params.use_metric);  // hack
#else
  if(params.use_metric)
    long_to_dec_str(trip_cons, decs, 2);
  else
    long_to_dec_str(trip_cons, decs, 1);
#endif

  sprintf_P(retbuf, pctspcts, decs, params.use_metric?"\001\002":"\006\007" );
}

// get_engine_on_time will return the time since the engine has started
void get_engine_on_time(char *retbuf) 
{
  unsigned long run_time;
  int hours, minutes, seconds;  //to store the time
  
#ifdef useECUState
  if (ECUconnection) //update with current time, if the car is running
#else
  if(has_rpm) //update with current time, if the car is running
#endif
    run_time = calcTimeDiff(engine_on, millis());    //We now have the number of ms
   else { //car is not running.  Display final time when stopped.
    run_time = calcTimeDiff(engine_on, engine_off);
  }
  //Lets display the running time
  //hh:mm:ss
  hours =   run_time / MILLIS_PER_HOUR;
  minutes = (run_time % MILLIS_PER_HOUR) / MILLIS_PER_MINUTE;
  seconds = (run_time % MILLIS_PER_MINUTE) / MILLIS_PER_SECOND;
  
  //Now we have our varriables parsed, lets display them
  sprintf_P(retbuf, PSTR("%d:%02d:%02d"), hours, minutes, seconds);
}

void has_engine_started_or_stopped() {
  // test if engine is started
  has_rpm = isEngineRunning();
  //set flag to indicate engine is now running
  if(engine_started == 0 && has_rpm != 0)
  {
    engine_started = 1;
    param_saved = 0;
    do_engine_on();
  }
  //hostPrintLn("3");
  // if engine was started but RPM is now 0
  // save param only once, by flopping param_saved
  if(has_rpm == 0 && param_saved == 0 && engine_started != 0)
  {
    do_engine_off();
    param_saved=1;
    engine_started=0;
  }
}

/**
 * calcTimeDiff
 * Calculate the time difference, and account for roll over too
 */
unsigned long calcTimeDiff(unsigned long start, unsigned long end)
{
  if (start < end)
  {
    return end - start;
  }
  else // roll over
  {
    return ULONG_MAX - start + end;
  }
}    

