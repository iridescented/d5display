#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include "d5display.c"

// define input located in Port A
// ADC inputs
#define PV PA0
#define WIND PA1
#define BUS_CURRENT PA2
#define BUS_VOLT PA3

// Digital Inputs
#define LOAD1 PA4
#define LOAD2 PA5
#define LOAD3 PA6

// define output located in Port D
#define MAINS PD7 // PWM
// Digital Outputs
#define LSW1 PD4
#define LSW2 PD5
#define LSW3 PD6

#define DB PD3
#define CB PD2

#define CLKFREQ 12000000 // 12MHz
#define MAXADC 1023
#define MAXBATTERY 10 // 10ah battery

// Display data
float count_power(uint16_t busvolt);

void pwm(void);
void timer_interrupt(void);
uint16_t read_adc(void);
void mux(uint16_t l);
void in_adc();
// input

void loadcalls(void);
void digital_inputs(void);
void bat_count(void);
// outputs
void digital_outputs(void);
void MainsReq(double a);
uint16_t Battery_Capacity = 0;
void Battery(uint16_t batt);
// global variables
uint16_t LoadCall1 = 0;
uint16_t LoadCall2 = 0;
uint16_t LoadCall3 = 0;
uint16_t DBattery;
uint16_t CBattery;
float sustain, main_current;
float power = 0;
uint16_t pv = 0;
uint16_t wind = 0;
uint16_t bc = 0;
uint16_t bv = 0;

volatile uint16_t count = 0x00;
volatile uint16_t count1 = 0x00;

uint16_t battery_percent = 0;
double BasicInfo[3] = {159.32, 1.25, 1.09};
uint8_t BatteryState = 0;
double Sources[3] = {1.35, 0.75, 0.25};
double Load[3] = {1.20, 2.00, 0.80};
uint8_t CallState[3] = {0, 1, 0};
uint8_t SupplyState[3] = {0, 0, 0}; //?Unentered

void digital_inputs(void)
{
	DDRA &= ~_BV(LOAD1);
	DDRA &= ~_BV(LOAD2);
	DDRA &= ~_BV(LOAD3); // Set bit 4 to 6 of Port A as inputs
}
void digital_outputs(void)
{
	DDRD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3) | _BV(DB) | _BV(CB); // Set bit 2 to 6 of Port D as outputs
}

void loadcalls(void)
{
	LoadCall1 = PINA & _BV(LOAD1) ? 1 : 0;
	LoadCall2 = PINA & _BV(LOAD2) ? 1 : 0;
	LoadCall3 = PINA & _BV(LOAD3) ? 1 : 0;
}

ISR(TIMER1_COMPA_vect)
{
	if (CBattery == 1)
	{
		Battery_Capacity++;
	}
	else if (DBattery == 1)
	{
		Battery_Capacity--;
	}
	else
	{
	}
}

void pwm(void)
{
	DDRD |= _BV(MAINS);
	TCCR2A |= _BV(WGM20); // Mode 1, PWM, Phase Correct, 10-bit
	TCCR2A |= _BV(COM2A1);
	TCCR2B |= _BV(CS22) | _BV(CS20); // F_CPU = 12MHz and pre-scaler = 1024, F_CLK = 11.71kHz
}
void timer_interrupt(void)
{
	// TCCR1A |= _BV(COM0A1) | _BV(COM0A0);
	TIMSK1 |= _BV(OCIE1A);			 // enable timer interrupt for timer1
	TCCR1B |= _BV(WGM12);			 // Clear Timer on Compare Match mode (CTC)
	TCCR1B |= _BV(CS12) | _BV(CS10); // F_CPU = 12MHz and pre-scaler = 1024, F_CLK = 11.71kHz
	OCR1A = 5858;					 // 1 seconds
}
void MainsReq(double a)
{
	OCR2A = a * 25.5;
	main_current = a * 0.2;
}

