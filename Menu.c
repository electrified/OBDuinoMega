/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Arduino\WProgram.h"
#include <avr/pgmspace.h>
#include "host.h"
#include "Common.h"
#include "LCD.h"
#include "Display.h"
#include "Comms.h"
#include "Utilities.h"
#include "Menu.h"
#include "Memory.h"

#define buttonsUp 0 // start with the buttons in the 'not pressed' state
byte buttonState = buttonsUp;

#define KEY_WAIT 1000 // Wait for potential other key press
#define ACCU_WAIT 500 // Only accumulate data so often.
#define BUTTON_DELAY  125


// use analog pins as digital pins for buttons
#ifdef MEGA  // Button pins for Arduino Mega
#define lbuttonPin 62 // Left Button, on analog 8
#define mbuttonPin 63 // Middle Button, on analog 9
#define rbuttonPin 64 // Right Button, on analog 10
#define lbuttonBit 1 //  pin62 is a bitmask 1 on port K
#define mbuttonBit 2  // pin63 is a bitmask 2 on port K
#define rbuttonBit 4  // pin64 is a bitmask 4 on port K
#else    // Button pins for Duemilanove or equivalent
#define lbuttonPin 17 // Left Button, on analog 3
#define mbuttonPin 18 // Middle Button, on analog 4
#define rbuttonPin 19 // Right Button, on analog 5
#define lbuttonBit 8 //  pin17 is a bitmask 8 on port C
#define mbuttonBit 16 // pin18 is a bitmask 16 on port C
#define rbuttonBit 32 // pin19 is a bitmask 32 on port C
#endif

byte active_screen=0;  // 0,1,2,... selected by left button

// Easy to read macros
#define LEFT_BUTTON_PRESSED (buttonState&lbuttonBit)
#define MIDDLE_BUTTON_PRESSED (buttonState&mbuttonBit)
#define RIGHT_BUTTON_PRESSED (buttonState&rbuttonBit)


prog_char select_no[]  PROGMEM="(NO) YES "; // for config menu
prog_char select_yes[] PROGMEM=" NO (YES)"; // for config menu
prog_char gasPrice[][10] PROGMEM={"-  %s\354 + ", "- $%s +  "}; // dual string for fuel price

// menu items used by menu_selection.
const char *topMenu[] PROGMEM = {"Configure menu", "Exit", "Display", "Adjust", "PIDs"};
const char *displayMenu[] PROGMEM = {"Display menu", "Exit", "Contrast", "Metric", "Fuel/Hour"};
const char *adjustMenu[] PROGMEM = {"Adjust menu", "Exit", "Tank Size", "Fuel Cost", "Fuel %", "Speed %", "Out Wait", "Trip Wait", "Eng Disp"};
const char *PIDMenu[] PROGMEM = {"PID Screen menu", "Exit", "Scr 1", "Scr 2", "Scr 3"};



// the buttons interrupt
// this is the interrupt handler for button presses
#ifdef MEGA
ISR(PCINT2_vect)
{
  static unsigned long last_millis = 0;
  unsigned long m = millis();

  if (m - last_millis > 20)
  {
    buttonState |= ~PINK;
  }
  //  else ignore interrupt: probably a bounce problem
  last_millis = m;
}
#else
ISR(PCINT1_vect)
{
  static unsigned long last_millis = 0;
  unsigned long m = millis();

  if (m - last_millis > 20)
  {
    buttonState |= ~PINC;
    hostPrintLn("buttons");
  }
  //  else ignore interrupt: probably a bounce problem
  last_millis = m;
}
#endif


void menu_init_buttons() {
  pinMode(lbuttonPin, INPUT);
  pinMode(mbuttonPin, INPUT);
  pinMode(rbuttonPin, INPUT);
  // "turn on" the internal pullup resistors
  digitalWrite(lbuttonPin, HIGH);
  digitalWrite(mbuttonPin, HIGH);
  digitalWrite(rbuttonPin, HIGH);

  // low level interrupt enable stuff
  // interrupt 1 for the 3 buttons
  #ifdef MEGA
  PCMSK2 |= (1 << PCINT16) | (1 << PCINT17) | (1 << PCINT18);
  PCICR  |= (1 << PCIE2);
  #else
  PCMSK1 |= (1 << PCINT11) | (1 << PCINT12) | (1 << PCINT13);
  PCICR  |= (1 << PCIE1);
  #endif
}

