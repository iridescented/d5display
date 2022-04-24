cls
C:\WinAVR-20100110\bin\avr-gcc -mmcu=atmega644p -DF_CPU=12000000 -Wall -Os -Wl,-u,vfprintf -lprintf_flt -lm algo.c lcd.c ili934x.c font.c -o prog.elf
C:\WinAVR-20100110\bin\avr-objcopy -O ihex prog.elf prog.hex
C:\WinAVR-20100110\bin\avrdude -c usbasp -p m644p -U flash:w:prog.hex