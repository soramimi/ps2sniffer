TARGET=ps2sniffer
MCU = atmega32u2
F_CPU = 16000000

OBJECTS = \
	lcd.o \
	ps2if.o \
	quckey.o \
	queue16.o \
	usb.o \
	main.o \
	waitloop.o

CFLAGS = -Os -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Wall -Wextra -Werror=return-type -Wno-array-bounds
CC = avr-gcc $(CFLAGS)
CXX = avr-g++ $(CFLAGS) -std=c++11

all: $(TARGET).elf $(TARGET).hex
	avr-size -C --mcu=$(MCU) $(TARGET).elf

$(TARGET).hex: $(TARGET).elf
	avr-objcopy -O ihex $< $@

$(TARGET).elf: $(OBJECTS)
	$(CXX) -mmcu=$(MCU) $^ -o $@

.cpp.o:
	$(CXX) -c -mmcu=$(MCU) $^ -o $@

clean:
	rm -f *.o
	rm -f *.elf
	rm -f *.hex

write: $(TARGET).hex
	avrdude -c avrisp -P /dev/ttyACM0 -b 19200 -p $(MCU) -U efuse:w:0xf4:m -U hfuse:w:0xd9:m -U lfuse:w:0x5e:m -U flash:w:$(TARGET).hex

write2: $(TARGET).hex
	avrdude -c avrisp -P /dev/ttyUSB0 -b 19200 -p $(MCU) -U efuse:w:0xf4:m -U hfuse:w:0xd9:m -U lfuse:w:0x5e:m -U flash:w:$(TARGET).hex

fetch:
	avrdude -c avrisp -P /dev/ttyACM0 -b 19200 -p $(MCU) -U efuse:r:-:h -U hfuse:r:-:h -U lfuse:r:-:h

write2:
	ftavr -w $(TARGET).hex --avr-write-fuse-e f4 --avr-write-fuse-h d9 --avr-write-fuse-l 5e