void system()
{
	switch (LoadCall1 * 4 + LoadCall2 * 2 + LoadCall3)
	{
	case 7: // 111, demand = 4A
		switch ((int)(sustain * 10))
		{
		case 40 ... 100:
			MainsReq(0.0); // 0A, now have 4A
			PORTD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3);
			SupplyState[0] = 1;
			SupplyState[1] = 1;
			SupplyState[2] = 1;
			// LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
			CBattery = 0;
			DBattery = 0;
			PORTD &= ~_BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 30 ... 39: // have 3.0A to 3.9A
			if ((Battery_Capacity > 0) && ((sustain * 10) == 30))
			{ // check if sustain is 3A or not, draw 1A, now have 4A
				DBattery = 1;
				CBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				MainsReq(0.0); // 0A, now have 4A
				PORTD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[1] = 1;
				SupplyState[2] = 1;
				// LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
			}

			else
			{ // no battery or sustain not equal to 3A
				MainsReq((40 - (int)(sustain * 10)) * 0.5);
				PORTD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[1] = 1;
				SupplyState[2] = 1;
				// LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;
		case 20 ... 29: // have 2.9A
			if (Battery_Capacity > 0)
			{ // 1A, now have 3.9A
				MainsReq((30 - (int)(sustain * 10)) * 0.5);
				DBattery = 1;
				CBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				PORTD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[1] = 1;
				SupplyState[2] = 1;
				// LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
			}
			else
			{ // 2.9A
				MainsReq((40 - (int)(sustain * 10)) * 0.5);
				PORTD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[1] = 1;
				SupplyState[2] = 1;
				// LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;
		case 12 ... 19: // have 1.9A
			if (Battery_Capacity > 0)
			{ // 1A, now have 2.9A
				MainsReq((30 - (int)(sustain * 10)) * 0.5);
				DBattery = 1;
				CBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				PORTD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[1] = 1;
				SupplyState[2] = 1;
				// LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
			}
			else
			{ // 1.9A
				MainsReq((32 - (int)(sustain * 10)) * 0.5);
				PORTD |= _BV(LSW1) | _BV(LSW2);
				SupplyState[0] = 1;
				SupplyState[1] = 1;
				PORTD &= ~_BV(LSW3); // give up Load 3
				SupplyState[2] = 0;
				// LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;
		case 10 ... 11: // 1.1A
			if (Battery_Capacity > 0)
			{ // 1A, now have 2.1A
				MainsReq((30 - (int)(sustain * 10)) * 0.5);
				DBattery = 1;
				CBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				PORTD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[1] = 1;
				SupplyState[2] = 1;
				// LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
			}
			else
			{ // 1.1A
				MainsReq((20 - (int)(sustain * 10)) * 0.5);
				PORTD |= _BV(LSW1) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[2] = 1;
				PORTD &= ~_BV(LSW2); // give up load 2
				SupplyState[1] = 0;
				// LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 0;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;
		case 2 ... 9: // 0.9A
			if (Battery_Capacity > 0)
			{ // 1A, now have 1.9A
				MainsReq((28 - (int)(sustain * 10)) * 0.5);
				DBattery = 1;
				CBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				PORTD |= _BV(LSW1) | _BV(LSW2);
				SupplyState[0] = 1;
				SupplyState[1] = 1;
				PORTD &= ~_BV(LSW3); // give up load 3
				SupplyState[2] = 0;
				// LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
			}
			else
			{ // 0.9A
				MainsReq((20 - (int)(sustain * 10)) * 0.5);
				PORTD |= _BV(LSW1) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[2] = 1;
				PORTD &= ~_BV(LSW2); // give up load 2
				SupplyState[1] = 0;
				// LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 0;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;

		case 1: // 0.1A
			if (Battery_Capacity > 0)
			{ // 1.1A
				DBattery = 1;
				CBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				MainsReq(4.5); // 0.9A, now have 2A
				PORTD |= _BV(LSW1) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[2] = 1;
				PORTD &= ~_BV(LSW2); // give up load 2
				SupplyState[1] = 0;
				// LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 0;
			}
			else
			{				   // 0.1A
				MainsReq(9.5); // 1.9A, now have 2A
				CBattery = 0;
				DBattery = 0;
				PORTD |= _BV(LSW1) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[2] = 1;
				PORTD &= ~_BV(LSW2); // give up load 2
				SupplyState[1] = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
				// LoadSw1 = 1; LoadSw2 = 0;LoadSw3 = 1;
			}
			break;
		default: // sustainable energy too low
			if (Battery_Capacity > 0)
			{ // 1A
				DBattery = 1;
				CBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				MainsReq(5.0); // 1A, now have 2A
				PORTD |= _BV(LSW1) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[2] = 1;
				PORTD &= ~_BV(LSW2); // give up load 2
				SupplyState[1] = 0;
				// LoadSw1 = 1; LoadSw2 = 0;LoadSw3 = 1;
			}
			else
			{
				MainsReq(10.0); // 1A, now have 2A
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
				PORTD |= _BV(LSW1) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[2] = 1;
				PORTD &= ~_BV(LSW2); // give up load 2
				SupplyState[1] = 0;
				// LoadSw1 = 1; LoadSw2 = 0;LoadSw3 = 1;
			}
			break;
		}
		break;
	case 6:					 // 110, demand = 3.2A
		PORTD &= ~_BV(LSW3); // load 3 not calling
		SupplyState[2] = 0;
		// LoadSw3 = 0;
		switch ((int)(sustain * 10))
		{
		case 37 ... 40: // 3.7A to 3.9A
			MainsReq((42 - (int)(sustain * 10)) * 0.5);
			PORTD |= _BV(LSW1) | _BV(LSW2);
			SupplyState[0] = 1;
			SupplyState[1] = 1;
			PORTD |= _BV(CB);
			PORTD &= ~_BV(DB);
			CBattery = 1;
			DBattery = 0;
			// LoadSw1 = 1; LoadSw2 = 1;
			break;

		case 32 ... 36:	   // 3.2A
			MainsReq(0.0); // 0A, now have 3.2
			PORTD |= _BV(LSW1) | _BV(LSW2);
			SupplyState[0] = 1;
			SupplyState[1] = 1;
			// LoadSw1 = 1; LoadSw2 = 1;
			PORTD &= ~_BV(CB);
			PORTD &= ~_BV(DB);
			CBattery = 0;
			DBattery = 0;
			break;
		case 23 ... 31: // 2.3A to 3.1A
			MainsReq((32 - (int)(sustain * 10)) * 0.5);
			PORTD |= _BV(LSW1) | _BV(LSW2);
			SupplyState[0] = 1;
			SupplyState[1] = 1;
			// LoadSw1 = 1; LoadSw2 = 1;
			CBattery = 0;
			DBattery = 0;
			PORTD &= ~_BV(CB);
			PORTD &= ~_BV(DB);
			break;

		case 12 ... 22: // 2.2A
			if (Battery_Capacity > 0)
			{ // 1A, now have 3.2A
				MainsReq((22 - (int)(sustain * 10)) * 0.5);
				DBattery = 1;
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				PORTD |= _BV(LSW1) | _BV(LSW2);
				SupplyState[0] = 1;
				SupplyState[1] = 1;
				// LoadSw1 = 1; LoadSw2 = 1;
				CBattery = 0;
			}
			else
			{
				MainsReq((32 - (int)(sustain * 10)) * 0.5);
				PORTD |= _BV(LSW1) | _BV(LSW2);
				SupplyState[0] = 1;
				SupplyState[1] = 1;
				// LoadSw1 = 1; LoadSw2 = 1;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;
		case 2 ... 11: // 1.1A
			if (Battery_Capacity > 0)
			{ // 1A, now have 2.3A
				MainsReq((22 - (int)(sustain * 10)) * 0.5);
				DBattery = 1;
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				PORTD |= _BV(LSW1) | _BV(LSW2);
				SupplyState[0] = 1;
				SupplyState[1] = 1;
				// LoadSw1 = 1; LoadSw2 = 1;
				CBattery = 0;
			}
			else
			{ // 1.1A
				MainsReq((12 - (int)(sustain * 10)) * 0.5);
				PORTD |= _BV(LSW1);
				SupplyState[0] = 1;
				PORTD &= ~_BV(LSW2); // give up load 2, not enough current
				SupplyState[1] = 0;
				// LoadSw1 = 1; LoadSw2 = 1;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;
		case 1: // 0.1A
			if (Battery_Capacity > 0)
			{ // 1A, now have 1.1A
				DBattery = 1;
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				MainsReq(0.5); // 0.1A, now have 1.2A
				PORTD |= _BV(LSW1);
				SupplyState[0] = 1;
				PORTD &= ~_BV(LSW2);
				SupplyState[1] = 0;
				// LoadSw1 = 1; LoadSw2 = 1;
				CBattery = 0;
			}
			else
			{				   // 0.1A
				MainsReq(5.5); // 1.1A, now have 1.2A
				PORTD |= _BV(LSW1);
				SupplyState[0] = 1;
				PORTD &= ~_BV(LSW2); // give up load 2
				SupplyState[1] = 0;
				// LoadSw1 = 1; LoadSw2 = 0;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;

		default: // sustainable energy too low
			if (Battery_Capacity > 0)
			{ // 1A
				DBattery = 1;
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				MainsReq(1.0); // 0.2A, now have 1.2A
				PORTD |= _BV(LSW1);
				SupplyState[0] = 1;
				PORTD &= ~_BV(LSW2); // give up load 2
				SupplyState[1] = 0;
				// LoadSw1 = 1; LoadSw2 = 0;
				CBattery = 0;
			}
			else
			{
				MainsReq(6.0); // 1.2A
				PORTD |= _BV(LSW1);
				SupplyState[0] = 1;
				PORTD &= ~_BV(LSW2); // give up load 2
				SupplyState[1] = 0;
				// LoadSw1 = 1; LoadSw2 = 0;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;
		}
		break;
	case 5:					 // 101, demand = 2A
		PORTD &= ~_BV(LSW2); // load 2 not calling
		SupplyState[1] = 0;
		// LoadSw2 = 0;
		switch ((int)(sustain * 10))
		{
		case 30 ... 100:   // 3A
			MainsReq(0.0); // 0A, now have 3A
			PORTD |= _BV(LSW1) | _BV(LSW3);
			SupplyState[0] = 1;
			SupplyState[2] = 1;
			// LoadSw1 = 1; LoadSw3 = 1;
			CBattery = 1;
			DBattery = 0;
			PORTD |= _BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 25 ... 29: // 2A to 2.9A
			MainsReq((30 - (int)(sustain * 10)) * 0.5);
			CBattery = 1;
			PORTD |= _BV(LSW1) | _BV(LSW3);
			SupplyState[0] = 1;
			SupplyState[2] = 1;
			// LoadSw1 = 1; LoadSw3 = 1;
			DBattery = 0;
			PORTD |= _BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 20 ... 24:
			MainsReq(0.0); // 0A, now have 2A to 2.4A which is enough
			PORTD |= _BV(LSW1) | _BV(LSW3);
			SupplyState[0] = 1;
			SupplyState[2] = 1;
			// LoadSw1 = 1; LoadSw3 = 1;
			CBattery = 0;
			DBattery = 0;
			PORTD &= ~_BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 11 ... 19: // 1.9A
			MainsReq((20 - (int)(sustain * 10)) * 0.5);
			PORTD |= _BV(LSW1) | _BV(LSW3);
			SupplyState[0] = 1;
			SupplyState[2] = 1;
			// LoadSw1 = 1; LoadSw3 = 1;
			CBattery = 0;
			DBattery = 0;
			PORTD &= ~_BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 1 ... 10: // 1A
			if (Battery_Capacity > 0)
			{ // 1A, now have 2A
				MainsReq((10 - (int)(sustain * 10)) * 0.5);
				DBattery = 1;
				CBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				PORTD |= _BV(LSW1) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[2] = 1;
				// LoadSw1 = 1; LoadSw3 = 1;
			}
			else
			{
				MainsReq((20 - (int)(sustain * 10)) * 0.5);
				PORTD |= _BV(LSW1) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[2] = 1;
				// LoadSw1 = 1; LoadSw3 = 1;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;

		default: // sustainable energy too low
			if (Battery_Capacity > 0)
			{ // 1A
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				MainsReq(5.0); // 1A, now have 2A
				PORTD |= _BV(LSW1) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[2] = 1;
				// LoadSw1 = 1; LoadSw3 = 1;
				CBattery = 0;
				DBattery = 1;
			}
			else
			{
				MainsReq(10.0); // 2A
				PORTD |= _BV(LSW1) | _BV(LSW3);
				SupplyState[0] = 1;
				SupplyState[2] = 1;
				// LoadSw1 = 1; LoadSw3 = 1;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;
		}
		break;
	case 4: // 100, demand = 1.2A
		PORTD &= ~_BV(LSW2);
		PORTD &= ~_BV(LSW3); // load 2, load 3 Not calling //LoadSw2 = 0; //LoadSw3 = 0;
		SupplyState[1] = 0;
		SupplyState[2] = 0;
		switch ((int)(sustain * 10))
		{
		case 22 ... 100:
			MainsReq(0.0); // 0A, now have 2.2A
			PORTD |= _BV(LSW1);
			SupplyState[0] = 1;
			// LoadSw1 = 1;
			CBattery = 1;
			DBattery = 0;
			PORTD |= _BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 17 ... 21:
			MainsReq((22 - (int)(sustain * 10)) * 0.5);
			PORTD |= _BV(LSW1);
			SupplyState[0] = 1;
			// LoadSw1 = 1;
			PORTD |= _BV(CB);
			PORTD &= ~_BV(DB);
			CBattery = 1;
			DBattery = 0;
			break;
		case 12 ... 16:
			MainsReq(0.0); // 0A, now have 1.2A to 1.6A
			PORTD |= _BV(LSW1);
			SupplyState[0] = 1;
			// LoadSw1 = 1;
			CBattery = 0;
			DBattery = 0;
			PORTD &= ~_BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 3 ... 11:
			MainsReq((12 - (int)(sustain * 10)) * 0.5);
			PORTD |= _BV(LSW1);
			SupplyState[0] = 1;
			// LoadSw1 = 1;
			CBattery = 0;
			DBattery = 0;
			PORTD &= ~_BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 1 ... 2: // 0.2A
			if (Battery_Capacity > 0)
			{ // 1A
				MainsReq((2 - (int)(sustain * 10)) * 0.5);
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				PORTD |= _BV(LSW1);
				SupplyState[0] = 1;
				// LoadSw1 = 1;
				CBattery = 0;
				DBattery = 1;
			}
			else
			{ // 0.2A
				MainsReq((12 - (int)(sustain * 10)) * 0.5);
				PORTD |= _BV(LSW1);
				SupplyState[0] = 1;
				// LoadSw1 = 1;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;
		default: // sustainable energy too low
			if (Battery_Capacity > 0)
			{ // 1A
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				MainsReq(1.0); // 0.2A, now have 1.2A
				PORTD |= _BV(LSW1);
				SupplyState[0] = 1;
				// LoadSw1 = 1;
				CBattery = 0;
				DBattery = 1;
			}
			else
			{
				MainsReq(6.0); // 1.2A
				PORTD |= _BV(LSW1);
				SupplyState[0] = 1;
				// LoadSw1 = 1;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
				CBattery = 0;
				DBattery = 0;
			}
			break;
		}
		break;
	case 3:					 // 011,demand = 2.8
		PORTD &= ~_BV(LSW1); // load 1 not calling
		SupplyState[0] = 0;
		// LoadSw1 = 0;
		switch ((int)(sustain * 10))
		{
		case 38 ... 100:
			MainsReq(0.0);
			PORTD |= _BV(LSW2) | _BV(LSW3);
			SupplyState[1] = 1;
			SupplyState[2] = 1;
			// LoadSw2 = 1; LoadSw3 = 1;
			CBattery = 1;
			DBattery = 0;
			PORTD |= _BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 33 ... 37: // can draw main to charge battery
			MainsReq((38 - (int)(sustain * 10)) * 0.5);
			PORTD |= _BV(LSW2) | _BV(LSW3);
			SupplyState[1] = 1;
			SupplyState[2] = 1;
			// LoadSw2 = 1; LoadSw3 = 1;
			CBattery = 1;
			DBattery = 0;
			PORTD |= _BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 28 ... 32:
			MainsReq(0.0); // 0A, now have 2.8A
			PORTD |= _BV(LSW2) | _BV(LSW3);
			SupplyState[1] = 1;
			SupplyState[2] = 1;
			// LoadSw2 = 1; LoadSw3 = 1;
			CBattery = 0;
			DBattery = 0;
			PORTD &= ~_BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 19 ... 27: // 1.9A to 2.7A
			MainsReq((28 - (int)(sustain * 10)) * 0.5);
			PORTD |= _BV(LSW2) | _BV(LSW3);
			SupplyState[1] = 1;
			SupplyState[2] = 1;
			// LoadSw2 = 1; LoadSw3 = 1;
			CBattery = 0;
			DBattery = 0;
			PORTD &= ~_BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 8 ... 18: // 0.8A to 1.8A
			if (Battery_Capacity > 0)
			{ // 1A, now have 2.8A
				MainsReq((18 - (int)(sustain * 10)) * 0.5);
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				PORTD |= _BV(LSW2) | _BV(LSW3);
				SupplyState[1] = 1;
				SupplyState[2] = 1;
				// LoadSw2 = 1; LoadSw3 = 1;
				CBattery = 0;
				DBattery = 1;
			}
			else
			{
				MainsReq((28 - (int)(sustain * 10)) * 0.5);
				PORTD |= _BV(LSW2) | _BV(LSW3);
				SupplyState[1] = 1;
				SupplyState[2] = 1;
				// LoadSw2 = 1; LoadSw3 = 1;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;
		case 1 ... 7: // 0.7A
			if (Battery_Capacity > 0)
			{ // 1A, now have 1.7A
				MainsReq((18 - (int)(sustain * 10)) * 0.5);
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				PORTD |= _BV(LSW2) | _BV(LSW3);
				SupplyState[1] = 1;
				SupplyState[2] = 1;
				// LoadSw2 = 1; LoadSw3 = 1;
				CBattery = 0;
				DBattery = 1;
			}
			else
			{ // 0.7A
				MainsReq((20 - (int)(sustain * 10)) * 0.5);
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
				PORTD |= _BV(LSW2);
				SupplyState[1] = 1;
				PORTD &= ~_BV(LSW3); // give up load 3
				SupplyState[2] = 0;
				// LoadSw2 = 1; LoadSw3 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			break;
		default: // sustainable energy too low
			if (Battery_Capacity > 0)
			{ // 1A
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				MainsReq(9.0); // 1.8A, now have 2.8A
				PORTD |= _BV(LSW2) | _BV(LSW3);
				SupplyState[1] = 1;
				SupplyState[2] = 1;
				// LoadSw2 = 1; LoadSw3 = 1;
				CBattery = 0;
				DBattery = 1;
			}
			else
			{
				MainsReq(10.0); // 2A
				PORTD |= _BV(LSW2);
				SupplyState[1] = 1;
				// LoadSw2 = 1;
				PORTD &= ~_BV(LSW3); // give up load 3
				SupplyState[2] = 0;
				// LoadSw3 = 0;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;
		}
		break;
	case 2: // 010, demand = 2A
		PORTD &= ~_BV(LSW1);
		PORTD &= ~_BV(LSW3); // load 1 and load 3 not calling
		SupplyState[0] = 0;
		SupplyState[2] = 0;
		// LoadSw1 = 1; LoadSw3 = 1;
		switch ((int)(sustain * 10))
		{
		case 30 ... 100:
			PORTD |= _BV(CB);
			PORTD &= ~_BV(DB);
			MainsReq(0.0); // 0A, now have 3A
			PORTD |= _BV(LSW2);
			SupplyState[1] = 1;
			// LoadSw2 = 1;
			CBattery = 1;
			DBattery = 0;
			break;
		case 25 ... 29: // 2.9A
			MainsReq((30 - (int)(sustain * 10)) * 0.5);
			CBattery = 1;
			PORTD |= _BV(LSW2);
			SupplyState[1] = 1;
			// LoadSw2 = 1;
			DBattery = 0;
			PORTD |= _BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 20 ... 24:	   // 2A to 2.4A
			MainsReq(0.0); // 0A, now have 2A
			PORTD |= _BV(LSW2);
			SupplyState[1] = 1;
			// LoadSw2 = 1;
			CBattery = 0;
			DBattery = 0;
			PORTD &= ~_BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 11 ... 19: // 1.1A to 1.9A
			MainsReq((20 - (int)(sustain * 10)) * 0.5);
			PORTD |= _BV(LSW2);
			SupplyState[1] = 1;
			// LoadSw2 = 1;
			CBattery = 0;
			DBattery = 0;
			PORTD &= ~_BV(CB);
			PORTD &= ~_BV(DB);
			break;
		case 1 ... 10: // 1A
			if (Battery_Capacity > 0)
			{ // 1A, now have 2A
				MainsReq((10 - (int)(sustain * 10)) * 0.5);
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				CBattery = 0;
				DBattery = 1;
				PORTD |= _BV(LSW2);
				SupplyState[1] = 1;
				// LoadSw2 = 1;
			}
			else
			{
				MainsReq((20 - (int)(sustain * 10)) * 0.5);
				PORTD |= _BV(LSW2);
				SupplyState[1] = 1;
				// LoadSw2 = 1;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;
		default: // sustainable energy too low
			if (Battery_Capacity > 0)
			{ // 1A
				PORTD &= ~_BV(CB);
				PORTD |= _BV(DB);
				MainsReq(5.0); // now have 2A
				PORTD |= _BV(LSW2);
				SupplyState[1] = 1;
				// LoadSw2 = 1;
				CBattery = 0;
				DBattery = 1;
			}
			else
			{
				MainsReq(10.0); // 2A
				PORTD |= _BV(LSW2);
				SupplyState[1] = 1;
				// LoadSw2 = 1;
				CBattery = 0;
				DBattery = 0;
				PORTD &= ~_BV(CB);
				PORTD &= ~_BV(DB);
			}
			break;
		}
		break;
	case 1: // Load 3 calliing, 001
		PORTD &= ~_BV(LSW1);
		PORTD &= ~_BV(LSW2); // load 1, load 2 Not calling //LoadSw1 = 0; //LoadSw2 = 0;
		SupplyState[0] = 0;
		SupplyState[1] = 0;
		switch ((int)(sustain * 10))
		{
		case 18 ... 100:
			MainsReq(0.0);
			PORTD |= _BV(CB);
			PORTD &= ~_BV(DB);
			PORTD |= _BV(LSW3);
			SupplyState[2] = 1;
			// LoadSw3 = 1;
			CBattery = 1;
			DBattery = 0;
			break;
		case 13 ... 17:
			MainsReq((18 - (int)(sustain * 10)) * 0.5);
			PORTD |= _BV(CB);
			PORTD &= ~_BV(DB);
			CBattery = 1;
			PORTD |= _BV(LSW3);
			SupplyState[2] = 1;
			// LoadSw3 = 1;
			DBattery = 0;
			break;
		case 8 ... 12:
			MainsReq(0.0); // 0A, now have 0.8A to 1.2A
			PORTD |= _BV(LSW3);
			SupplyState[2] = 1;
			// LoadSw3 = 1;
			PORTD &= ~_BV(CB);
			PORTD &= ~_BV(DB);
			CBattery = 1;
			DBattery = 0;
			break;
		case 1 ... 7: // 0.1A to 0.7A
			MainsReq((8 - (int)(sustain * 10)) * 0.5);
			PORTD |= _BV(LSW3);
			SupplyState[2] = 1;
			// LoadSw3 = 1;
			CBattery = 0;
			DBattery = 0;
			PORTD &= ~_BV(CB);
			PORTD &= ~_BV(DB);
			break;
		default:		   // sustainable energy too low
			MainsReq(4.0); // 0.8
			PORTD |= _BV(LSW3);
			SupplyState[2] = 1;
			// LoadSw3 = 1;
			CBattery = 0;
			DBattery = 0;
			PORTD &= ~_BV(CB);
			PORTD &= ~_BV(DB);
			break;
		}
		break;
	case 0: // no demand, 000
		PORTD &= ~_BV(LSW1);
		PORTD &= ~_BV(LSW2);
		PORTD &= ~_BV(LSW3); // load 1,load 2 and load 3 not calling
		SupplyState[0] = 0;
		SupplyState[1] = 0;
		SupplyState[2] = 0;
		// LoadSw1 = 0;LoadSw2 = 0; LoadSw3 = 0;
		DBattery = 0; // no need discharging
		PORTD &= ~_BV(DB);
		switch ((int)(sustain * 10))
		{
		case 10 ... 100: // 1A and above
			MainsReq(0.0);
			CBattery = 1; // 1A
			PORTD |= _BV(CB);
			break;
		case 5 ... 9: // 0.9A
			MainsReq((10 - (int)(sustain * 10)) * 0.5);
			DBattery = 0;
			CBattery = 1; // 1A
			PORTD |= _BV(CB);
			break;

		default: // sustainable energy too low, not worth to charge battery
			// PORTD &= ~_BV(LSW1) | ~_BV(LSW2) | ~_BV(LSW3);//load 1, load 2 and load 3 Not calling
			// SupplyState[0] = 0; SupplyState[1] = 0; SupplyState[2] = 0;
			// LoadSw1 = 0;
			// LoadSw2 = 0;
			// LoadSw3 = 0;
			MainsReq(0.0); // 0A
			PORTD &= ~_BV(CB);
			CBattery = 0;
			break;
		}
		break;
	default:
		break;
	}
}

void in_adc(void)
{
	ADCSRA |= _BV(ADEN);							// Enable ADC
	ADCSRA |= _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // Set pre-scaler to 128 F_CPU = 93.75kHz
}
void mux(uint16_t a)
{
	ADMUX = a;
}
uint16_t read_adc(void)
{
	ADCSRA |= _BV(ADSC); // Start convertions
	while (ADCSRA & _BV(ADSC))
		;		// Wait for conversion to finish
	return ADC; // returning the 10bit result
}

int main(void)
{
	Start();
	// init_lcd();
	// set_orientation(0);
	// clear_screen();
	// rectangle background = {0,LCDWIDTH-1,0,LCDHEIGHT-1};
	// fill_rectangle(background,WHITE);

	digital_inputs();
	digital_outputs();
	pwm();
	sei(); // enable interrupt
	timer_interrupt();
	// double result;
	// init_debug_uart0();

	in_adc();
	uint16_t e = 0, f = 0, g = 0, h = 0;
	double j = 0.0, k = 0.0, l = 0.0, m = 0.0;
	for (;;)
	{
		loadcalls();

		mux(0); // pv
		e = read_adc();
		j = ((double)e * 3.3) / MAXADC;
		// printf("ADC value = %d , PV = %d \n", e, (int)j);

		mux(1); // wind
		f = read_adc();
		k = ((double)f * 3.3) / MAXADC;
		// printf("ADC value = %d , Wind = %d \n", f, (int)k);

		mux(2); // bus current
		g = read_adc();
		l = ((double)g * 7.07106781187) / MAXADC; // 10/sqrt(2)
		// printf("ADC value = %d , BusCurrent = %d \n", g, (int)l);

		mux(3); // bus voltage
		h = read_adc();
		m = ((double)h * 2.85) / MAXADC;
		// printf("ADC value = %d , BusVoltage = %d \n", h, (int)m);

		sustain = j + k;
		system();

		power = power + ((m * l) / 1000);
		Sources[2] = j;
		Sources[1] = k;
		Sources[0] = main_current;
		BasicInfo[2] = l;
		BasicInfo[1] = m;
		BasicInfo[0] = power;
		CallState[0] = LoadCall1;
		CallState[1] = LoadCall2;
		CallState[2] = LoadCall3;
		Load[0] = (LoadCall1) ? 1.20 : 0.00;
		Load[1] = (LoadCall2) ? 2.00 : 0.00;
		Load[2] = (LoadCall3) ? 0.80 : 0.00;

		BatteryState = CBattery * 1 + DBattery * -1;
		battery_percent = (Battery_Capacity / 10);
		Update(battery_percent, BasicInfo, BatteryState, Sources, Load, CallState, SupplyState);

		// int loop = 0;
		// char tempch[20];
		// char type[4] = {"pv","wind","bc","bv"};
		// double value[4] = {j,k,l,m};
		// for (loop=0;loop <4; loop++) {
		//	sprintf(tempch,"%s: %f",type[loop],value[loop]);
		//	cprint(tempch,10,10+20*loop);
		// }
		_delay_ms(1);
	}

	/*
	while (1) {
		adc_pv();
		adc_wind();
		adc_bc();
		adc_bv();
		count_sustain();
		//loadcalls();
		mainsystem();
		bat_count();
		count_power();
	}
	*/
	return 0;
}