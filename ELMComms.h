#ifndef ELMComms_h
#define ELMComms_h

#include "Arduino/WProgram.h"
#include "project_defs.h"

byte elm_read(char *str, byte size);
void elm_write(char *str);
byte elm_check_response(const char *cmd, char *str);
byte elm_compact_response(byte *buf, char *str);
byte elm_command(char *str, char *cmd);
void elm_init(void);

boolean get_pid_elm(byte pid, byte buf[]);

#endif
