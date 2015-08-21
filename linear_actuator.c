/*
This file was written to test the Firgelli linear actuator and LAC.

Once the user button is released, the linear actuator is extended a specified amount
in order to create a droplet.
*/


#include "NU32.h"          // config bits, constants, funcs for startup and UART



// 1 kHz PWM - Timer3 - OC1 (D0)
void __ISR(_EXTERNAL_2_VECTOR, IPL2SOFT) Ext2ISR(void) { // step 1: the ISR

	// Wait until USER button is released, then wait 1M core ticks before clearing interrupt flag
	while(!PORTDbits.RD13) {		// Pin D13 is the USER switch, low if pressed.
		;
	}

	int counter=0;
	while(counter<1000000) {
		counter++;
	}

	// Set PWM duty cycle OC1RS from 0 - 20,000 in increments of 250 (0.2/50 mm)
	OC1RS = OC1RS + 250;

	if(OC1RS > 20000) {
		OC1RS = 20000;
	}

	// clear interrupt flag IFS0<3>
	IFS0CLR = 1 << 11;
}



int main(void) {

  NU32_Startup();         				// cache on, min flash wait, interrupts on, LED/button init, UART init

  __builtin_disable_interrupts(); // step 2: disable interrupts
  INTCONCLR = 0x1;                // step 3: INT0 triggers on falling edge
  IPC2CLR = 0x1F << 24;           // step 4: clear the 5 pri and subpri bits
  IPC2 |= 9 << 24;                // step 4: set priority to 2, subpriority to 1
  IFS0bits.INT2IF = 0;            // step 5: clear the int flag, or IFS0CLR=1<<3
  IEC0SET = 1 << 11;              // step 6: enable INT0 by setting IEC0<3>
  __builtin_enable_interrupts();  // step 7: enable interrupts


  OC1CONbits.OCTSEL = 1;  			 		// Select Timer3 for comparison

  T3CONbits.TCKPS = 2;    					// Timer3 prescaler N=2 (1:4)
  PR3 = 19999;              				// period = (PR3+1) * N * 12.5 ns = 1 kHz
  TMR3 = 0;                					// initial TMR3 count is 0

  OC1CONbits.OCM = 0b110; 					// PWM mode with no fault pin; other OC1CON bits are defaults
  OC1RS = 0;           							// duty cycle = OC1RS/(PR3+1) = 75%
  OC1R = 0;             						// initialize before turning OC1 on; afterward it is read-only

  T3CONbits.ON = 1;        					// turn on Timer3
  OC1CONbits.ON = 1;       					// turn on OC1



  while(1) {
		;
	}

  return 0;
}
