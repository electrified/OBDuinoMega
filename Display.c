// Display.pde
//#include "PString.h"
#include "project_defs.h"
#include "Arduino\WProgram.h"
#include "Common.h"
#include "Comms.h"
#include "Memory.h"
#include "LCD.h"
#include "Utilities.h"
#include "Display.h"
#include <avr/pgmspace.h>
#include <limits.h>
#include "ELMComms.h"
// Time information
#define MILLIS_PER_HOUR    3600000L
#define MILLIS_PER_MINUTE    60000L
#define MILLIS_PER_SECOND     1000L
#define MINUTES_GRANULARITY 10

// flag used to save distance/average consumption in eeprom only if required

byte param_saved=0;

bool has_rpm = false;
bool engine_started = false;
long vss = 0;  // speed
long maf = 0;  // MAF


#ifdef carAlarmScreen
boolean refreshAlarmScreen; // Used to cause non-repeating screen data to display
#endif

// some globals, for trip calculation and others
unsigned long engine_on, engine_off; //used to track time of trip.

prog_char * tripNames[NBTRIP] PROGMEM =
{
  "Tank",
  "Trip",
  "Outing"
};

prog_char pctd[] PROGMEM="- %d + "; // used in a couple of place
prog_char pctdpctpct[] PROGMEM="- %d%% + "; // used in a couple of place
prog_char pctspcts[] PROGMEM="%s %s"; // used in a couple of place


