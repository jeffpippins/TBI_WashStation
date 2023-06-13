//TBAILEY INC WHEELABRATOR WASHSTATION CONTROL CODE
//Wash Station Operates in two modes. Automatic and Manual.

//In Automatic Mode The Start Signal is Tripped From The Photo Eye Input
//When the Photo Eye Then Fails To Trip The Stop Timer is Started defined by AUTOSTOPDELAY

//In Manual Mode The Start and Stop are Triggered From Button Inputs START and STOP
//There is a watchdog failsafe that turns off the system after MANUALTIMEOUT seconds.

//All Button Inputs are active LOW. Pullup resistors are enabled on all GPI Pins.
//The Relay Outputs are Active High.

//The Cycle Sequence Is As Follows

//1. CYCLE_STOP - System is Idle
//2  CYCLE_VALVE_OPENING Set VALVEOPENCONTROLPIN HIGH. Delay for VAVLEOPENTIME second before proceeding.
//3  CYCLE_NORMAL_OPERATION Set VFDCONTROLPIN HIGH Until a Stop Signal Happens.
//4  CYCLE_VALVE_CLOSING Set
//4  If OPERATION_AUTOMATIC then CYCLE_TIMED_STOP and wait for AUTOSTOPDELAY seconds before powering down unit.

#include <Arduino.h>
//#define DEBUG

//Pin Definitions
//-----------------------------------------------------------------------------------------------------
#define STARTPIN 2
#define STOPPIN 3
#define OPERATIONMODEPIN 4
#define ESTOPPIN 5
#define PHOTOEYEPIN 6

#define VALVEOPENCONTROLPIN A3 // The Pin Controls a Active LOW Relay Module.
#define VALVECLOSECONTROLPIN A4
#define VFDCONTROLPIN A5 //The Pin Controls a Active LOW Relay Module
//-----------------------------------------------------------------------------------------------------


//Process Timings
//-----------------------------------------------------------------------------------------------------
#define VAVLEOPENTIME 5000 //milli seconds
#define AUTOSTOPDELAY 12000 //millisecond seconds
#define WATCHDOGTIMEOUT 7200000 //milliseconds 7200000 = 2 hours = 120 min = 7200 sec
//-----------------------------------------------------------------------------------------------------


//Pin State Definitions To Describe What Active States Go To What Pin Levels
//For Comparisons and Clarity
//-----------------------------------------------------------------------------------------------------
#define RELAY_ON_PIN_STATE HIGH
#define RELAY_OFF_PIN_STATE LOW

#define ESTOP_TRIGGERED_PIN_STATE HIGH
#define ESTOP_NOT_TRIGGERED_PIN_STATE LOW

#define AUTOMATIC_OPERATION_MODE_PIN_STATE HIGH

#define START_BUTTON_TRIGGERED_PIN_STATE HIGH
#define STOP_BUTTON_TRIGGERED_PIN_STATE HIGH
#define PHOTOEYE_TRIGGERED_PIN_STATE HIGH

//State Machine Processsing Functions
//-----------------------------------------------------------------------------------------------------
void updateButtons();
void processStateMachine();
void processAllStop();
void pollOperationMode();

#ifdef DEBUG
void printDebugInfo();
unsigned long debugtimer;
#endif
//-----------------------------------------------------------------------------------------------------


//State Machine Enumerated Types
//-----------------------------------------------------------------------------------------------------
enum State_t {STATE_STOPPED, STATE_CYCLING};
enum OperationCycle_t {CYCLE_STOP, CYCLE_VALVE_OPENING, CYCLE_NORMAL_OPERATION, CYCLE_TIMED_STOP};
enum OperationModes_t {OPERATION_AUTOMATIC, OPERATION_MANUAL};
//-----------------------------------------------------------------------------------------------------


//State Machine Global Variables
//-----------------------------------------------------------------------------------------------------
State_t systemstate;
OperationCycle_t cycle;
OperationModes_t operationmode;
//-----------------------------------------------------------------------------------------------------


//Timer Globals
//-----------------------------------------------------------------------------------------------------
unsigned long valveopentimer;
unsigned long automodestoptimer;
unsigned long watchdogtimer;
//-----------------------------------------------------------------------------------------------------



//Initial MicroController Setup.
//-----------------------------------------------------------------------------------------------------

