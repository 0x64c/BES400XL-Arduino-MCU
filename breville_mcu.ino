#define PIN_HEATER 8
#define PIN_PUMP 9
#define PIN_LED_ON 12
#define PIN_LED_COFFEE 11
#define PIN_LED_STEAM 10
#define PIN_BUTTONS 14
#define PIN_TEMP 15

#define BTN_VOLT_POWER  0x100
#define BTN_VOLT_COFFEE 0x200
#define BTN_VOLT_STEAM  0x300
#define BTN_VOLT_NONE   0x400
#define BTN_NONE 0
#define BTN_POWER 1
#define BTN_COFFEE 2
#define BTN_STEAM 3

#define BREWING_TEMPERATURE 0x200
#define STEAMING_TEMPERATURE 0x148
#define STEAM_DURATION 300000
#define BREW_DURATION 18000
#define TEMP_REGULATION_DELAY 100
#define ONE_SECOND 1000
#define AUTO_SHUTDOWN_TIME 900000

//machine status
int Heater = 0;
int Pump = 0;
int ButtonState = 0;
int LastButtonState = 0;
#define POWER_OFF 0b0
#define COFFEE_TIME 0b1
#define STEAM_TIME 0b10
//#define TEMP_CALIBRATION 0b100
#define COFFEE_TEMP 0b1000
#define STEAM_TEMP 0b10000
#define BREWING 0b100000
unsigned int machineState = POWER_OFF;

//timing
unsigned long LastDebounceTime = 0;
unsigned long DebounceDelay = 50;
unsigned long PumpShutoffTime = 0;
unsigned long SteamShutoffTime = 0;
unsigned long LoopTimer = 0;
unsigned long LastButtonPress = 0;

//setup arduino and turn everything off
void setup() {
  pinMode(PIN_HEATER,OUTPUT);
  pinMode(PIN_PUMP,OUTPUT);
  pinMode(PIN_LED_ON,OUTPUT);
  pinMode(PIN_LED_COFFEE,OUTPUT);
  pinMode(PIN_LED_STEAM,OUTPUT);
  pinMode(PIN_BUTTONS,INPUT);
  pinMode(PIN_TEMP,INPUT);

  digitalWrite(PIN_LED_ON,HIGH);
  digitalWrite(PIN_LED_COFFEE,HIGH);
  digitalWrite(PIN_LED_STEAM,HIGH);
  digitalWrite(PIN_HEATER,HIGH);
  digitalWrite(PIN_PUMP,LOW);
}

//handle integer rollover
unsigned long lastMillis = 0;
unsigned long millisRollover(){
  unsigned long currentMillis=millis();
  return (unsigned long)(currentMillis-lastMillis);
}

//produce a blink code for debugging, visual feedback, etc
//takes hex-encoded integer
//the low 3 nybbles correspond to each button's LED
void blinkCode(int count,int blink_delay){
  int count_100 = (count&0xF00) >> 8;
  int count_10 = (count&0x0F0) >> 4;
  int count_1 = count&0x00F;
  for(int i = 0; i < count_100; i++){
    digitalWrite(PIN_LED_ON,LOW);
    delay(blink_delay);
    digitalWrite(PIN_LED_ON,HIGH);
    delay(blink_delay);
  }
  delay(blink_delay);
  for(int i = 0; i < count_10; i++){
    digitalWrite(PIN_LED_COFFEE,LOW);
    delay(blink_delay);
    digitalWrite(PIN_LED_COFFEE,HIGH);
    delay(blink_delay);
  }
  delay(blink_delay);
  for(int i = 0; i < count_1; i++){
    digitalWrite(PIN_LED_STEAM,LOW);
    delay(blink_delay);
    digitalWrite(PIN_LED_STEAM,HIGH);
    delay(blink_delay);
  }
  delay(blink_delay);
}

//takes a raw value from the ADC
//each button produces a different voltage using voltage dividers
//tells you what button was pressed
int buttonReadingToType(int reading){
  int return_value = BTN_NONE;
  
  if( reading < BTN_VOLT_POWER )
    return_value = BTN_POWER;
  else if( reading < BTN_VOLT_COFFEE )
    return_value = BTN_COFFEE;
  else if( reading < BTN_VOLT_STEAM )
    return_value = BTN_STEAM;

  return return_value;
}

//reads data from the ADC pin connected to the buttons
//debounces input using a timer
void readButtons(){
  int button_reading = analogRead(PIN_BUTTONS);

  int button_state = buttonReadingToType(button_reading);

  if( button_state != LastButtonState )
    LastDebounceTime = millisRollover();

  if ( ( millisRollover() - LastDebounceTime ) > DebounceDelay ){
    if( button_state != ButtonState ) {
      ButtonState = button_state;
      parseButtons();
    }
  }

  LastButtonState = button_state;
}

//makes sure the heater doesn't get too hot
//reads the raw value at the ADC pin connected to the thermistor
void regulateTemp(){
  if(millisRollover() - LoopTimer > TEMP_REGULATION_DELAY){
    LoopTimer = millisRollover();
  }else{
    return;
  }

  int this_temp = analogRead(PIN_TEMP);

  if(this_temp < STEAMING_TEMPERATURE){
    machineState|=(STEAM_TEMP|COFFEE_TEMP);
    setHeater(0);
    if((machineState&STEAM_TIME)==0){
      blinkCode(0x2,200);
    }
  }else if(this_temp < BREWING_TEMPERATURE){
    machineState|=COFFEE_TEMP;
    machineState&=(~STEAM_TEMP);
    if((machineState&STEAM_TIME) > 0){
      blinkCode(0x2,100);
      setHeater(1);
    }else{
      setHeater(0);
    }
  }else{
    if((machineState&BREWING) == 0){
      blinkCode(0x20,100);
      machineState&=~(STEAM_TEMP|COFFEE_TEMP);
      setHeater(1);
    }
  }
}

