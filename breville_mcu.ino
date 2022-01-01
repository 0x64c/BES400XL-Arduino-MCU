//isr timer
#define TIMER_INTERRUPT_DEBUG 0
#define _TIMERINTERRUPT_LOGLEVEL_ 0
#define USE_TIMER_1 true
#include "TimerInterrupt.h"

//time durations in ms
#define STEAM_DURATION 300000
#define BREW_DURATION 40000
#define ONE_SECOND 1000
#define AUTO_SHUTDOWN_TIME 900000
#define TIMER_INTERVAL_MS 20

//time constants in frames
#define BLINK_FRAMES 5
#define MIN_STABLE_FRAMES 3
#define MIN_LONGPRESS_FRAMES 50//full second
#define HEATER_FRAMES 30

//pin values
#define PIN_HEATER 8
#define PIN_PUMP 9
#define PIN_LED_ON 12
#define PIN_LED_COFFEE 11
#define PIN_LED_STEAM 10
#define PIN_BUTTONS 14
#define PIN_TEMP 15

//analogue voltage thresholds
//buttons
#define BTN_VOLT_POWER  0x100
#define BTN_VOLT_COFFEE 0x200
#define BTN_VOLT_STEAM  0x300
#define BTN_VOLT_NONE   0x400

//thermistor temperature values
//ADC readings were calculated using the linear equation (A-T)/B, where A=124.22, B=0.08, T=actual temperature in Celcius degrees
//this equation was derived from temperatures measured with a thermocouple placed near the thermistor, collected for various ADC target values
#define STEAMING_TEMPERATURE 0x12F//100
#define MAX_TEMPERATURE_INDEX 3
int temp_index = 0;
int brewing_temperatures[MAX_TEMPERATURE_INDEX+1]={0x1EA,0x1AC,0x186,0x15F};//85,90,93,96

//helpful macros
#define SET_PIN(b,a) digitalWrite(b,((a > 0) ? LOW : HIGH))
#define SET_HEATER(a) digitalWrite(PIN_HEATER,((a > 0) ? LOW : HIGH))
#define SET_PUMP(a) digitalWrite(PIN_PUMP,((a > 0) ? HIGH : LOW))

//functional states
#define OFF_STATE 0
#define COFFEE_STATE 1
#define COFFEE_BREWING 2
#define STEAM_STATE 3
//temperature states
#define COLD 0
#define COFFEE_READY 1
#define STEAM_READY 2
//button states
#define BTN_NONE 0
#define BTN_POWER 1
#define BTN_COFFEE 2
#define BTN_STEAM 3

//machine status
volatile int button_state = BTN_NONE;
volatile int button_longpress = 0;
volatile int func_state = OFF_STATE;
volatile int temp_state = COLD;

volatile int isr_started = 0;
void TimerHandler()
{
  //temperature regulation
  static int this_temp;
  static int heaterframes;
  //input handling
  static int this_button_state;
  static int last_button_state;
  static int button_reading;
  static int frames_stable;
  //led
  static int blinkframes;
  static int blinkylights;
  static int blinkylights2;
  static int blink_counter;

  //initialize local variables
  if(!isr_started){
    this_temp = 0x0;
    frames_stable = 0;
    blinkframes = 0;
    blinkylights = 0;
    blinkylights2 = 0;
    blink_counter = 0;
    heaterframes = 0;
    button_reading = BTN_VOLT_NONE;
    this_button_state = BTN_NONE;
    last_button_state = BTN_NONE;
    isr_started=1;
  }
  
  //read button input
  button_reading = analogRead(PIN_BUTTONS);
  //decode and store current button state
  if ( button_reading < BTN_VOLT_POWER ){
    this_button_state = BTN_POWER;
  }else if ( button_reading < BTN_VOLT_COFFEE ){
    this_button_state = BTN_COFFEE;
  }else if ( button_reading < BTN_VOLT_STEAM ){
    this_button_state = BTN_STEAM;
  }else{
    this_button_state = BTN_NONE;
  }

  //if button has been pressed and released, check if it was a short or long press and store it in global
  if(this_button_state==BTN_NONE && last_button_state>BTN_NONE){
    if(frames_stable > MIN_LONGPRESS_FRAMES){
      //button was a long press
      button_longpress = 1;
    }else{
      button_longpress = 0;
    }
    //button was pressed long enough to register, so store the value
    if(frames_stable > MIN_STABLE_FRAMES){
      button_state = last_button_state;
    }
  }

  //keep track of how many frames the button state has been stable for
  if(this_button_state==last_button_state){
    frames_stable += 1;
  }else{
    frames_stable = 0;
  }

  //store the last button state
  last_button_state = this_button_state;

  //regulate temp
  heaterframes++;
  //check if enough frames have passed
  if(heaterframes>HEATER_FRAMES){
    heaterframes=0;
    this_temp = analogRead(PIN_TEMP);
    if(func_state > OFF_STATE){
      if (this_temp < STEAMING_TEMPERATURE) {
        //steam temp reached
        temp_state = STEAM_READY;
        SET_HEATER(0);
      } else if (this_temp < brewing_temperatures[temp_index]){ 
        //coffee temp reached
        temp_state = COFFEE_READY;
        if (func_state >= STEAM_STATE) {
          SET_HEATER(1);
        } else {
          SET_HEATER(0);
        }
      } else {
        temp_state = COLD;
        SET_HEATER(1);
      }
    }else{
      SET_HEATER(0);
    }
  }

  //set pump
  if (func_state != COFFEE_BREWING) {
    SET_PUMP(0);
  }else{
    SET_PUMP(1);
  }

  //calculate LED blink;
  blinkframes += 1;
  if(blinkframes > BLINK_FRAMES){
    //steam/coffee LED blink
    blinkylights=(blinkylights>0)?0:1;
    blinkframes = 0;

    //power LED blink
    blink_counter+=1;
    blinkylights2=blinkylights;
    if(temp_index==0){
      blinkylights2=1;
    }else{
      if(blink_counter > 2*temp_index+2){
        blinkylights2=0;
        if(blink_counter > 2*(temp_index)+3){
          blink_counter=0;
        }
      }
    }
  }
  
  //set leds
  if(func_state > OFF_STATE){
    //blink power LED to indicate selected temperature
    SET_PIN(PIN_LED_ON,blinkylights2);
    if(temp_state < COFFEE_READY){
      //blink coffee LED to indicate temperature below target
      SET_PIN(PIN_LED_COFFEE,blinkylights);
    }else{
      SET_PIN(PIN_LED_COFFEE,1);
    }
    if(func_state<=COFFEE_BREWING){
      SET_PIN(PIN_LED_STEAM,0);
    }else if(temp_state < STEAM_READY){
      //blink steam LED to indicate temperature below target
      SET_PIN(PIN_LED_STEAM,blinkylights);
    }else{
      SET_PIN(PIN_LED_STEAM,1);
    }
    //SET_PIN(PIN_LED_STEAM, ((temp_state>=STEAM_READY)? 1 : 0) );
    //SET_PIN(PIN_LED_COFFEE, ((temp_state>=COFFEE_READY)? 1 : 0) );
  }else{
    SET_PIN(PIN_LED_ON,0);
    SET_PIN(PIN_LED_COFFEE,0);
    SET_PIN(PIN_LED_STEAM,0);
  }
}

