#ifndef LCD_h
#define LCD_h
/* LCD Display parameters */
/* Adjust LCD_width or LCD_rows if LCD is different than 16 characters by 2 rows*/
// How many rows of characters for the LCD (must be at least two)
#define LCD_ROWS 2

void lcd_gotoXY(uint8_t x, uint8_t y);
void lcd_print(char *string);
void lcd_print_P(char *string);
void lcd_cls_print_P(char *string);
void lcd_init(uint8_t contrast);
void lcd_cls(void);
void lcd_tickleEnable(void);
void lcd_commandWriteSet(void);
void lcd_pushNibble(uint8_t value);
void lcd_commandWrite(uint8_t value);
void lcd_dataWrite(uint8_t value);
void lcd_setBrightness(void);
void needBacklight(boolean on);
void lcd_increase_brightness(void);

#endif
