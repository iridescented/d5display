#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdlib.h>

//define input located in Port A
//ADC inputs
#define PV PA0
#define WIND PA1
#define BUS_CURRENT PA2
#define BUS_VOLT PA3

//Digital Inputs
#define LOAD1 PA4
#define LOAD2 PA5
#define LOAD3 PA6

//define output located in Port D
#define MAINS PD7 //PWM
//Digital Outputs
#define LSW1 PD6
#define LSW2 PD5
#define LSW3 PD4
#define DB PD3
#define CB PD2

#define CLKFREQ 12000000 //12MHz
#define MAXADC 1023
#define MAXBATTERY 10 //10ah battery

//Display data
float count_sustain (void);
float count_power(void);


void pwm(void);
void timer_interrupt(void);

//input
uint16_t adc_wind(void);
uint16_t adc_pv(void);
uint16_t adc_bc(void);
uint16_t adc_bv(void);
void loadcalls(void);
void digital_inputs(void);
void bat_count(void);
//outputs
void digital_outputs(void);
void MainsReq(uint16_t a);
uint16_t Battery_Capacity;
void Battery(uint16_t batt);
//global variables
uint16_t LoadCall1;
uint16_t LoadCall2;
uint16_t LoadCall3;
uint16_t DBattery;
uint16_t CBattery;
float sustain, main_current;

volatile uint16_t count = 0x00;
volatile uint16_t count1 = 0x00;

float count_sustain(void) {//count the sustainable energy
	float a, b;
	a = (adc_pv()/ MAXADC)*5.0; //varying from 0 to 5V
	b = (adc_wind()/ MAXADC)*5.0;//varying from 0 to 5V
	sustain = a + b;
	return sustain;
}

float count_power(void) {
	float a, b;
	int temp1, temp2;
	//temp1 = adc_bc();
	temp1 = main_current;//varying from -4 to 4V
	temp2 = adc_bv();//varying from -10 to 10V
	if (temp1 > 512) {
		a = (adc_bc() / MAXADC) * 4;
	}
	else {
		a = (adc_bc() / MAXADC) * -4;
	}
	if (temp2 > 512) {
		b = (adc_bv() / MAXADC) * 10;
	}
	else {
		b = (adc_bv() / MAXADC) * -10;
	}

	return a * b;
}
uint16_t adc_pv(void) {
	uint16_t pv;
	DDRA |= 0x00; //Set bit 0 of Port A as inputs
	PORTA |= _BV(PV);
	ADCSRA |= _BV(ADEN); //Enable ADC
	ADCSRA |= _BV(ADPS1) | _BV(ADPS2);  //Set pre-scaler to 64 F_CPU = 187.5kHz
	ADCSRA |= _BV(ADSC); //Start convertions
	while (!(ADCSRA & (1 << ADIF))) {
		pv = ADC;
	}// Wait for conversion to finish
	ADMUX = 0; //select the appropriate channel
	ADCSRA |= _BV(ADIF); //Start convertions
	return pv;                // returning the 10bit result
}
uint16_t adc_wind(void) {
	uint16_t wind;
	DDRA |= 0x00; //Set bit 1 of Port A as inputs
	PORTA |= _BV(WIND);
	ADCSRA |= _BV(ADEN); //Enable ADC
	ADCSRA |= _BV(ADPS1) | _BV(ADPS2);  //Set pre-scaler to 64 F_CPU = 187.5kHz
	ADCSRA |= _BV(ADSC); //Start convertions
	while (!(ADCSRA & (1 << ADIF))) {
		wind = ADC;
	}// Wait for conversion to finish
	ADMUX = 1; //select the appropriate channel
	ADCSRA |= _BV(ADIF);// Clear ADIF,ADIF is cleared by writing a 1 to ADSCRA
	return wind;

}



