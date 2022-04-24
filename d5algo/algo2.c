#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include "debug.h"
// #include "d5display.c"

// #include <stdlib.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <time.h>
// #include <conio.h>

// Digital Inputs
//! #define Pv PA0
//! #define WIND PA1
//! #define BUS_CURRENT PA2
//! #define BUS_VOLT PA3
#define LOAD1 PA4
#define LOAD2 PA5
#define LOAD3 PA6

// Digital Outputs
#define LSW1 PD4
#define LSW2 PD5
#define LSW3 PD6
#define DB PD3
#define CB PD2
#define MAINS PD7 // PWM // define output located in Port D

#define CLKFREQ 12000000 // 12MHz
#define MAXADC 1023
#define MAXBATTERY 10 // 10ah battery

#define MAXSUPPLY 3
int screen = 0, active = 0, timecount;
int BattMode, BattPerc, BattThresh, LoadCall, LoadSupply;
double CurrentNeeded, Supplied, Called, Renewable, Power, BusCurrent, BusVoltage, Wind, Pv, Main, TotalCurrent;

//*ISR, runs per second
ISR(TIMER1_COMPA_vect)
{
    timecount++;
    if (BattMode == 1)
        BattPerc++;
    else if (BattMode == -1 && BattPerc > 0)
        BattPerc--;
}
void LoadSupply1()
{
    if (LoadSupply / 4 % 2)
        PORTD |= _BV(LSW1);
    else
        PORTD &= ~_BV(LSW1);
}
void LoadSupply2()
{
    if (LoadSupply / 2 % 2)
        PORTD |= _BV(LSW2);
    else
        PORTD &= ~_BV(LSW2);
}
void LoadSupply3()
{
    if (LoadSupply % 2)
        PORTD |= _BV(LSW3);
    else
        PORTD &= ~_BV(LSW3);
}
void CBattery()
{
    if (BattMode == 1)
        PORTD |= _BV(CB);
    else
        PORTD &= ~_BV(CB);
}
void DBattery()
{
    if (BattMode == -1)
        PORTD |= _BV(DB);
    else
        PORTD &= ~_BV(DB);
}
double read_adc(uint16_t a)
{
    ADMUX = a;
    ADCSRA |= _BV(ADSC); // Start convertions
    while (ADCSRA & _BV(ADSC))
        ;               // Wait for conversion to finish
    return (double)ADC; // returning the 10bit result
}

void initializePins()
{
    sei();
    DDRA &= ~_BV(LOAD1); //*digital inputs
    DDRA &= ~_BV(LOAD2);
    DDRA &= ~_BV(LOAD3); // Set bit 4 to 6 of Port A as inputs

    DDRD |= _BV(LSW1) | _BV(LSW2) | _BV(LSW3) | _BV(DB) | _BV(CB); // Set bit 2 to 6 of Port D as outputs //*digital outputs

    DDRD |= _BV(MAINS);
    TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20); // Mode 1, PWM, Phase Correct, 10-bit //*pwm
    TCCR2B |= _BV(CS20);                            // F_CPU = 12MHz and disable pre-scalar, F_CLK = 11.71kHz

    TIMSK1 |= _BV(OCIE1A);           // enable timer interrupt for timer1
    TCCR1B |= _BV(CS12) | _BV(CS10); // F_CPU = 12MHz and pre-scaler = 1024, F_CLK = 11.71kHz //*timer_interrupt
    TCCR1B |= _BV(WGM12);            // Clear Timer on Compare Match mode (CTC)
    OCR1A = 11718.75;                // 1 seconds

    ADCSRA |= _BV(ADEN);                            // Enable ADC //*in_adc
    ADCSRA |= _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // Set pre-scaler to 128 F_CPU = 93.75kHz
    printf("\n\x1B[33m~Pins Initialized~\x1B[37m");
}
void updateInput()
{
    uint16_t LoadCall1 = PINA & _BV(4) ? 1 : 0;
    uint16_t LoadCall2 = PINA & _BV(5) ? 1 : 0;
    uint16_t LoadCall3 = PINA & _BV(6) ? 1 : 0;
    LoadCall = (LoadCall1)*4 + (LoadCall2)*2 + (LoadCall3);
    Pv = (read_adc(0) * 3.3) / MAXADC;                   //*0 - 3.3
    Wind = (read_adc(1) * 3.3) / MAXADC;                 //*0 - 3.3
    BusCurrent = (read_adc(2) * 7.07106781187) / MAXADC; //* 0-(10/root(2))
    BusVoltage = (read_adc(3) * 2.85 * 100) / MAXADC;    //* 0-(4/root(2))
    Power += ((BusVoltage * BusCurrent) / 1000) * (timecount / 60);
}
void updateOutput()
{
    LoadSupply1();
    LoadSupply2();
    LoadSupply3();
    CBattery();
    DBattery();
    OCR2A = Main / 4 * 255;
    // double BasicInfo[3] = {Power, BusVoltage, BusCurrent};
    // double Sources[3] = {Main, Wind, Pv};
    // double Load[3] = {(LoadCall % 2) ? 1.20 : 0.00, (LoadCall / 2 % 2) ? 2.00 : 0.00, (LoadCall / 4 % 2) ? 0.80 : 0.00};
    // uint8_t CallState[3] = {LoadCall / 4 % 2, LoadCall / 2 % 2, LoadCall % 2};
    // uint8_t SupplyState[3] = {LoadSupply / 4 % 2, LoadSupply / 2 % 2, LoadSupply % 2};
    // if (screen)
    //     Update(BattPerc, BasicInfo, BattMode, Sources, Load, CallState, SupplyState);
}

