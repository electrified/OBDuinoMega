
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/version.h>
#include "Common.h"
#include "Button.h"
#include "Menu2.h"

char PowerSave = FALSE;

unsigned char state;  // holds the current state, according to "menu.h"


__attribute__ ((OS_main)) int main(void)
{    
//  unsigned char state, nextstate;
    unsigned char nextstate;
    // mt static char __flash *statetext;
    PGM_P statetext;
    char (*pStateFunc)(char);
    char input;
    uint8_t i, j; // char i;
    char buttons;
    char last_buttons;

    last_buttons='\0';  // mt

    // Initial state variables
    state = ST_AVRBF;
    nextstate = ST_AVRBF;
    statetext = MT_AVRBF;
    pStateFunc = NULL;


    // Program initalization
    Initialization();
    sei(); // mt __enable_interrupt();

    for (;;)            // Main loop
    {
            // Plain menu text
            if (statetext)
            {
                LCD_puts_f(statetext, 1);
                LCD_Colon(0);
                statetext = NULL;
            }
 
            input = getkey();           // Read buttons
    
            if (pStateFunc)
            {
                // When in this state, we must call the state function
                nextstate = pStateFunc(input);
            }
            else if (input != KEY_NULL)
            {
                // Plain menu, clock the state machine
                nextstate = StateMachine(state, input);
            }
    
            if (nextstate != state)
            {
                state = nextstate;
                // mt: for (i=0; menu_state[i].state; i++)
                for (i=0; (j=pgm_read_byte(&menu_state[i].state)); i++)
                {
                    //mt: if (menu_state[i].state == state)
                    //mt 1/06 if (pgm_read_byte(&menu_state[i].state) == state)
                    if (j == state)
                    {
                        statetext =  (PGM_P) pgm_read_word(&menu_state[i].pText);
                        pStateFunc = (PGM_VOID_P) pgm_read_word(&menu_state[i].pFunc);
                        // mtE
                        break;
                    }
                }
            }
        }
       
        // Check if the joystick has been in the same position for some time, 
        // then activate auto press of the joystick
        buttons = (~PINB) & PINB_MASK;
        buttons |= (~PINE) & PINE_MASK;
        
        if( buttons != last_buttons ) 
        {
            last_buttons = buttons;
            gAutoPressJoystick = FALSE;
        }
        else if( buttons )
        {
            if( gAutoPressJoystick == TRUE)
            {
                PinChangeInterrupt();
                gAutoPressJoystick = AUTO;
            }
            else    
                gAutoPressJoystick = AUTO;
        }
        // SMCR-reset is not needed since (from avr-libc Manual): 
        // This [the sleep_mode] macro automatically takes care to 
        // enable the sleep mode in the CPU before going to sleep, 
        // and disable it again afterwards
        // SMCR = 0;                       // Just woke, disable sleep

    } //End Main loop

    return 0; // mt
}


/*****************************************************************************
*
*   Function name : StateMachine
*
*   Returns :       nextstate
*
*   Parameters :    state, stimuli
*
*   Purpose :       Shifts between the different states
*
*****************************************************************************/
unsigned char StateMachine(char state, unsigned char stimuli)
{
    unsigned char nextstate = state;    // Default stay in same state
    unsigned char i, j;

    // mt: for (i=0; menu_nextstate[i].state; i++)
    for (i=0; ( j=pgm_read_byte(&menu_nextstate[i].state) ); i++ )
    {
        // mt: if (menu_nextstate[i].state == state && menu_nextstate[i].input == stimuli)
        // mt 1/06 : if (pgm_read_byte(&menu_nextstate[i].state) == state && 
        if ( j == state && 
             pgm_read_byte(&menu_nextstate[i].input) == stimuli)
        {
            // This is the one!
            // mt: nextstate = menu_nextstate[i].nextstate;
            nextstate = pgm_read_byte(&menu_nextstate[i].nextstate);
            break;
        }
    }

    return nextstate;
}