uint16_t adc_bc(void) {
	uint16_t bc;
	DDRA |= 0x00; //Set bit 2 of Port A as inputs
	PORTA |= _BV(BUS_CURRENT);
	ADCSRA |= _BV(ADEN); //Enable ADC
	ADCSRA |= _BV(ADPS1) | _BV(ADPS2);  //Set pre-scaler to 64 F_CPU = 187.5kHz
	ADCSRA |= _BV(ADSC); //Start convertions
	while (!(ADCSRA & (1 << ADIF))) {
		bc = ADC;
	}// Wait for conversion to finish
	ADMUX = 2; //select the appropriate channel
	ADCSRA |= _BV(ADIF); //Start convertions
	return bc;                // returning the 10bit result
}

uint16_t adc_bv(void) {
	uint16_t bv;
	DDRA |= 0x00; //Set bit 2 of Port A as inputs
	PORTA |= _BV(BUS_VOLT);
	ADCSRA |= _BV(ADEN); //Enable ADC
	ADCSRA |= _BV(ADPS1) | _BV(ADPS2);  //Set pre-scaler to 64 F_CPU = 187.5kHz
	ADCSRA |= _BV(ADSC); //Start convertions
	while (!(ADCSRA & (1 << ADIF))) {
		bv = ADC;
	}// Wait for conversion to finish
	ADMUX = 3; //select the appropriate channel
	ADCSRA |= _BV(ADIF); //Start convertions
	return bv;                // returning the 10bit result
}


void digital_inputs(void) {
	DDRA &= ~(_BV(LOAD1)) | ~(_BV(LOAD2)) | ~(_BV(LOAD3)); //Set bit 4 to 6 of Port A as inputs
}

void digital_outputs(void) {
	DDRD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3) | _BV(DB) | _BV(CB); //Set bit 2 to 6 of Port D as outputs
}

void loadcalls(void) 
{
	LoadCall1 = PINA & LOAD1 ? 1 : 0;
	LoadCall2 = PINA & LOAD2 ? 1 : 0;
	LoadCall3 = PINA & LOAD3 ? 1 : 0;
}


ISR(TIMER1_COMPA_vect) {
	if (CBattery == 1) {
		count++;
	}
	else if (DBattery == 1) {
		count1++;
	}
	else {
		count = count;
		count1 = count1;
	}
}
void pwm(void) {
	DDRD |= _BV(MAINS);
	TCCR2A |= _BV(WGM20);//Mode 1, PWM, Phase Correct, 10-bit
	TCCR2A |= _BV(COM2A1);
	TCCR2B |= _BV(CS22) | _BV(CS20);//F_CPU = 12MHz and pre-scaler = 1024, F_CLK = 11.71kHz
}
void timer_interrupt(void) {
	TCCR1A |= _BV(COM0A1) | _BV(COM0A0);//disconnecting the pins
	TCCR1B |= _BV(CS12) | _BV(CS10);//F_CPU = 12MHz and pre-scaler = 1024, F_CLK = 11.71kHz
	OCR1A |= 5858;
}
void MainsReq(uint16_t a) {
	OCR2A = a * 25;
	main_current = a * 0.2;
}

