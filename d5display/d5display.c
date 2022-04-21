#include "d5display.h"
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

void cprint(char *str, uint16_t x, uint16_t y)
{
	display.x = x;
	display.y = y;
	display_string(str);
}

void intprint(int i, uint16_t x, uint16_t y)
{
	char tempch[10];
	sprintf(tempch, "%d", i);
	cprint(tempch, x, y);
}

void floatprint(float i, uint16_t x, uint16_t y)
{
	char tempch[10];
	switch (((int)i / 100 == 0) * 2 + ((int)i / 10 == 0))
	{
	case 2:
		sprintf(tempch, " %.2f", (double)i);
		break;
	case 3:
		sprintf(tempch, "  %.2f", (double)i);
		break;
	default:
		sprintf(tempch, "%.2f", (double)i);
		break;
	}
	cprint(tempch, x, y);
}

void draw_pixel(uint16_t x, uint16_t y, uint16_t col)
{
	write_cmd(COLUMN_ADDRESS_SET);
	write_data16(x);
	write_data16(x);
	write_cmd(PAGE_ADDRESS_SET);
	write_data16(y);
	write_data16(y);
	write_cmd(MEMORY_WRITE);
	write_data16(col);
}

void cut_corners(rectangle r, uint16_t col)
{
	draw_pixel(r.left, r.top, col);
	draw_pixel(r.left + 1, r.top, col);
	draw_pixel(r.left + 2, r.top, col);
	draw_pixel(r.left, r.top + 1, col);
	draw_pixel(r.left, r.top + 2, col);
	draw_pixel(r.left, r.bottom, col);
	draw_pixel(r.left + 1, r.bottom, col);
	draw_pixel(r.left + 2, r.bottom, col);
	draw_pixel(r.left, r.bottom - 1, col);
	draw_pixel(r.left, r.bottom - 2, col);
	draw_pixel(r.right, r.top, col);
	draw_pixel(r.right - 1, r.top, col);
	draw_pixel(r.right - 2, r.top, col);
	draw_pixel(r.right, r.top + 1, col);
	draw_pixel(r.right, r.top + 2, col);
	draw_pixel(r.right, r.bottom, col);
	draw_pixel(r.right - 1, r.bottom, col);
	draw_pixel(r.right - 2, r.bottom, col);
	draw_pixel(r.right, r.bottom - 1, col);
	draw_pixel(r.right, r.bottom - 2, col);
}

//                             brown           ,  whitish            ,     shadow_brown   ,  blue_complement
// uint16_t colorchart[4] = {RGB_CONV(186,154,136),RGB_CONV(229,229,229),RGB_CONV(172,126,98),RGB_CONV(136,164,186)};

// https://i.pinimg.com/originals/80/13/ab/8013ab7051ceef19cd08ab0303084baa.png
//                         1A8B                                      F7FE                                            FF2D
uint16_t colorchart[5] = {RGB_CONV(26, 83, 92), RGB_CONV(62, 205, 196), RGB_CONV(247, 255, 247), RGB_CONV(255, 107, 107), RGB_CONV(255, 230, 109)};

