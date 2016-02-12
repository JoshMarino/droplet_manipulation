//#define NU32_STANDALONE		// uncomment if program is standalone, not bootloaded
#include "NU32.h"						// config bits, constants, funcs for startup and UART
#include "LCD.h"
#include <math.h>						// math library: sine, sqrt, PI


#define MAX_MESSAGE_LENGTH 200


// Capture a decimation video
//
// Will record one frame per PPOD reference signal
// Droplet created at first reference signal after receiving serial command from master
// - Serial message: [NUMIMAGES, FPS, PULSETIME, DELAYTIME]
// - FPS & DELATIME not currently used for DelayFrames




// Synchronization with PPOD controller
//
// CN17 - RF4 - 5V Tolerant


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







// ***********************************************************************************************
// Variables to be accessed from multiple functions
// ***********************************************************************************************

// Hold the port F read
unsigned int oldF = 0, newF = 0;

// Create Record flag and turn off
unsigned char RecordFlag = 0;

// Properties set by master
int NUMIMAGES, FPS, PULSETIME;
float DELAYTIME;

// Determine how many frames to delay in order for droplet to contact bath at fixed phase
int DelayFrames = 0;

// Counter for number of PPOD reference signals received since pressed USER button
int RefCounter = 0;




// ***********************************************************************************************
// ISR using Change Notification for synchronization with PPOD controller
// ***********************************************************************************************
void __ISR(_CHANGE_NOTICE_VECTOR, IPL3SOFT) CNISR(void) { // INT step 1
  
	// Since pins on port F are being monitored by CN, must read it to allow continued functioning
  newF = PORTF;

	// Save the current values for future use
  oldF = newF;


	// =============================================================================================
	// If RecordFlag is on, start the procedure of recording droplet bouncing
	// =============================================================================================
	if (RecordFlag == 1) {

		// -------------------------------------------------------------------------------------------
		// Create ExSync Trigger to record one frame per PPOD reference signal
		// -------------------------------------------------------------------------------------------

		// Initialization of local variables
		int counter2;

		// Set A3 to LOW
		LATAbits.LATA3 = 0;

		// Remain LOW for 100 clock cycles
		counter2 = 0;
		while( counter2 < 100 ) {
			counter1++;
		}

		// Remain HIGH until next Trigger
		LATAbits.LATA3 = 1;


		// -------------------------------------------------------------------------------------------
		// If first reference signal, create droplet
		// -------------------------------------------------------------------------------------------
		if (RefCounter == 0) {

				// ---------------------------------------------------------------------------------------
				// Create droplet by sending square wave to piezoelectric buzzer for T microseconds
				// ---------------------------------------------------------------------------------------

				//+Vcc: set L1 to LOW, L2 to HIGH, PWMA to >0
				OC1RS = 4000;
				LATAbits.LATA10 = 0;
				LATAbits.LATA2 = 1;

				//Remain +Vcc for PULSETIME microseconds
				int counter1=0;
				while(counter1<(PULSETIME*40)) {
					counter1++;
				}

				//-Vcc: set L1 to HIGH, L2 to LOW, PWMA to >0
				OC1RS = 4000;
				LATAbits.LATA10 = 1;
				LATAbits.LATA2 = 0;

		}


		// -------------------------------------------------------------------------------------------
		// Increment reference counter
		// -------------------------------------------------------------------------------------------
		RefCounter++;


		// -------------------------------------------------------------------------------------------
		// If last reference counter, stop recording procedure
		// -------------------------------------------------------------------------------------------
		if (RefCounter == NUMIMAGES) {

			// Turn RecordFlag flag OFF
			RecordFlag = 0;

			// Reset RefCounter back to 0
			RefCounter = 0;

		}
	}


	// Clear the interrupt flag
	IFS1CLR = 1;
}





int main(void) {

	// Cache on, min flash wait, interrupts on, LED/button init, UART init
  NU32_Startup();
  LCD_Setup();

  char message[MAX_MESSAGE_LENGTH];


	// =============================================================================================
	// Change Notification Digital Input Interrupt from PPOD
	// =============================================================================================
  CNPUEbits.CNPUE17 = 0;  // CN17/RF4 input has no internal pull-up

  oldF = PORTF;           // all pins of port F are inputs, by default

  __builtin_disable_interrupts(); // step 1: disable interrupts
  CNCONbits.ON = 1;               // step 2: configure peripheral: turn on CN
  CNENbits.CNEN17 = 1; 						//         listen to CN17/RF4
  IPC6bits.CNIP = 3;              // step 3: set interrupt priority
  IPC6bits.CNIS = 2;              // step 4: set interrupt subpriority
  IFS1bits.CNIF = 0;              // step 5: clear the interrupt flag
  IEC1bits.CNIE = 1;              // step 6: enable the _CN interrupt
  __builtin_enable_interrupts();  // step 7: CPU enabled for mvec interrupts



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



	// =============================================================================================
	// TrackCam ExSync Trigger
	// =============================================================================================

	// Set A3 to digital output pin
	TRISAbits.TRISA3 = 0;						// RA3 is an output pin

	//Set A3 to HIGH
	LATAbits.LATA3 = 1;



	// =============================================================================================
	// Keep program running to look for command from master to start record procedure
	// =============================================================================================
  while(1) {
		// Get message from computer
    NU32_ReadUART1(message, MAX_MESSAGE_LENGTH);

		//Serial message: [NUMIMAGES, FPS, PULSETIME, DELAYTIME]
		sscanf(message, "%d%*c %d%*c %d%*c %f", &NUMIMAGES, &FPS, &PULSETIME, &DELAYTIME);		//%*c reads in comma and ignores it

		// Convert DELAYTIME to DelayFrames
		DelayFrames = DELAYTIME*FPS;

		// Turn RecordFlag ON since we have received from master
		RecordFlag = 1;

		// Display properties on LCD
    LCD_Clear();
    LCD_Move(0,0);
		sprintf(message, "%d, %d", NUMIMAGES, FPS);
    LCD_WriteString(message);                     	

		sprintf(message, "%d, %f", PULSETIME, DELAYTIME);
    LCD_Move(1,0);
    LCD_WriteString(message);
  }

  return 0;
}
