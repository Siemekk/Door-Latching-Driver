#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// Analog pins, for motor driver
#define PIN_MOSFET_ON 14
#define PIN_MOTOR_R   15
#define PIN_MOTOR_L   16

// GPIO PINS, from CAR and lock
#define PIN_T3_1 17
#define PIN_T3_2 19
#define PIN_T8_4 18

// MOTOR STATUS variables
const int STOP = 0;
const int FORWARD = 1;
const int REVERSE = 2;

// Time step for all process
const int checkDoorsClose_step  = 5;
const int doorLatching_step     = 12;

// Timer variables
unsigned long currentTime           = 0;
unsigned long savedTime_main        = 0;
unsigned long savedTime_checkSleep  = 0;
unsigned long checkSleepTime        = 7500UL;

// Variables with limit switches values
int status_PIN_3_1; 
int status_PIN_3_2;
int status_PIN_8_4;

// Counters, and fixes - for example, if user close doors fast, latching is not necessary.
// Other examples, on latching process (75% closed, switches return's 0, so it's necessary to chceck it with some delay)
int doorLatchingCounter;
int counterCheckDoorsClose;
bool emergencyLockProblem; 
bool lastChanceBeforeSleep;
void setup() 
{
  // Define motor driver as OUTPUT
  pinMode(PIN_MOSFET_ON, OUTPUT);
  pinMode(PIN_MOTOR_L, OUTPUT);
  pinMode(PIN_MOTOR_R, OUTPUT);
  
  // Define input pins
  pinMode(PIN_T3_1, INPUT);
  pinMode(PIN_T3_2, INPUT);
  pinMode(PIN_T8_4, INPUT);

  // Init Counters
  counterCheckDoorsClose = checkDoorsClose_step;
  doorLatchingCounter     = 0;
  emergencyLockProblem    = false;
  lastChanceBeforeSleep   = false;
}

// Like as function name
void readLimitSwitches()
{
  status_PIN_3_1 = digitalRead(PIN_T3_1);
  status_PIN_3_2 = digitalRead(PIN_T3_2);
  status_PIN_8_4 = digitalRead(PIN_T8_4);
}

// This function is used to run and stop motor
void setMotorState(int state)
{
  if (state == STOP)
  {
    analogWrite(PIN_MOSFET_ON, 0);
    analogWrite(PIN_MOTOR_L, 0);
    analogWrite(PIN_MOTOR_R, 0); 
    return; 
  }

  analogWrite(PIN_MOSFET_ON, 255);
  if(state == FORWARD)
  {
    analogWrite(PIN_MOTOR_L, 255);
    analogWrite(PIN_MOTOR_R, 0);  
  }
  else
  {
    analogWrite(PIN_MOTOR_L, 0);
    analogWrite(PIN_MOTOR_R, 255); 
  }
}
// This function setting module to sleep mode, it's very important, because module w/o sleep consumpt 12mA, so it ~50mA in all 4 doors!
// With this function module consumt 0.8mA at sleep mode
void sleepModule()
{
    static byte prevADCSRA = ADCSRA;
    ADCSRA = 0;
    set_sleep_mode (SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    MCUCR = bit (BODS) | bit (BODSE);
    MCUCR = bit (BODS);
    
    noInterrupts();
    attachInterrupt(0, wakeUp, CHANGE);

    // Allow interrupts now
    interrupts();

    // And enter sleep mode as set above
    sleep_cpu();

    // Re-enable ADC if it was previously running
    ADCSRA = prevADCSRA;
}

// This function is called, when module going to wakeUP
void wakeUp()
{
  // Reinit Counters
  doorLatchingCounter = 0;
  counterCheckDoorsClose = checkDoorsClose_step;
  lastChanceBeforeSleep = false;
  detachInterrupt(0);
  
  delay(10);
}

// This function calls sleep
void doCheckSleep()
{
   if(analogRead(PIN_MOSFET_ON) > 0) 
   {
    // NEVER go sleep when, any motor is ON!
    return;
   }
   
   readLimitSwitches(); 
   if((status_PIN_3_1 == HIGH) && (status_PIN_8_4 == LOW))
   {
      if(!lastChanceBeforeSleep)
      { 
        lastChanceBeforeSleep = true;
        return;
      }
   }
   
   lastChanceBeforeSleep = false; 
   setMotorState  (STOP); // Stop motors!
   sleepModule    ();     // Go sleep!
}

// Main process for motor forward STATE
void processLatching()
{
  if(!doorLatchingCounter) doorLatchingCounter = doorLatching_step;
  setMotorState(FORWARD); 
  doorLatchingCounter--;
  if(doorLatchingCounter <= 0)
  {
    if(status_PIN_8_4 == LOW)
    {
      // Some troubles with latching - setting to emergency mode
      emergencyLockProblem = true;
    } 
  }
}

// POST method for reset counters, and reverse motor!
void processReverse()
{
   if(status_PIN_3_2 == LOW)
        setMotorState(REVERSE);
   else setMotorState(STOP);

   // Reset counters
   doorLatchingCounter        = 0;
   counterCheckDoorsClose  = checkDoorsClose_step;
}

// Main process for door
void processDoor()
{
  readLimitSwitches();

  if(status_PIN_3_1 == LOW && !doorLatchingCounter)
  { 
    processReverse();
    emergencyLockProblem    = false;
  }
  else
  {
    if((status_PIN_8_4 == LOW || doorLatchingCounter) && !emergencyLockProblem)
    {  
      if(counterCheckDoorsClose > 0)  counterCheckDoorsClose--;
      else                            processLatching();
    }
    else
      processReverse();
  }
}
// the loop function runs over and over again forever
void loop() 
{
  currentTime       = millis();
  if(currentTime -  savedTime_checkSleep >= checkSleepTime)  
  {
    savedTime_checkSleep = currentTime;
    doCheckSleep();
  } 
  
  if(currentTime - savedTime_main >= 50UL)
  {
    savedTime_main = currentTime; 
    processDoor();
  } 
}