void Start()
{
	init_lcd();
	set_orientation(0);
	clear_screen();
	rectangle background = {0, LCDWIDTH - 1, 0, LCDHEIGHT - 1};
	rectangle brownboxes[3] = {{10, 230, 10, 125}, {10, 230, 133, 208}, {10, 230, 216, 310}};
	rectangle whiteboxes[3] = {{19, 221, 29, 30}, {19, 221, 153, 154}, {19, 221, 247, 248}};
	rectangle rfix[4] = {{21, 25, 24, 24}, {21, 25, 148, 148}, {21, 23, 231, 231}, {24, 25, 231, 231}};
	uint16_t i = 0;

	fill_rectangle(background, colorchart[1]);
	for (i = 0; i < 3; i++)
	{
		fill_rectangle(brownboxes[i], colorchart[0]);
		cut_corners(brownboxes[i], colorchart[2]);
	}
	for (i = 0; i < 3; i++)
		fill_rectangle(whiteboxes[i], colorchart[3]);
	display.background = colorchart[0];

	for (i = 0; i < 100; i++)
		draw_pixel(15 + i % 10, 15 + i / 10, pgm_read_word(&(icon_info[i])));
	fill_rectangle(rfix[0], colorchart[0]);

	for (i = 0; i < 100; i++)
		draw_pixel(15 + i % 10, 139 + i / 10, pgm_read_word(&(icon_energy[i])));
	fill_rectangle(rfix[1], colorchart[0]);

	for (i = 0; i < 100; i++)
		draw_pixel(15 + i % 10, 222 + i / 10, pgm_read_word(&(icon_load[i])));
	fill_rectangle(rfix[2], colorchart[4]);
	fill_rectangle(rfix[3], colorchart[0]);
	for (i = 0; i < 6400; i++)
		draw_pixel(141 + i % 80, 34 + i / 80, pgm_read_word(&(battery_level[i])));

	cprint("Usage..:       kW/h", 29, 55);
	cprint("Voltage:       V", 29, 70);
	cprint("Current:       A", 29, 85);
	cprint("Battery:", 29, 100);
	cprint("Team B", 65, 35);

	cprint("Main Power.........:       A", 29, 160);
	cprint("Wind Turbine.......:       A", 29, 175);
	cprint("Photo-Voltaic Array:       A", 29, 190);

	cprint("Load 1:        A", 29, 255);
	cprint("Load 2:        A", 29, 270);
	cprint("Load 3:        A", 29, 285);

	display.foreground = colorchart[3];
	cprint("Call | Supp", 135, 237);
	cprint("BASIC~INFO~~~~~~~~~~~~~~~~~~~~~~", 30, 17);
	cprint("SOURCE~~~~~~~~~~~~~~~~~~~~~~~~~~", 30, 141);
	cprint("LOADS~~~~~~~~~~~~~~~~~~~~~~~~~~~", 30, 224);
	display.foreground = WHITE;

	for (i = 0; i < 3; i++)
	{
		rectangle call_supply_default[2] = {{134, 161, 254 + 15 * i, 263 + 15 * i}, {174, 201, 254 + 15 * i, 263 + 15 * i}};
		fill_rectangle(call_supply_default[0], colorchart[3]);
		fill_rectangle(call_supply_default[1], colorchart[3]);
	}
}

void Update(uint16_t battery_percent, double *BasicInfo, uint8_t BatteryState, double *Sources, double *Load, uint8_t *CallState, uint8_t *SupplyState)
{
	uint16_t i = 0;
	rectangle batt_lvl[5] = {{164, 197, 98, 110}, {164, 197, 84, 96}, {164, 197, 70, 82}, {164, 197, 56, 68}, {164, 197, 43, 54}};
	uint16_t batt_col[6] = {RGB_CONV(238, 28, 37), RGB_CONV(247, 148, 29), RGB_CONV(254, 242, 0), RGB_CONV(140, 198, 62), RGB_CONV(80, 186, 77), 0xFFFF};

	for (i = 0; i < 5 - battery_percent / 20; i++)
		fill_rectangle(batt_lvl[4 - i], batt_col[5]);
	for (i = 0; i < battery_percent / 20; i++)
		fill_rectangle(batt_lvl[i], batt_col[battery_percent / 20 - 1]);
	cprint("     ", 210, 100);
	intprint(battery_percent, 210, 100);

	for (i = 0; i < 3; i++)
	{
		rectangle call_supply[2] = {{136, 159, 255 + 15 * i, 262 + 15 * i}, {176, 199, 255 + 15 * i, 262 + 15 * i}};
		floatprint(BasicInfo[i], 80, 55 + 15 * i);
		floatprint(Sources[i], 150, 160 + 15 * i);
		floatprint(Load[i], 80, 255 + 15 * i);
		if (CallState[i] == 1)
			fill_rectangle(call_supply[0], batt_col[4]);
		else
			fill_rectangle(call_supply[0], batt_col[0]);
		if (SupplyState[i] == 1)
			fill_rectangle(call_supply[1], batt_col[4]);
		else
			fill_rectangle(call_supply[1], batt_col[5]);
	}
	if (BatteryState == 1)
		cprint("Charging   ", 80, 100);
	else if (BatteryState == -1)
		cprint("Discharging", 80, 100);
	else
		cprint("Idle       ", 80, 100);
}

int unused()
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