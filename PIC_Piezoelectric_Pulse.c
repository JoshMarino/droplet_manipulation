//#define NU32_STANDALONE  // uncomment if program is standalone, not bootloaded
#include "NU32.h"          // config bits, constants, funcs for startup and UART


#define PULSETIME 1200				// microseconds - Piezoelectric disk

#define NUMIMAGES 1000
#define FPS 1750
#define DELAYFRAMES 0



// Droplet created 5M core ticks after user button has been released






// Exsync Trigger: LOW trigger tells camera to record one frame 
//
// CPU running at 40 MHz
// 
// LOW:  -0.3 V TO 0.8 V {0.0 V}
// HIGH:  2.0 V to 5.5 V {3.3 V}


// Piezoelectric waveform: -Vcc changes to +Vcc for T microseconds, then back to -Vcc
//
// PWMA = 20 kHz PWM - Timer3 - OC1 (D0)
// L1 = Digital Output RA10
// L2 = Digital Output RA2
// VM = 24V / 600mA
//
// CPU running at 40 MHz
//
// FWD-SPD: L1 LOW, L2 HIGH
// BCK-SPD: L1 HIGH, L2 LOW
// 
// LOW: -0.3 V TO 1.5 V {0.0 V}
// HIGH: 2.3 V TO 7.0 V {3.3 V}






// =============================================================================================
// If USER button has been pressed, start the procedure of recording droplet bouncing
// =============================================================================================
void __ISR(_EXTERNAL_2_VECTOR, IPL2SOFT) Ext2ISR(void) { // step 1: the ISR

	// Wait until USER button is released, then wait 5M core ticks before clearing interrupt flag
	while(!PORTDbits.RD13) {		// Pin D13 is the USER switch, low if pressed.
		;
	}

	int counter=0;
	while(counter<5000000) {
		counter++;
	}

/*
	// -------------------------------------------------------------------------------------------
	// Create ExSync Trigger to record NUMIMAGES frames at FPS frames per second
	// -------------------------------------------------------------------------------------------

	// Initialization of local variables
	int counter1, counter2;
	int current_frame = 0;
	int T1 = (40000000)/(FPS*1.0);			// time between triggers

	// Create ExSycn Trigger by setting digital output pin
	while( current_frame < (NUMIMAGES) ) {

		// Set A3 to LOW
		LATAbits.LATA3 = 0;

		// Remain LOW for 100 clock cycles
		counter1 = 0;
		while( counter1 < 100 ) {
			counter1++;
		}

		// Set A3 to HIGH
		LATAbits.LATA3 = 1;

		// Remain HIGH until next Trigger
		if( current_frame != DELAYFRAMES) {
			counter2 = 0;
			while( counter2 < (T1-100) ) {
				counter2++;
			}
		}

		// Delay W frames for phase relationship of droplet contacting bath before creating droplet
		else {
			// -----------------------------------------------------------------------------------------
			// Create droplet by sending square wave to piezoelectric buzzer for T microseconds
			// -----------------------------------------------------------------------------------------

			//+Vcc: set L1 to LOW, L2 to HIGH, PWMA to >0
			OC1RS = 4000;
			LATAbits.LATA10 = 0;
			LATAbits.LATA2 = 1;

			//Remain +Vcc for PULSETIME microseconds
			int counter3=0;
			while(counter3<(PULSETIME*40)) {     //24,000 clock cycles
				counter3++;
			}

			//-Vcc: set L1 to HIGH, L2 to LOW, PWMA to >0
			OC1RS = 4000;
			LATAbits.LATA10 = 1;
			LATAbits.LATA2 = 0;

			// Continue rest of delay until next Trigger
			counter2 = 0;
			while( counter2 < (T1-100-(PULSETIME*40)) ) {
				counter2++;
			}
		}

		// Increment current frame
		current_frame++;
	}
*/


	//+Vcc: set L1 to LOW, L2 to HIGH, PWMA to >0
	OC1RS = 4000;
	LATAbits.LATA10 = 0;
	LATAbits.LATA2 = 1;

	//Remain +Vcc for T microseconds
	int counter3=0;
	int T = 1450;
	while(counter3<(T*40)) {
		counter3++;
	}

	//-Vcc: set L1 to HIGH, L2 to LOW, PWMA to >0
	OC1RS = 4000;
	LATAbits.LATA10 = 1;
	LATAbits.LATA2 = 0;



	// Clear interrupt flag
	IFS0CLR = 1 << 11;
}




int main(void) {

	// Cache on, min flash wait, interrupts on, LED/button init, UART init
  NU32_Startup();


	// =============================================================================================
	// USER Button Interrupt
	// =============================================================================================
  __builtin_disable_interrupts(); 	// step 2: disable interrupts
  INTCONCLR = 0x1;               	 	// step 3: INT0 triggers on falling edge
  IPC2CLR = 0x1F << 24;           	// step 4: clear the 5 pri and subpri bits
  IPC2 |= 9 << 24;               	 	// step 4: set priority to 2, subpriority to 1
  IFS0bits.INT2IF = 0;           	 	// step 5: clear the int flag, or IFS0CLR=1<<3
  IEC0SET = 1 << 11;              	// step 6: enable INT0 by setting IEC0<3>
  __builtin_enable_interrupts();  	// step 7: enable interrupts


	// =============================================================================================
	// PWM and digital output for piezoelectric droplet generator
	// =============================================================================================
  OC1CONbits.OCTSEL = 1;  			 		// Select Timer3 for comparison

  T3CONbits.TCKPS = 0;     					// Timer3 prescaler N=1 (1:1)
  PR3 = 3999;              					// period = (PR3+1) * N * 12.5 ns = 20 kHz
  TMR3 = 0;                					// initial TMR3 count is 0

  OC1CONbits.OCM = 0b110; 					// PWM mode with no fault pin; other OC1CON bits are defaults
  OC1RS = 0;           							// duty cycle = OC1RS/(PR3+1) = 75%
  OC1R = 0;             						// initialize before turning OC1 on; afterward it is read-only

  T3CONbits.ON = 1;        					// turn on Timer3
  OC1CONbits.ON = 1;       					// turn on OC1


	// Set A10/A2 to digital output pins
	TRISAbits.TRISA10 = 0;						// RA10 is an output pin
	TRISAbits.TRISA2 = 0;							// RA2 is an output pin

	//-Vcc: set L1 to HIGH, L2 to LOW, PWMA to >0
	OC1RS = 4000;
	LATAbits.LATA10 = 1;
	LATAbits.LATA2 = 0;

/*
	// =============================================================================================
	// TrackCam ExSync Trigger
	// =============================================================================================

	// Set A3 to digital output pin
	TRISAbits.TRISA3 = 0;						// RA3 is an output pin

	//Set A3 to HIGH
	LATAbits.LATA3 = 1;
*/


	// =============================================================================================
	// Keep program running to look for interrupts
	// =============================================================================================
  while(1) {
		;
	}

  return 0;
}
