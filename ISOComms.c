//ISOComms.pde

#ifndef ELM

#include <avr/pgmspace.h>
#include "WProgram.h"
#include "ISOComms.h"
#include "LCD.h"
#include "Serial.h"


// ISO 9141 communication variables
byte ISO_InitStep = 0;  // Init is multistage, this is the counter
bool ECUconnection;  // Have we connected to the ECU or not

/*
 * for ISO9141-2 Protocol
 */
#define K_IN    0
#define K_OUT   1
#ifdef useL_Line
  #define L_OUT 2
#endif

/**
 * serial_rx_on
 */
void serial_rx_on() {
  OBD2.begin(10400);		//setting enable bit didn't work, so do beginSerial
}

/**
 * serial_rx_off
 */
void serial_rx_off() {
  UCSR0B &= ~(_BV(RXEN0));  //disable UART RX
}

/**
 * serial_tx_off
 */
void serial_tx_off() {

   UCSR0B &= ~(_BV(TXEN0));   //disable UART TX
   delay(20);                 //allow time for buffers to flush
}

#ifdef DEBUG
#define READ_ATTEMPTS 2
#else
#define READ_ATTEMPTS 125
#endif

/**
 * iso_read_byte
 * User must pass in a pointer to a byte to recieve the data.
 * Return value reflects success of the read attempt.
 */
bool iso_read_byte(byte * b)
{
  int readData;
  bool success = true;
  byte t=0;

  while(t != READ_ATTEMPTS  && (readData=OBD2.read())==-1) {
    delay(1);
    t++;
  }
  if (t>=READ_ATTEMPTS) {
    success = false;
  }
  if (success)
  {
    *b = (byte) readData;
  }

  return success;
}

/**
 * iso_write_byte
 */
void iso_write_byte(byte b)
{
  serial_rx_off();
  OBD2.print(b);
  delay(10);		// ISO requires 5-20 ms delay between bytes.
  serial_rx_on();
}

/**
 * iso_checksum
 * inspired by SternOBDII\code\checksum.c
 */
byte iso_checksum(byte *data, byte len)
{
  byte i;
  byte crc;

  crc=0;
  for(i=0; i<len; i++)
    crc=crc+data[i];

  return crc;
}

/**
 * iso_write_data
 * inspired by SternOBDII\code\iso.c
 */
byte iso_write_data(byte *data, byte len)
{
  byte i, n;
  byte buf[20];


  #ifdef ISO_9141
  // ISO header
  buf[0]=0x68;
  buf[1]=0x6A;		// 0x68 0x6A is an OBD-II request
  buf[2]=0xF1;		// our requester?s address (off-board tool)
  #else
  // 14230 protocol header
  buf[0]=0xc2; // Request of 2 bytes
  buf[1]=0x33; // Target address
  buf[2]=0xF1; // our requester?s address (off-board tool)
  #endif

  // append message
  for(i=0; i<len; i++)
    buf[i+3]=data[i];

  // calculate checksum
  i+=3;
  buf[i]=iso_checksum(buf, i);

  // send char one by one
  n=i+1;
  for(i=0; i<n; i++)
  {
    iso_write_byte(buf[i]);
  }

  return 0;
}

/**
 * iso_read_data
 * read n byte(s) of data (+ header + cmd and crc)
 * return the count of bytes of message (includes all data in message)
 */
byte iso_read_data(byte *data, byte len)
{
  byte i;
  byte buf[20];
  byte dataSize = 0;

  // header 3 bytes: [80+datalen] [destination=f1] [source=01]
  // data 1+1+len bytes: [40+cmd0] [cmd1] [result0]
  // checksum 1 bytes: [sum(header)+sum(data)]
  // a total of six extra bytes of data

  for(i=0; i<len+6; i++)
  {
    if (iso_read_byte(buf+i))
    {
      dataSize++;
    }
  }
  
  // test, skip header comparison
  // ignore failure for the moment (0x7f)
  // ignore crc for the moment

  // we send only one command, so result start at buf[4] Actually, result starts at buf[5], buf[4] is pid requested...
  memcpy(data, buf+5, len);

  delay(55);    //guarantee 55 ms pause between requests

  return dataSize;
}

/**
 * iso_init
 * ISO 9141 init
 * The init process is done in timed sections now so that during the reinit
 * process the user can use the buttons, and the screen can be updated.
 * Note: Due to the timed nature of this init process, if the display screen
 * takes up too much CPU time, this will not succeed
 */
