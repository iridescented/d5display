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
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdlib.h>

int LoadCall[3] = {0, 0, 0};
int LoadSupply[3] = {0, 0, 0};
int LoadPriority[8] = {0, 1, 3, 4, 4, 5, 10, 11};
float InputEnergy[3] = {0.0, 0.0, 0.0};
void updatePins();
void algomain()
{
    int bincases = LoadCall[2] * 4 + LoadCall[1] * 2 + LoadCall[0];
    switch (bincases)
    {
    case 7: //* State 111
        break;
    case 6: //* State 110
        break;
    case 5: //* State 101
        break;
    case 4: //* State 100
        break;
    case 3: //* State 011
        break;
    case 2: //* State 010
        break;
    case 1: //* State 001
        break;
    default: //* State 000
        LoadSupply = {0, 0, 0};
        break;
    }
    updatePins();
}

int main()
{
    return 0;
}