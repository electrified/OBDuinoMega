#ifndef Display_h
#define Display_h
// to differentiate trips
#define TANK         0
#define TRIP         1
#define OUTING       2  //Tracks your current outing 
#define NBTRIP       3

void accu_trip(void);
//void getMafFromMap(char *str, long *tempLong);
void updateFuelUsed(unsigned long *delta_time);
void do_engine_on(void);
void do_engine_off(void);
void get_instant_fuel_cons(char *retbuf);
void get_fuel_cons(char *retbuf, byte ctrip);
void has_engine_started_or_stopped(void);
void display(byte location, byte pid);
void display_PID_names(byte screenId);
void get_fuel(char *retbuf, byte ctrip);
void get_waste(char *retbuf, byte ctrip);
void get_dist(char *retbuf, byte ctrip);
void get_remain_dist(char *retbuf);
void displayAlarmScreen(void);
void eco_visual(char *retbuf);
void get_engine_on_time(char *retbuf);
void get_cost(char *retbuf, byte ctrip);
void display_wasted(void);
unsigned int convertToGallons(unsigned int litres);
unsigned int convertToLitres(unsigned int gallons);
unsigned long calcTimeDiff(unsigned long start, unsigned long end);
void calc_init(void);

#endif
