#ifndef LCD_h
#define LCD_h
/* LCD Display parameters */
/* Adjust LCD_width or LCD_rows if LCD is different than 16 characters by 2 rows*/
// How many rows of characters for the LCD (must be at least two)
#define LCD_ROWS 2

void lcd_gotoXY(byte x, byte y);
void lcd_print(char *string);
void lcd_print_P(char *string);
void lcd_cls_print_P(char *string);
void lcd_init(byte contrast);
void lcd_cls();
void lcd_tickleEnable();
void lcd_commandWriteSet();
void lcd_pushNibble(byte value);
void lcd_commandWrite(byte value);
void lcd_dataWrite(byte value);
void lcd_setBrightness();
void needBacklight(bool On);
void lcd_increase_brightness();

#endif
