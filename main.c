/************** ECE2049 DEMO CODE ******************/
/**************  13 March 2019   ******************/
/***************************************************/





#include <msp430.h>

/* Peripherals.c and .h are where the functions that implement
 * the LEDs and keypad, etc are. It is often useful to organize
 * your code by putting like functions together in files.
 * You include the header associated with that file(s)
 * into the main file of your project. */
#include "peripherals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


// Function Prototypes
void swDelay(char numLoops);
//void makeDate(unsigned int days, char *s);
//void displayTime(long unsigned int seconds);
//void displayTemp(float inAvgTempC);
//void makeDate(long unsigned int month, long unsigned int days, char *s);


// Declare globals here
long unsigned int timer_cnt = 0;
long unsigned int startTime = 0;
long unsigned int currentTime = 0;
int i;
long unsigned int seconds;

int currButton;

// variables here
char date[7] = {0};



#define secondsInDay 86400
#define secondsInHour 3600
#define secondsInMinute 60
#define minutesInHour 60
#define hoursInDays 24
#define daysInYear 365

#define daysTilJanEnd 31
#define daysTilFebEnd 59
#define daysTilMarEnd 90
#define daysTilAprEnd 120

// Temperature Sensor Calibration = Reading at 30 degrees C is stored at addr 1A1Ah
// See end of datasheet for TLV table memory mapping
#define CALADC12_15V_30C  *((unsigned int *)0x1A1A)
// Temperature Sensor Calibration = Reading at 85 degrees C is stored at addr 1A1Ch
#define CALADC12_15V_85C  *((unsigned int *)0x1A1C)
// Resolution for current sensor:  1.0A/4096
// (Here, we add the f suffix so the compiler will know this constant is a float)
#define MA_PER_BIT        (0.0244f)

unsigned int in_temp, in_current;

enum LCD_STATE {monthS = 0, dayS =1, hourS=2, minuteS=3, secondS=4, display = 5};

// Main
void main(void)