//The Textual Description of each PID
prog_char *PID_Desc[] PROGMEM=
{
"PID00-21", // 0x00   PIDs supported
"Stat DTC", // 0x01   Monitor status since DTCs cleared.
"Frz DTC",  // 0x02   Freeze DTC
"Fuel SS",  // 0x03   Fuel system status
"Eng Load", // 0x04   Calculated engine load value
"CoolantT", // 0x05   Engine coolant temperature
"ST F%T 1", // 0x06   Short term fuel % trim?Bank 1
"LT F%T 1", // 0x07   Long term fuel % trim?Bank 1
"ST F%T 2", // 0x08   Short term fuel % trim?Bank 2
"LT F%T 2", // 0x09   Long term fuel % trim?Bank 2
"Fuel Prs", // 0x0A   Fuel pressure
"  MAP  ",  // 0x0B   Intake manifold absolute pressure
"  RPM  ",  // 0x0C   Engine RPM
" Speed ",  // 0x0D   Vehicle speed
"Timing A", // 0x0E   Timing advance
"Intake T", // 0x0F   Intake air temperature
"MAF rate", // 0x10   MAF air flow rate
"Throttle", // 0x11   Throttle position
"Cmd SAS",  // 0x12   Commanded secondary air status
"Oxy Sens", // 0x13   Oxygen sensors present
"Oxy B1S1", // 0x14   Oxygen Sensor Bank 1, Sensor 1
"Oxy B1S2", // 0x15   Oxygen Sensor Bank 1, Sensor 2
"Oxy B1S3", // 0x16   Oxygen Sensor Bank 1, Sensor 3
"Oxy B1S4", // 0x17   Oxygen Sensor Bank 1, Sensor 4
"Oxy B2S1", // 0x18   Oxygen Sensor Bank 2, Sensor 1
"Oxy B2S2", // 0x19   Oxygen Sensor Bank 2, Sensor 2
"Oxy B2S3", // 0x1A   Oxygen Sensor Bank 2, Sensor 3
"Oxy B2S4", // 0x1B   Oxygen Sensor Bank 2, Sensor 4
"OBD Std",  // 0x1C   OBD standards this vehicle conforms to
"Oxy Sens", // 0x1D   Oxygen sensors present
"AuxInpt",  // 0x1E   Auxiliary input status
"Run Time", // 0x1F   Run time since engine start
"PID21-40", // 0x20   PIDs supported 21-40
"Dist MIL", // 0x21   Distance traveled with malfunction indicator lamp (MIL) on
"FRP RMF",  // 0x22   Fuel Rail Pressure (relative to manifold vacuum)
"FRP Dies", // 0x23   Fuel Rail Pressure (diesel)
"OxyS1 V",  // 0x24   O2S1_WR_lambda(1): ER Voltage
"OxyS2 V",  // 0x25   O2S2_WR_lambda(1): ER Voltage
"OxyS3 V",  // 0x26   O2S3_WR_lambda(1): ER Voltage
"OxyS4 V",  // 0x27   O2S4_WR_lambda(1): ER Voltage
"OxyS5 V",  // 0x28   O2S5_WR_lambda(1): ER Voltage
"OxyS6 V",  // 0x29   O2S6_WR_lambda(1): ER Voltage
"OxyS7 V",  // 0x2A   O2S7_WR_lambda(1): ER Voltage
"OxyS8 V",  // 0x2B   O2S8_WR_lambda(1): ER Voltage
"Cmd EGR",  // 0x2C   Commanded EGR
"EGR Err",  // 0x2D   EGR Error
"Cmd EP",   // 0x2E   Commanded evaporative purge
"Fuel LI",  // 0x2F   Fuel Level Input
"WarmupCC", // 0x30   # of warm-ups since codes cleared
"Dist CC",  // 0x31   Distance traveled since codes cleared
"Evap SVP", // 0x32   Evap. System Vapor Pressure
"Barometr", // 0x33   Barometric pressure
"OxyS1 C",  // 0x34   O2S1_WR_lambda(1): ER Current
"OxyS2 C",  // 0x35   O2S2_WR_lambda(1): ER Current
"OxyS3 C",  // 0x36   O2S3_WR_lambda(1): ER Current
"OxyS4 C",  // 0x37   O2S4_WR_lambda(1): ER Current
"OxyS5 C",  // 0x38   O2S5_WR_lambda(1): ER Current
"OxyS6 C",  // 0x39   O2S6_WR_lambda(1): ER Current
"OxyS7 C",  // 0x3A   O2S7_WR_lambda(1): ER Current
"OxyS8 C",  // 0x3B   O2S8_WR_lambda(1): ER Current
"C T B1S1", // 0x3C   Catalyst Temperature Bank 1 Sensor 1
"C T B1S2", // 0x3D   Catalyst Temperature Bank 1 Sensor 2
"C T B2S1", // 0x3E   Catalyst Temperature Bank 2 Sensor 1
"C T B2S2", // 0x3F   Catalyst Temperature Bank 2 Sensor 2
"PID41-60", // 0x40   PIDs supported 41-60
" MStDC",   // 0x41   Monitor status this drive cycle
"Ctrl M V", // 0x42   Control module voltage
"Abs L V",  // 0x43   Absolute load value
"Cmd E R",  // 0x44   Command equivalence ratio
"R ThrotP", // 0x45   Relative throttle position
"Amb Temp", // 0x46   Ambient air temperature
"Acc PP B", // 0x47   Absolute throttle position B
"Acc PP C", // 0x48   Absolute throttle position C
"Acc PP D", // 0x49   Accelerator pedal position D
"Acc PP E", // 0x4A   Accelerator pedal position E
"Acc PP F", // 0x4B   Accelerator pedal position F
"Cmd T A",  // 0x4C   Commanded throttle actuator
"T MIL On", // 0x4D   Time run with MIL on
"T TC Crl", // 0x4E   Time since trouble codes cleared
"  0x4F",   // 0x4F   Unknown
"  0x50",   // 0x50   Unknown
"Fuel Typ", // 0x51   Fuel Type
"Ethyl F%", // 0x52   Ethanol fuel %
"", // 0x53   
"", // 0x54   
"", // 0x55   
"", // 0x56   
"", // 0x57   
"", // 0x58   
"", // 0x59   
"", // 0x5A   
"", // 0x5B   
"", // 0x5C   
"", // 0x5D   
"", // 0x5E   
"", // 0x5F   
"", // 0x60   
"", // 0x61   
"", // 0x62   
"", // 0x63   
"", // 0x64   
"", // 0x65   
"", // 0x66   
"", // 0x67   
"", // 0x68   
"", // 0x69   
"", // 0x6A   
"", // 0x6B   
"", // 0x6C   
"", // 0x6D   
"", // 0x6E   
"", // 0x6F   
"", // 0x70   
"", // 0x71   
"", // 0x72   
"", // 0x73   
"", // 0x74   
"", // 0x75   
"", // 0x76   
"", // 0x77   
"", // 0x78   
"", // 0x79   
"", // 0x7A   
"", // 0x7B   
"", // 0x7C   
"", // 0x7D   
"", // 0x7E   
"", // 0x7F   
"", // 0x80   
"", // 0x81   
"", // 0x82   
"", // 0x83   
"", // 0x84   
"", // 0x85   
"", // 0x86   
"", // 0x87   
"", // 0x88   
"", // 0x89   
"", // 0x8A   
"", // 0x8B   
"", // 0x8C   
"", // 0x8D   
"", // 0x8E   
"", // 0x8F   
"", // 0x90   
"", // 0x91   
"", // 0x92   
"", // 0x93   
"", // 0x94   
"", // 0x95   
"", // 0x96   
"", // 0x97   
"", // 0x98   
"", // 0x99   
"", // 0x9A   
"", // 0x9B   
"", // 0x9C   
"", // 0x9D   
"", // 0x9E   
"", // 0x9F   
"", // 0xA0   
"", // 0xA1   
"", // 0xA2   
"", // 0xA3   
"", // 0xA4   
"", // 0xA5   
"", // 0xA6   
"", // 0xA7   
"", // 0xA8   
"", // 0xA9   
"", // 0xAA   
"", // 0xAB   
"", // 0xAC   
"", // 0xAD   
"", // 0xAE   
"", // 0xAF   
"", // 0xB0   
"", // 0xB1   
"", // 0xB2   
"", // 0xB3   
"", // 0xB4   
"", // 0xB5   
"", // 0xB6   
"", // 0xB7   
"", // 0xB8   
"", // 0xB9   
"", // 0xBA   
"", // 0xBB   
"", // 0xBC   
"", // 0xBD   
"", // 0xBE   
"", // 0xBF   
"", // 0xC0   
"", // 0xC1   
"", // 0xC2   
"", // 0xC3   Unknown
"", // 0xC4   Unknown
"", // 0xC5   
"", // 0xC6   
"", // 0xC7   
"", // 0xC8   
"", // 0xC9   
"", // 0xCA   
"", // 0xCB   
"", // 0xCC   
"", // 0xCD   
"", // 0xCE   
"", // 0xCF   
"", // 0xD0   
"", // 0xD1   
"", // 0xD2   
"", // 0xD3   
"", // 0xD4   
"", // 0xD5   
"", // 0xD6   
"", // 0xD7   
"", // 0xD8   
"", // 0xD9   
"", // 0xDA   
"", // 0xDB   
"", // 0xDC   
"", // 0xDD   
"", // 0xDE   
"", // 0xDF   
"", // 0xE0   
"", // 0xE1   
"", // 0xE2   
"", // 0xE3   
"", // 0xE4   
"", // 0xE5   
"", // 0xE6   
"", // 0xE7   
"", // 0xE8   
"OutWaste", // 0xE9   outing waste
"TrpWaste", // 0xEA   trip waste
"TnkWaste", // 0xEB   tank waste  
"Out Cost", // 0xEC   outing cost
"Trp Cost", // 0xED   trip cost
"Tnk Cost", // 0xEE   tank cost
"Out Time", // 0xEF   The length of time car has been running
"No Disp",  // 0xF0   No display
"InstCons", // 0xF1   instant cons
"Tnk Cons", // 0xF2   average cons of tank
"Tnk Fuel", // 0xF3   fuel used in tank
"Tnk Dist", // 0xF4   distance for tank
"Dist2MT",  // 0xF5   remaining distance of tank
"Trp Cons", // 0xF6   average cons of trip
"Trp Fuel", // 0xF7   fuel used in trip
"Trp Dist", // 0xF8   distance of trip
"Batt Vlt", // 0xF9   Battery Voltage
"Out Cons", // 0xFA   cons since the engine turned on
"Out Fuel", // 0xFB   fuel used since engine turned on
"Out Dist", // 0xFC   distance since engine turned on
"Can Stat", // 0xFD   Can Status
"PID_SEC",  // 0xFE   
"Eco Vis",  // 0xFF   Visually dispay relative economy with text
};


