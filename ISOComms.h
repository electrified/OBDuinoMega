#include "WProgram.h"

void serial_rx_on();
void serial_rx_off();
void serial_tx_off();
bool iso_read_byte(byte * b);
void iso_write_byte(byte b);
byte iso_checksum(byte *data, byte len);
byte iso_write_data(byte *data, byte len);
byte iso_read_data(byte *data, byte len);
void iso_init();
void iso_init_loop();

boolean get_pid_iso(byte pid, byte reslen, byte buf[]);
