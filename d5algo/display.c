#include "display.h"
#include "font.h"
#include "ili934x.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

/*	Command Fronts
C:\WinAVR-20100110\bin\avr-gcc -mmcu=atmega644p -DF_CPU=12000000 -Wall -Os d5display.c lcd.c ili934x.c font.c -o prog.elf
C:\WinAVR-20100110\bin\avr-objcopy -O ihex prog.elf prog.hex
C:\WinAVR-20100110\bin\avrdude -c usbasp -p m644p -U flash:w:prog.hex 2>&1 | tee LOG.txt

./turboAVR d5display.c lcd.c ili934x.c font.c
*/

rectangle background = {0, LCDWIDTH - 1, 0, LCDHEIGHT - 1};

void print(char *str, uint16_t x, uint16_t y)
{
    display.x = x;
    display.y = y;
    display_string(str);
}
void printi(int i, uint16_t x, uint16_t y)
{
    char tempch[10];
    sprintf(tempch, "%d", i);
    print(tempch, x, y);
}
void printfl(float i, uint16_t x, uint16_t y)
{
    char tempch[10];
    sprintf(tempch, "%5.2f", (double)i);
    print(tempch, x, y);
}

void draw_pixel(uint16_t x, uint16_t y, uint16_t col)
{
    write_cmd(COLUMN_ADDRESS_SET);
    write_data16(x);
    write_cmd(PAGE_ADDRESS_SET);
    write_data16(y);
    write_cmd(MEMORY_WRITE);
    write_data16(col);
}

void Start()
{
    rectangle brownboxes[3] = {{10, 230, 10, 125}, {10, 230, 133, 208}, {10, 230, 216, 310}};
    rectangle whiteboxes[3] = {{19, 221, 29, 30}, {19, 221, 153, 154}, {19, 221, 247, 248}};
    init_lcd();
    set_orientation(0);
    clear_screen();
    int i, j, k;
    fill_rectangle(background, WHITE);
    for (k = 0; k < 27; k++)
        for (j = 0; j < 9; j++)
            for (i = 0; i < 324; i++)
                draw_pixel(j * 27 + i % 27, k * 12 + i / 27, pgm_read_word(&(hexagon[i])));
    // for (i = 0; i < 3; i++)
    //     fill_rectangle(brownboxes[i], colorchart[4]);
    // for (i = 0; i < 1600; i++)
    //     draw_pixel(10 + i % 40, 10 + i / 40, pgm_read_word(&(d5logo[i])));
    // for (i = 0; i < 6400; i++)
    //     draw_pixel(10 + i % 80, 50 + i / 80, pgm_read_word(&(battery_level[i])));
}

void Update(uint16_t battery_percent, double *BasicInfo, uint8_t BatteryState, double *Sources, double *Load, uint8_t *CallState, uint8_t *SupplyState)
{
}

int main()
{
    uint16_t battery_percent = 0;
    double BasicInfo[3] = {159.32, 1.25, 1.09};
    uint8_t BatteryState = 1;
    double Sources[3] = {1.35, 15.15, 2.15};
    double Load[3] = {1.14, 2.00, 0.80};
    uint8_t CallState[3] = {0, 1, 0};
    uint8_t SupplyState[3] = {1, 1, 0};
    uint8_t activestate = 1;
    Start();
    while (activestate)
    {
        for (battery_percent = 0; battery_percent < 101; battery_percent++)
        {
            Update(battery_percent, BasicInfo, BatteryState, Sources, Load, CallState, SupplyState);
            _delay_ms(100);
        }
        _delay_ms(1000);
    }

    return 0;
}