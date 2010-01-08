#ifndef GPS_h
#define GPS_h

void initGps();
bool processGpsBuffer();
void gpsdump(TinyGPS &gps);
#endif
