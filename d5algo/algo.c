/*
inputs:
    pv: max 1A
    wind: max 3A
    main: max 2A
    batt(discharge): max 1A
outputs:
    load1: 1.2A
    load2: 2A
    load3: 0.8A
    batt(charge): max 1A
*/

// #include <stdlib.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <time.h>
// #include <conio.h>

int active = 1;
int DBatt, CBatt, BattPerc, BattThreshold, BattPriority, BatteryState; // BattPriority

int LoadCall;   //* written in dec [LoadCall1,LoadCall2,LoadCall3]
int LoadSupply; //* written in dec [LoadSupply1,LoadSupply2,LoadSupply3]
// int LoadPriority[8] = {0, 1, 3, 4, 4, 5, 10, 11};
double LoadRequirements[8] = {0.0, 0.8, 2.0, 2.8, 1.2, 2.0, 3.2, 4.0};
double pv, Wind, Main, WantedMain, BusVoltage, BusCurrent, Power, TotalCurrent, Renewable, SuppliedLoad, TotalLoad; //* PV, Wind, Main
int timecount;
/*
void delay(int milliseconds)
{
    long pause;
    clock_t now, then;

    pause = milliseconds * (CLOCKS_PER_SEC / 1000);
    now = then = clock();
    while ((now - then) < pause)
        now = clock();
}
*/
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include "d5display.c"
// define input located in Port A
// ADC inputs

//! #define PV PA0
//! #define WIND PA1
//! #define BUS_CURRENT PA2
//! #define BUS_VOLT PA3

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

ISR(TIMER1_COMPA_vect)
{
    timecount++;
    if (CBatt == 1)
        BattPerc++;
    else if (DBatt == 1 && BattPerc > 0)
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
    if (CBatt)
        PORTD |= _BV(CB);
    else
        PORTD &= ~_BV(CB);
}
void DBattery()
{
    if (DBatt)
        PORTD |= _BV(DB);
    else
        PORTD &= ~_BV(DB);
}

uint16_t read_adc(uint16_t a)
{
    ADMUX = a;
    ADCSRA |= _BV(ADSC); // Start convertions
    while (ADCSRA & _BV(ADSC))
        ;       // Wait for conversion to finish
    return ADC; // returning the 10bit result
}

void initializePins()
{
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
    OCR1A = 5858;                    // 1 seconds

    ADCSRA |= _BV(ADEN);                            // Enable ADC //*in_adc
    ADCSRA |= _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // Set pre-scaler to 128 F_CPU = 93.75kHz
}
void updateInput()
{
    LoadCall = (PINA & _BV(LOAD1)) * 4 + (PINA & _BV(LOAD2)) * 2 + (PINA & _BV(LOAD3));
    pv = ((double)read_adc(0) * 3.3) / MAXADC;
    Wind = ((double)read_adc(1) * 3.3) / MAXADC;
    BusVoltage = ((double)read_adc(2) * 7.07106781187) / MAXADC;
    BusCurrent = ((double)read_adc(3) * 2.85) / MAXADC;
    Power += ((BusVoltage * BusCurrent) / 1000) * (timecount / 60);
}
void updateOutput()
{
    LoadSupply1();
    LoadSupply2();
    LoadSupply3();
    CBattery();
    DBattery();
    OCR2A = Main / 2 * 255;

    Sources[2] =
        Sources[1] = Wind;
    Sources[0] = Main;
    BasicInfo[2] = BusCurrent;
    BasicInfo[1] = BusVoltage;
    BasicInfo[0] = Power;
    CallState[0] = LoadCall % 2;
    CallState[1] = LoadCall / 2 % 2;
    CallState[2] = LoadCall / 4 % 2;
    SupplyState[0] = LoadSupply % 2;
    SupplyState[1] = LoadSupply / 2 % 2;
    SupplyState[2] = LoadSupply / 4 % 2;
    Load[0] = (LoadCall % 2) ? 1.20 : 0.00;
    Load[1] = (LoadCall / 2 % 2) ? 2.00 : 0.00;
    Load[2] = (LoadCall / 4 % 2) ? 0.80 : 0.00;

    BatteryState = CBatt * 1 + DBatt * -1;
    Update(BattPerc, BasicInfo, BatteryState, Sources, Load, CallState, SupplyState);
}

int chargeBattery(int mode)
{
    switch (mode)
    {
    case 1:
        CBatt = 1;
        DBatt = 0;
        break;
    case -1:
        CBatt = 0;
        DBatt = 1;
        break;
    default:
        CBatt = 0;
        DBatt = 0;
        break;
    }
    return mode;
}