// This helps reduce code size by containing repeated functionality.
void displaySecondLine(byte position, char * str)
{
  lcd_gotoXY(position,1);
  lcd_print(str);
  delay_reset_button(); 
}


// common code used in a couple of menu section
byte menu_select_yes_no(byte p)
{
  boolean exitMenu = false;

  // set value with left/right and set with middle
  delay_reset_button();  // make sure to clear button

  do
  {
    if(LEFT_BUTTON_PRESSED)
      p=0;
    else if(RIGHT_BUTTON_PRESSED)
      p=1;
    else if(MIDDLE_BUTTON_PRESSED)
      exitMenu = true;
  
    lcd_gotoXY(4,1);
    if(p==0)
      lcd_print_P(select_no);
    else
      lcd_print_P(select_yes);

    delay_reset_button();
  }
  while(!exitMenu);
  
  return p;
}


/*
 * Configuration menu
 */

void delay_reset_button()
{
  // accumulate data for trip while in the menu config, do not pool too often.
  // but anyway you should not configure your OBDuino while driving!
  
  // If there has been a key press, then don't accumulate trip data just yet,
  // wait a little past the last key press before doing trip data.
  // Rapid key presses take priority...
 /* static unsigned long lastButtonTime = 0;
 
  if (buttonState != buttonsUp)
  {
    lastButtonTime = millis();
    
    buttonState = buttonsUp;
    delay(BUTTON_DELAY);
  }
  else
  {
    if (calcTimeDiff(lastButtonTime, millis()) > KEY_WAIT &&
        calcTimeDiff(old_time, millis()) > ACCU_WAIT)
    {
      accu_trip();
    }
  }*/
}


// Reworked a little to allow all trip types to be reset from one function.
void trip_reset(byte ctrip, boolean ask)
{
/*  boolean reset = true;
  char str[STRLEN];
 
  // Display the intent
  lcd_cls();
  sprintf_P(str, PSTR("Zero %s data"), (char*)pgm_read_word(&(tripNames[ctrip])));
  lcd_print(str);
  
  if(ask)
  {
    reset=menu_select_yes_no(0);  // init to "no"
  }

  if(reset)
  {
    params.trip[ctrip].dist=0L;
    params.trip[ctrip].fuel=0L;
    params.trip[ctrip].waste=0L;
    
    if (ctrip == OUTING && ask)
    {
      // Reset the start time to now too
      engine_on = millis();
    }
  }
 
  if (!ask)
  {
    delay(750); // let user see (if they are paying attention)
  }  */
}

void test_buttons()
{
    //hostPrintLn("Entering test_buttons");
  // middle + left = tank reset
if(MIDDLE_BUTTON_PRESSED && LEFT_BUTTON_PRESSED)
  {
    needBacklight(true);
    trip_reset(TANK, true);
  }
  // middle + right = trip reset
  else if(MIDDLE_BUTTON_PRESSED && RIGHT_BUTTON_PRESSED)
  {
    // Added choice to reset OUTING trip also. We could merge TANK here too, and then just use the menu selection
    // to select the trip type to reset (maybe ask confirmation or not, since the menu has an exit).
    needBacklight(true);    
    trip_reset(TRIP, true);
    trip_reset(OUTING, true);
  }
  // left + right = flash pid info
  else if(LEFT_BUTTON_PRESSED && RIGHT_BUTTON_PRESSED)
  {
    display_PID_names(active_screen);
  }
  // left is cycle through active screen
  else if(LEFT_BUTTON_PRESSED)
  {
    hostPrintLn("LEFTD");
    active_screen = (active_screen + 1) % NBSCREEN;
    display_PID_names(active_screen);
  }
  // right is cycle through brightness settings
  else if(RIGHT_BUTTON_PRESSED)
  {
    hostPrintLn("RI");
	lcd_increase_brightness();
    delay(500);
  }
  // middle is go into menu
  
  
  else if(MIDDLE_BUTTON_PRESSED)
  {
    hostPrintLn("MI");
    needBacklight(true);    
    //config_menu();
  }
  
  // reset buttons state
  if(buttonState!=buttonsUp)
  {
    #ifdef carAlarmScreen
      refreshAlarmScreen = true;
    #endif
   
    delay_reset_button();
    needBacklight(false);    
  }
}