void iso_init()
{
  long currentTime = millis();
  static long initTime;
#ifdef ISO_9141
  switch (ISO_InitStep)
  {
    case 0:
      // setup
      ECUconnection = false; 
      serial_tx_off(); //disable UART so we can "bit-Bang" the slow init.
      serial_rx_off();
      initTime = currentTime + 3000;
      ISO_InitStep++;
      break;
    case 1:
      if (currentTime >= initTime)
      {
        // drive K line high for 300ms
        digitalWrite(K_OUT, HIGH);
        #ifdef useL_Line
          digitalWrite(L_OUT, HIGH);
        #endif
        initTime = currentTime + 300;
        ISO_InitStep++;
      }
      break;
    case 2:       
    case 7:
      if (currentTime >= initTime)
      {
        // start or stop bit
        digitalWrite(K_OUT, (ISO_InitStep == 2 ? LOW : HIGH));
        #ifdef useL_Line
          digitalWrite(L_OUT, (ISO_InitStep == 2 ? LOW : HIGH));
        #endif
        initTime = currentTime + (ISO_InitStep == 2 ? 200 : 260);
        ISO_InitStep++;
      }
      break;
    case 3:       
    case 5:       
      if (currentTime >= initTime)
      {
        // two bits HIGH
        digitalWrite(K_OUT, HIGH);
        #ifdef useL_Line
          digitalWrite(L_OUT, HIGH);
        #endif
        initTime = currentTime + 400;
        ISO_InitStep++;
      }
      break;
    case 4:       
    case 6:       
      if (currentTime >= initTime)
      {
        // two bits LOW
        digitalWrite(K_OUT, LOW);
        #ifdef useL_Line
          digitalWrite(L_OUT, LOW);
          // Note: after this do we drive the L line back up high, or just leave it alone???          
        #endif
        initTime = currentTime + 400;
        ISO_InitStep++;
      }
      break;
    case 8:      
      if (currentTime >= initTime)
      {
        #ifdef useL_Line
          digitalWrite(L_OUT, LOW);
        #endif

        // bit banging done, now verify connection at 10400 baud
        byte b = 0;
        // switch now to 10400 bauds
        OBD2.begin(10400);

        // wait for 0x55 from the ECU (up to 300ms)
        //since our time out for reading is 125ms, we will try it up to three times
        byte i=0;
        while(i<3 && !iso_read_byte(&b))
        {
          i++;
        }
        
        if(b == 0x55)
        {
          ISO_InitStep++;
        }
        else 
        {
          // oops unexpected data, try again
          ISO_InitStep = 0;
        }
      }
      break;
    case 9:        
      if (currentTime >= initTime)
      {
        byte b;
        iso_read_byte(&b);  // read kw1
        iso_read_byte(&b);  // read kw2
        
        // send ~kw2 (invert of last keyword)
        iso_write_byte(~b);

        // ECU answer by 0xCC (~0x33)
        iso_read_byte(&b);
        if(b == 0xCC)
        {
           ECUconnection = true;
           // update for correct delta time in trip calculations.
           old_time = millis();
        }
        ISO_InitStep = 0;
      }
      break;
  }
#elif defined ISO_14230_fast
  switch (ISO_InitStep)
  {
    case 0:
      // setup
      ECUconnection = false; 
      serial_tx_off(); //disable UART so we can "bit-Bang" the slow init.
      serial_rx_off();
      initTime = currentTime + 3000;
      ISO_InitStep++;
      break;
    case 1:
      if (currentTime >= initTime)
      {
        // drive K line high for 300ms
        digitalWrite(K_OUT, HIGH);
        #ifdef useL_Line
          digitalWrite(L_OUT, HIGH);
        #endif
        initTime = currentTime + 300;
        ISO_InitStep++;
      }
      break;
    case 2:       
    case 3:       
      if (currentTime >= initTime)
      {
        // start or stop bit
        digitalWrite(K_OUT, (ISO_InitStep == 2 ? LOW : HIGH));
        #ifdef useL_Line
          digitalWrite(L_OUT, (ISO_InitStep == 2 ? LOW : HIGH));
        #endif
        initTime = currentTime + (ISO_InitStep == 2 ? 25 : 25);
        ISO_InitStep++;
      }
      break;
    case 4:      
      if (currentTime >= initTime)
      {
        // bit banging done, now verify connection at 10400 baud
        byte dataStream[] = {0xc1, 0x33, 0xf1, 0x81, 0x66};
        byte dataStreamSize = ARRAY_SIZE(dataStream);
        boolean gotData = false;
        const byte dataResponseSize = 10;
        byte dataResponse[dataResponseSize];
        byte responseIndex = 0;
        byte dataCaught = '\0';
           
        // switch now to 10400 bauds
        OBD2.begin(10400);

        // Send the message
        for (byte i = 0; i < dataStreamSize; i++)
        {
          iso_write_byte(dataStream[i]);
        }

        // Wait for response for 300 ms
        initTime = currentTime + 300;
        do
        {
           // If we find any data, keep catching it until it ends
           while (iso_read_byte(&dataCaught))
           {
              gotData = true;
              dataResponse[responseIndex] = dataCaught;
              responseIndex++;
           }           
        } while (millis() <= initTime && !gotData);

        if (gotData) // or better yet validate the data...
        {
           ECUconnection = true;
           // update for correct delta time in trip calculations.
           old_time = millis();
           
           // Note: we do not actually validate this connection. It would be best to validate the connection.
           // Can someone validate this with a car that actually uses this connection?
        }
        
        ISO_InitStep = 0;
      }
      break;
  }
#elif defined ISO_14230_slow
  switch (ISO_InitStep)
  {
    case 0:
      // setup
      ECUconnection = false; 
      serial_tx_off(); //disable UART so we can "bit-Bang" the slow init.
      serial_rx_off();
      initTime = currentTime + 3000;
      ISO_InitStep++;
      break;
    case 1:
      if (currentTime >= initTime)
      {
        // drive K line high for 300ms
        digitalWrite(K_OUT, HIGH);
        #ifdef useL_Line
          digitalWrite(L_OUT, HIGH);
        #endif
        initTime = currentTime + 300;
        ISO_InitStep++;
      }
      break;
    case 2:       
    case 7:
      if (currentTime >= initTime)
      {
        // start or stop bit
        digitalWrite(K_OUT, (ISO_InitStep == 2 ? LOW : HIGH));
        #ifdef useL_Line
          digitalWrite(L_OUT, (ISO_InitStep == 2 ? LOW : HIGH));
        #endif
        initTime = currentTime + (ISO_InitStep == 2 ? 200 : 260);
        ISO_InitStep++;
      }
      break;
    case 3:       
    case 5:       
      if (currentTime >= initTime)
      {
        // two bits HIGH
        digitalWrite(K_OUT, HIGH);
        #ifdef useL_Line
          digitalWrite(L_OUT, HIGH);
        #endif
        initTime = currentTime + 400;
        ISO_InitStep++;
      }
      break;
    case 4:       
    case 6:       
      if (currentTime >= initTime)
      {
        // two bits LOW
        digitalWrite(K_OUT, LOW);
        #ifdef useL_Line
          digitalWrite(L_OUT, LOW);
          // Note: after this do we drive the L line back up high, or just leave it alone???          
        #endif
        initTime = currentTime + 400;
        ISO_InitStep++;
      }
      break;
    case 8:      
      if (currentTime >= initTime)
      {
        // bit banging done, now verify connection at 10400 baud
        byte dataStream[] = {0xc1, 0x33, 0xf1, 0x81, 0x66};
        byte dataStreamSize = ARRAY_SIZE(dataStream);
        boolean gotData = false;
        const byte dataResponseSize = 10;
        byte dataResponse[dataResponseSize];
        byte responseIndex = 0;
        byte dataCaught = '\0';
      
        // switch now to 10400 bauds
        OBD2.begin(10400);

        // Send the message
        for (byte i = 0; i < dataStreamSize; i++)
        {
          iso_write_byte(dataStream[i]);
        }

        // Wait for response for 300 ms
        initTime = currentTime + 300;

        do
        {
           // If we find any data, keep catching it until it ends
           while (iso_read_byte(&dataCaught))
           {
              gotData = true;
              dataResponse[responseIndex] = dataCaught;
              responseIndex++;
           }           
        } while (millis() <= initTime && !gotData);
 
        if (gotData)
        {
           ECUconnection = true;
           // update for correct delta time in trip calculations.
           old_time = millis();

           // Note: we do not actually validate this connection. It would be best to validate the connection.
           // Can someone validate this with a car that actually uses this connection?
        }
        
        ISO_InitStep = 0;
      }
      break;
  }
//#else
//#error No ISO protocol defined
#endif // protocol
}

