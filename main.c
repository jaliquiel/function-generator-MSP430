/************** ECE2049 DEMO CODE ******************/
/**************  13 March 2019   ******************/
/***************************************************/


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
#include <msp430.h>


// Function Prototypes
void swDelay(char numLoops);


// Declare globals here
long unsigned int timer_cnt = 0;
long unsigned int last_cnt = 0;

long unsigned int startTime = 0;
long unsigned int currentTime = 0;
int i;
long unsigned int seconds;

int currButton;

// variables here
char date[7] = {0};


// saw tooth variables
double codes, delta_t, delta_v;
double sum = 0;
unsigned int in_temp;
unsigned int delta_v_input = 0;
float last_float = 0;


// measure state variables
float measure_voltage;

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

unsigned int in_voltage;
float input_voltage;

enum LCD_STATE {display = 0, DC = 1, squareWave = 2, sawtoothWave = 3, triangleWave = 4, measure = 5};

// Main
void main(void)

{

    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer. Always need to stop this!!
    __bis_SR_register(GIE); // Global INterrupt enable


    // Useful code starts here
    initLeds();
    configDisplay();
    configKeypad();
    initButtons();
    initPushButons();
    setupSPI_DAC();
    DACInit();

    enum LCD_STATE state = display;
	
    // set up ADC  *****************************************************
    REFCTL0 &= ~REFMSTR;    // Reset REFMSTR to hand over control of
                            // internal reference voltages to
                            // ADC12_A control registers
    ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON ;     // Internal ref = 1.5V

    ADC12CTL1 = ADC12SHP ;                     // Enable sample timer
    // Using ADC12MEM0 to store reading
    ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_0;  // ADC i/p ch A0 = current sense
                                              // ACD12SREF_1 = internal ref = 1.5v
    P6SEL |= BIT0;
    __delay_cycles(100);                    // delay to allow Ref to settle
    ADC12CTL0 |= ADC12ENC;              // Enable conversion
    // set up ADC  *****************************************************

    unsigned int voltageOut;
    unsigned int dac_code;

    // *** Intro Screen ***
    Graphics_clearDisplay(&g_sContext); // Clear the display


    runTimerA2();
    startTime = timer_cnt;

    while (1)    // Forever loop
    {

        currButton = readButtons();
//        if (currButton == 1){
//            state = DC;
//            Graphics_clearDisplay(&g_sContext); // Clear the display
//            currButton = NULL;
//        }
//        if (currButton == 2){
//            state = squareWave;
//            Graphics_clearDisplay(&g_sContext); // Clear the display
//            currButton = NULL;
//        }
//        if (currButton == 3){
//            state = sawtoothWave;
//            Graphics_clearDisplay(&g_sContext); // Clear the display
//            currButton = NULL;
//        }
//        if (currButton == 4){
//            state = triangleWave;
//            Graphics_clearDisplay(&g_sContext); // Clear the display
//            currButton = NULL;
//        }
        if(currButton == 5)
        {
             state = measure;
             Graphics_clearDisplay(&g_sContext); // Clear the display
             currButton = NULL;
        }

        switch(state){
        case display:

            currButton = readButtons();
            if (currButton == 1){
                state = DC;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
            }
            if (currButton == 2){
                state = squareWave;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
            }
            if (currButton == 3){
                state = sawtoothWave;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
            }
            if (currButton == 4){
                state = triangleWave;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
            }
            if(currButton == 5)
            {
                 state = measure;
                 Graphics_clearDisplay(&g_sContext); // Clear the display
                 currButton = NULL;
            }
            currentTime = timer_cnt;
            // check that one second has elapsed

            // Display Strings
            Graphics_drawStringCentered(&g_sContext, "Welcome", AUTO_STRING_LENGTH, 48, 65, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "Button 1 = DC", AUTO_STRING_LENGTH, 48, 75, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "Button 2 = Square Wave, etc.", AUTO_STRING_LENGTH, 48, 85, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);
            break;

        case DC:
            currButton = readButtons();
            if (currButton == 1){
                state = display;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }
            if (currButton == 2){
                state = squareWave;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }
            if (currButton == 3){
                state = sawtoothWave;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }
            if (currButton == 4){
                state = triangleWave;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }

            Graphics_clearDisplay(&g_sContext); // Clear the display
            Graphics_drawStringCentered(&g_sContext, "DC state", AUTO_STRING_LENGTH, 48, 65, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);

//            updateScroll();
//
//            DACSetValue(in_voltage);

            testLinearity();


            break;

        case squareWave:
            currButton = readButtons();
            if (currButton == 1){
                state = DC;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }
            if (currButton == 2){
                state = display;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }
            if (currButton == 3){
                state = sawtoothWave;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }
            if (currButton == 4){
                state = triangleWave;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }

            Graphics_clearDisplay(&g_sContext); // Clear the display
            Graphics_drawStringCentered(&g_sContext, "Square Wave State", AUTO_STRING_LENGTH, 48, 65, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);

            while(currButton != 2){
                currButton = readButtons();
                updateScroll();
                last_cnt = timer_cnt;
                while(timer_cnt < last_cnt + 50){ // + 5 if MAXCNT 32 ,
                    DACSetValue(0);
                }

                last_cnt = timer_cnt;

                while(timer_cnt < last_cnt + 50){
                    DACSetValue(in_voltage);
                }
            }

            state = display;
            Graphics_clearDisplay(&g_sContext); // Clear the display
            currButton = NULL;
            break;

        case sawtoothWave:
            currButton = readButtons();
            if (currButton == 1){
                state = DC;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }
            if (currButton == 2){
                state = squareWave;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }
            if (currButton == 3){
                state = display;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }
            if (currButton == 4){
                state = triangleWave;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }
            Graphics_clearDisplay(&g_sContext); // Clear the display
            Graphics_drawStringCentered(&g_sContext, "Saw Tooth State", AUTO_STRING_LENGTH, 48, 65, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);

            sum = 0;
            delta_v_input = 0;
            last_float = 0;

            while(currButton != 3){
                currButton = readButtons();

//                updateScroll();
//                codes = in_voltage * ( 4095/ 3.3);
//                delta_t =  0.0001 / (0.02 / codes);
//                delta_v = 3.3 / 4095;

                last_cnt = timer_cnt + 200;

                while(timer_cnt < last_cnt ){
                    DACSetValue(delta_v_input += 32);
                }
                delta_v_input = 0;
            }

            state = display;
            Graphics_clearDisplay(&g_sContext); // Clear the display
            currButton = NULL;

            break;

        case triangleWave:
            currButton = readButtons();


            if (currButton == 1){
                state = DC;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }
            if (currButton == 2){
                state = squareWave;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }
            if (currButton == 3){
                state = sawtoothWave;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }
            if (currButton == 4){
                state = display;
                Graphics_clearDisplay(&g_sContext); // Clear the display
                currButton = NULL;
                break;
            }

            Graphics_clearDisplay(&g_sContext); // Clear the display
            Graphics_drawStringCentered(&g_sContext, "Triangle Wave State", AUTO_STRING_LENGTH, 48, 65, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);


            sum = 0;
            delta_v_input = 0;
            last_float = 0;


            while(currButton != 4){
                currButton = readButtons();

//                updateScroll();
//                codes = in_voltage * ( 4095/ 3.3);
//                delta_t =  0.0001 / (0.02 / codes);
//                delta_v = 3.3 / 4095;

                last_cnt = timer_cnt + 100;

                while(timer_cnt < last_cnt ){
                    DACSetValue(delta_v_input += 62);
                }

//                delta_v_input = 4095;

                last_cnt = timer_cnt + 100;
                while(timer_cnt < last_cnt ){
                    DACSetValue(delta_v_input -= 62);

                }

                delta_v_input = 0;

            }

            state = display;
            Graphics_clearDisplay(&g_sContext); // Clear the display
            currButton = NULL;

            break;

        case measure:

            // set up ADC  *****************************************************
            REFCTL0 &= ~REFMSTR;    // Reset REFMSTR to hand over control of
                                    // internal reference voltages to
                                    // ADC12_A control registers
            ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON ;     // Internal ref = 1.5V

            ADC12CTL1 = ADC12SHP ;                     // Enable sample timer
            // Using ADC12MEM0 to store reading
            ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_0;  // ADC i/p ch A0 = current sense
                                                      // ACD12SREF_1 = internal ref = 1.5v
            P6SEL |= BIT0;
            P6DIR |= BIT0;
            __delay_cycles(100);                    // delay to allow Ref to settle
            ADC12CTL0 |= ADC12ENC;              // Enable conversion

            // set up ADC  *****************************************************


            Graphics_clearDisplay(&g_sContext); // Clear the display
            Graphics_drawStringCentered(&g_sContext, "Measure State", AUTO_STRING_LENGTH, 48, 65, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);


            ADC12CTL0 &= ~ADC12SC;  // clear the start bit
            ADC12CTL0 |= ADC12SC;       // Sampling and conversion start
                                // Single conversion (single channel)
            // Poll busy bit waiting for conversion to complete
            while (ADC12CTL1 & ADC12BUSY)
                __no_operation();
            in_voltage = ADC12MEM0 & 0x0FFF;

            DACSetValue(in_voltage);

            __no_operation();
//            measure_voltage = testLinearity();



            break;

        }// end of switch

    }  // end while (1)
}   //end main