void bat_count(void) {
	if (count == 60) {
		Battery_Capacity++;
		count = 0;
	}
	else if (count1 == 60) {
		Battery_Capacity--;
		count1 = 0;
	}
}
int main(void)
{
	digital_inputs();
	digital_outputs();
	pwm();
	sei();//enable interrupt
	timer_interrupt();
	//double result;
	while (1) {
		count_sustain();
		loadcalls();
		if (LoadCall1 && LoadCall2 && LoadCall3) { //111, demand = 4A
			if (sustain == 4) {
				MainsReq(5);//1A, now have 5A
				PORTD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3);
				//LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 3) {//have 3A
				if (Battery_Capacity > 0) {//1A, now have 4A
					DBattery = 1;
					CBattery = 0;
					MainsReq(0);//0A, now have 4A
					PORTD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3);
					//LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
					
				}
				else {//3A
					MainsReq(10); //5A
					PORTD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3);
					//LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
					CBattery = 1;
					DBattery = 0;
				}
			}
			else if (sustain >= 2.2) { //have 2.2A
				if (Battery_Capacity > 0) {//1A, now have 3.2A
					DBattery = 1;
					CBattery = 0;
					MainsReq(4); //0.8A, now have 4A
					PORTD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3);
					//LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
				}
				else {//2.2A
					MainsReq(9);//1.8A, now have 4A
					PORTD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3);
					//LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
					CBattery = 0;
					DBattery = 0;
				}

			}
			else if (sustain >= 1.2) {//1.2A
				if (Battery_Capacity > 0) {//2.2A
					DBattery = 1;
					CBattery = 0;
					MainsReq(9); //1.8A, now have 4A
					PORTD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3);
					//LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
				}
				else {
					MainsReq(10);//2A, now have 3.2A
					PORTD |= _BV(LSW1) | _BV(LSW2);
					PORTD &= ~_BV(LSW3);//give up load 3
					//LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 0;
					CBattery = 0;
					DBattery = 0;
				}

			}
			else if (sustain >= 0.2) {//0.2A
				if (Battery_Capacity > 0) {//1.2A
					DBattery = 1;
					CBattery = 0;
					MainsReq(10); //2A, now have 3.2A
					PORTD |= _BV(LSW1) | _BV(LSW2);
					PORTD &= ~_BV(LSW3);//give up load 3
					//LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 0;
				}
				else {
					MainsReq(10); //2A
					CBattery = 0;
					DBattery = 0;
					PORTD |= _BV(LSW1) | _BV(LSW3);
					PORTD &= ~_BV(LSW2);//give up load 2
					//LoadSw1 = 1; LoadSw2 = 0;LoadSw3 = 1;
				}
			}
			else {//sustainable energy too low
				if (Battery_Capacity > 0) {//1A
					DBattery = 1;
					CBattery = 0;
					MainsReq(5); //1A, now have 2A
					PORTD |= _BV(LSW1) | _BV(LSW3);
					PORTD &= ~_BV(LSW2);//give up load 2
					//LoadSw1 = 1; LoadSw2 = 0;LoadSw3 = 1;
				}
				else {
					MainsReq(10);//1A, now have 2A
					CBattery = 0;
					DBattery = 0;
					PORTD |= _BV(LSW1) | _BV(LSW3);
					PORTD &= ~_BV(LSW2);//give up load 2
					//LoadSw1 = 1; LoadSw2 = 0;LoadSw3 = 1;
				}

			}
		}


		else if (LoadCall1 && LoadCall2 && !LoadCall3) {//110, demand = 3.2A
			PORTD &= ~_BV(LSW3);//load 3 not calling
			//LoadSw3 = 0;
			if (sustain >= 4) {//4A
				MainsReq(1);//0.2A, now have 4.2
				PORTD |= _BV(LSW1) | _BV(LSW2);
				//LoadSw1 = 1; LoadSw2 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 3.8) {//3.8A
				MainsReq(2);//0.4A, now have 4.2
				PORTD |= _BV(LSW1) | _BV(LSW2);
				CBattery = 1;
				DBattery = 0;
				//LoadSw1 = 1; LoadSw2 = 1;
			}
			else if (sustain >= 3.2) {//3.2A
				MainsReq(0);//0A, now have 3.2
				PORTD |= _BV(LSW1) | _BV(LSW2);
				//LoadSw1 = 1; LoadSw2 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else if (sustain >= 2.2) {//2.2A
				if (Battery_Capacity > 0) {//1A, now have 3.2A
					DBattery = 1;
					MainsReq(0);//0A, now have 3.2
					PORTD |= _BV(LSW1) | _BV(LSW2);
					//LoadSw1 = 1; LoadSw2 = 1;
					CBattery = 0;
				}
				else {
					MainsReq(5);//1A, now have 3.2
					PORTD |= _BV(LSW1) | _BV(LSW2);
					//LoadSw1 = 1; LoadSw2 = 1;
					CBattery = 0;
					DBattery = 0;
				}
			}
			else if (sustain >= 1) {//1A
				if (Battery_Capacity > 0) {//1A
					DBattery = 1;
					CBattery = 0;
					MainsReq(6);//1.2A, now have 3.2A
					PORTD |= _BV(LSW1) | _BV(LSW2);
					//LoadSw1 = 1; LoadSw2 = 1;
				}
				else {
					MainsReq(1);//0.2A, now have 1.2A
					PORTD |= _BV(LSW1);
					PORTD &= ~_BV(LSW2);//give up load 2
					//LoadSw1 = 1; LoadSw2 = 0;
					CBattery = 0;
					DBattery = 0;
				}

			}

			else if (sustain >= 0.2) {//0.2A
				if (Battery_Capacity > 0) {//1A
					DBattery = 1;
					MainsReq(10);//2A, now have 3.2A
					PORTD |= _BV(LSW1) | _BV(LSW2);
					//LoadSw1 = 1; LoadSw2 = 1;
					CBattery = 0;
				}
				else {
					MainsReq(5);//1A, now have 1.2A
					PORTD |= _BV(LSW1);
					PORTD &= ~_BV(LSW2);//give up load 2
					//LoadSw1 = 1; LoadSw2 = 0;
					CBattery = 0;
					DBattery = 0;
				}
			}
			else {//sustainable energy too low
				if (Battery_Capacity > 0) {//1A
					DBattery = 1;
					MainsReq(1);//0.2A, now have 1.2A
					PORTD |= _BV(LSW1);
					PORTD &= ~_BV(LSW2);//give up load 2
					//LoadSw1 = 1; LoadSw2 = 0;
					CBattery = 0;
				}
				else {
					MainsReq(6);//1.2A
					PORTD |= _BV(LSW1);
					PORTD &= ~_BV(LSW2);//give up load 2
					//LoadSw1 = 1; LoadSw2 = 0;	
					CBattery = 0;
					DBattery = 0;
				}
			}

		}
		else if (LoadCall1 && !LoadCall2 && LoadCall3) { //101, demand = 2A
			PORTD &= ~_BV(LSW2);//load 2 not calling

			//LoadSw2 = 0;
			if (sustain >= 3) {//3A
				MainsReq(0);//0A, now have 3A
				PORTD |= _BV(LSW1) | _BV(LSW3);
				//LoadSw1 = 1; LoadSw3 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 2.8) {//2.8A
				MainsReq(1);//0.2A, now have 3A
				PORTD |= _BV(LSW1) | _BV(LSW3);
				//LoadSw1 = 1; LoadSw3 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 2.6) {//2.6A
				MainsReq(6);//0.4A, now have 3A
				PORTD |= _BV(LSW1) | _BV(LSW3);
				//LoadSw1 = 1; LoadSw3 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 2) {//2A
				MainsReq(0);//0A, now have 2A
				PORTD |= _BV(LSW1) | _BV(LSW3);
				//LoadSw1 = 1; LoadSw3 = 1;
				CBattery = 0;
				DBattery = 0;
			}

			else if (sustain >= 1) {//1A
				if (Battery_Capacity > 0) {//1A, now have 2A
					MainsReq(0);//0A, now have 2A
					DBattery = 1;
					CBattery = 0;
					PORTD |= _BV(LSW1) | _BV(LSW3);
					//LoadSw1 = 1; LoadSw3 = 1;
				}
				else {
					MainsReq(5);//1A, now have 2A
					PORTD |= _BV(LSW1) | _BV(LSW3);
					//LoadSw1 = 1; LoadSw3 = 1;
					CBattery = 0;
					DBattery = 0;
				}
			}
			else if (sustain >= 0.2) {//0.2A
				if (Battery_Capacity > 0) {//1A
					MainsReq(4);//0.8A, now have 2A
					PORTD |= _BV(LSW1) | _BV(LSW3);
					//LoadSw1 = 1; LoadSw3 = 1;
					CBattery = 0;
					DBattery = 1;
				}
				else {
					MainsReq(9);//1.8A, now have 2A
					PORTD |= _BV(LSW1) | _BV(LSW3);
					//LoadSw1 = 1; LoadSw3 = 1;
					CBattery = 0;
					DBattery = 0;
				}
			}
			else { //sustainable energy too low
				if (Battery_Capacity > 0) {//1A
					MainsReq(5);//1A, now have 2A
					PORTD |= _BV(LSW1) | _BV(LSW3);
					//LoadSw1 = 1; LoadSw3 = 1;
					CBattery = 0;
					DBattery = 1;
				}
				else {
					MainsReq(10);//2A
					PORTD |= _BV(LSW1) | _BV(LSW3);
					//LoadSw1 = 1; LoadSw3 = 1;
					CBattery = 0;
					DBattery = 0;
				}
			}
		
		}

		else if (!LoadCall1 && LoadCall2 && !LoadCall3) {//010, demand = 2A
		PORTD &= ~_BV(LSW1) | ~_BV(LSW3);//load 1 and load 3 not calling
		//LoadSw1 = 1; LoadSw3 = 1;
			if (sustain >= 3) {
				MainsReq(0);//0A, now have 3A
				PORTD |= _BV(LSW2);
				//LoadSw2 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 2.8) {// 2.8A
				MainsReq(1);//0.2A, now have 3A
				PORTD |= _BV(LSW2);
				//LoadSw2 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 2.6) {// 2.6A
				MainsReq(2);//0.4A, now have 3A
				PORTD |= _BV(LSW2);
				//LoadSw2 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 2) {// 2A
				MainsReq(0);//0A, now have 2A
				PORTD |= _BV(LSW2);
				//LoadSw2 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else if (sustain >= 1.8) {// 1.8A
				MainsReq(1);//0.2A, now have 2A
				PORTD |= _BV(LSW2);
				//LoadSw2 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else if (sustain >= 1.6) {// 1.6A
				MainsReq(2);//0.4A, now have 2A
				PORTD |= _BV(LSW2);
				//LoadSw2 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else if (sustain >= 1.4) {// 1.4A
				MainsReq(3);//0.6A, now have 2A
				PORTD |= _BV(LSW2);
				//LoadSw2 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else if (sustain >= 1.2) {// 1.2A
				MainsReq(4);//0.8A, now have 2A
				PORTD |= _BV(LSW2);
				//LoadSw2 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else if (sustain >= 1) {// 1A
				if (Battery_Capacity > 0) {//1A, now have 2A
					MainsReq(0);//0A, now have 1A
					CBattery = 0;
					DBattery = 1;
					PORTD |= _BV(LSW2);
					//LoadSw2 = 1;
					
				}
				else {
					MainsReq(5);//1A, now have 2A
					PORTD |= _BV(LSW2);
					//LoadSw2 = 1;
					CBattery = 0;
					DBattery = 0;
				}
			}
			else {// sustainable energy too low
				if (Battery_Capacity > 0) {//1A
					MainsReq(5);//now have 2A
					PORTD |= _BV(LSW2);
					//LoadSw2 = 1;
					CBattery = 0;
					DBattery = 1;
				}

				else {
					MainsReq(10);//2A
					PORTD |= _BV(LSW2);
					//LoadSw2 = 1;
					CBattery = 0;
					DBattery = 0;
				}
			}
		}

		else if (!LoadCall1 && LoadCall2 && LoadCall3) { // 011,demand = 2.8
		PORTD &= ~_BV(LSW1);//load 1 not calling
			//LoadSw1 = 0;
			if (sustain >= 3.8) {
				PORTD |= _BV(LSW2) | _BV(LSW3);
				//LoadSw2 = 1; LoadSw3 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 2.8) {
				MainsReq(0);//0A, now have 2.8A
				PORTD |= _BV(LSW2) | _BV(LSW3);
				//LoadSw2 = 1; LoadSw3 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else if (sustain >= 1.8) {
				if (Battery_Capacity > 0) {//1A, now have 2.8A
					MainsReq(0);//0A, now have 2.8A
					PORTD |= _BV(LSW2) | _BV(LSW3);
					//LoadSw2 = 1; LoadSw3 = 1;
					CBattery = 0;
					DBattery = 1;
				}
				else {
					MainsReq(5);//1A, now have 2.8A
					PORTD |= _BV(LSW2) | _BV(LSW3);
					//LoadSw2 = 1; LoadSw3 = 1;
					CBattery = 0;
					DBattery = 0;
				}
			}
			else if (sustain >= 0.8) {
				if (Battery_Capacity > 0) {//1A
					MainsReq(5);//1A, now have 2.8A
					PORTD |= _BV(LSW2) | _BV(LSW3);
					//LoadSw2 = 1; LoadSw3 = 1;
					CBattery = 0;
					DBattery = 1;
				}
				else {
					MainsReq(10);//2A, now have 2.8A
					PORTD |= _BV(LSW2) | _BV(LSW3);
					//LoadSw2 = 1; LoadSw3 = 1;
					CBattery = 0;
					DBattery = 0;
				}

			}
			else if (sustain >= 0.2) {
				if (Battery_Capacity > 0) {//1A
					MainsReq(8);//1.6A, now have 2.8A
					PORTD |= _BV(LSW2) | _BV(LSW3);
					//LoadSw2 = 1; LoadSw3 = 1;
					CBattery = 0;
					DBattery = 1;
					
				}
				else {
					MainsReq(9);//1.8A, now have 2A
					PORTD |= _BV(LSW2);
					//LoadSw2 = 1;
					PORTD &= ~_BV(LSW3);//give up load 3
					//LoadSw3 = 0;
					CBattery = 0;
					DBattery = 0;
				}
			}
			else {//sustainable energy too low
				if (Battery_Capacity > 0) {//1A
					MainsReq(9);//1.8A, now have 2.7A
					PORTD |= _BV(LSW2) | _BV(LSW3);
					//LoadSw2 = 1; LoadSw3 = 1;
					CBattery = 0;
					DBattery = 1;
				}
				else {
					MainsReq(10);//2A
					PORTD |= _BV(LSW2);
					//LoadSw2 = 1;
					PORTD &= ~_BV(LSW3);//give up load 3
					//LoadSw3 = 0;
					CBattery = 0;
					DBattery = 0;
				}
			}
		}
		else if (LoadCall1 && !LoadCall2 && !LoadCall3) {//100, demand = 1.2A
		PORTD &= ~_BV(LSW2) | ~_BV(LSW3);//load 2, load 3 Not calling
										 //LoadSw2 = 0;
										 //LoadSw3 = 0;
			if (sustain >= 2.2) {
				MainsReq(0);//0A, now have 2.2A
				PORTD |= _BV(LSW1);
				//LoadSw1 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 1.2) {
				MainsReq(0);//0A, now have 1.2A
				PORTD |= _BV(LSW1);
				//LoadSw1 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else if (sustain >= 1) {
				MainsReq(1);//0.2A, now have 1.2A
				PORTD |= _BV(LSW1);
				//LoadSw1 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else if (sustain >= 0.8) {
				MainsReq(2);//0.4A, now have 1.2A
				PORTD |= _BV(LSW1);
				//LoadSw1 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else if (sustain >= 0.6) {
				MainsReq(3);//0.6A, now have 1.2A
				PORTD |= _BV(LSW1);
				//LoadSw1 = 1;
				CBattery = 0;
				CBattery = 0;
			}
			else if (sustain >= 0.4) {
				MainsReq(4);//0.8A, now have 1.2A
				PORTD |= _BV(LSW1);
				//LoadSw1 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else if (sustain >= 0.2) {
				MainsReq(5);//1A, now have 1.2A
				PORTD |= _BV(LSW1);
				//LoadSw1 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else {//sustainable energy too low
				if (Battery_Capacity > 0) {//1A
					MainsReq(1);//0.2A, now have 1.2A
					PORTD |= _BV(LSW1);
					//LoadSw1 = 1;
					CBattery = 0;
					DBattery = 1;
				}
				else {
					MainsReq(6);//1.2A
					PORTD |= _BV(LSW1);
					//LoadSw1 = 1;
					CBattery = 0;
					DBattery = 0;
				}
			}
		}
		else if (!LoadCall1 && !LoadCall2 && LoadCall3) { //001, demand = 0.8
		PORTD &= ~_BV(LSW1) | ~_BV(LSW2);//load 1, load 2 Not calling
										 //LoadSw1 = 0;
										 //LoadSw2 = 0;
			if (sustain >= 1.8) {
				PORTD |= _BV(LSW3);
				//LoadSw3 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 1.6) {
				MainsReq(1);//0.2A, now have 1.8A
				PORTD |= _BV(LSW3);
				//LoadSw3 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 1.4) {
				MainsReq(2);//0.4A, now have 1.8A
				PORTD |= _BV(LSW3);
				//LoadSw3 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 1.2) {
				MainsReq(3);//0.6A, now have 1.8A
				PORTD |= _BV(LSW3);
				//LoadSw3 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 1) {
				MainsReq(4);//0.8A, now have 1.8A
				PORTD |= _BV(LSW3);
				//LoadSw3 = 1;
				CBattery = 1;
				DBattery = 0;
			}
			else if (sustain >= 0.8) {
				MainsReq(0);//0A, now have 0.8A
				PORTD |= _BV(LSW3);
				//LoadSw3 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else if (sustain >= 0.6) {
				MainsReq(1);//0.2A, now have 0.8A
				PORTD |= _BV(LSW3);
				//LoadSw3 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else if (sustain >= 0.4) {
				MainsReq(2);//0.4A, now have 0.8A
				PORTD |= _BV(LSW3);
				//LoadSw3 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else if (sustain >= 0.2) {
				MainsReq(3);//0.6A, now have 0.8A
				PORTD |= _BV(LSW3);
				//LoadSw3 = 1;
				CBattery = 0;
				DBattery = 0;
			}
			else {//sustainable energy too low
				MainsReq(4);//0.8
				PORTD |= _BV(LSW3);
				//LoadSw3 = 1;
				CBattery = 0;
				DBattery = 0;
			}
		}
		else { //no demand, 000
		PORTD &= ~_BV(LSW1) | ~_BV(LSW2) | ~_BV(LSW3);//load 1,load 2 and load 3 not calling
			//LoadSw1 = 0;LoadSw2 = 0; LoadSw3 = 0;
			DBattery = 0;//no need discharge
			if (sustain >= 1) {
				MainsReq(0);
				CBattery = 1;//1A
			}
			else if (sustain >= 0.8) {
				DBattery = 0;
				MainsReq(1);//0.2A, now have 1A
				CBattery = 1;//1A
			}
			else if (sustain >= 0.6) {
				DBattery = 0;
				MainsReq(2);//0.4A, now have 1A
				CBattery = 1;//1A
			}
			else if (sustain >= 0.4) {
				MainsReq(3);//0.6A, now have 1A
				CBattery = 1;//1A
			}
			else {//sustainable energy too low, not worth to charge battery
				PORTD &= ~_BV(LSW1) | ~_BV(LSW2) | ~_BV(LSW3);//load 1, load 2 and load 3 Not calling
				//LoadSw1 = 0;
				//LoadSw2 = 0;
				//LoadSw3 = 0;
				MainsReq(0);//0A
				CBattery = 0;
			}
		}
	}
	bat_count();
	count_power();
}