double LoadRequirements[8] = {0.0, 0.8, 2.0, 2.8, 1.2, 2.0, 3.2, 4.0};

void algomain()
{
    updateInput();
    // TODO Battery Priority Discharge

    Renewable = Wind + Pv; //* 4A
    Called = LoadRequirements[LoadCall];

    if ((Called - Renewable > 2.0) & (BattPerc > 0))
        BattMode = -1;
    else if (((BattPerc < BattThresh) & (Renewable - Called + 2 > 1)) | (Renewable - Called > 1)) //* ((Battery needs charging)&(Main+Renewable can supply))|(Renewable-Call can sustain)
        BattMode = 1;
    else
        BattMode = 0;
    CurrentNeeded = Called - Renewable + BattMode; //* Charge  = 1,
    CurrentNeeded = (CurrentNeeded > 2) ? 2 : CurrentNeeded;
    CurrentNeeded = (CurrentNeeded < 0) ? 0 : CurrentNeeded;

    switch (LoadCall)
    {
    case 7:                                                                                                 //* State 111
        LoadSupply = (CurrentNeeded + Renewable - BattMode - LoadRequirements[4] >= -0.1) ? 4 : 0;          //* 1
        LoadSupply = (CurrentNeeded + Renewable - BattMode - LoadRequirements[5] >= -0.1) ? 5 : LoadSupply; //* 1 and 3
        LoadSupply = (CurrentNeeded + Renewable - BattMode - LoadRequirements[6] >= -0.1) ? 6 : LoadSupply; //* 1 and 2
        break;
    case 6: //* State 110
    case 5: //* State 101
        LoadSupply = (CurrentNeeded + Renewable - BattMode - LoadRequirements[4] >= -0.1) ? 4 : 0;
        break;
    case 3: //* State 011
        LoadSupply = (CurrentNeeded + Renewable - BattMode - LoadRequirements[2] >= -0.1) ? 2 : 0;
        break;
    default: //* State 000
        LoadSupply = (CurrentNeeded + Renewable - BattMode - Called >= -0.1) ? LoadCall : 0;
        break;
    }
    LoadSupply = (CurrentNeeded + Renewable - BattMode - Called >= -0.1) ? LoadCall : LoadSupply; //* All Req

    Supplied = LoadRequirements[LoadSupply];
    Main = Supplied - Renewable + BattMode;
    Main = (Main > 2) ? 2 : Main;
    Main = (Main < 0) ? 0 : Main;
    TotalCurrent = Renewable + Main - BattMode - Supplied;
    updateOutput();
}
/*
void printtofile()
{
    int num;
    FILE *fptr;
    fptr = fopen("outputsim.csv", "w");

    if (fptr == NULL)
    {
        printf("Error!");
        exit(1);
    }

    fprintf(fptr, "Renewable,LC,Main,LS,Bmode\n");

    for (int i = 0; i < 400; i++)
    {
        Renewable = (double)(i / 100);
        printf("%d\n", i);
        BattPerc = 20;
        for (LoadCall = 0; LoadCall < 8; LoadCall++)
        {
            algomain();
            fprintf(fptr, "%d,%d,%f,%d,%d\n", i, LoadCall, Main, LoadSupply, BattMode);
        }
        //     BattPerc = 20;
        //     for (LoadCall = 0; LoadCall < 8; LoadCall++)
        //     {
        //         algomain();
        //         fprintf(fptr, "%d,%d,%f,%d,%d\n", i, LoadCall, BattPerc, Main, LoadSupply, BattMode);
        //     }
    }
    fclose(fptr);
}
*/
int main()
{
    init_debug_uart0();
    BattPerc = BattMode = timecount = LoadCall = LoadSupply = 0; //* written in dec [LoadSupply1,LoadSupply2,LoadSupply3]
    Pv = Wind = Main = CurrentNeeded = 0.0;
    BattThresh = 30;
    printf("\n\n\x1B[93m~Booted Up~\n\n\x1B[37m");
    // if (screen)
    // Start();
    int debug = 1;
    initializePins();
    while (active)
    {
        algomain();
        printf("\n---------------------------------\n---------------------------------\n");
        printf("Load 1: %d | %d\n", LoadCall / 4 % 2, LoadSupply / 4 % 2);
        printf("Load 2: %d | %d\n", LoadCall / 2 % 2, LoadSupply / 2 % 2);
        printf("Load 3: %d | %d\n", LoadCall % 2, LoadSupply % 2);
        printf("CB: %d| DB: %d        Batt: %d\n", BattMode == 1, BattMode == -1, BattPerc);
        printf("---------------------------------\n");
        printf("Main | Wind | Pv\n");
        printf("%.2f | %.2f | %.2f\n", Main, Wind, Pv);
        if (debug)
        {
            printf("---------------------------------\n");
            printf("CurrentNeeded: %.2f\n", CurrentNeeded);
            printf("Called: %.2f\n", Called);
            printf("Supplied: %.2f\n", Supplied);
            printf("Renewable: %.2f\n", Renewable);
            printf("TotalCurrent: %.2f\n", TotalCurrent);
        }
        // _delay_ms(1000);
        //* In: Renewable (0-4), LoadCall, BatteryPerc
        //* Out: Main, LoadSupply, BatteryMode
    }
    // printtofile();
    return 0;
}