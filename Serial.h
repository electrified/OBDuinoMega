// Define serial ports so they can be easily switched for the Mega vs Duemilanove, etc
#ifdef MEGA
#define HOST Serial
#define HOST_BAUD_RATE 38400
#define OBD2 Serial1
#define OBD2_BAUD_RATE 38400
#define GPS  Serial2
#define GPS_BAUD_RATE 57600
#define VDIP Serial3
#define VDIP_BAUD_RATE 9600
#else
#define OBD2 Serial
#define OBD2_BAUD_RATE 38400
#ifdef DEBUG
#define HOST Serial
#define HOST_BAUD_RATE 38400
#endif
#endif