{
    //    seconds = 3500000;
//        seconds = 57;
    //        seconds = 82800;

//    seconds = 345600;

    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer. Always need to stop this!!
    __bis_SR_register(GIE); // Global INterrupt enable


    // Useful code starts here
    initLeds();
    configDisplay();
    configKeypad();
    initPushButons();

    enum LCD_STATE state = display;

    // set up temperature sensor *****************************************************
    float temperatureAvg [36] = {0};
    int tempAvgIndex = 0;
    int tempIndex = 0;
    volatile float temperatureDegC;
    volatile float temperatureDegF;
    volatile float milliamps;
    volatile float degC_per_bit;
    volatile unsigned int bits30, bits85;

    REFCTL0 &= ~REFMSTR;    // Reset REFMSTR to hand over control of
                            // internal reference voltages to
                            // ADC12_A control registers
    ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON | ADC12MSC;     // Internal ref = 1.5V

    ADC12CTL1 = ADC12SHP | ADC12CONSEQ_1;                     // Enable sample timer
    // Using ADC12MEM0 to store reading
    ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_0;  // ADC i/p ch A0 = current sense
                                              // ACD12SREF_1 = internal ref = 1.5v
    ADC12MCTL1 = ADC12SREF_1 | ADC12INCH_10 + ADC12EOS; // ADC i/p ch A10 = temp sense
    P6SEL |= BIT0;
    __delay_cycles(100);                    // delay to allow Ref to settle
    ADC12CTL0 |= ADC12ENC;              // Enable conversion
    // Use calibration data stored in info memory
    bits30 = CALADC12_15V_30C;
    bits85 = CALADC12_15V_85C;
    degC_per_bit = ((float)(85.0 - 30.0))/((float)(bits85-bits30));
    // set up temperature sensor *****************************************************


    // *** Intro Screen ***
    Graphics_clearDisplay(&g_sContext); // Clear the display


    runTimerA2();
    startTime = timer_cnt;

    while (1)    // Forever loop
    {
        switch(state){
        case display:
            currButton = readButtons();
            if (currButton == 2){
                state = monthS;
                currButton = NULL;
            }
            currentTime = timer_cnt;
            // check that one second has elapsed
            if(currentTime > startTime + 1){
                // calculate temp of the last 36 seconds since one second has passed
                ADC12CTL0 &= ~ADC12SC;  // clear the start bit
                ADC12CTL0 |= ADC12SC;       // Sampling and conversion start
                                    // Single conversion (single channel)
                // Poll busy bit waiting for conversion to complete
                while (ADC12CTL1 & ADC12BUSY)
                    __no_operation();
                in_current = ADC12MEM0;
                in_temp = ADC12MEM1;      // Read in results if conversion

                // Temperature in Celsius. See the Device Descriptor Table section in the
                // System Resets, Interrupts, and Operating Modes, System Control Module
                // chapter in the device user's guide for background information on the
                // formula.
                temperatureDegC = (float)((long)in_temp - CALADC12_15V_30C) * degC_per_bit +30.0;

//                milliamps = ((float)in_current) * MA_PER_BIT;

                // Temperature in Fahrenheit
                //temperatureDegF = (temperatureDegC * 1.8) + 32;  //conversion formula for celcius to fahrenheit
                __no_operation();

                temperatureAvg[tempAvgIndex%36] = temperatureDegC;
                tempAvgIndex++;
            }
            // check that three seconds have passed
            if(currentTime > startTime + 3){
                startTime = timer_cnt;
                seconds+= 3;

                // calculate average of last 36 readings
                float avgTemp = 0;
                for (tempIndex = 0; tempIndex < sizeof(temperatureAvg) / sizeof(temperatureAvg[0]); tempIndex++){
                    avgTemp += temperatureAvg[tempIndex];
                }
                avgTemp /= 36;

                // Display temp and time
                Graphics_clearDisplay(&g_sContext); // Clear the display
                displayTemp(&avgTemp);
                displayTime(seconds);
                Graphics_flushBuffer(&g_sContext);

            }// end if statement
            break;

        case monthS:
            currButton = readButtons();
            if(currButton == 2){
                state = dayS;
                currButton = NULL;
                // save data

            }

            // update milliamps variable
            getCurrent(&milliamps);
            long unsigned int monthTemp = (int) (milliamps * (11/ 99.9) + 1); // months from 0 to 11
            char monthString[4];

            // Make strings
            // makeDate(days, date);
            memcpy(monthString, getMonth(monthTemp), 4+1);

            // Display Strings
            Graphics_clearDisplay(&g_sContext); // Clear the display
            Graphics_drawStringCentered(&g_sContext, "Edit months", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, monthString, AUTO_STRING_LENGTH, 48, 45, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);

            // update seconds global and return to display
            if (currButton == 1){
                state = display;
                currButton = NULL;
                long unsigned int days, hours, min, sec;
                getTime(&days, &hours, &min, &sec);
                long unsigned int temp;
                getMonthSeconds2(monthTemp, &temp); // 2670400
                seconds = temp + (days) * secondsInDay + hours * secondsInHour + min * secondsInMinute + sec - 3;
            }
            break;


        case dayS:
            currButton = readButtons();
            if(currButton == 2){
                state = hourS;
                currButton = NULL;
            }

            // update milliamps variable
            getCurrent(&milliamps);
            long unsigned int daysTemp = (int) (milliamps * (30/ 99.9) + 1);
            char dayString[3];

            // Make strings
            dayString[0] = ( daysTemp / 10 ) % 10 + '0';
            dayString[1] = daysTemp % 10 + '0';
            dayString[2] = '\0';

            // Display Strings
            Graphics_clearDisplay(&g_sContext); // Clear the display
            Graphics_drawStringCentered(&g_sContext, "Edit days", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, dayString, AUTO_STRING_LENGTH, 48, 45, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);

            // update seconds global and return to display
            if (currButton == 1){
                state = display;
                currButton = NULL;
                long unsigned int days, hours, min, sec;
                getTime(&days, &hours, &min, &sec);
                long unsigned int temp = getMonthSeconds(days);
                temp = (daysTemp - 1) * secondsInDay;
                seconds = getMonthSeconds(days) + (daysTemp - 1) * secondsInDay + hours * secondsInHour + min * secondsInMinute + sec - 3;
            }
            break;


        case hourS:
            currButton = readButtons();
            if(currButton == 2){
                state = minuteS;
                currButton = NULL;
            }

            // update milliamps variable
            getCurrent(&milliamps);
            long unsigned int hoursTemp = (int) (milliamps * (23/ 99.9));
            char hourString[3];

            // Make strings
            hourString[0] = ( hoursTemp / 10 ) % 10 + '0';
            hourString[1] = hoursTemp % 10 + '0';
            hourString[2] = '\0';

            // Display Strings
            Graphics_clearDisplay(&g_sContext); // Clear the display
            Graphics_drawStringCentered(&g_sContext, "Edit hours", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, hourString, AUTO_STRING_LENGTH, 48, 45, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);

            // update seconds global and return to display
            if (currButton == 1){
                state = display;
                currButton = NULL;
                long unsigned int days, hours, min, sec;
                getTime(&days, &hours, &min, &sec);
                seconds = days * secondsInDay + hoursTemp * secondsInHour + min * secondsInMinute + sec - 3;
            }
            break;


        case minuteS:
            currButton = readButtons();
            if(currButton == 2){
                state = secondS;
                currButton = NULL;
            }

            // update milliamps variable
            getCurrent(&milliamps);
            long unsigned int minutesTemp = (int) (milliamps * (59/ 99.9));
            char minuteString[3];

            // Make strings
            minuteString[0] = ( minutesTemp / 10 ) % 10 + '0';
            minuteString[1] = minutesTemp % 10 + '0';
            minuteString[2] = '\0';

            // Display Strings
            Graphics_clearDisplay(&g_sContext); // Clear the display
            Graphics_drawStringCentered(&g_sContext, "Edit minutes", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, minuteString, AUTO_STRING_LENGTH, 48, 45, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);

            // update seconds global and return to display
            if (currButton == 1){
                state = display;
                currButton = NULL;
                long unsigned int days, hours, min, sec;
                getTime(&days, &hours, &min, &sec);
                seconds = days * secondsInDay + hours * secondsInHour + minutesTemp * secondsInMinute + sec - 3;
            }
            break;

        case secondS:
            currButton = readButtons();
            if(currButton == 2){
                state = monthS;
                currButton = NULL;
            }
            // update milliamps variable
            getCurrent(&milliamps);
            long unsigned int secondsTemp = (int) (milliamps * (59/ 99.9));
            char secondString[3];

            // Make strings
            secondString[0] = ( secondsTemp / 10 ) % 10 + '0';
            secondString[1] = secondsTemp % 10 + '0';
            secondString[2] = '\0';

            // Display Strings
            Graphics_clearDisplay(&g_sContext); // Clear the display
            Graphics_drawStringCentered(&g_sContext, "Edit seconds", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, secondString, AUTO_STRING_LENGTH, 48, 45, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);

            // update seconds global and return to display
            if (currButton == 1){
                state = display;
                currButton = NULL;
                long unsigned int days, hours, min, sec;
                getTime(&days, &hours, &min, &sec);
                seconds = days * secondsInDay + hours * secondsInHour + min * secondsInMinute + secondsTemp - 3;
            }
            break;


        } // end of switch



    }  // end while (1)
}   //end main