/*
void config_menu(void)
{
  char str[STRLEN];
  char decs[16];
  int lastButton = 0;  //we'll use this to speed up button pushes
  unsigned int fuelUnits = 0;
  boolean changed = false;

#ifdef ELM
#ifndef DEBUG  // it takes 98 bytes
  // display protocol, just for fun
  lcd_cls();
  memset(str, 0, STRLEN);
  elm_command(str, PSTR("ATDP\r"));
  if(str[0]=='A')  // string start with "AUTO, ", skip it
  {
    lcd_print(str+6);
    lcd_gotoXY(0,1);
    lcd_print(str+6+16);
  }
  else
  {
    lcd_print(str);
    lcd_gotoXY(0,1);
    lcd_print(str+16);
  }
  delay(2000);
#endif
#endif

  boolean saveParams = false;  // Currently a button press will cause a save, smarter would be to verify a change in value...
  byte selection = 0;
  byte oldByteValue;             // used to determine if new value is different and we need to save the change
  unsigned int oldUIntValue;     // ditto

  do
  {
    hostPrintLn("cmn");
    selection = menu_selection(topMenu, ARRAY_SIZE(topMenu));

    if (selection == 1) // display
    {
      byte displaySelection = 0;
    
      do
      {
        displaySelection = menu_selection(displayMenu, ARRAY_SIZE(displayMenu));

        if (displaySelection == 1) // Contrast
        {
          lcd_cls_print_P(PSTR("LCD contrast"));
          oldByteValue = params.contrast;
          
          do
          {
            if(LEFT_BUTTON_PRESSED && params.contrast!=0)
              params.contrast-=10;
            else if(RIGHT_BUTTON_PRESSED && params.contrast!=100)
              params.contrast+=10;

            analogWrite(ContrastPin, params.contrast);  // change dynamicaly
            sprintf_P(str, pctd, params.contrast);
            displaySecondLine(5, str);
          } while(!MIDDLE_BUTTON_PRESSED);

          if (oldByteValue != params.contrast)
          {
            saveParams = true;
          }
        }
        else if (displaySelection == 2)  // Metric
        {
          lcd_cls_print_P(PSTR("Use metric unit"));
          oldByteValue = params.use_metric;
          params.use_metric=menu_select_yes_no(params.use_metric);
          if (oldByteValue != params.use_metric)
          {
            saveParams = true;
          }

          // Only if metric do we have the option of using the comma as a decimal
          if(params.use_metric)
          {
            lcd_cls_print_P(PSTR("Use comma format"));
            oldByteValue = (byte) params.use_comma;
            params.use_comma = menu_select_yes_no(params.use_comma);
   
            if (oldByteValue != (byte) params.use_comma)
            {
              saveParams = true;
            }
          }
        }
        else if (displaySelection == 3) // Display speed
        {
          oldByteValue = params.per_hour_speed;

          // speed from which we toggle to fuel/hour
          lcd_cls_print_P(PSTR("Fuel/hour speed"));
          // set value with left/right and set with middle
          do
          {
            if(LEFT_BUTTON_PRESSED && params.per_hour_speed!=0)
              params.per_hour_speed--;
            else if(RIGHT_BUTTON_PRESSED && params.per_hour_speed!=255)
              params.per_hour_speed++;

            sprintf_P(str, pctd, params.per_hour_speed);
            displaySecondLine(5, str);
          } while(!MIDDLE_BUTTON_PRESSED);

          if (oldByteValue != params.per_hour_speed)
          {
            saveParams = true;
          }
        }
      } while (displaySelection != 0); // exit from this menu
    }
    else if (selection == 2) // Adjust
    {
      byte adjustSelection = 0;
      byte count = ARRAY_SIZE(adjustMenu);
      if (is_pid_supported(MAF_AIR_FLOW, 0))
      {
        // Use the "Eng Displ" parameter (the last one) only when MAF_AIR_FLOW is not supported
        count--;
      }
  
      do
      {
        adjustSelection = menu_selection(adjustMenu, count);

        if (adjustSelection == 1) 
        {   
          lcd_cls_print_P(PSTR("Tank size ("));

          oldUIntValue = params.tank_size;

          // convert in gallon if requested
          if(!params.use_metric)
          {
            lcd_print_P(PSTR("G)"));
            fuelUnits = convertToGallons(params.tank_size);
          }  
          else
          {
            lcd_print_P(PSTR("L)"));
            fuelUnits = params.tank_size;
          }
  
          // set value with left/right and set with middle
          do
          {
            if(LEFT_BUTTON_PRESSED)
            {
              changed = true;
              fuelUnits--;
            }
            else if(RIGHT_BUTTON_PRESSED)
            {
              changed = true;
              fuelUnits++;
            }

            long_to_dec_str(fuelUnits, decs, 1);
            sprintf_P(str, PSTR("- %s + "), decs);
            displaySecondLine(4, str);
          } while(!MIDDLE_BUTTON_PRESSED);
   
          if (changed)
          {
            if(!params.use_metric)
            {
              params.tank_size = convertToLitres(fuelUnits);
            }
            else
            {
              params.tank_size = fuelUnits;
            } 
            changed = false;
          }

          if (oldUIntValue != params.tank_size)
          {
            saveParams = true;
          }          
        }
        else if (adjustSelection == 2)  // cost
        {
          int lastButton = 0;
 
          lcd_cls_print_P(PSTR("Fuel Price ("));
          oldUIntValue = params.gas_price;

          // convert in gallons if requested
          if(!params.use_metric)
          {
            lcd_print_P(PSTR("G)"));
            // Convert unit price to litres for the cost per gallon. (ie $1 a litre = $3.785 per gallon)
            fuelUnits = convertToLitres(params.gas_price);
          }
          else
          {
            lcd_print_P(PSTR("L)"));
            fuelUnits = params.gas_price;
          }
  
          // set value with left/right and set with middle
          do
          {
            if(LEFT_BUTTON_PRESSED){
              changed = true;
              lastButton--;      
              if(lastButton >= 0) {
                lastButton = 0;
                fuelUnits--;
              } else if (lastButton < -3 && lastButton > -7) {
                fuelUnits-=2;
              } else if (lastButton <= -7) {
                fuelUnits-=10;
              } else {
                fuelUnits--;
              }
            } else if(RIGHT_BUTTON_PRESSED){
              changed = true;
              lastButton++;      
              if(lastButton <= 0) {
                lastButton = 0;
                fuelUnits++;
              } else if (lastButton > 3 && lastButton < 7) {
                fuelUnits+=2;
              } else if (lastButton >= 7) {
                fuelUnits+=10;
              } else {
                fuelUnits++;
              }      
            }

            long_to_dec_str(fuelUnits, decs, fuelUnits > 999 ? 3 : 1);
            sprintf_P(str, gasPrice[fuelUnits > 999], decs);
            displaySecondLine(3, str);
          } while(!MIDDLE_BUTTON_PRESSED);

          if (changed)
          {
            if(!params.use_metric)
            {
              params.gas_price = convertToGallons(fuelUnits);
            }
            else
            {
              params.gas_price = fuelUnits;
            }
            changed = false;
          }

          if (oldUIntValue != params.gas_price)
          {
            saveParams = true;
          }          
        }
        else if (adjustSelection == 3)
        {
          lcd_cls_print_P(PSTR("Fuel adjust"));
          oldByteValue = params.fuel_adjust;

          do
          {
            if(LEFT_BUTTON_PRESSED)
              params.fuel_adjust--;
            else if(RIGHT_BUTTON_PRESSED)
              params.fuel_adjust++;

            sprintf_P(str, pctdpctpct, params.fuel_adjust);
            displaySecondLine(4, str);
          } while(!MIDDLE_BUTTON_PRESSED);

          if (oldByteValue != params.fuel_adjust)
          {
            saveParams = true;
          }     
        }
        else if (adjustSelection == 4)
        {
          lcd_cls_print_P(PSTR("Speed adjust"));
          oldByteValue = params.speed_adjust;

          do
          { 
            if(LEFT_BUTTON_PRESSED)
              params.speed_adjust--;
            else if(RIGHT_BUTTON_PRESSED)
              params.speed_adjust++;

            sprintf_P(str, pctdpctpct, params.speed_adjust);
            displaySecondLine(4, str);
          } while(!MIDDLE_BUTTON_PRESSED);

          if (oldByteValue != params.fuel_adjust)
          {
            saveParams = true;
          }     
        }
        else if (adjustSelection == 5)
        {
          lcd_cls_print_P(PSTR("Outing stop over"));
          oldByteValue = params.OutingStopOver;

          do
          {
            if(LEFT_BUTTON_PRESSED && params.OutingStopOver > 0)
              params.OutingStopOver--;
            else if(RIGHT_BUTTON_PRESSED && params.OutingStopOver < UCHAR_MAX)
              params.OutingStopOver++;

            sprintf_P(str, PSTR("- %2d Min + "), params.OutingStopOver * MINUTES_GRANULARITY);
            displaySecondLine(3, str);
          } while(!MIDDLE_BUTTON_PRESSED);

          if (oldByteValue != params.OutingStopOver)
          {
            saveParams = true;
          }     

        }
        else if (adjustSelection == 6)
        {
          lcd_cls_print_P(PSTR("Trip stop over"));
          oldByteValue = params.TripStopOver;

          do
          {
            unsigned long TripStopOver;   // Allowable stop over time (in milliseconds). Exceeding time starts a new outing.
  
            if(LEFT_BUTTON_PRESSED && params.TripStopOver > 1)
              params.TripStopOver--;
            else if(RIGHT_BUTTON_PRESSED && params.TripStopOver < UCHAR_MAX)
              params.TripStopOver++;

            sprintf_P(str, PSTR("- %2d Hrs + "), params.TripStopOver);
            displaySecondLine(3, str);
          } while(!MIDDLE_BUTTON_PRESSED);

          if (oldByteValue != params.TripStopOver)
          {
            saveParams = true;
          }     
        }
        else if (adjustSelection == 7)
        {
          lcd_cls_print_P(PSTR("Eng dplcmt (MAP)"));
          oldByteValue = params.eng_dis;

          // the following setting is for MAP only
          // engine displacement

          do
          {
            if(LEFT_BUTTON_PRESSED && params.eng_dis!=0)
              params.eng_dis--;
            else if(RIGHT_BUTTON_PRESSED && params.eng_dis!=100)
              params.eng_dis++;

            long_to_dec_str(params.eng_dis, decs, 1);
            sprintf_P(str, PSTR("- %sL + "), decs);
            displaySecondLine(4, str);
          }
          while(!MIDDLE_BUTTON_PRESSED);

          if (oldByteValue != params.eng_dis)
          {
            saveParams = true;
          }     
        }
      } while (adjustSelection != 0);
    }   
    else if (selection == 3) // PIDs
    { 
      // go through all the configurable items
      byte PIDSelection = 0;
      byte cur_screen;
      byte pid = 0;
     
      // Set PIDs required for the selected screen
      do
      {
        PIDSelection = menu_selection(PIDMenu, ARRAY_SIZE(PIDMenu));
       
        if (PIDSelection != 0 && PIDSelection <= NBSCREEN)
        {
          cur_screen = PIDSelection - 1;
          for(byte current_PID=0; current_PID<LCD_PID_count; current_PID++)
          {
            lcd_cls();
            sprintf_P(str, PSTR("Scr %d      PID %d"), cur_screen+1, current_PID+1);
            lcd_print(str);
            oldByteValue = pid = params.screen[cur_screen].PID[current_PID];

            do
            {
              if(LEFT_BUTTON_PRESSED)
              {
                // while we do not find a supported PID, decrease
                while(!is_pid_supported(--pid, 1));
              }
              else if(RIGHT_BUTTON_PRESSED)
              {
                // while we do not find a supported PID, increase
                while(!is_pid_supported(++pid, 1));
              }
              
              sprintf_P(str, PSTR("- %8s +  "), (char*)pgm_read_word(&(PID_Desc[pid])));
              displaySecondLine(2, str);
            } while(!MIDDLE_BUTTON_PRESSED);
 
            // PID has changed so set it
            if (oldByteValue != pid)
            {
              params.screen[cur_screen].PID[current_PID]=pid;
              saveParams = true;
            }
          }
        }
      } while (PIDSelection != 0);
    }  
  } while (selection != 0);  

  if (saveParams)
  {
    // save params in EEPROM
    lcd_cls_print_P(PSTR("Saving config"));
    lcd_gotoXY(0,1);
    lcd_print_P(PSTR("Please wait..."));
    params_save();
  }
}*/

