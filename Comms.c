// Comms.pde
#include <avr/pgmspace.h>
#include "Arduino\WProgram.h"
#include "Common.h"
#include "ELMComms.h"
#include "Comms.h"
#include "Utilities.h"
#include "Memory.h"
#include "Host.h"
//#include "ARduino\Library\PString\PString.h"

prog_char pctldpcts[] PROGMEM="%ld %s"; // used in a couple of place

long tempLong;

unsigned long  pid01to20_support;  // this one always initialized at setup()
unsigned long  pid21to40_support=0;
unsigned long  pid41to60_support=0;

// returned length of the PID response.
// constants so put in flash
prog_uchar pid_reslen[] PROGMEM=
{
  // pid 0x00 to 0x1F
  4,4,2,2,1,1,1,1,1,1,1,1,2,1,1,1,
  2,1,1,1,2,2,2,2,2,2,2,2,1,1,1,4,

  // pid 0x20 to 0x3F
  4,2,2,2,4,4,4,4,4,4,4,4,1,1,1,1,
  1,2,2,1,4,4,4,4,4,4,4,4,2,2,2,2,

  // pid 0x40 to 0x4E
  4,8,2,2,2,1,1,1,1,1,1,1,1,2,2
};

byte getPidResponseLength(byte *pid) {
	return pgm_read_byte_near(pid_reslen + *pid);
}



unsigned long getpid_time;
byte nbpid_per_second=0;
unsigned long old_time;
unsigned long time_now;

void setOldTime(){
  old_time = millis();  // epoch
  getpid_time = old_time;
}


void getElapsedTime(unsigned long *delta_time) {
  time_now = millis();
  *delta_time = time_now - old_time;
  old_time = time_now;
}

/**
 * get_pid
 * Get value of a PID, and place in long pointer and also formatted for
 * string output in the return buffer. Return value denotes successful
 * retrieval of PID. User must pass in a long pointer to get the PID value.
 */