// updates milliamps current
void getCurrent(float * milliamps){

    ADC12CTL0 &= ~ADC12SC;  // clear the start bit
    ADC12CTL0 |= ADC12SC;       // Sampling and conversion start
                        // Single conversion (single channel)
    // Poll busy bit waiting for conversion to complete
    while (ADC12CTL1 & ADC12BUSY)
        __no_operation();
    in_current = ADC12MEM0;
    in_temp = ADC12MEM1;      // Read in results if conversion

    *milliamps = ((float)in_current) * MA_PER_BIT;

    __no_operation();

//    return current;
}

// returns month string, given a number from 1 to 12
char* getMonth(int monthNumber){
    char monthStr[4];

    if(monthNumber == 1){
        monthStr[0] = 'J';
        monthStr[1] = 'A';
        monthStr[2] = 'N';
    }else if(monthNumber == 2){
        monthStr[0] = 'F';
        monthStr[1] = 'E';
        monthStr[2] = 'B';
    }else if(monthNumber == 3){
        monthStr[0] = 'M';
        monthStr[1] = 'A';
        monthStr[2] = 'R';
    }else if(monthNumber == 4){
        monthStr[0] = 'A';
        monthStr[1] = 'P';
        monthStr[2] = 'R';
    }else if(monthNumber == 5){
        monthStr[0] = 'M';
        monthStr[1] = 'A';
        monthStr[2] = 'Y';
    }else if(monthNumber == 6){
        monthStr[0] = 'J';
        monthStr[1] = 'U';
        monthStr[2] = 'N';
    }else if(monthNumber == 7){
        monthStr[0] = 'J';
        monthStr[1] = 'U';
        monthStr[2] = 'L';
    }else if(monthNumber == 8){
        monthStr[0] = 'A';
        monthStr[1] = 'U';
        monthStr[2] = 'G';
    }else if(monthNumber == 9){
        monthStr[0] = 'S';
        monthStr[1] = 'E';
        monthStr[2] = 'P';
    }else if(monthNumber == 10){
        monthStr[0] = 'O';
        monthStr[1] = 'C';
        monthStr[2] = 'T';
    }else if(monthNumber == 11){
        monthStr[0] = 'N';
        monthStr[1] = 'O';
        monthStr[2] = 'V';
    }else if(monthNumber == 12){
        monthStr[0] = 'D';
        monthStr[1] = 'E';
        monthStr[2] = 'C';
    }
    monthStr[3]='\0';

    return monthStr;
}