void display(byte location, byte pid)
{
  char str[STRLEN];

  /* check if it's a real PID or our internal one */
  if(pid==NO_DISPLAY)
    return;
  else if(pid==OUTING_COST)
    get_cost(str, OUTING);
  else if(pid==TRIP_COST)
    get_cost(str, TRIP);
  else if(pid==TANK_COST)
    get_cost(str, TANK);
  else if(pid==ENGINE_ON)
    get_engine_on_time(str);
  else if(pid==FUEL_CONS)
    get_instant_fuel_cons(str);
  else if(pid==TANK_CONS)
    get_fuel_cons(str, TANK);
  else if(pid==TANK_FUEL)
    get_fuel(str, TANK);
  else if (pid==TANK_WASTE)
    get_waste(str,TANK);
  else if(pid==TANK_DIST)
    get_dist(str, TANK);
  else if(pid==REMAIN_DIST)
    get_remain_dist(str);
  else if(pid==TRIP_CONS)
    get_fuel_cons(str, TRIP);
  else if(pid==TRIP_FUEL)
    get_fuel(str, TRIP);
  else if (pid==TRIP_WASTE)
    get_waste(str,TRIP);
  else if(pid==TRIP_DIST)
    get_dist(str, TRIP);
#ifdef ELM
  else if(pid==BATT_VOLTAGE)
    elm_command(str, PSTR("ATRV\r"));
  else if(pid==CAN_STATUS)
    elm_command(str, PSTR("ATCS\r"));
#endif
  else if (pid==OUTING_CONS)
    get_fuel_cons(str,OUTING);
  else if (pid==OUTING_FUEL)
    get_fuel(str,OUTING);
  else if (pid==OUTING_WASTE)
    get_waste(str,OUTING);
  else if (pid==OUTING_DIST)
    get_dist(str,OUTING);
  else if(pid==PID_SEC)
    sprintf_P(str, PSTR("%d pid/s"), comms_pids_per_second());
#ifdef DEBUG
  else if(pid==FREE_MEM)
    sprintf_P(str, PSTR("%d free"), memoryTest());
#else
  else if(pid==ECO_VISUAL)
    eco_visual(str);
#endif
  else {
    long tl;
    get_pid(pid, str, &tl);
  }

  // left locations are left aligned
  // right locations are right aligned
  
  // truncate any string that is too long to display correctly
  str[LCD_split] = '\0';
  
  byte row = location / 2;  // Two PIDs per line
  boolean isLeft = location % 2 == 0; // First PID per line is always left
  byte textPos    = isLeft ? 0 : LCD_width - strlen(str);  
  byte clearStart = isLeft ? strlen(str) : LCD_split; 
  byte clearEnd   = isLeft ? LCD_split : textPos;  
    
  lcd_gotoXY(textPos, row);
  lcd_print(str);

  // clean up any possible leading or trailing data
  lcd_gotoXY(clearStart, row);
  for (byte cleanup = clearStart; cleanup < clearEnd; cleanup++)
  {
    lcd_dataWrite(' ');
  }  
}

