###############################################################################
# Makefile for the project OBDuinoMega
###############################################################################

## General Flags
PROJECT = OBDuinoMega
MCU = atmega328p
TARGET = OBDuinoMega.elf
CC = avr-gcc

CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -gdwarf-2 -std=gnu99   -Wall          -DF_CPU=16000000UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-Map=OBDuinoMega.map


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Objects that must be built in order to link
OBJECTS = OBDuinoMega.o Comms.o Display.o ELMComms.o GPS.o Host.o LCD.o Memory.o Menu.o PowerFail.o utilities.o VDIP.o HardwareSerial.o pins_arduino.o Print.o WInterrupts.o wiring.o wiring_analog.o wiring_digital.o wiring_shift.o

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) OBDuinoMega.hex OBDuinoMega.eep OBDuinoMega.lss size

## Compile
OBDuinoMega.o: ../OBDuinoMega.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

Comms.o: ../Comms.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

Display.o: ../Display.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

ELMComms.o: ../ELMComms.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

GPS.o: ../GPS.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

Host.o: ../Host.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

LCD.o: ../LCD.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

Memory.o: ../Memory.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

Menu.o: ../Menu.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

PowerFail.o: ../PowerFail.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

utilities.o: ../utilities.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

VDIP.o: ../VDIP.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

HardwareSerial.o: ../Arduino/HardwareSerial.cpp
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

pins_arduino.o: ../Arduino/pins_arduino.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

Print.o: ../Arduino/Print.cpp
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

WInterrupts.o: ../Arduino/WInterrupts.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

wiring.o: ../Arduino/wiring.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

wiring_analog.o: ../Arduino/wiring_analog.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

wiring_digital.o: ../Arduino/wiring_digital.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

wiring_shift.o: ../Arduino/wiring_shift.c
	$(CPP) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CPP) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) OBDuinoMega.elf dep/* OBDuinoMega.hex OBDuinoMega.eep OBDuinoMega.lss OBDuinoMega.map


## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