// given an amount of days, it returns the amount of seconds up to that month
// i.e. given Feb 6, will return the amount of seconds up to february 1st
long unsigned int getMonthSeconds(long unsigned int days){
    int month = 0; // months from 0 to 11

    if(days <= daysTilJanEnd){
    }else if(days > daysTilJanEnd && days <= daysTilFebEnd){
        month = 1;
        days = days - daysTilJanEnd;
    }else if(days > daysTilFebEnd && days <= daysTilMarEnd){
        month = 2;
        days = days - daysTilFebEnd;
    }else if(days > daysTilMarEnd && days <= daysTilAprEnd){
        month = 3;
        days = days - daysTilMarEnd;
    }else if(days > daysTilAprEnd && days <= 151){
        month = 4;
        days = days - daysTilAprEnd;
    }else if(days > 151 && days <= 181){
        month = 5;
        days = days - 151;
    }else if(days > 181 && days <= 212){
        month = 6;
        days = days - 181;
    }else if(days > 212 && days <= 243){
        month = 7;
        days = days - 212;
    }else if(days > 243 && days <= 273){
        month = 8;
        days = days - 243;
    }else if(days > 273 && days <= 304){
        month = 9;
        days = days - 273;
    }else if(days > 304 && days <= 334){
        month = 10;
        days = days - 304;
    }else if(days > 334 && days <= 365){
        month = 11;
        days = days - 334;
    }else{
        month = 9999;
        days = 0;
    }

    long unsigned int seconds = 0;

    switch(month-1){
    case 11: // dec
        seconds += 31 * (secondsInDay);
    case 10:
        seconds += 30 * (secondsInDay);
    case 9:
        seconds += 31 * (secondsInDay);
    case 8:
        seconds += 30 * (secondsInDay);
    case 7:
        seconds += 31 * (secondsInDay);
    case 6:
        seconds += 31 * (secondsInDay);
    case 5: // jun
        seconds += 30 * (secondsInDay);
    case 4: // may
        seconds += 31 * (secondsInDay);
    case 3:
        seconds += 30 * (secondsInDay);
    case 2:
        seconds += 31 * (secondsInDay);
    case 1:
        seconds += 28 * (secondsInDay);
    case 0:
        seconds += 31 * (secondsInDay);
    default:
        break;

    }// end of switch

    return seconds;
}

