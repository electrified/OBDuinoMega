#ifdef ENABLE_VDIP
/**
 * Copyright 2009 Jonathan Oxer <jon@oxer.com.au>
 * Distributed under the same terms as OBDuino
 */

unsigned long lastLogWrite = 0;  // Milliseconds since boot that the last entry was written

/**
 * initVdip
 * Set up the Vinculum VDIP flash storage device
 */
void initVdip()
{
  hostPrint(" * Initialising VDIP            ");
  pinMode(VDIP_STATUS_LED, OUTPUT);
  digitalWrite(VDIP_STATUS_LED, HIGH);

  pinMode(VDIP_WRITE_LED, OUTPUT);
  digitalWrite(VDIP_WRITE_LED, LOW);

  pinMode(VDIP_RTS_PIN, INPUT);

  pinMode(VDIP_RESET, OUTPUT);
  digitalWrite(VDIP_RESET, LOW);
  digitalWrite(VDIP_STATUS_LED, HIGH);
  digitalWrite(VDIP_WRITE_LED, HIGH);
  delay( 100 );
  digitalWrite(VDIP_RESET, HIGH);
  delay( 100 );
  VDIP.begin(VDIP_BAUD_RATE);  // Port for connection to Vinculum flash memory module
  VDIP.print("IPA");     // Sets the VDIP to ASCII mode
  VDIP.print(13, BYTE);

  digitalWrite(VDIP_STATUS_LED, LOW);
  digitalWrite(VDIP_WRITE_LED, LOW);
  hostPrintLn("[OK]");
}

/**
 * processVdipBuffer
 * Reads data sent by the VDIP module and passes it on to the host
 */
void processVdipBuffer()
{
  byte incomingByte;
  
  while( VDIP.available() > 0 )
  {
    incomingByte = VDIP.read();
    if( incomingByte == 13 ) {
      HOST.println();
    }
    HOST.print( incomingByte, BYTE );
  }
}

/**
 * modeButton
 * ISR attached to falling-edge interrupt 1 on digital pin 3
 */
void modeButton()
{
  if((millis() - logButtonTimestamp) > 300)  // debounce
  {
    logButtonTimestamp = millis();
    //HOST.println(logButtonTimestamp);
    digitalWrite(LOG_LED, !digitalRead(LOG_LED));
  }
}
void writeLogToFlash() {
 /////////////////////// WRITE TO FLASH //////////////////////////////
  if( logActive == 1 )
  {
    if(millis() - lastLogWrite > LOG_INTERVAL)
    {
      digitalWrite(VDIP_WRITE_LED, HIGH);
      byte position = 0;

      HOST.print(logEntry.length());
      HOST.print(": ");
      HOST.println(logEntry);

      VDIP.print("WRF ");
      VDIP.print(logEntry.length() + 1);  // 1 extra for the newline
      VDIP.print(13, BYTE);

      while(position < logEntry.length())
      {
        if(digitalRead(VDIP_RTS_PIN) == LOW)
        {
          VDIP.print(vdipBuffer[position]);
          position++;
        } else {
          HOST.println("BUFFER FULL");
        }
      }
      VDIP.print(13, BYTE);               // End the log entry with a newline
      digitalWrite(VDIP_WRITE_LED, LOW);

      lastLogWrite = millis();
    }
  }
}
#endif
