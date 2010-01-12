// menu.h
#include "button.h"
// mt __flash typedef struct
typedef struct PROGMEM
{
    unsigned char state;
    unsigned char input;
    unsigned char nextstate;
} MENU_NEXTSTATE;


// mt __flash typedef struct
typedef struct PROGMEM
{
    unsigned char state;
    // char __flash *pText;
    PGM_P pText;
    char (*pFunc)(char input);
} MENU_STATE;


// Menu text
// mtA, these where all of the same structure as in the follow. line
// __flash char MT_AVRBF[]                         = "AVR Butterfly";
//const char MT_AVRBF[] PROGMEM                     = "AVR Butterfly GCC";
const char MT_AVRBF[] PROGMEM                     = 
	{'A','V','R',' ','B','U','T','T','E','R','F','L','Y',' ','G'+0x80,'C'+0x80,'C'+0x80,'\0'};
const char MT_TIME[] PROGMEM                      = "Time";
const char MT_TIME_CLOCK[] PROGMEM                = "Clock";
const char MT_TIME_CLOCK_ADJUST[] PROGMEM         = "Adjust Clock";
const char MT_TIME_CLOCKFORMAT_ADJUST[] PROGMEM   = "Change Clock Format";
// mtE

// mt MENU_NEXTSTATE menu_nextstate[] = { 
const MENU_NEXTSTATE menu_nextstate[] PROGMEM = {
//  STATE                       INPUT       NEXT STATE
    {ST_AVRBF,                  KEY_PLUS,   ST_OPTIONS},
    {ST_AVRBF,                  KEY_NEXT,   ST_AVRBF_REV},
    {ST_AVRBF,                  KEY_MINUS,  ST_TIME},

    {ST_AVRBF_REV,              KEY_PREV,   ST_AVRBF},

    {ST_TIME,                   KEY_PLUS,   ST_AVRBF},
    {ST_TIME,                   KEY_NEXT,   ST_TIME_CLOCK},
    {ST_TIME,                   KEY_PREV,   ST_AVRBF},
    {ST_TIME,                   KEY_MINUS,  ST_MUSIC},

    {ST_TIME_CLOCK,             KEY_PLUS,   ST_TIME_DATE},
    {ST_TIME_CLOCK,             KEY_NEXT,   ST_TIME_CLOCK_FUNC},
    {ST_TIME_CLOCK,             KEY_PREV,   ST_TIME},
    {ST_TIME_CLOCK,             KEY_MINUS,  ST_TIME_DATE},

    {ST_TIME_CLOCK_ADJUST,      KEY_PLUS,   ST_TIME_CLOCKFORMAT_ADJUST}, 
    {ST_TIME_CLOCK_ADJUST,      KEY_ENTER,  ST_TIME_CLOCK_ADJUST_FUNC},
    {ST_TIME_CLOCK_ADJUST,      KEY_PREV,   ST_TIME_CLOCK_FUNC},    
    {ST_TIME_CLOCK_ADJUST,      KEY_MINUS,  ST_TIME_CLOCKFORMAT_ADJUST}, 

    {ST_TIME_CLOCKFORMAT_ADJUST, KEY_PLUS,  ST_TIME_CLOCK_ADJUST},
    {ST_TIME_CLOCKFORMAT_ADJUST, KEY_ENTER, ST_TIME_CLOCKFORMAT_ADJUST_FUNC},
    {ST_TIME_CLOCKFORMAT_ADJUST, KEY_PREV,  ST_TIME_CLOCK_FUNC},
    {ST_TIME_CLOCKFORMAT_ADJUST, KEY_MINUS, ST_TIME_CLOCK_ADJUST},

    {0,                         0,          0}
};


// mt MENU_STATE menu_state[] = {
const MENU_STATE menu_state[] PROGMEM = {
//  STATE                               STATE TEXT                  STATE_FUNC
    {ST_AVRBF,                          MT_AVRBF,                   NULL},
    {ST_AVRBF_REV,                      NULL,                       SetClockFormat},
    {ST_TIME,                           MT_TIME,                    NULL},
    {ST_TIME_CLOCK,                     MT_TIME_CLOCK,              NULL},
    {ST_TIME_CLOCK_FUNC,                NULL,                       SetClockFormat},
    {ST_TIME_CLOCK_ADJUST,              MT_TIME_CLOCK_ADJUST,       NULL},
    {ST_TIME_CLOCK_ADJUST_FUNC,         NULL,                       SetClockFormat},
    {ST_TIME_CLOCKFORMAT_ADJUST,        MT_TIME_CLOCKFORMAT_ADJUST, NULL},
    {ST_TIME_CLOCKFORMAT_ADJUST_FUNC,   NULL,                       SetClockFormat},
    {0,                                 NULL,                       NULL}

};

unsigned char StateMachine(char state, unsigned char stimuli);