// given an amount of days, it returns the amount of seconds up to that month
// i.e. given Feb 6, will return the amount of seconds up to february 1st
void getMonthSeconds2(long unsigned int month, long unsigned int* temp){
    // MONTH IS FROM 0 TO 11

    long unsigned int secondsInMonth = 0;

    switch(month-1){
    case 12: // dec
        secondsInMonth += 31 * (secondsInDay);
    case 11:
        secondsInMonth += 30 * (secondsInDay);
    case 10:
        secondsInMonth += 31 * (secondsInDay);
    case 9:
        secondsInMonth += 30 * (secondsInDay);
    case 8:
        secondsInMonth += 31 * (secondsInDay);
    case 7:
        secondsInMonth += 31 * (secondsInDay);
    case 6: // jun
        secondsInMonth += 30 * (secondsInDay);
    case 5: // may
        secondsInMonth += 31 * (secondsInDay);
    case 4:
        secondsInMonth += 30 * (secondsInDay);
    case 3:
        secondsInMonth += 31 * (secondsInDay);
    case 2:
        secondsInMonth += (long unsigned int) 28 * (long unsigned int) (secondsInDay);
    case 1:
        secondsInMonth += (long unsigned int)31 * (long unsigned int)(secondsInDay); // TODO THIS MULTIPLICATION IS OFF ASK
    default:
        break;

    }// end of switch

    *temp = secondsInMonth;

//    return secondsInMonth;
}



void getTime(long unsigned int* days,long unsigned int* hours, long unsigned int* min, long unsigned int* sec){
    long unsigned int currSeconds;

    // save our current time
    currSeconds = seconds;

    *days = (int)(currSeconds / secondsInDay) % daysInYear;

    *hours = (int)(currSeconds / secondsInHour) % hoursInDays;

    *min = (int)(currSeconds / secondsInMinute) % minutesInHour;

    *sec = (int)(currSeconds % secondsInMinute);

}


void displayTime(long unsigned int seconds){
    long unsigned int days, hours, min, sec, currSeconds;
    char time[9];
    char date[7];

    unsigned int elapsedSec = timer_cnt;
    // save our current time
    currSeconds = seconds;

    days = (int)(currSeconds / secondsInDay) % daysInYear;

    hours = (int)(currSeconds / secondsInHour) % hoursInDays;

    min = (currSeconds / 60);
    min %= 60;

    sec = (int)(currSeconds % secondsInMinute);

    // Make strings
    // makeDate(days, date);
    memcpy(date, makeDate(days, date), 7+1);

    //makeDate(days, date);
    memcpy(time, makeTime(hours, min, sec), 9+1);


    // Display Strings
    Graphics_drawStringCentered(&g_sContext, date, AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, time, AUTO_STRING_LENGTH, 48, 45, TRANSPARENT_TEXT);
//    Graphics_flushBuffer(&g_sContext);

}

void displayTemp(float* inAvgTempC){

    char cString[8];
    char fString[8];

    // get copy of float
    float celciusTemp = *inAvgTempC;

    // Convert celcius to fahrenheit
    float fahrenTemp = 1.8 * (celciusTemp) + 32;

    // Make strings
    // Celsius
    memcpy(cString, makeTemp(&celciusTemp, true), 8+1);

    //Fahrenheit
    memcpy(fString, makeTemp(&fahrenTemp, false), 8+1);


    // Display Strings
    Graphics_drawStringCentered(&g_sContext, cString, AUTO_STRING_LENGTH, 48, 65, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, fString, AUTO_STRING_LENGTH, 48, 75, TRANSPARENT_TEXT);
    //refreshes display
//    Graphics_flushBuffer(&g_sContext);
}

