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
uint16_t LoadCall1 = 1;
uint16_t LoadCall2 = 1;
uint16_t LoadCall3 = 1;
uint16_t DBattery;
uint16_t CBattery;
float sustain, main_current;

volatile uint16_t count = 0x00;
volatile uint16_t count1 = 0x00;

float count_sustain(void) {//count the sustainable energy
	float a, b;
	//!Point of temp?
	a = (adc_pv()/ MAXADC)*5.0; //varying from 0 to 5V
	b = (adc_wind()/ MAXADC)*5.0;//varying from 0 to 5V
	sustain = a + b;
	return sustain;
}
float count_power(void) {
	float a, b;
	//temp1 = adc_bc();
	a = (main_current>512) ? (adc_bc() / MAXADC) * 4 : (adc_bc() / MAXADC) * -4;
	b = (adc_bv()>512) ? (adc_bv() / MAXADC) * 10 : (adc_bv() / MAXADC) * -10;
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
		switch(LoadCall1*4+LoadCall2*2+LoadCall3) {
			case 7: //111, demand = 4A
				switch((int)(sustain*10)) {
					case 40 ... 100:
						MainsReq(5);//1A, now have 5A
						PORTD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3);
						//LoadSw1 = 1; LoadSw2 = 1;LoadSw3 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 30 ... 39:  //have 3.0A
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
						break;
					case 22 ... 29:  //have 2.2A				
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
						break;
					case 12 ... 21: //1.2A
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
						break;
					case 2 ... 11: //0.2A
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
						break;
					default: //sustainable energy too low
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
						break;
				}
				break;
			case 6: //110, demand = 3.2A
				PORTD &= ~_BV(LSW3);//load 3 not calling
				//LoadSw3 = 0;
				switch((int)(sustain*10) {
					case 40 ... 100: //4A
						MainsReq(1);//0.2A, now have 4.2
						PORTD |= _BV(LSW1) | _BV(LSW2);
						//LoadSw1 = 1; LoadSw2 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 38 ... 39: //3.8A
						MainsReq(2);//0.4A, now have 4.2
						PORTD |= _BV(LSW1) | _BV(LSW2);
						CBattery = 1;
						DBattery = 0;
						//LoadSw1 = 1; LoadSw2 = 1;
						break;
					case 32 ... 37: //3.2A
						MainsReq(0);//0A, now have 3.2
						PORTD |= _BV(LSW1) | _BV(LSW2);
						//LoadSw1 = 1; LoadSw2 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 22 ... 31: //2.2A
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
						break;
					case 10 ... 21; //1A
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
						break;
					case 2 ... 9: //0.2A
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
						break;
					default: //sustainable energy too low
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
						break;
				}
				break;
			case 5: //101, demand = 2A
				PORTD &= ~_BV(LSW2);//load 2 not calling
				//LoadSw2 = 0;
				switch((int)(sustain*10)) {
					case 30 ... 100: //3A
						MainsReq(0);//0A, now have 3A
						PORTD |= _BV(LSW1) | _BV(LSW3);
						//LoadSw1 = 1; LoadSw3 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 28 ... 29: //2.8A
						MainsReq(1);//0.2A, now have 3A
						PORTD |= _BV(LSW1) | _BV(LSW3);
						//LoadSw1 = 1; LoadSw3 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 26 ... 27: //2.6A
						MainsReq(6);//0.4A, now have 3A
						PORTD |= _BV(LSW1) | _BV(LSW3);
						//LoadSw1 = 1; LoadSw3 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 20 ... 25: //2A
						MainsReq(0);//0A, now have 2A
						PORTD |= _BV(LSW1) | _BV(LSW3);
						//LoadSw1 = 1; LoadSw3 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 10 ... 19: //1A
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
						break;
					case 2 ... 9: //0.2A
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
						break;
					default: //sustainable energy too low
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
						break;
				}
				break;
			case 4: //100, demand = 1.2A
				PORTD &= ~_BV(LSW2) | ~_BV(LSW3);//load 2, load 3 Not calling //LoadSw2 = 0; //LoadSw3 = 0;
				switch((int)(sustain*10)) {
					case 22 ... 100:
						MainsReq(0);//0A, now have 2.2A
						PORTD |= _BV(LSW1);
						//LoadSw1 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 12 ... 21: 
						MainsReq(0);//0A, now have 1.2A
						PORTD |= _BV(LSW1);
						//LoadSw1 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 10 ... 11: 
						MainsReq(1);//0.2A, now have 1.2A
						PORTD |= _BV(LSW1);
						//LoadSw1 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 8 ... 9:
						MainsReq(2);//0.4A, now have 1.2A
						PORTD |= _BV(LSW1);
						//LoadSw1 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 6 ... 7:
						MainsReq(3);//0.6A, now have 1.2A
						PORTD |= _BV(LSW1);
						//LoadSw1 = 1;
						CBattery = 0;
						CBattery = 0;
						break;
					case 4 ... 5:
						MainsReq(4);//0.8A, now have 1.2A
						PORTD |= _BV(LSW1);
						//LoadSw1 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 2 ... 3:
						MainsReq(5);//1A, now have 1.2A
						PORTD |= _BV(LSW1);
						//LoadSw1 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					default://sustainable energy too low
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
						break;
				}
				break;
			case 3: // 011,demand = 2.8
				PORTD &= ~_BV(LSW1);//load 1 not calling
				//LoadSw1 = 0;
				switch((int)(sustain*10)) {
					case 38 ... 100:
						PORTD |= _BV(LSW2) | _BV(LSW3);
						//LoadSw2 = 1; LoadSw3 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 28 ... 37: 
						MainsReq(0);//0A, now have 2.8A
						PORTD |= _BV(LSW2) | _BV(LSW3);
						//LoadSw2 = 1; LoadSw3 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 18 ... 27:
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
						break;
					case 8 ... 17:
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
						break;
					case 2 ... 7:
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
						break;
					default: //sustainable energy too low
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
						break;
				}
				break;
			case 2: //010, demand = 2A
				PORTD &= ~_BV(LSW1) | ~_BV(LSW3);//load 1 and load 3 not calling
				//LoadSw1 = 1; LoadSw3 = 1;
				switch((int)(sustain*10)) {
					case 30 ... 100:
						MainsReq(0);//0A, now have 3A
						PORTD |= _BV(LSW2);
						//LoadSw2 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 28 ... 29: // 2.8A
						MainsReq(1);//0.2A, now have 3A
						PORTD |= _BV(LSW2);
						//LoadSw2 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 26 ... 27: // 2.6A
						MainsReq(2);//0.4A, now have 3A
						PORTD |= _BV(LSW2);
						//LoadSw2 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 20 ... 25: // 2A
						MainsReq(0);//0A, now have 2A
						PORTD |= _BV(LSW2);
						//LoadSw2 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 18 ... 19: // 1.8A
						MainsReq(1);//0.2A, now have 2A
						PORTD |= _BV(LSW2);
						//LoadSw2 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 16 ... 17: // 1.6A
						MainsReq(2);//0.4A, now have 2A
						PORTD |= _BV(LSW2);
						//LoadSw2 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 14 ... 15: // 1.4A
						MainsReq(3);//0.6A, now have 2A
						PORTD |= _BV(LSW2);
						//LoadSw2 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 12 ... 13: // 1.2A
						MainsReq(4);//0.8A, now have 2A
						PORTD |= _BV(LSW2);
						//LoadSw2 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 10 ... 11: // 1A
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
						break;
					default: // sustainable energy too low
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
						break;
				}
				break;
			case 1: //no demand, 000
				PORTD &= ~_BV(LSW1) | ~_BV(LSW2);//load 1, load 2 Not calling //LoadSw1 = 0; //LoadSw2 = 0;
				switch((int)(sustain*10)) {
					case 18 ... 100:
						PORTD |= _BV(LSW3);
						//LoadSw3 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 16 ... 17:
						MainsReq(1);//0.2A, now have 1.8A
						PORTD |= _BV(LSW3);
						//LoadSw3 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 14 ... 15: 
						MainsReq(2);//0.4A, now have 1.8A
						PORTD |= _BV(LSW3);
						//LoadSw3 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 12 ... 13: 
						MainsReq(3);//0.6A, now have 1.8A
						PORTD |= _BV(LSW3);
						//LoadSw3 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 10 ... 11: 
						MainsReq(4);//0.8A, now have 1.8A
						PORTD |= _BV(LSW3);
						//LoadSw3 = 1;
						CBattery = 1;
						DBattery = 0;
						break;
					case 8 ... 9:
						MainsReq(0);//0A, now have 0.8A
						PORTD |= _BV(LSW3);
						//LoadSw3 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 6 ... 7:
						MainsReq(1);//0.2A, now have 0.8A
						PORTD |= _BV(LSW3);
						//LoadSw3 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 4 ... 5:
						MainsReq(2);//0.4A, now have 0.8A
						PORTD |= _BV(LSW3);
						//LoadSw3 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					case 2 ... 3:
						MainsReq(3);//0.6A, now have 0.8A
						PORTD |= _BV(LSW3);
						//LoadSw3 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
					default: //sustainable energy too low
						MainsReq(4);//0.8
						PORTD |= _BV(LSW3);
						//LoadSw3 = 1;
						CBattery = 0;
						DBattery = 0;
						break;
				}
				break;
			case 0: //101, demand = 2A
				PORTD &= ~_BV(LSW1) | ~_BV(LSW2) | ~_BV(LSW3);//load 1,load 2 and load 3 not calling
				//LoadSw1 = 0;LoadSw2 = 0; LoadSw3 = 0;
				DBattery = 0;//no need discharge
				switch((int)(sustain*10)) {
					case 10 ... 100:
						MainsReq(0);
						CBattery = 1;//1A
						break;
					case 8 ... 9:
						DBattery = 0;
						MainsReq(1);//0.2A, now have 1A
						CBattery = 1;//1A
						break;
					case 6 ... 7:
						DBattery = 0;
						MainsReq(2);//0.4A, now have 1A
						CBattery = 1;//1A
						break;
					case 4 ... 5:
						MainsReq(3);//0.6A, now have 1A
						CBattery = 1;//1A
						break;
					default: //sustainable energy too low, not worth to charge battery
						PORTD &= ~_BV(LSW1) | ~_BV(LSW2) | ~_BV(LSW3);//load 1, load 2 and load 3 Not calling
						//LoadSw1 = 0;
						//LoadSw2 = 0;
						//LoadSw3 = 0;
						MainsReq(0);//0A
						CBattery = 0;
						break;
				}
				break;
			default: 
				break;
		}
	}
	bat_count();
	count_power();
}