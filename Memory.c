//Memory.pde
#include "WProgram.h"
#include "Host.h"
//S#include "

#include "Comms.h"
#include <avr/eeprom.h>

#include "Memory.h"
// parameters default values
params_t params=
{
  40,
  1,
  true,
  20,
  100,
  100,
  16,
  905,
  450,
  6, // 60 minutes (6 X 10) stop or less will not cause outing reset
  12, // 12 hour stop or less will not cause trip reset
  {
    { 0,0,0 }, // tank: dist, fuel, waste
    { 0,0,0 }, // trip: dist, fuel, waste 
    { 0,0,0 }  // outing:dist, fuel, waste
  },
  {
    { {FUEL_CONS,LOAD_VALUE,TANK_CONS,OUTING_FUEL
       #if LCD_ROWS == 4
         ,OUTING_WASTE,OUTING_COST,ENGINE_ON,LOAD_VALUE
       #endif
       } },
    { {TRIP_CONS,TRIP_DIST,TRIP_FUEL,COOLANT_TEMP
       #if LCD_ROWS == 4
         ,TRIP_WASTE,TRIP_COST,INT_AIR_TEMP,THROTTLE_POS
       #endif
       } },
    { {TANK_CONS,TANK_DIST,TANK_FUEL,REMAIN_DIST
       #if LCD_ROWS == 4
         ,TANK_WASTE,TANK_COST,ENGINE_RPM,VEHICLE_SPEED
       #endif
       } }
  }
};

/*
 * Memory related functions
 */

// we have 512 bytes of EEPROM on the 168P, more than enough
void params_save()
{
  uint16_t crc;
  byte *p;

  // CRC will go at the end
  crc=0;
  p=(byte*)&params;
  for(byte i=0; i<sizeof(params_t); i++)
    crc+=p[i];

  // start at address 0
  eeprom_write_block((const void*)&params, (void*)0, sizeof(params_t));
  // write CRC after params struct
  eeprom_write_word((uint16_t*)sizeof(params_t), crc);
}

void params_load()
{
  hostPrint(" * Loading default parameters   ");
  params_t params_tmp;
  uint16_t crc, crc_calc;
  byte *p;

  // read params
  eeprom_read_block((void*)&params_tmp, (void*)0, sizeof(params_t));
  // read crc
  crc=eeprom_read_word((const uint16_t*)sizeof(params_t));

  // calculate crc from read stuff
  crc_calc=0;
  p=(byte*)&params_tmp;
  for(byte i=0; i<sizeof(params_t); i++)
    crc_calc+=p[i];

  // compare CRC
  if(crc==crc_calc)     // good, copy read params to params
    params=params_tmp;
    
  hostPrintLn("[OK]");
}

#ifdef DEBUG  // how can this takes 578 bytes!!
// this function will return the number of bytes currently free in RAM
// there is about 670 bytes free in memory when OBDuino is running
extern int  __bss_end;
extern int  *__brkval;
int memoryTest()
{
  int free_memory;
  if((int)__brkval == 0)
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  else
    free_memory = ((int)&free_memory) - ((int)__brkval);
  return free_memory;
}
#endif

params_t getParameters() {
	return params;
}