char * makeDate(unsigned int days, char str[]){
    days += 1; // because days start at 0 instead of 1
    char date[7];

    if(days <= daysTilJanEnd){
        date[0] = 'J';
        date[1] = 'A';
        date[2] = 'N';
    }else if(days > daysTilJanEnd && days <= daysTilFebEnd){
        date[0] = 'F';
        date[1] = 'E';
        date[2] = 'B';
        days = days - daysTilJanEnd;
    }else if(days > daysTilFebEnd && days <= daysTilMarEnd){
        date[0] = 'M';
        date[1] = 'A';
        date[2] = 'R';
        days = days - daysTilFebEnd;
    }else if(days > daysTilMarEnd && days <= daysTilAprEnd){
        date[0] = 'A';
        date[1] = 'P';
        date[2] = 'R';
        days = days - daysTilMarEnd;
    }else if(days > daysTilAprEnd && days <= 151){
        date[0] = 'M';
        date[1] = 'A';
        date[2] = 'Y';
        days = days - daysTilAprEnd;
    }else if(days > 151 && days <= 181){
        date[0] = 'J';
        date[1] = 'U';
        date[2] = 'N';
        days = days - 151;
    }else if(days > 181 && days <= 212){
        date[0] = 'J';
        date[1] = 'U';
        date[2] = 'L';
        days = days - 181;
    }else if(days > 212 && days <= 243){
        date[0] = 'A';
        date[1] = 'U';
        date[2] = 'G';
        days = days - 212;
    }else if(days > 243 && days <= 273){
        date[0] = 'S';
        date[1] = 'E';
        date[2] = 'P';
        days = days - 243;
    }else if(days > 273 && days <= 304){
        date[0] = 'O';
        date[1] = 'C';
        date[2] = 'T';
        days = days - 273;
    }else if(days > 304 && days <= 334){
        date[0] = 'N';
        date[1] = 'O';
        date[2] = 'V';
        days = days - 304;
    }else if(days > 334 && days <= 365){
        date[0] = 'D';
        date[1] = 'E';
        date[2] = 'C';
        days = days - 334;
    }else{
        days = 0;
    }
    date[3] = ' ';


    date[4] = (int)(days / 10) % 10 + '0';
    date[5] = (days % 10) + '0'; // plus one because first day is 1 not 0
    date[6] = '\0';

    memcpy(str, date, strlen(date)+1);

    printf("%s",date);

    return date;
}

char * makeTime(long unsigned int hours, long unsigned int min, long unsigned int sec)
{
    char time[9];
    // example: "08:45:00";
    time[0] = (int)(hours / 10) % 10 + '0';
    time[1] = hours % 10 + '0';
    time[2] = ':';
    time[3] = (int)(min / 10) % 10 + '0';
    time[4] = min % 10 + '0';
    time[5] = ':';
    time[6] = (int)(sec / 10) % 10 + '0';
    time[7] = sec % 10 + '0';
    time[8] = '\0';

    return time;
}

char * makeTemp(float* temp, bool isCelsius)
{
    char tempString[8];
    float temperature = *temp;

//    tempString[0] = (int)(temperature / 100.0) % 10 + '0';
    tempString[0] = ' ';
    tempString[1] = (int)(temperature / 10.0) % 10 + '0';
    tempString[2] = (int)temperature % 10 + '0';
    tempString[3] = '.';
    tempString[4] = (int)(temperature * 10.0) % 10 + '0';
    tempString[5] = ' ';
    if(isCelsius == true){
        tempString[6] = 'C';
    }else{
        tempString[6] = 'F';
    }
    tempString[7] = '\0';

    return tempString;
}

void swDelay(char numLoops)
{
    // This function is a software delay. It performs
    // useless loops to waste a bit of time
    //
    // Input: numLoops = number of delay loops to execute
    // Output: none
    //
    // smj, ECE2049, 25 Aug 2013

    volatile unsigned int i,j;  // volatile to prevent removal in optimization
                                // by compiler. Functionally this is useless code

    for (j=0; j<numLoops; j++)
    {
        i = 50000 ;                 // SW Delay
        while (i > 0)               // could also have used while (i)
           i--;
    }
}

void runTimerA2(void){
    TA2CTL = TASSEL_1 + ID_0 + MC_1;
    TA2CCR0 = 32767; // interrupt every 1 second
    TA2CCTL0 = CCIE;
}

void stopTimerA2(int reset){
    TA2CTL = MC_0; // stop timer
    TA2CCTL0 &= ~CCIE; // TA2CCR0 interrupt disabled
    if(reset)
        timer_cnt=0;
}

//------------------------------------------------------------------------------
// Timer2 A2 Interrupt Service Routine
//------------------------------------------------------------------------------
#pragma vector=TIMER2_A0_VECTOR
__interrupt void TIMER_A2_ISR (void)
{
    // Display is using Timer A1
    // Not sure where Timer A1 is configured?
//    Sharp96x96_SendToggleVCOMCommand();  // display needs this toggle < 1 per sec
    timer_cnt++;

}
