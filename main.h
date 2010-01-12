
//extern "C" void __cxa_pure_virtual() {}

#include "Arduino/WProgram.h"

// Menu state machine states
#define ST_AVRBF                        10
#define ST_AVRBF_REV                    11
#define ST_TIME                         20
#define ST_TIME_CLOCK                   21
#define ST_TIME_CLOCK_FUNC              22
#define ST_TIME_CLOCK_ADJUST            23
#define ST_TIME_CLOCK_ADJUST_FUNC       24
#define ST_TIME_CLOCKFORMAT_ADJUST      25
#define ST_TIME_CLOCKFORMAT_ADJUST_FUNC 36
void setup(void);
void loop(void);