void algomain()
{

    // updateInput();
    Renewable = Wind + pv;
    TotalLoad = LoadCall % 2 * 0.8 + LoadCall / 2 % 2 * 2 + LoadCall / 4 % 2 * 1.2;

    if (((Renewable >= LoadRequirements[LoadCall] + 1.0) || (BattPriority == 1)) && (BattPerc < 120))
        chargeBattery(1);
    else if ((TotalLoad - Renewable > 0) && ((BattPerc > BattThreshold) || (LoadCall > 5) || (BattPriority = -1)))
        chargeBattery(-1);
    else
        chargeBattery(0);

    WantedMain = (TotalLoad - Renewable - DBatt + CBatt > 0) ? ((TotalLoad - Renewable - DBatt + CBatt) < (2.0)) ? (TotalLoad - Renewable - DBatt + CBatt) : (2.00) : 0; //* Define max main value

    if ((LoadCall < 6) && (BattPerc < BattThreshold) && (timecount / 60 <= 23))
        BattPriority = 1;
    else if (timecount / 60 > 23)
        BattPriority = -1;
    else
        BattPriority = 0;

    switch (LoadCall)
    {
    case 7: //* State 111
        LoadSupply = (Renewable + WantedMain + DBatt >= LoadRequirements[4]) ? 4 : 0;
        LoadSupply = (Renewable + WantedMain + DBatt >= LoadRequirements[5]) ? 5 : LoadSupply;
        LoadSupply = (Renewable + WantedMain + DBatt >= LoadRequirements[6]) ? 6 : LoadSupply;
        break;
    case 6: //* State 110
    case 5: //* State 101
        LoadSupply = (Renewable + WantedMain + DBatt >= LoadRequirements[4]) ? 4 : 0;
        break;
    case 3: //* State 011
        LoadSupply = (Renewable + WantedMain + DBatt >= LoadRequirements[2]) ? 2 : 0;
        LoadSupply = (Renewable + WantedMain + DBatt >= LoadRequirements[LoadCall]) ? LoadCall : LoadSupply;
        break;
    default: //* State 000
        LoadSupply = (Renewable + WantedMain + DBatt >= LoadRequirements[LoadCall]) ? LoadCall : 0;
        break;
    }
    LoadSupply = (Renewable + WantedMain + DBatt >= LoadRequirements[LoadCall]) ? LoadCall : LoadSupply;

    SuppliedLoad = LoadSupply % 2 * 0.8 + LoadSupply / 2 % 2 * 2 + LoadSupply / 4 % 2 * 1.2;
    Main = (WantedMain) ? ((SuppliedLoad - Renewable - DBatt + CBatt) > (0.0)) ? (SuppliedLoad - Renewable - DBatt + CBatt) : (0.0) : 0; //*actual main value
    TotalCurrent = Renewable + Main + DBatt - CBatt - SuppliedLoad;
    // updateOutput();
}

int main()
{
    BattPerc = 0;
    BattThreshold = 30;
    timecount = 0;
    BattPriority = 0;
    LoadCall = 0;   //* written in dec [LoadCall1,LoadCall2,LoadCall3]
    LoadSupply = 0; //* written in dec [LoadSupply1,LoadSupply2,LoadSupply3]
    pv = 0.0;
    Wind = 0.0;
    Main = 0.0; //* PV, Wind, Main

    // timecount = 60 * 24;
    // int c;
    // int debug = 1;
    // int LoadActive[3] = {LoadCall / 4 % 2, LoadCall / 2 % 2, LoadCall % 2};

    while (active)
    {
        // printf("%x\n", c);
        algomain();
        /*
        system("@cls||clear");
        printf("\n---------------------------------\n---------------------------------\n");
        printf("Load 1: %d | %d\n", LoadCall / 4 % 2, LoadSupply / 4 % 2);
        printf("Load 2: %d | %d\n", LoadCall / 2 % 2, LoadSupply / 2 % 2);
        printf("Load 3: %d | %d\n", LoadCall % 2, LoadSupply % 2);
        printf("CB: %d| DB: %d\n", CBatt, DBatt);
        printf("---------------------------------\n");
        printf("Main | Wind | PV\n");
        printf("%.2f | %.2f | %.2f\n", Main, Wind, pv);
        if (debug)
        {
            printf("---------------------------------\n");
            printf("Total Load: %.2f\n", TotalLoad);
            printf("Supplied Load: %.2f\n", SuppliedLoad + CBatt);
            printf("Renewable: %.2f\n", Renewable);
            printf("Total Current: %.2f\n", TotalCurrent);
        }
        c = getch(); //*30-39: Numpad

        if (c == 0x34 && Wind >= 0.01)
            Wind -= 0.1;
        else if (c == 0x37 && Wind <= 2.99)
            Wind += 0.1;
        else if (c == 0x35 && pv >= 0.01)
            pv -= 0.1;
        else if (c == 0x38 && pv <= 0.99)
            pv += 0.1;
        else if (c == 0x31)
            LoadActive[0] = !LoadActive[0];
        else if (c == 0x32)
            LoadActive[1] = !LoadActive[1];
        else if (c == 0x33)
            LoadActive[2] = !LoadActive[2];
        LoadCall = LoadActive[0] * 4 + LoadActive[1] * 2 + LoadActive[2];
        */
    }
    return 0;
}