//turns the heater on, off, or toggles it
void setHeater(int status){
  switch(status){
    case 0:
      digitalWrite(PIN_HEATER,HIGH);
      Heater = 0;
      break;
    case 1:
      digitalWrite(PIN_HEATER,LOW);
      Heater = 1;
      break;
    case 2:
      if(Heater>0){
        digitalWrite(PIN_HEATER,HIGH);
        Heater = 0;
      }else{
        digitalWrite(PIN_HEATER,LOW);
        Heater = 1;
      }
      break;
  }
}

//turns the pump on, off, or toggles it
void setPump(int status){
  switch(status){
    case 0:
      digitalWrite(PIN_PUMP,LOW);
      Pump = 0;
      break;
    case 1:
      digitalWrite(PIN_PUMP,HIGH);
      Pump = 1;
      break;
    case 2:
      if(Pump>0){
        digitalWrite(PIN_PUMP,LOW);
        Pump = 0;
      }else{
        digitalWrite(PIN_PUMP,HIGH);
        Pump = 1;
      }
      break;
    default:
      break;
  }
}

//get some water in the lines and presoak the beans
void preInfusion(){
  setPump(1);
  delay(2000);
  setPump(0);
  delay(200);
  
  setPump(1);
  delay(100);
  setPump(0);
  delay(400);
  
  setPump(1);
  delay(100);
  setPump(0);
  delay(400);
  
  setPump(1);
  delay(100);
  setPump(0);
  delay(400);
}

void brewCycle(){
  while((machineState&COFFEE_TEMP) < 0){
    regulateTemp();
    readButtons();
    setLEDs();
  }
  preInfusion();
  delay(2000);
  setPump(1);
  PumpShutoffTime = millisRollover()+BREW_DURATION;
}

//figures out what to do given the last pressed button
void parseButtons(){
  //keep track of last button press for auto shutdown
  LastButtonPress = millisRollover();
  //blink the LED that corresponds to the button pressed
  switch(ButtonState){
    case BTN_POWER:
      blinkCode(0x500,30);
      break;
    case BTN_COFFEE:
      blinkCode(0x050,30);
      break;
    case BTN_STEAM:
      blinkCode(0x005,30);
      break;
    case BTN_NONE:
    default:
      break;
  }
  
  //handle buttons based on machine state
  if((machineState&COFFEE_TIME) > 0){
    switch(ButtonState){
      case BTN_POWER:
        shutDown();
        break;
      case BTN_COFFEE:
        if((machineState&BREWING) > 0){
          //stop brew cycle
          machineState&=(~BREWING);
          setPump(0);
        }else{
          //start brew cycle
          machineState|=BREWING;
          brewCycle();
        }
        break;
      case BTN_STEAM:
        blinkCode(0x005,30);
        if((machineState&BREWING) > 0){
          machineState&=(~BREWING);
          setPump(0);
        }
        //start steaming
        SteamShutoffTime = millisRollover()+STEAM_DURATION;
        machineState|=STEAM_TIME;
        machineState&=(~COFFEE_TIME);
        break;
    }
  }else if((machineState&STEAM_TIME) > 0){
    switch(ButtonState){
      case BTN_POWER:
        shutDown();
        break;
      case BTN_COFFEE:
      case BTN_STEAM:
        machineState&=(~STEAM_TIME);
        machineState|=COFFEE_TIME;
        break;
    }
  }else if(machineState == POWER_OFF){
    //power is off, mode selector
    switch(ButtonState){
      case BTN_POWER:
      //normal operation
        startUp();
        break;
      //calibration modes
    }
  }
}

//sets the button LEDs depending on the current state of the machine
void setLEDs(){
  if(machineState==POWER_OFF){
    digitalWrite(PIN_LED_ON,HIGH);
    digitalWrite(PIN_LED_COFFEE,HIGH);
    digitalWrite(PIN_LED_STEAM,HIGH);
  }else if((machineState&(COFFEE_TIME|STEAM_TIME))>0){
    digitalWrite(PIN_LED_ON,LOW);
    if((machineState&COFFEE_TEMP)>0)
      digitalWrite(PIN_LED_COFFEE,LOW);
    else
      digitalWrite(PIN_LED_COFFEE,HIGH);
    if((machineState&STEAM_TEMP)>0)
      digitalWrite(PIN_LED_STEAM,LOW);
    else
      digitalWrite(PIN_LED_STEAM,HIGH);
  }
}

//turn everything off
void shutDown(){
  machineState=POWER_OFF;
  setHeater(0);
  setPump(0);
}

//boot up the machine
void startUp(){
  machineState=COFFEE_TIME;
  preInfusion();
}

//turns off the pump if brew time is up
void checkBrew(){
  if( (millisRollover() - PumpShutoffTime) < ONE_SECOND ){
    setPump(0);
    machineState&=(~BREWING);
  }
}

//stops the steam heating if steam time is up
void checkSteam(){
  if( (millisRollover() - SteamShutoffTime) < ONE_SECOND ){
    machineState&=(~STEAM_TIME);
    machineState|=COFFEE_TIME;
  }
}

void loop() {
  if(machineState>POWER_OFF){
    //if machine is on do these
    regulateTemp();
    if((machineState&STEAM_TIME) > 1)
      checkSteam();
    if((machineState&BREWING) > 1)
      checkBrew();
    //auto shutdown
    if(millisRollover() - LastButtonPress > AUTO_SHUTDOWN_TIME){
      shutDown();
    }
  }
  //always do these
  readButtons();
  setLEDs();
}
