// #include <stdlib.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <time.h>
// #include <conio.h> //? PC mode

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include "debug.h"
#include "d5display.c" //? Ilmatto mode

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
int active = 1;
int DBatt, CBatt, BattPerc, BattThreshold, BattPriority, BatteryState;                                               // BattPriority
int LoadCall;                                                                                                        //* written in dec [LoadCall1,LoadCall2,LoadCall3]
int LoadSupply;                                                                                                      //* written in dec [LoadSupply1,LoadSupply2,LoadSupply3] // int LoadPriority[8] = {0, 1, 3, 4, 4, 5, 10, 11};
double Pv, Wind, Main, WantedMain, WantedTotal, Affordabily, BusVoltage, BusCurrent, Power, TotalCurrent, Renewable; //* Pv, Wind, Main
double MainThresh = 0.05;
int timecount;
//? PC mode
// void delay(int milliseconds)
// {
//     long pause;
//     clock_t now, then;

//     pause = milliseconds * (CLOCKS_PER_SEC / 1000);
//     now = then = clock();
//     while ((now - then) < pause)
//         now = clock();
// }

// define input located in Port A
// ADC inputs

//! #define Pv PA0
//! #define WIND PA1
//! #define BUS_CURRENT PA2
//! #define BUS_VOLT PA3
//? Ilmatto mode
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
    OCR1A = 5858;                    // 1 seconds

    ADCSRA |= _BV(ADEN);                            // Enable ADC //*in_adc
    ADCSRA |= _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2); // Set pre-scaler to 128 F_CPU = 93.75kHz
}
void updateInput()
{
    uint16_t LoadCall1 = PINA & _BV(4) ? 1 : 0;
    uint16_t LoadCall2 = PINA & _BV(5) ? 1 : 0;
    uint16_t LoadCall3 = PINA & _BV(6) ? 1 : 0;
    LoadCall = (LoadCall1)*4 + (LoadCall2)*2 + (LoadCall3);
    Pv = ((double)read_adc(0) * 3.3) / MAXADC;
    Wind = ((double)read_adc(1) * 3.3) / MAXADC;
    BusCurrent = ((double)read_adc(2) * 7.07106781187) / MAXADC;
    BusVoltage = ((double)read_adc(3) * 2.85) / MAXADC;
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
    double BasicInfo[3] = {Power, BusVoltage, BusCurrent};
    double Sources[3] = {Main, Wind, Pv};
    double Load[3] = {(LoadCall % 2) ? 1.20 : 0.00, (LoadCall / 2 % 2) ? 2.00 : 0.00, (LoadCall / 4 % 2) ? 0.80 : 0.00};
    uint8_t CallState[3] = {LoadCall / 4 % 2, LoadCall / 2 % 2, LoadCall % 2};
    uint8_t SupplyState[3] = {LoadSupply / 4 % 2, LoadSupply / 2 % 2, LoadSupply % 2};
    BatteryState = CBatt * 1 + DBatt * -1;
    Update(BattPerc, BasicInfo, BatteryState, Sources, Load, CallState, SupplyState);
}

//? Ilmatto mode
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
double LoadRequirements[8] = {0.0, 0.8, 2.0, 2.8, 1.2, 2.0, 3.2, 4.0};

void algomain()
{
    // TODO Priority Discharge

    updateInput(); //? Ilmatto mode
    Renewable = Wind + Pv;

    if ((Renewable >= LoadRequirements[LoadCall] + 1.0)) //* Can afford to charge battery with renewable
        chargeBattery(1);
    else
        chargeBattery(0);

    if ((LoadRequirements[LoadCall] - Renewable > 0) && (LoadCall != 3) && (LoadCall < 6))                                                //*Power Needed, not case 3,6,7
        WantedMain = (LoadRequirements[LoadCall] - Renewable + MainThresh > 2) ? 2 : LoadRequirements[LoadCall] - Renewable + MainThresh; //* Max 2.00, may not be enough cause call issues due to threshold

    else if ((LoadRequirements[LoadCall] - Renewable > 0) && (BattPerc > 0)) //*Power Needed, case 3,6,7, can afford Battery
    {
        chargeBattery(-1);
        WantedMain = LoadRequirements[LoadCall] - Renewable + MainThresh - DBatt;
    }
    if ((WantedMain < 1) && (BattPerc < BattThreshold) && (LoadCall != 3) && (LoadCall < 6)) //* Priority Recharge State, not case 3,6,7
    {
        chargeBattery(1);
        WantedMain = LoadRequirements[LoadCall] - Renewable + MainThresh + CBatt;
    }
    WantedTotal = WantedMain + Renewable;

    switch (LoadCall)
    {
    case 7: //* State 111
        LoadSupply = (WantedTotal >= LoadRequirements[4]) ? 4 : 0;
        LoadSupply = (WantedTotal >= LoadRequirements[5]) ? 5 : LoadSupply;
        LoadSupply = (WantedTotal >= LoadRequirements[6]) ? 6 : LoadSupply;
        break;
    case 6: //* State 110
    case 5: //* State 101
        LoadSupply = (WantedTotal >= LoadRequirements[4]) ? 4 : 0;
        break;
    case 3: //* State 011
        LoadSupply = (WantedTotal >= LoadRequirements[2]) ? 2 : 0;
        LoadSupply = (WantedTotal >= LoadRequirements[LoadCall]) ? LoadCall : LoadSupply;
        break;
    default: //* State 000
        LoadSupply = (WantedTotal >= LoadRequirements[LoadCall]) ? LoadCall : 0;
        break;
    }
    LoadSupply = (WantedTotal >= LoadRequirements[LoadCall]) ? LoadCall : LoadSupply;

    Main = ((WantedMain > 0) && (LoadRequirements[LoadSupply] - Renewable - DBatt + CBatt + MainThresh) > (0.0)) ? (LoadRequirements[LoadSupply] - Renewable - DBatt + CBatt + MainThresh) : 0; //*actual main value
    if ((Main > 1) && (BattPerc > BattThreshold) && (!DBatt))
    {
        chargeBattery(-1);
        Main -= 1;
    }
    Main = (Main > 2) ? 2 : Main;
    TotalCurrent = Renewable + Main + DBatt - CBatt - LoadRequirements[LoadSupply];
    updateOutput(); //? Ilmatto mode
}