boolean get_pid(byte pid, char *retbuf, long *ret)
{
  //hostPrintLn("entering get_pid");
  byte buf[10];   // to receive the result
  byte reslen;
  char decs[16];
  unsigned long time_now, delta_time;
  static byte nbpid=0;
	params_t params = getParameters();
  nbpid++;
  // time elapsed
  time_now = millis();
  delta_time = time_now - getpid_time;
  if(delta_time>1000)
  {
    nbpid_per_second=nbpid;
    nbpid=0;
    getpid_time=time_now;
  }

  // check if PID is supported (should not happen except for some 0xFn)
  if(!is_pid_supported(pid, 0))
  {
    // nope
    sprintf_P(retbuf, PSTR("%02X N/A"), pid);
    return false;
  }

  // receive length depends on pid
  reslen = getPidResponseLength(&pid);
  //hostPrintLn("after supported");
#ifdef BLAH
  #ifdef ELM
  if (!get_pid_elm(pid, buf))
  #else
  if (!get_pid_iso(pid, reslen, buf))
  #endif
  {
    #ifndef DEBUG
      sprintf_P(retbuf, PSTR("ERROR"));
      return false;
    #endif     
  }
#endif
  // A lot of formulas are the same so calculate a default return value here
  // Even if it's scrapped later we still saved 40 bytes!
  *ret = buf[0] * 256U + buf[1];

  // formula and unit for each PID
  switch(pid)
  {
  case ENGINE_RPM:
#ifdef DEBUG
    *ret=1726;
#else
    *ret=*ret/4U;
#endif
    sprintf_P(retbuf, PSTR("%ld RPM"), *ret);
    break;
  case MAF_AIR_FLOW:
#ifdef DEBUG
    *ret=2048;
#endif
    // ret is not divided by 100 for return value!
    long_to_dec_str(*ret, decs, 2);
    sprintf_P(retbuf, PSTR("%s g/s"), decs);
    break;
  case VEHICLE_SPEED:
#ifdef DEBUG
    *ret=100;
#else

    *ret=(buf[0] * params.speed_adjust) / 100U;
#endif
    if(!params.use_metric)
      *ret=(*ret*1000U)/1609U;
    sprintf_P(retbuf, pctldpcts, *ret, params.use_metric?"\003\004":"\006\004");
    // do not touch vss, it is used by fuel calculation later, so reset it
#ifdef DEBUG
    *ret=100;
#else
    *ret=(buf[0] * params.speed_adjust) / 100U;
#endif
    break;
  case FUEL_STATUS:
#ifdef DEBUG
    *ret=0x0200;
#endif
    if(buf[0]==0x01)
      sprintf_P(retbuf, PSTR("OPENLOWT"));  // open due to insufficient engine temperature
    else if(buf[0]==0x02)
      sprintf_P(retbuf, PSTR("CLSEOXYS"));  // Closed loop, using oxygen sensor feedback to determine fuel mix. should be almost always this
    else if(buf[0]==0x04)
      sprintf_P(retbuf, PSTR("OPENLOAD"));  // Open loop due to engine load, can trigger DFCO
    else if(buf[0]==0x08)
      sprintf_P(retbuf, PSTR("OPENFAIL"));  // Open loop due to system failure
    else if(buf[0]==0x10)
      sprintf_P(retbuf, PSTR("CLSEBADF"));  // Closed loop, using at least one oxygen sensor but there is a fault in the feedback system
    else
      sprintf_P(retbuf, PSTR("%04lX"), *ret);
    break;
  case LOAD_VALUE:
  case THROTTLE_POS:
  case REL_THR_POS:
  case EGR:
  case EGR_ERROR:
  case FUEL_LEVEL:
  case ABS_THR_POS_B:
  case ABS_THR_POS_C:
  case ACCEL_PEDAL_D:
  case ACCEL_PEDAL_E:
  case ACCEL_PEDAL_F:
  case CMD_THR_ACTU:
#ifdef DEBUG
    *ret=17;
#else
    *ret=(buf[0]*100U)/255U;
#endif
    sprintf_P(retbuf, PSTR("%ld %%"), *ret);
    break;
  case B1S1_O2_V:
  case B1S2_O2_V:
  case B1S3_O2_V:
  case B1S4_O2_V:
  case B2S1_O2_V:
  case B2S2_O2_V:
  case B2S3_O2_V:
  case B2S4_O2_V:
    *ret=buf[0]*5U;  // not divided by 1000 for return!!
    if(buf[1]==0xFF)  // not used in trim calculation
      sprintf_P(retbuf, PSTR("%ld mV"), *ret);
    else
      sprintf_P(retbuf, PSTR("%ldmV/%d%%"), *ret, ((buf[1]-128)*100)/128);
    break;
  case O2S1_WR_V:
  case O2S2_WR_V:
  case O2S3_WR_V:
  case O2S4_WR_V:
  case O2S5_WR_V:
  case O2S6_WR_V:
  case O2S7_WR_V:
  case O2S8_WR_V:
  case O2S1_WR_C:
  case O2S2_WR_C:
  case O2S3_WR_C:
  case O2S4_WR_C:
  case O2S5_WR_C:
  case O2S6_WR_C:
  case O2S7_WR_C:
  case O2S8_WR_C:
  case CMD_EQUIV_R:
    *ret=(*ret*100)/32768; // not divided by 1000 for return!!
    long_to_dec_str(*ret, decs, 2);
    sprintf_P(retbuf, PSTR("l:%s"), decs);
    break;
  case DIST_MIL_ON:
  case DIST_MIL_CLR:
    if(!params.use_metric)
      *ret=(*ret*1000U)/1609U;
    sprintf_P(retbuf, pctldpcts, *ret, params.use_metric?"\003":"\006");
    break;
  case TIME_MIL_ON:
  case TIME_MIL_CLR:
    sprintf_P(retbuf, PSTR("%ld min"), *ret);
    break;
  case COOLANT_TEMP:
  case INT_AIR_TEMP:
  case AMBIENT_TEMP:
  case CAT_TEMP_B1S1:
  case CAT_TEMP_B2S1:
  case CAT_TEMP_B1S2:
  case CAT_TEMP_B2S2:
    if(pid>=CAT_TEMP_B1S1 && pid<=CAT_TEMP_B2S2)
#ifdef DEBUG
      *ret=600;
#else
      *ret=*ret/10U - 40;
#endif
    else
#ifdef DEBUG
      *ret=40;
#else
      *ret=buf[0]-40;
#endif
    if(!params.use_metric)
      *ret=(*ret*9)/5+32;
    sprintf_P(retbuf, PSTR("%ld\005%c"), *ret, params.use_metric?'C':'F');
    break;
  case STFT_BANK1:
  case LTFT_BANK1:
  case STFT_BANK2:
  case LTFT_BANK2:
    *ret=(buf[0]-128)*7812;  // not divided by 10000 for return value
    long_to_dec_str(*ret/100, decs, 2);
    sprintf_P(retbuf, PSTR("%s %%"), decs);
    break;
  case FUEL_PRESSURE:
  case MAN_PRESSURE:
  case BARO_PRESSURE:
    *ret=buf[0];
    if(pid==FUEL_PRESSURE)
      *ret*=3U;
    sprintf_P(retbuf, PSTR("%ld kPa"), *ret);
    break;
  case TIMING_ADV:
    *ret=(buf[0]/2)-64;
    sprintf_P(retbuf, PSTR("%ld\005"), *ret);
    break;
  case CTRL_MOD_V:
    long_to_dec_str(*ret/10, decs, 2);
    sprintf_P(retbuf, PSTR("%s V"), decs);
    break;
#ifndef DEBUG  // takes 254 bytes, may be removed if necessary
  case OBD_STD:
    *ret=buf[0];
    if(buf[0]==0x01)
      sprintf_P(retbuf, PSTR("OBD2CARB"));
    else if(buf[0]==0x02)
      sprintf_P(retbuf, PSTR("OBD2EPA"));
    else if(buf[0]==0x03)
      sprintf_P(retbuf, PSTR("OBD1&2"));
    else if(buf[0]==0x04)
      sprintf_P(retbuf, PSTR("OBD1"));
    else if(buf[0]==0x05)
      sprintf_P(retbuf, PSTR("NOT OBD"));
    else if(buf[0]==0x06)
      sprintf_P(retbuf, PSTR("EOBD"));
    else if(buf[0]==0x07)
      sprintf_P(retbuf, PSTR("EOBD&2"));
    else if(buf[0]==0x08)
      sprintf_P(retbuf, PSTR("EOBD&1"));
    else if(buf[0]==0x09)
      sprintf_P(retbuf, PSTR("EOBD&1&2"));
    else if(buf[0]==0x0a)
      sprintf_P(retbuf, PSTR("JOBD"));
    else if(buf[0]==0x0b)
      sprintf_P(retbuf, PSTR("JOBD&2"));
    else if(buf[0]==0x0c)
      sprintf_P(retbuf, PSTR("JOBD&1"));
    else if(buf[0]==0x0d)
      sprintf_P(retbuf, PSTR("JOBD&1&2"));
    else
      sprintf_P(retbuf, PSTR("OBD:%02X"), buf[0]);
    break;
#endif
    // for the moment, everything else, display the raw answer
  default:
    // transform buffer to an hex value
    *ret=0;
    byte i;
    for(i = 0; i < reslen; i++)
    {
      *ret*=256L;
      *ret+=buf[i];
    }
    sprintf_P(retbuf, PSTR("%08lX"), *ret);
    break;
  }
  //hostPrintLn("exiting get_pid");
  return true;
}




