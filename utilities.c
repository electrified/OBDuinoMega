#include "Arduino\WProgram.h"
#include <avr/pgmspace.h>

// ex: get a long as 687 with prec 2 and output the string "6.87"
// precision is 1 or 2
void long_to_dec_str(long value, char *decs, byte prec)
{
  byte pos;

  // sprintf_P does not allow * for the width so manually change precision
  sprintf_P(decs, prec==2?PSTR("%03ld"):PSTR("%02ld"), value);

  pos=strlen(decs)+1;  // move the \0 too
  // a simple loop takes less space than memmove()
  for(byte i=0; i<=prec; i++)
  {
    decs[pos]=decs[pos-1];  // move digit
    pos--;
  }

  // then insert decimal separator
  decs[pos] = '.';//useComma ? ',' : '.';
}


#ifdef ENABLE_GPS

/**
 * printFloat
 */
void printFloat(double number, int digits)
{
  // Handle negative numbers
  if( number < 0.0 )
  {
     HOST.print( '-' );
     number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for( uint8_t i=0; i<digits; ++i )
    rounding /= 10.0;
  
  number += rounding;

  // Extract the integer part of the number and print it
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  HOST.print( int_part );

  // Print the decimal point, but only if there are digits beyond
  if( digits > 0 )
    HOST.print( "." ); 

  // Extract digits from the remainder one at a time
  while( digits-- > 0 )
  {
    remainder *= 10.0;
    int toPrint = int( remainder );
    HOST.print( toPrint );
    remainder -= toPrint; 
  } 
}

#endif