void setup()
{
#ifdef DEBUG
  Serial.begin(9600);
#endif

  delay(1000); //Some MicroControllers need time to reboot

  //Setup Pins
  pinMode(STARTPIN, INPUT);
  pinMode(STOPPIN, INPUT);
  pinMode(ESTOPPIN, INPUT);
  pinMode(OPERATIONMODEPIN, INPUT);
  pinMode(PHOTOEYEPIN, INPUT);

  pinMode(VALVEOPENCONTROLPIN, OUTPUT);
  pinMode(VALVECLOSECONTROLPIN, OUTPUT);
  pinMode(VFDCONTROLPIN, OUTPUT);
  digitalWrite(VALVEOPENCONTROLPIN, RELAY_OFF_PIN_STATE);
  digitalWrite(VALVECLOSECONTROLPIN, RELAY_ON_PIN_STATE);
  digitalWrite(VFDCONTROLPIN, RELAY_OFF_PIN_STATE);


  //Set State Machine Values
  systemstate = STATE_STOPPED;
  cycle = CYCLE_STOP;

  pollOperationMode();

#ifdef DEBUG
  debugtimer = millis();
#endif
}
//-----------------------------------------------------------------------------------------------------
//Main Program Loop
void loop()
{
#ifdef DEBUG
  if ((millis() - debugtimer) > 5000)
  {
    Serial.println(" ");
    Serial.println(" ");
    Serial.println(" ");
    Serial.println("Main Loop Debug Info");
    if (digitalRead(ESTOPPIN) == ESTOP_TRIGGERED_PIN_STATE)
    {
      Serial.println("System in ESTOP");
    }
    printDebugInfo();
    debugtimer = millis();
  }
#endif

  if (digitalRead(ESTOPPIN) == ESTOP_NOT_TRIGGERED_PIN_STATE) //ESTOP IS NOT TRIPPED CONTINUE LOOP
  {
    //Process The Buttons For System Handling
    updateButtons();

    //Operation Mode Can Only Be Updated When The System is Stopped
    if (systemstate == STATE_STOPPED)
    {
      pollOperationMode();
    }

    //Process the State Machine If The System is Cycling.
    if (systemstate == STATE_CYCLING)
    {
      processStateMachine(); //Processes The State of the Machine
    }
  }
  else //ESTOP IS TRIPPED CONTINUE TO MAKE SURE ALL SIGNALS ARE OFF
  {
    processAllStop();
  }
}


#ifdef DEBUG
//State Machine Processing Functions ------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
void printDebugInfo()
{
  /*
    enum State_t {STATE_STOPPED, STATE_CYCLING};
    enum OperationCycle_t {CYCLE_STOP, CYCLE_VALVE_OPENING, CYCLE_NORMAL_OPERATION, CYCLE_TIMED_STOP};
    enum OperationModes_t {OPERATION_AUTOMATIC, OPERATION_MANUAL};
  */

  Serial.println("-------------------------------");
  Serial.println("STATE MACHINE STATE:");
  switch (operationmode)
  {
    case OPERATION_AUTOMATIC:
      Serial.println("operationmode = OPERATION_AUTOMATIC");
      break;
    case OPERATION_MANUAL:
      Serial.println("operationmode = OPERATION_MANUAL");
      break;
  }
  switch (systemstate)
  {
    case STATE_STOPPED:
      Serial.println("systemstate = STATE_STOPPED");
      break;
    case STATE_CYCLING:
      Serial.println("systemstate = STATE_CYCLING");
      break;
  }
  switch (cycle)
  {
    case CYCLE_STOP:
      Serial.println("cycle = CYCLE_STOP");
      break;
    case CYCLE_VALVE_OPENING:
      Serial.println("cycle = CYCLE_VALVE_OPENING");
      Serial.print(VAVLEOPENTIME - (millis() - valveopentimer));
      Serial.println(" milliseconds left");

      break;
    case CYCLE_NORMAL_OPERATION:
      Serial.println("cycle = CYCLE_NORMAL_OPERATION");
      break;
    case CYCLE_TIMED_STOP:
      Serial.println("cycle = CYCLE_TIMED_STOP");
      break;
  }
  Serial.println("-------------------------------");
  Serial.println("Output Signal States.");
  switch (digitalRead(VALVEOPENCONTROLPIN))
  {
    case RELAY_ON_PIN_STATE:
      Serial.println("VALVEOPENCONTROLPIN Relay is ON");
      break;
    case RELAY_OFF_PIN_STATE:
      Serial.println("VALVEOPENCONTROLPIN Relay is OFF");
      break;
  }
  switch (digitalRead(VALVECLOSECONTROLPIN))
  {
    case RELAY_ON_PIN_STATE:
      Serial.println("VALVEOPENCONTROLPIN Relay is ON");
      break;
    case RELAY_OFF_PIN_STATE:
      Serial.println("VALVEOPENCONTROLPIN Relay is OFF");
      break;
  }
  switch (digitalRead(VFDCONTROLPIN))
  {
    case RELAY_ON_PIN_STATE:
      Serial.println("VFDCONTROLPIN Relay is ON");
      break;
    case RELAY_OFF_PIN_STATE:
      Serial.println("VFDCONTROLPIN Relay is OFF");
      break;
  }
  Serial.println("-------------------------------");
}
#endif