//timing
unsigned long PumpShutoffTime = 0;
unsigned long SteamShutoffTime = 0;
unsigned long LastButtonPress = 0;

//setup arduino and turn everything off
void setup() {
  pinMode(PIN_HEATER, OUTPUT);
  pinMode(PIN_PUMP, OUTPUT);
  pinMode(PIN_LED_ON, OUTPUT);
  pinMode(PIN_LED_COFFEE, OUTPUT);
  pinMode(PIN_LED_STEAM, OUTPUT);
  pinMode(PIN_BUTTONS, INPUT);
  pinMode(PIN_TEMP, INPUT);

  SET_PIN(PIN_LED_ON,0);
  SET_PIN(PIN_LED_COFFEE,0);
  SET_PIN(PIN_LED_STEAM,0);
  SET_PUMP(0);
  SET_HEATER(0);
  func_state=OFF_STATE;

  //setup start interrupt timer code
  ITimer1.init();
  ITimer1.attachInterruptInterval(TIMER_INTERVAL_MS,TimerHandler);
}

//handle integer rollover
unsigned long lastMillis = 0;
unsigned long millisRollover() {
  unsigned long currentMillis = millis();
  return (unsigned long)(currentMillis - lastMillis);
}

void brewCycle() {
  func_state = COFFEE_BREWING;
  PumpShutoffTime = millisRollover() + BREW_DURATION;
}

void steamCycle(){
  func_state = STEAM_STATE;
  SteamShutoffTime = millisRollover() + STEAM_DURATION;
}

//figures out what to do given the last pressed button
void parseButtons() {
  //handle buttons based on machine state
  switch(func_state){
    case OFF_STATE:
      if(button_state > BTN_NONE){
        func_state = COFFEE_STATE;
        temp_index = 0;
      }
      break;
    case COFFEE_STATE:
      switch(button_state){
        case BTN_POWER:
          func_state = OFF_STATE;
          break;
        case BTN_COFFEE:
          brewCycle();
          break;
        case BTN_STEAM:
          //increment index of selected target temperature
          //when max index is reached, switch to steam mode
          if(button_longpress>0){
            steamCycle();
            temp_index = 0;
          }else{
            temp_index+=1;
            if(temp_index>MAX_TEMPERATURE_INDEX){
              temp_index = 0;
            }
          }
          //temp_setting = (temp_setting>3) ? (0) : (temp_setting+1);
          //temp_setting += 1;//temperature_target -= 0x1;//func_state = STEAM_STATE;
          break;
      }
      break;
    case COFFEE_BREWING:
      switch(button_state){
        case BTN_POWER:
          func_state = OFF_STATE;
          break;
        case BTN_COFFEE:
          func_state = COFFEE_STATE;
          break;
        case BTN_STEAM:
          func_state = COFFEE_STATE;
          break;
      }
      break;
    case STEAM_STATE:
      switch(button_state){
        case BTN_POWER:
          func_state = OFF_STATE;
          break;
        case BTN_COFFEE:
        case BTN_STEAM:
          func_state = COFFEE_STATE;
          temp_index = 0;
          break;
      }
      break;
  }
  //clear button state since we handled it already
  if(button_state > BTN_NONE){
    button_state=BTN_NONE;
    //button_longpress=0;
    //keep track of last button press for auto shutdown
    LastButtonPress = millisRollover();
  }
}

//turns off the pump if brew time is up
void checkBrew() {
  if ( (millisRollover() - PumpShutoffTime) < ONE_SECOND ) {
    func_state = COFFEE_STATE;
  }
}

//stops the steam heating if steam time is up
void checkSteam() {
  if ( (millisRollover() - SteamShutoffTime) < ONE_SECOND ) {
    func_state = COFFEE_STATE;
  }
}

void loop() {
  if (func_state > OFF_STATE) {
    //if machine is on do these
    if (func_state == STEAM_STATE)
      checkSteam();
    if (func_state == COFFEE_BREWING)
      checkBrew();
    //auto shutdown
    if (millisRollover() - LastButtonPress > AUTO_SHUTDOWN_TIME) {
      func_state = OFF_STATE;
    }
  }
  //always do these
  parseButtons();
}