/**
 * display_PID_names
 */
void display_PID_names(byte screenId)
{
	params_t params = getParameters();
  needBacklight(true);    
  lcd_cls();
  // Lets flash up the description of the PID's we use when screen changes
  byte count = 0;
  for (byte row = 0; row < LCD_ROWS; row++)
  {
    for (byte col = 0; col == 0 || col == LCD_split; col+=LCD_split)
    {
      lcd_gotoXY(col,row);  
      lcd_print((char*)pgm_read_word(&(PID_Desc[params.screen[screenId].PID[count++]])));
    }  
  }
  
  delay(750); // give user some time to see new PID titles
} 


// trip 0 is tank
// trip 1 is trip
// trip 2 is outing
void get_fuel(char *retbuf, byte ctrip)
{
  unsigned long cfuel;
  char decs[16];
params_t params = getParameters();
  // convert from ?L to cL
  cfuel = params.trip[ctrip].fuel/10000;

  // convert in gallon if requested
  if(!params.use_metric)
    cfuel = convertToGallons(cfuel);
    
  long_to_dec_str(cfuel, decs, 2);
  sprintf_P(retbuf, pctspcts, decs, params.use_metric?"L":"G" );
}

// trip 0 is tank
// trip 1 is trip
// trip 2 is outing
void get_waste(char *retbuf, byte ctrip)
{
  unsigned long cfuel;
  char decs[16];
params_t params = getParameters();
  // convert from ?L to cL
  cfuel=params.trip[ctrip].waste/10000;

  // convert in gallon if requested
  if(!params.use_metric)
    cfuel = convertToGallons(cfuel);

  long_to_dec_str(cfuel, decs, 2);
  sprintf_P(retbuf, pctspcts, decs, params.use_metric?"L":"G" );
}