//State Machine Processing Functions ------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
void processStateMachine()
{
  //Master WatchDog Timer
  if ((millis() - watchdogtimer) > WATCHDOGTIMEOUT)
  {
    cycle = CYCLE_STOP;
  }

  //Cycle Processing.
  switch (cycle)
  {
    case CYCLE_VALVE_OPENING:
      digitalWrite(VALVECLOSECONTROLPIN, RELAY_OFF_PIN_STATE);
      digitalWrite(VALVEOPENCONTROLPIN, RELAY_ON_PIN_STATE);
      digitalWrite(VFDCONTROLPIN, RELAY_OFF_PIN_STATE);
      if ((millis() - valveopentimer) > VAVLEOPENTIME)
      {
        cycle = CYCLE_NORMAL_OPERATION;
#ifdef DEBUG
        printDebugInfo();
#endif
      }
      break;
    case CYCLE_NORMAL_OPERATION:
      digitalWrite(VALVECLOSECONTROLPIN, RELAY_OFF_PIN_STATE);
      digitalWrite(VALVEOPENCONTROLPIN, RELAY_ON_PIN_STATE);
      digitalWrite(VFDCONTROLPIN, RELAY_ON_PIN_STATE);
      break;
    case CYCLE_TIMED_STOP:
      digitalWrite(VALVECLOSECONTROLPIN, RELAY_OFF_PIN_STATE);
      digitalWrite(VALVEOPENCONTROLPIN, RELAY_ON_PIN_STATE);
      digitalWrite(VFDCONTROLPIN, RELAY_ON_PIN_STATE);
      if ((millis() - automodestoptimer) > AUTOSTOPDELAY)
      {
        cycle = CYCLE_STOP;
#ifdef DEBUG
        printDebugInfo();
#endif
      }
      break;
    case CYCLE_STOP:
      processAllStop();
      break;
  }
}
//-----------------------------------------------------------------------------------------------------
void updateButtons()
{
  if (digitalRead(STARTPIN) == START_BUTTON_TRIGGERED_PIN_STATE)
  {
    if (operationmode == OPERATION_MANUAL && systemstate == STATE_STOPPED)
    {
      systemstate = STATE_CYCLING;
      cycle = CYCLE_VALVE_OPENING;
      valveopentimer = millis();
      watchdogtimer = millis();
#ifdef DEBUG
      Serial.println(" ");
      Serial.println(" ");
      Serial.println(" ");
      Serial.println("Start Button On Released Fired");
      printDebugInfo();
#endif
      return;
    }
  }



  if (digitalRead(STOPPIN) == STOP_BUTTON_TRIGGERED_PIN_STATE)
  {
    if (systemstate == STATE_CYCLING)
    {
      cycle = CYCLE_STOP;
#ifdef DEBUG
      Serial.println(" ");
      Serial.println(" ");
      Serial.println(" ");
      Serial.println("Stop Button On Released Fired");
      printDebugInfo();
#endif
      return;
    }
  }

}
//-----------------------------------------------------------------------------------------------------
void pollOperationMode()
{

  //The Operation Mode Pin Operates on a Single Switch
  //If the state is HIGH it's One Operation.
  //If it's Low it''s the other
  if (digitalRead(OPERATIONMODEPIN) == AUTOMATIC_OPERATION_MODE_PIN_STATE)
  {
    operationmode = OPERATION_AUTOMATIC;
  }
  else
  {
    operationmode = OPERATION_MANUAL;
  }
}
//-----------------------------------------------------------------------------------------------------

void processAllStop()
{
  digitalWrite(VALVEOPENCONTROLPIN, RELAY_OFF_PIN_STATE);
  digitalWrite(VALVECLOSECONTROLPIN, RELAY_ON_PIN_STATE);
  digitalWrite(VFDCONTROLPIN, RELAY_OFF_PIN_STATE);
  systemstate = STATE_STOPPED;
  cycle = CYCLE_STOP;
}

//-----------------------------------------------------------------------------------------------------
