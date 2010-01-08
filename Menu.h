
// Number of screens of PIDs
#define NBSCREEN  3  // 12 PIDs should be enough for everyone

void displaySecondLine(byte position, char * str);
byte menu_select_yes_no(byte p);
void delay_reset_button();
void trip_reset(byte ctrip, boolean ask);
void test_buttons();
void config_menu(void);
byte menu_selection(char ** menu, byte arraySize);
void menu_init_buttons();
void displayPids();