// Menu selection
// 
// This function is passed in a array of strings which comprise of the menu
// The first string is the MENU TITLE,
// The second string is the EXIT option (always first option)
// The following strings are the other options in the menu
//
// The returned value denotes the selection of the user:
// A return of zero represents the exit
// A return of a real number represents the selection from the menu past exit (ie 2 would be the second item past EXIT)
byte menu_selection(char ** menu, byte arraySize)
{
  byte selection = 1; // Menu title takes up the first string in the list so skip it
  byte screenChars = 0;  // Characters currently sent to screen
  byte menuItem = 0;     // Menu items past current selection
  boolean exitMenu = false;

  // Note: values are changed with left/right and set with middle
  // Default selection is always the first selection, which should be 'Exit'
  
  lcd_cls();
  lcd_print((char *)pgm_read_word(&(menu[0])));
  delay_reset_button();  // make sure to clear button

  do
  {
    if(LEFT_BUTTON_PRESSED && selection > 1)
    {
      selection--;
    }
    else if(RIGHT_BUTTON_PRESSED && selection < arraySize - 1)
    {
      selection++;
    }
    else if (MIDDLE_BUTTON_PRESSED && true)
    {
      exitMenu = true;
    }   
    // Potential improvements:
    // Currently the selection is ALWAYS the first presented menu item.
    // Current selection could be in the middle if possible.
    // If few selections and screen size permits, selections could be centered?
    
    lcd_gotoXY(0,1);
    screenChars = 1;
    lcd_dataWrite('('); // Wrap the current selection with brackets
    menuItem = 0;
    do
    {
      lcd_print((char*)pgm_read_word(&(menu[selection+menuItem])));

      if (menuItem == 0) 
      {
        // include closing bracket
        lcd_dataWrite(')');
        screenChars++;
      }  
      lcd_dataWrite(' ');
      screenChars += (strlen((char*)pgm_read_word(&(menu[selection+menuItem]))) + 1);
      menuItem++;
    }
    while (screenChars < LCD_width && selection + menuItem < arraySize);

    // Do any cover up of old data
    while (screenChars < LCD_width)
    {
      lcd_dataWrite(' ');
      screenChars++;
    }

    // Clean up button presses 
    delay_reset_button();
    
  }
  while(!exitMenu);

  return selection - 1;
}

void displayPids() {
    // for some reason the display on LCD
    for(byte current_PID=0; current_PID<LCD_PID_count; current_PID++)
      display(current_PID, params.screen[active_screen].PID[current_PID]);
}