//
// updates milliamps current
void updateScroll(void){

    ADC12CTL0 &= ~ADC12SC;  // clear the start bit
    ADC12CTL0 |= ADC12SC;       // Sampling and conversion start
                        // Single conversion (single channel)
    // Poll busy bit waiting for conversion to complete
    while (ADC12CTL1 & ADC12BUSY)
        __no_operation();
    in_voltage = ADC12MEM0 & 0x0FFF;

    __no_operation();

}

float testLinearity(void){
//    // set up ADC  *****************************************************
//    REFCTL0 &= ~REFMSTR;    // Reset REFMSTR to hand over control of
//                            // internal reference voltages to
//                            // ADC12_A control registers
//    ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON ;     // Internal ref = 1.5V
//
//    ADC12CTL1 = ADC12SHP ;                     // Enable sample timer
//    // Using ADC12MEM0 to store reading
//    ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_1;  // ADC i/p ch A0 = current sense
//                                              // ACD12SREF_1 = internal ref = 1.5v
//    P6SEL |= BIT0;
//    __delay_cycles(100);                    // delay to allow Ref to settle
//    ADC12CTL0 |= ADC12ENC;              // Enable conversion
//    // set up ADC  *****************************************************
//
//    ADC12CTL0 &= ~ADC12SC;  // clear the start bit
//    ADC12CTL0 |= ADC12SC;       // Sampling and conversion start
//                        // Single conversion (single channel)
//    // Poll busy bit waiting for conversion to complete
//    while (ADC12CTL1 & ADC12BUSY)
//        __no_operation();
//    input_voltage = ADC12MEM0 & 0x0FFF;
//
////    __no_operation();
//
//    return input_voltage;
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

	volatile unsigned int i,j;	// volatile to prevent removal in optimization
			                    // by compiler. Functionally this is useless code

	for (j=0; j<numLoops; j++)
    {
    	i = 50000 ;					// SW Delay
   	    while (i > 0)				// could also have used while (i)
	       i--;
    }
}



void runTimerA2(void){
    TA2CTL = TASSEL_1 + ID_0 + MC_1; // SMCLK =
    // 0.001 = maxcnt + 1 * (1/32768)
//    TA2CCR0 = 32; // interrupt every 0.001 seconds
    TA2CCR0 = 2; // interrupt every 0.02/4095 seconds
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
