#ifndef Comms_h
#define Comms_h


extern long tempLong;
//PID.h

/* PID stuff */

#define PID_SUPPORT00 0x00
#define MIL_CODE      0x01
#define FREEZE_DTC    0x02
#define FUEL_STATUS   0x03
#define LOAD_VALUE    0x04
#define COOLANT_TEMP  0x05
#define STFT_BANK1    0x06
#define LTFT_BANK1    0x07
#define STFT_BANK2    0x08
#define LTFT_BANK2    0x09
#define FUEL_PRESSURE 0x0A
#define MAN_PRESSURE  0x0B
#define ENGINE_RPM    0x0C
#define VEHICLE_SPEED 0x0D
#define TIMING_ADV    0x0E
#define INT_AIR_TEMP  0x0F
#define MAF_AIR_FLOW  0x10
#define THROTTLE_POS  0x11
#define SEC_AIR_STAT  0x12
#define OXY_SENSORS1  0x13
#define B1S1_O2_V     0x14
#define B1S2_O2_V     0x15
#define B1S3_O2_V     0x16
#define B1S4_O2_V     0x17
#define B2S1_O2_V     0x18
#define B2S2_O2_V     0x19
#define B2S3_O2_V     0x1A
#define B2S4_O2_V     0x1B
#define OBD_STD       0x1C
#define OXY_SENSORS2  0x1D
#define AUX_INPUT     0x1E
#define RUNTIME_START 0x1F
#define PID_SUPPORT20 0x20
#define DIST_MIL_ON   0x21
#define FUEL_RAIL_P   0x22
#define FUEL_RAIL_DIESEL 0x23
#define O2S1_WR_V     0x24
#define O2S2_WR_V     0x25
#define O2S3_WR_V     0x26
#define O2S4_WR_V     0x27
#define O2S5_WR_V     0x28
#define O2S6_WR_V     0x29
#define O2S7_WR_V     0x2A
#define O2S8_WR_V     0x2B
#define EGR           0x2C
#define EGR_ERROR     0x2D
#define EVAP_PURGE    0x2E
#define FUEL_LEVEL    0x2F
#define WARM_UPS      0x30
#define DIST_MIL_CLR  0x31
#define EVAP_PRESSURE 0x32
#define BARO_PRESSURE 0x33
#define O2S1_WR_C     0x34
#define O2S2_WR_C     0x35
#define O2S3_WR_C     0x36
#define O2S4_WR_C     0x37
#define O2S5_WR_C     0x38
#define O2S6_WR_C     0x39
#define O2S7_WR_C     0x3A
#define O2S8_WR_C     0x3B
#define CAT_TEMP_B1S1 0x3C
#define CAT_TEMP_B2S1 0x3D
#define CAT_TEMP_B1S2 0x3E
#define CAT_TEMP_B2S2 0x3F
#define PID_SUPPORT40 0x40
#define MONITOR_STAT  0x41
#define CTRL_MOD_V    0x42
#define ABS_LOAD_VAL  0x43
#define CMD_EQUIV_R   0x44
#define REL_THR_POS   0x45
#define AMBIENT_TEMP  0x46
#define ABS_THR_POS_B 0x47
#define ABS_THR_POS_C 0x48
#define ACCEL_PEDAL_D 0x49
#define ACCEL_PEDAL_E 0x4A
#define ACCEL_PEDAL_F 0x4B
#define CMD_THR_ACTU  0x4C
#define TIME_MIL_ON   0x4D
#define TIME_MIL_CLR  0x4E

#define LAST_PID      0x4E  // same as the last one defined above

/* Our internal fake PIDs */

#define FIRST_FAKE_PID 0xE9 // same as the first one defined below

#define OUTING_WASTE  0xE9    // fuel wasted since car started
#define TRIP_WASTE    0xEA    // fuel wasted during trip
#define TANK_WASTE    0xEB    // fuel wasted for this tank
#define OUTING_COST   0xEC    // the money spent since car started
#define TRIP_COST     0xED    // money spent since on trip
#define TANK_COST     0xEE    // money spent of current tank
#define ENGINE_ON     0xEF    // The length of time car has been running.
#define NO_DISPLAY    0xF0   
#define FUEL_CONS     0xF1    // instant cons
#define TANK_CONS     0xF2    // average cons of tank
#define TANK_FUEL     0xF3    // fuel used in tank
#define TANK_DIST     0xF4    // distance for tank
#define REMAIN_DIST   0xF5    // remaining distance of tank
#define TRIP_CONS     0xF6    // average cons of trip
#define TRIP_FUEL     0xF7    // fuel used in trip
#define TRIP_DIST     0xF8    // distance of trip
#define BATT_VOLTAGE  0xF9
#define OUTING_CONS   0xFA    // cons since the engine turned on
#define OUTING_FUEL   0xFB    // fuel used since engine turned on
#define OUTING_DIST   0xFC    // distance since engine turned on
#define CAN_STATUS    0xFD
#define PID_SEC       0xFE
#ifdef DEBUG                  //why waste a valueable PID space!!
#define FREE_MEM      0xFF
#else
#define ECO_VISUAL    0xFF   // Visually dispay relative economy with text (at end of program)
#endif

#include <avr/pgmspace.h>
extern prog_char pctldpcts[];

uint8_t getPidResponseLength(uint8_t *pid);
bool get_pid(uint8_t pid, char *retbuf, long *ret);
void check_supported_pids(long *tempLong);
bool is_pid_supported(uint8_t pid, uint8_t mode);
bool verifyECUAlive(void);
void setOldTime(void);
void getElapsedTime(unsigned long *delta_time);
uint8_t comms_pids_per_second(void);
void check_mil_code(long *tempLong);
void init_comms(void);
bool isEngineRunning(void);
#endif
