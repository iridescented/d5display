/*
Turbo AVR V 2.3
Author: Ivan Ling
Desc:

Turbo AVR is a quick utility designed to help students in loading code into 
Il Matto boards. Information about the Il-Matto board can be found here:
https://www.ecs.soton.ac.uk/outreach/kits/micro-arcana-series
	
*/


#include <stdio.h>
#include <windows.h>




int main( int argc, char *argv[] )  {
	
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD saved_attributes;
	FILE *fptr;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    saved_attributes = consoleInfo.wAttributes;    	
	char command[255];
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);
	system("cls");
	printf("********************************************\n");
	printf("Turbo AVR V3.5\n");
	printf("Created by: Ivan Ling (2020)\n");
	printf("********************************************\n");
	SetConsoleTextAttribute(hConsole, saved_attributes);
	SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN|FOREGROUND_RED);
	for(int i = 0; i<20; i++)
	printf("%c ",1);
	SetConsoleTextAttribute(hConsole, saved_attributes);		
	SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN|FOREGROUND_BLUE);
	if(argv[1] == NULL)
	{
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED|FOREGROUND_INTENSITY);
		printf("\nThat's not how this works, you dumbass!");
		printf("\nRead the documentations, punk!\n");
		printf("\nHint: \n\nturboAVR yourfilename.c\n\n");
		system("@echo off");
		printf("\nTake 5 seconds to read this properly!\n");
		for(int i = 0; i<5; i++)
		{
			printf("%c ",'*');
			system("sleep 1");
		}
		SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED|FOREGROUND_INTENSITY);
		return 0;
	}
	
	
	
	printf("\n\nPlease make sure Il-Matto is in programming mode...\n");
	system("pause");
	
	printf("\n\nCompiling codes...\n");
	system("@echo off");
	
	int i = 0;
	printf("\n\nLinking files and begin compilation...\n");
	sprintf(command, "avr-gcc -mmcu=atmega644p -DF_CPU=12000000 -Wall -Os -Wl,-u,vfprintf -lprintf_flt -lm ");
	for(i = 1; i < argc; i++)
	{
		strcat(command, argv[i]);
		strcat(command, " ");
	}
	strcat(command, "-o prog.elf");
	printf("\nCommand is: %s\n", command);
	
	system(command);
	printf("ELF generated...\n");
	printf("3 seconds till HEX loaded...\n");
	for(int i = 0; i<3; i++)
	{
		printf("%c ",'*');
		system("sleep 1");
	}
	printf("\nLoading HEX...\n");
	system("avr-objcopy -O ihex prog.elf prog.hex");
	system("C:\\WinAVR-20100110\\bin\\avrdude -c usbasp -p m644p -U flash:w:prog.hex 2>&1 | tee LOG.txt");
	system("pause");
	//system("avrdude -c usbasp -p m644p -U flash:w:prog.hex 2>&1 LOG.txt");
	system("cls");
	
/* 	fptr = fopen("LOG.txt","r");
	char *content;
	char ch;
	int j = 0;
	while (ch != EOF)
	{
		ch = fgetc(fptr);
		content[j] = ch;
		j++;
		printf("%c ", ch);
	}
	content[j] = '\0'; */
	//printf("Hello! %s", content);
	
	printf("PROGRAMMING DONE!\n");
	printf("Do check log for status!\n");
	printf("Thank you for using TurboAVR!\n");
	SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_RED|FOREGROUND_INTENSITY);
	SetConsoleTextAttribute(hConsole, saved_attributes);
    return 0;
}