void iso_init_loop() {
  boolean success;
  // init pinouts
  pinMode(K_OUT, OUTPUT);
  pinMode(K_IN, INPUT);
  #ifdef useL_Line
  pinMode(L_OUT, OUTPUT);
  #endif

  do // init loop
  {
    lcd_gotoXY(2,1);
    #ifdef ISO_9141
      lcd_print_P(PSTR("ISO9141 Init"));
      hostPrint("ISO9141 Init ");
    #elif defined ISO_14230_fast
      lcd_print_P(PSTR("ISO14230 Fast"));
      hostPrint("ISO14230 Fast");
    #elif defined ISO_14230_slow
      lcd_print_P(PSTR("ISO14230 Slow"));
      hostPrint("ISO14230 Slow");
    #endif
    
    
    #ifdef DEBUG // In debug mode we need to skip init.
      success=true;
    #else 
      ISO_InitStep = 0;
      do
      {
        iso_init();
      } while (ISO_InitStep != 0);

      success = ECUconnection;
      #ifdef useECUState
        oldECUconnection != ECUconnection; // force 'turn on' stuff in main loop
      #endif
   #endif
    
    lcd_gotoXY(2,1);
    lcd_print_P(success ? PSTR("Successful!  ") : PSTR("Failed!         "));

    delay(1000);
  }
  while(!success); // end init loop
  }

  boolean get_pid_iso(byte pid, byte reslen, byte buf[]) {
  byte cmd[2];    // to send the command
  cmd[0] = 0x01;    // ISO cmd 1, get PID
  cmd[1] = pid;
  // send command, length 2
  iso_write_data(cmd, 2);
  // read requested length, n bytes received in buf
  if (!iso_read_data(buf, reslen))
  {
    #ifndef DEBUG
      //sprintf_P(retbuf, PSTR("ERROR"));
      return false;
    #endif     
  }
}
#endif
