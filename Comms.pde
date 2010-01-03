
#ifdef useECUState
boolean verifyECUAlive(void)
{
#ifdef ELM
  char cmd_str[6];   // to send to ELM
  char str[STRLEN];   // to receive from ELM
  sprintf_P(cmd_str, PSTR("01%02X\r"), ENGINE_RPM);
  elm_write(cmd_str);
  elm_read(str, STRLEN);
  return elm_check_response(cmd_str, str) == 0;
#else //ISO
  #ifdef do_ISO_Reinit
  if (!ECUconnection) // only check for off, finding active ECU is handled by successful reiniting 
  {
    return ECUconnection;
  }
  #endif
    // Send command to ECU, if it is active, we will get data back.
    // Set RPM to 1 if ECU active and RPM above 0, otherwise zero.
    char str[STRLEN];
    boolean connected = get_pid(ENGINE_RPM, str, &tempLong);
    has_rpm = (connected && tempLong > 0) ? 1 : 0;

    return connected;
#endif
}
#endif
