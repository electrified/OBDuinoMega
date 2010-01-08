#ifndef Display_h
#define Display_h

// to differentiate trips
#define TANK         0
#define TRIP         1
#define OUTING       2  //Tracks your current outing 


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
void display_wasted();
unsigned int convertToGallons(unsigned int litres);
unsigned int convertToLitres(unsigned int gallons);

#endif
