#include <Adafruit_SoftServo.h> // reference the SoftServo library

// declare variables

volatile uint8_t counter = 0; // global variable used in  SIGNAL(TIMER0_COMPA_vect)  function

unsigned long StartGateTime = 0;
unsigned long StagedTime = 0;
unsigned long RaceStartTime = 0;

Adafruit_SoftServo esc;            	// ESC = Electronic Speed Controller
	
#define SERVOPIN  0                	// Servo control output pin
#define LEDPIN    1                	// LED output pin
#define IRBEAMPIN 2                	// IR beam input pin
	
#define OFFSPEED    0              	// Lowest ESC speed (range 0-180)
#define STAGEDSPEED 18             	// ESC speed when staged before race start
#define RACINGSPEED 180            	// Maximum ESC speed when race start detected
	
#define MIN_STARTGATE_DURATION 5000	// 5 seconds in milliseconds
#define MIN_STAGED_DURATION    5000 // 5 seconds in milliseconds
#define MAX_RACE_DURATION      2000	// 2 seconds in milliseconds

// declare enumeration: a distinct datatype whose value is restricted to a range of explicitly named constants

enum raceState {  
  STANDBYMODE,    // booted and ready to load into starting gate            	- motor off, light off
  STARTGATEMODE,  // in the start gate with motor off (min 5 seconds here)  	- motor off, light on
  STAGEDMODE,     // motor spun up to 10% and waiting 5 seconds             	- motor  10%, light off
  NOTURNBACKMODE, // motor spun up to 10% and ready to enter racing mode    	- motor  10%, light on
  RACINGMODE,     // motor at 100% speed                                    	- motor 100%, light off
  DONEMODE        // Do not allow motor to turn back on after race				- motor off, light on
};

raceState currentState = STANDBYMODE; //declare currentState variable of datatype raceState

void setup() {
	
  // --- ESC (Electronic Speed Controller) ---
		pinMode(SERVOPIN, OUTPUT);  		// Set up ESC control pin as an output
		esc.attach(SERVOPIN);       		// Attach the pin to the software servo library
  // --- LED (Light Emiting Diode) ---
		pinMode(LEDPIN, OUTPUT);  	  		// Set Trinket pin 1 as an output for LED
  // --- IR BEAM (Infrared beam) ---	
		pinMode(IRBEAMPIN, INPUT);     		// Set up IR beam pin as input
		digitalWrite(IRBEAMPIN, HIGH); 		// Enable then internal pull up resistor
  // --- Interupt Registers	---
		OCR0A = 0xAF;			// Set up a 2ms interrupt
		TIMSK |= _BV(OCIE0A); 
		
} // END OF  setup() function
// run the loop every 20ms:
// The SIGNAL(TIMER0_COMPA_vect) function is the interrupt that will be called by the microcontroller every 2ms
SIGNAL(TIMER0_COMPA_vect) {
  counter += 2;                             // Add two milliseconds to the elapsed time
  if (counter >= 20) {                      // Has 20ms elapsed?
    counter = 0;                            // Reset the timer
    esc.refresh();                          // Refesh the software based servo control
  }
} // END OF  SIGNAL(TIMER0_COMPA_vect)  function 

void loop() {
	  // ------------------------ (START) Set race modes ------------------------

	  // Use the IR beam sensor to determine the current state of the race
	  // millis() : function that returns a count of milliseconds since car was powered on 

	  if (digitalRead(IRBEAMPIN) == HIGH) { // START: IR beam is NOT broken
		if (currentState == NOTURNBACKMODE) {  // gate drops while in "no turn back mode"
		  RaceStartTime = millis();            // save racing mode start time
		  currentState = RACINGMODE;           // change to racing mode
		}
		// if car removed before "no turn back mode", reset back to standby
		else if (currentState == STARTGATEMODE || currentState == STAGEDMODE) { 
		  currentState = STANDBYMODE;
		  StartGateTime = 0;
		  StagedTime = 0;
		}
		// if in all other modes, ignore while beam not broken
	  } // END: IR beam is NOT broken
	  
	  else { // START: IR beam IS BROKEN
	  
		if (currentState == STANDBYMODE) {	// if first beam break since power on
		  StartGateTime = millis();       	// save start gate time
		  currentState = STARTGATEMODE;   	// change to start gate mode
		} 
		if (currentState == STARTGATEMODE) {                         // if in start gate mode
		  if ((millis() - StartGateTime) > MIN_STARTGATE_DURATION) { // check time elapsed 
			StagedTime = millis();                                   // save staged mode start time
			currentState = STAGEDMODE;                               // change to staged mode
		  }
		} 
		
		if (currentState == STAGEDMODE) {                     	// if still in staged gate 
		  if ((millis() - StagedTime) > MIN_STAGED_DURATION) {	// check time elapsed 
			currentState = NOTURNBACKMODE;                    	// change to "no turn back" mode
		  }
		} 
		
	  } // END: IR beam IS BROKEN

	  // ------------------------ (END) Set race modes ------------------------
	  
	  
	  // ------------------------ (START) Set what to do in race modes ------------------------

	  // Turn off the motor after the specified race duration
	  if (currentState == RACINGMODE) {                         // If the car is currently racing
		// Measure the race duration and compare it to maximum race duration
		if ((millis() - RaceStartTime) > MAX_RACE_DURATION) {   
		  currentState = DONEMODE;                              // change to done mode (ESC off, LED on)
		}
	  }
	  
	  // dictate what to do in other modes:
	  
	  if (currentState == STANDBYMODE) {
		esc.write(OFFSPEED);          // motor off
		digitalWrite(LEDPIN, LOW);    // LED off
	  }
	  
	  else if (currentState == STARTGATEMODE) {
		esc.write(OFFSPEED);          // motor off
		digitalWrite(LEDPIN, HIGH);   // LED on
	  }
	  
	  else if (currentState == STAGEDMODE) {
		esc.write(STAGEDSPEED);       // motor 10%
		digitalWrite(LEDPIN, LOW);    // LED off
	  }
	  
	  else if (currentState == NOTURNBACKMODE) {
		esc.write(STAGEDSPEED);       // motor 10%
		digitalWrite(LEDPIN, HIGH);   // LED on
	  }
	  
	  else if (currentState == RACINGMODE) {
		esc.write(RACINGSPEED);       // motor 100%
		digitalWrite(LEDPIN, LOW);    // LED off
	  }
	  
	  else { // if anything else, but functionally: if (currentState == DONEMODE) 
		esc.write(OFFSPEED);          // motor off
		digitalWrite(LEDPIN, HIGH);    // LED on
	  }
	  
	  // ------------------------ (END) Set what to do in race modes ------------------------
	  
	  delay(1); // delay 1 ms before loop starts again 

} // END OF  loop() function