// trip 0 is tank
// trip 1 is trip
// trip 2 is outing
void get_dist(char *retbuf, byte ctrip)
{
  unsigned long cdist;
  char decs[16];
params_t params = getParameters();
  // convert from cm to hundreds of meter
  cdist=params.trip[ctrip].dist/10000;

  // convert in miles if requested
  if(!params.use_metric)
    cdist=(cdist*1000)/1609;

  long_to_dec_str(cdist, decs, 1);
  sprintf_P(retbuf, pctspcts, decs, params.use_metric?"\003":"\006" );
}

// distance you can do with the remaining fuel in your tank
void get_remain_dist(char *retbuf)
{
  long tank_tmp;
  long remain_dist;
  long remain_fuel;
  long tank_cons;
params_t params = getParameters();
  // tank size is in litres (converted at input time)
  tank_tmp=params.tank_size;

  // convert from ?L to dL
  remain_fuel=tank_tmp - params.trip[TANK].fuel/100000;

  // calculate remaining distance using tank cons and remaining fuel
  if(params.trip[TANK].dist<1000)
    remain_dist=9999;
  else
  {
    tank_cons = params.trip[TANK].fuel/(params.trip[TANK].dist/1000);
    remain_dist = remain_fuel*1000/tank_cons;

    if(!params.use_metric)  // convert to miles
      remain_dist = (remain_dist*1000)/1609;
  }

  sprintf_P(retbuf, pctldpcts, remain_dist, params.use_metric?"\003":"\006" );
}


#ifdef carAlarmScreen
// This screen will display a fake security heading,
// then emulate an array of LED's blinking in Knight Rider style.
// This could be modified to blink a real LED (or maybe a short array depending on available pins)
void displayAlarmScreen()
{
  static byte pingPosition;
  static boolean pingDirection;
  static long nextMoveTime;
  const long pingTimeOut = 1000;
  const byte lastLCDChar = 15;
 
  if (refreshAlarmScreen)
  {
    pingPosition = 0;
    pingDirection = 0;

    lcd_cls_print_P(PSTR("OBDuino Security" ));
    lcd_gotoXY(pingPosition,1);
    lcd_dataWrite('*');
    
    refreshAlarmScreen = false;
    nextMoveTime = millis() + pingTimeOut;
  }
  else if (millis() > nextMoveTime)
  {
    lcd_gotoXY(pingPosition,1);
    lcd_dataWrite(' ');

    if(pingPosition == 0 || pingPosition == lastLCDChar)
    {
      // Change direction
      pingDirection = !pingDirection;
    }
    
    // Move the character    
    if(pingDirection)
    {
      pingPosition+= 3;
    }
    else
    {
      pingPosition-=3;
    }
    
    lcd_gotoXY(pingPosition,1);
    lcd_dataWrite('*');
    
    nextMoveTime = millis() + pingTimeOut;
  }
}  
#endif

prog_char * econ_Visual[] PROGMEM=
{
  "Yuck!!8{",
  "Awful :(",
  "Poor  :[",
  "OK    :|",
  "Good  :]",
  "Great :)",
  "Adroit:D",
  "HyprM 8D"
};

