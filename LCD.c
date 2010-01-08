//LCD.c

#include "WProgram.h"

#include <avr/pgmspace.h>
#include "Common.h"
#include "Host.h"

#include "LCD.h"

byte brightnessIdx = 2;
// LCD pin assignments
#ifdef MEGA // LCD pins for an Arduino Mega
#define DIPin 54 // register select RS
#define DB4Pin 56
#define DB5Pin 57
#define DB6Pin 58
#define DB7Pin 59
#define ContrastPin 6
#define EnablePin 55
#define BrightnessPin 5
#else  // LCD Pins same as mpguino for a Duemilanove or equivalent
#define DIPin 4 // register select RS
#define DB4Pin 7
#define DB5Pin 8
#define DB6Pin 12
#define DB7Pin 13
#define ContrastPin 6
#define EnablePin 5
#define BrightnessPin 9
#endif

/*
 * LCD functions
 * x=0..16, y=0..1  (for 16x2)
 * x=0..20, y=0..3  (for 20x4)
 */
/**
 * lcd_gotoXY
 */
void lcd_gotoXY(byte x, byte y)
{
  byte dr = 0x80 + x;
  if (y&1) dr+= 0x40;
  if (y&2) dr+= 0x14;
  lcd_commandWrite( dr );
}

/**
 * lcd_print
 */
void lcd_print(char *string)
{
  while(*string != 0)
    lcd_dataWrite(*string++);
}

/**
 * lcd_print_P
 */
void lcd_print_P(char *string)
{
  char str[STRLEN];

  sprintf_P(str, string);
  lcd_print(str);
}

void lcd_cls_print_P(char *string)
{
  lcd_cls();
  lcd_print_P(string);
}

// do the lcd initialization voodoo
// thanks to Yoshi "SuperMID" for tips :)
void lcd_init(byte contrast)
{
  analogWrite(ContrastPin, contrast);
  pinMode(EnablePin,OUTPUT);
  pinMode(DIPin,OUTPUT);
  pinMode(DB4Pin,OUTPUT);
  pinMode(DB5Pin,OUTPUT);
  pinMode(DB6Pin,OUTPUT);
  pinMode(DB7Pin,OUTPUT);
  
  delay(16);                    // wait for more than 15 msec

  for(byte i=0; i<3; i++)
  {
    lcd_pushNibble(B00110000);  // send (B0011) to DB7-4
    lcd_commandWriteSet();
    delay(5);                     // wait for more than 4.1 msec or 100 usec
  }

  lcd_pushNibble(B00100000);  // send (B0010) to DB7-4 for 4bit
  lcd_commandWriteSet();
  delay(1);                     // wait for more than 100 usec
  // ready to use normal CommandWrite() function now!

  lcd_commandWrite(B00101000);   // 4-bit interface, 2 display lines, 5x8 font
  lcd_commandWrite(B00001100);   // display control on, no cursor, no blink
  lcd_commandWrite(B00000110);   // entry mode set: increment automatically, no display shift

  //creating the custom fonts (8 char max)
  // char 0 is not used
  // 1&2 is the L/100 datagram in 2 chars only
  // 3&4 is the km/h datagram in 2 chars only
  // 5 is the � char (degree)
  // 6&7 is the mi/g char
#define NB_CHAR  7
  // set cg ram to address 0x08 (B001000) to skip the
  // first 8 rows as we do not use char 0
  lcd_commandWrite(B01001000);
  static prog_uchar chars[] PROGMEM ={
    B10000,B00000,B10000,B00010,B00111,B11111,B00010,
    B10000,B00000,B10100,B00100,B00101,B10101,B00100,
    B11001,B00000,B11000,B01000,B00111,B10101,B01000,
    B00010,B00000,B10100,B10000,B00000,B00000,B10000,
    B00100,B00000,B00000,B00100,B00000,B00100,B00111,
    B01001,B11011,B11111,B00100,B00000,B00000,B00100,
    B00001,B11011,B10101,B00111,B00000,B00100,B00101,
    B00001,B11011,B10101,B00101,B00000,B00100,B00111,
  };

  for(byte x=0;x<NB_CHAR;x++)
    for(byte y=0;y<8;y++)  // 8 rows
      lcd_dataWrite(pgm_read_byte(&chars[y*NB_CHAR+x])); //write the character data to the character generator ram

  lcd_cls();
  lcd_commandWrite(B10000000);  // set dram to zero
}

void lcd_cls()
{
  lcd_commandWrite(B00000001);  // Clear Display
  lcd_commandWrite(B00000010);  // Return Home
}

void lcd_tickleEnable()
{
  // send a pulse to enable
  digitalWrite(EnablePin,HIGH);
  delayMicroseconds(1);  // pause 1 ms according to datasheet
  digitalWrite(EnablePin,LOW);
  delayMicroseconds(1);  // pause 1 ms according to datasheet
}

void lcd_commandWriteSet()
{
  digitalWrite(EnablePin,LOW);
  delayMicroseconds(1);  // pause 1 ms according to datasheet
  digitalWrite(DIPin,0);
  lcd_tickleEnable();
}

void lcd_pushNibble(byte value)
{
  digitalWrite(DB7Pin, value & 128);
  digitalWrite(DB6Pin, value & 64);
  digitalWrite(DB5Pin, value & 32);
  digitalWrite(DB4Pin, value & 16);
}

void lcd_commandWrite(byte value)
{
  lcd_pushNibble(value);
  lcd_commandWriteSet();
  value<<=4;
  lcd_pushNibble(value);
  lcd_commandWriteSet();
  delay(5);
}

void lcd_dataWrite(byte value)
{
  digitalWrite(DIPin, HIGH);
  lcd_pushNibble(value);
  lcd_tickleEnable();
  value<<=4;
  lcd_pushNibble(value);
  lcd_tickleEnable();
  delay(5);
}

#define brightnessLength 7 //array size
const byte brightness[brightnessLength]={
   0xFF,
   0xFF/brightnessLength*(brightnessLength-1),
   0xFF/brightnessLength*(brightnessLength-2),
   0xFF/brightnessLength*(brightnessLength-3),
   0xFF/brightnessLength*(brightnessLength-4),
   0xFF/brightnessLength*(brightnessLength-5),
   0x00}; // right button cycles through these brightness settings (off to on full)

void lcd_setBrightness() {
  analogWrite(BrightnessPin, brightness[brightnessIdx]);
}

void lcd_increase_brightness() {
    char str[STRLEN] = {0};

    brightnessIdx = (brightnessIdx + 1) % brightnessLength;
    
    lcd_setBrightness(brightnessIdx);

    lcd_cls_print_P(PSTR(" LCD backlight"));
    lcd_gotoXY(6, 1);
    sprintf_P(str, PSTR("%d / %d"), brightnessIdx + 1, brightnessLength);
    lcd_print(str);
}

/**
 * needBacklight
 */
void needBacklight(bool On)
{
  //only if ECU or engine are off do we need the backlight.
#ifdef useECUState
//  if (!ECUconnection)
#else
//  if (!engine_started)
#endif
//  {
    // Assume backlight is normally off, so set according to input On 
	if(On) 
    analogWrite(BrightnessPin, brightness[On ? brightnessIdx : 0]);
//  }
}
