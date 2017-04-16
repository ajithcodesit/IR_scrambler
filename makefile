#This is the make file for the IR scrambler program

#Project variables
PROJECT=ir_scramble
SOURCES=ir_scramble.c
MMCU=attiny85

#Compile and build settings
CC=avr-gcc
OBJCOPY=avr-objcopy
OPTIMIZE=s
PROGRAMMER=usbasp

#The compile is optimized for the size of the hex on the flash
CFLAGS=-Wall -g -O$(OPTIMIZE) -mmcu=$(MMCU)

#Generating the hex file from the elf file
$(PROJECT).hex: $(PROJECT).elf
	$(OBJCOPY) -j .data -j .text -O ihex $(PROJECT).elf $(PROJECT).hex

#Genrating elf file from the object file
$(PROJECT).elf: $(PROJECT).o
	$(CC) $(CFLAGS) -o $(PROJECT).elf $(PROJECT).o

#Generating object file from the source file
$(PROJECT).o: $(SOURCES)
	$(CC) $(CFLAGS) -c $(SOURCES)

#______________________________RULES___________________________________#

#Command to upload the hex file to the microcontroller
program: $(PROJECT).hex
	sudo avrdude -c $(PROGRAMMER) -p $(MMCU) -U flash:w:$(PROJECT).hex
	
#Command to clear the files created during the compile operation
clean: 
	rm -f $(PROJECT).hex $(PROJECT).elf $(PROJECT).o

