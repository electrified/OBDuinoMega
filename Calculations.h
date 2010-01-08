#ifndef Calculations_h
#define Calculations_h

#define NBTRIP       3

void accu_trip();
void getMafFromMap(char *str, long *tempLong);
void updateFuelUsed(unsigned long *delta_time);

void do_engine_on();
void do_engine_off();
void get_instant_fuel_cons(char *retbuf);
void get_fuel_cons(char *retbuf, byte ctrip);
void has_engine_started_or_stopped();

unsigned long calcTimeDiff(unsigned long start, unsigned long end);
void calc_init();
#endif