/*
Adj %
	 0   	 1 	 2 	 3 	 4 	4	5	6	7	8     <==star count
1%	91%	92%	93%	94%	95%	105%	106%	107%	108%	109%
2%	88%	89%	91%	93%	95%	105%	107%	109%	111%	114%
3%	84%	87%	89%	92%	95%	105%	108%	111%	115%	118%
4%	81%	84%	88%	91%	95%	105%	109%	114%	118%	123%
5%	77%	81%	86%	90%	95%	105%	110%	116%	122%	128%
6%	74%	79%	84%	89%	95%	105%	111%	118%	125%	133%
7%	71%	76%	82%	88%	95%	105%	112%	120%	129%	138%
8%	68%	74%	80%	87%	95%	105%	113%	122%	132%	143%
9%	65%	72%	79%	86%	95%	105%	114%	125%	136%	148%
10%	62%	69%	77%	86%	95%	105%	116%	127%	140%	154%
11%	60%	67%	75%	85%	95%	105%	117%	129%	144%	159%
12%	57%	65%	74%	84%	95%	105%	118%	132%	148%	165%
13%	54%	63%	72%	83%	95%	105%	119%	134%	152%	171%
*/
#define PERCENTAGE_RANGE 108  //108 = 8%
void eco_visual(char *retbuf) {
  //enable our varriables
  unsigned long tank_cons, outing_cons;
  unsigned long tfuel, tdist;
  int stars;
	params_t params = getParameters();

  tfuel = params.trip[OUTING].fuel;
  tdist = params.trip[OUTING].dist;
  
  if(tdist > 100 && tfuel!=0) {//Make sure no divisions by Zero.
    outing_cons = tfuel / (tdist / 1000);  //our current trip since engine start
    tfuel = params.trip[TANK].fuel;
    tdist = params.trip[TANK].dist;
    tank_cons = tfuel / (tdist / 1000);  //our results for the current tank of gas
  } else {  //give some dummy numbers to avoid divide by zero numbers
    tank_cons = 100;
    outing_cons = 101;
  }

  //lets start off in the middle
  stars = 3; // 3 = Average.
  if ( outing_cons < tank_cons ) {          //doing good :)
    outing_cons = (outing_cons*105) / 100; //Check if within 5% of TANK for Average result
    //Loop to check how much better we are doing
    //Each time the smaller number will be increased by a set percentage
    //in order to add or subtract from our star count.
    while(outing_cons < tank_cons && stars < 7) {  
      outing_cons = (outing_cons*PERCENTAGE_RANGE) / 100;
      stars++;
    }
    outing_cons=0;
  } else if (outing_cons > tank_cons) {  //doing bad... so far...
    tank_cons = (tank_cons*105) / 100;   //Check if within 5% of TANK for Average result
    while(outing_cons > tank_cons  && stars > 0) {  //Loop to check how much worse we are doing
      tank_cons = (tank_cons*PERCENTAGE_RANGE) / 100;
      stars--;
    } 
  } //else they are equal, do nothing.
 
  //Now we have our star count, use it as an index to access the text
  sprintf_P(retbuf, PSTR("%s"), (char*)pgm_read_word(&(econ_Visual[stars])));
} 



void get_cost(char *retbuf, byte ctrip)
{
  unsigned long cents;
  unsigned long fuel;
  char decs[16];
  params_t params = getParameters();
  fuel = params.trip[ctrip].fuel / 10000; //cL
  cents =  fuel * params.gas_price / 1000; //now have $$$$cc
  long_to_dec_str(cents, decs, 2);
  sprintf_P(retbuf, PSTR("$%s"), decs);
}

// Display how much fuel for the tank we wasted.
void display_wasted() {
  char str[STRLEN] = {0};
  lcd_gotoXY(0,1);
  lcd_print_P(PSTR("Wasted:"));
  lcd_gotoXY(LCD_split, 1);
  get_waste(str, TANK);
  lcd_print(str);
}


unsigned int convertToGallons(unsigned int litres)
{
  return (unsigned int) ((float) litres / 3.785411);
}

unsigned int convertToLitres(unsigned int gallons)
{
  return (unsigned int) ((float) gallons / 0.264172);
}

void calc_init() {
  engine_off = engine_on = millis();
}

/*
 * accumulate data for trip, called every loop()
 */
void accu_trip()
{
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
      //getMafFromMap(str, &tempLong);
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
    needBacklight(false);

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
    sprintf_P(retbuf, pctspcts, decs, (vss < toggle_speed) ? "L\004":"\001\002" );
  }
  else
  {
    // MPG
    // 6.17 pounds per gallon
    // 454 g in a pound
    // 14.7 * 6.17 * 454 * (VSS * 0.621371) / (3600 * MAF / 100)
    // multipled by 10 for single digit precision

    // new comment: convert from L/100 to MPG

    if(vss < toggle_speed)
        cons = (cons * 10)/378;   // convert to gallon, can be 0 G/h
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

  long_to_dec_str(trip_cons, decs, 1+params.use_metric);  // hack

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