int main()
{
    init_debug_uart0();
    BattPerc = 0;
    BattThreshold = 30;
    timecount = 0;
    BattPriority = 0;
    LoadCall = 0;   //* written in dec [LoadCall1,LoadCall2,LoadCall3]
    LoadSupply = 0; //* written in dec [LoadSupply1,LoadSupply2,LoadSupply3]
    Pv = 0.0;
    Wind = 0.0;
    Main = 0.0; //* Pv, Wind, Main
    WantedMain = 0.0;
    printf("\n\nLoaded\n\n");
    Start(); //? Ilmatto mode

    // timecount = 60 * 24; //? PC mode
    // int c;                                                                  //? PC mode
    int debug = 1; //? PC mode
    // int LoadActive[3] = {LoadCall / 4 % 2, LoadCall / 2 % 2, LoadCall % 2}; //? PC mode
    initializePins(); //? Ilmatto mode
    while (active)
    {
        // printf("%x\n", c); //? PC mode
        algomain();
        //? PC mode
        // system("@cls||clear");
        printf("\n---------------------------------\n---------------------------------\n");
        printf("Load 1: %d | %d\n", LoadCall / 4 % 2, LoadSupply / 4 % 2);
        printf("Load 2: %d | %d\n", LoadCall / 2 % 2, LoadSupply / 2 % 2);
        printf("Load 3: %d | %d\n", LoadCall % 2, LoadSupply % 2);
        printf("CB: %d| DB: %d        Batt: %d\n", CBatt, DBatt, BattPerc);
        printf("---------------------------------\n");
        printf("Main | Wind | Pv\n");
        printf("%.2f | %.2f | %.2f\n", Main, Wind, Pv);
        printf("Wanted Main: %.2f\n", WantedMain);
        if (debug)
        {
            printf("---------------------------------\n");
            printf("Total Load: %.2f\n", LoadRequirements[LoadCall]);
            printf("Supplied Load: %.2f\n", LoadRequirements[LoadSupply] + CBatt);
            printf("Renewable: %.2f\n", Renewable);
            printf("Total Current: %.2f\n", TotalCurrent);
        }
        // c = getch(); //*30-39: Numpad

        // if (c == 0x34 && Wind >= 0.01)
        //     Wind -= 0.1;
        // else if (c == 0x37 && Wind <= 2.99)
        //     Wind += 0.1;
        // else if (c == 0x35 && Pv >= 0.01)
        //     Pv -= 0.1;
        // else if (c == 0x38 && Pv <= 0.99)
        //     Pv += 0.1;
        // else if (c == 0x31)
        //     LoadActive[0] = !LoadActive[0];
        // else if (c == 0x32)
        //     LoadActive[1] = !LoadActive[1];
        // else if (c == 0x33)
        //     LoadActive[2] = !LoadActive[2];
        // LoadCall = LoadActive[0] * 4 + LoadActive[1] * 2 + LoadActive[2];
        //? PC mode
        // _delay_ms(1000);
        // timecount++;
        // if (CBatt == 1)
        //     BattPerc++;
        // else if (DBatt == 1 && BattPerc > 0)
        //     BattPerc--;
        // printf("%d: Loads | %d | %d | %d || %d\n", timecount, LoadCall / 4 % 2, LoadCall / 2 % 2, LoadCall % 2, LoadCall);
    }
    return 0;
}