void check_supported_pids(long *tempLong)
{
  char str[STRLEN];

#ifdef DEBUG
  pid01to20_support=0xBE1FA812;
#else
  pid01to20_support  = (get_pid(PID_SUPPORT00, str, tempLong)) ? *tempLong : 0;
#endif

  if(is_pid_supported(PID_SUPPORT20, 0))
    pid21to40_support = (get_pid(PID_SUPPORT20, str, tempLong)) ? *tempLong : 0;

  if(is_pid_supported(PID_SUPPORT40, 0))
    pid41to60_support = (get_pid(PID_SUPPORT40, str, tempLong)) ? *tempLong : 0;
}

// return false if pid is not supported, true if it is.
// mode is 0 for get_pid() and 1 for menu config to allow pid > 0xF0
boolean is_pid_supported(byte pid, byte mode)
{
   return !((pid>0x00 && pid<=0x20 && ( 1L<<(0x20-pid) & pid01to20_support ) == 0 ) ||
            (pid>0x20 && pid<=0x40 && ( 1L<<(0x40-pid) & pid21to40_support ) == 0 ) ||
            (pid>0x40 && pid<=0x60 && ( 1L<<(0x60-pid) & pid41to60_support ) == 0 ) ||
            (pid>LAST_PID && (pid<FIRST_FAKE_PID || mode==0)));
 }

#ifdef useECUState
boolean verifyECUAlive(void)
{
  char cmd_str[6];   // to send to ELM
  char str[STRLEN];   // to receive from ELM
  sprintf_P(cmd_str, PSTR("01%02X\r"), ENGINE_RPM);
  elm_write(cmd_str);
  elm_read(str, STRLEN);
  return elm_check_response(cmd_str, str) == 0;
}
#endif

byte comms_pids_per_second() {
	return nbpid_per_second;
}

bool isEngineRunning() {
	char str[STRLEN];
	return (get_pid(ENGINE_RPM, str, &tempLong) && tempLong > 0) ? 1 : 0;
}

