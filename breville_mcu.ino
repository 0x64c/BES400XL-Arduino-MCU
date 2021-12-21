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

#define MAX_TEMP 0x200
#define STEAM_TEMP 0x150
#define STEAM_DURATION 300000
#define BREW_DURATION 40000
#define TEMP_REGULATION_DELAY 1000
#define ONE_SECOND 1000
#define AUTO_SHUTDOWN_TIME 900000

//machine status
int Heater = 0;
int Pump = 0;
//int Temp = 0;
int ButtonState = 0;
int LastButtonState = 0;
int CoffeeTime = 0;
int SteamTime = 0;
int BrewTime = 0;
int CoffeeTemp = 0;
int SteamTemp = 0;

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
  int this_temp = analogRead(PIN_TEMP);
  
  if(this_temp < STEAM_TEMP){
    //we have reached steam temperature
    SteamTemp = 1;
    CoffeeTemp = 1;
    setHeater(0);
    if(SteamTime < 1){
      blinkCode(0x1,50);//blinks the steam button LED
    }
  }else if(this_temp < MAX_TEMP){
    //we have reached brew temperature
    CoffeeTemp = 1;
    SteamTemp = 0;
    if(SteamTime < 1){
      //turn off the heater since we're not making steam
      setHeater(0);
    }else{
      //keep the heater on to make steam
      setHeater(1);
      blinkCode(0x2,100);//blinks the steam button LED
    }
  }else{
    //heat up the boiler
    CoffeeTemp=0;
    SteamTemp=0;
    setHeater(1);
    blinkCode(0x20,100);//blinks the brew button LED
  }
  //Temp = this_temp;
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
    default:
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

//figures out what to do given the last pressed button
void parseButtons(){
  LastButtonPress = millisRollover();
  switch(ButtonState){
    case BTN_POWER:
      blinkCode(0x500,30);
      if(CoffeeTime)
        shutDown();
      else
        startUp();
      break;
    case BTN_COFFEE:
      blinkCode(0x050,30);
      if(BrewTime > 0){
        //stop brewing
        BrewTime = 0;
        setPump(0);
      }else{
        //start brewing
        BrewTime = 1;
        preInfusion();
        delay(2000);
        setPump(1);
        PumpShutoffTime = millisRollover()+BREW_DURATION;
      }
      break;
    case BTN_STEAM:
      blinkCode(0x005,30);
      if(SteamTime>0){
        //stop steaming
        //setPump(0);
        SteamTime = 0;
      }else{
        //start steaming
        SteamShutoffTime = millisRollover()+STEAM_DURATION;
        SteamTime = 1;
      }
    case BTN_NONE:
    default:
      break;
  }
}

//sets the button LEDs depending on the current state of the machine
void setLEDs(){
  if(CoffeeTime<1){
    digitalWrite(PIN_LED_ON,HIGH);
    digitalWrite(PIN_LED_COFFEE,HIGH);
    digitalWrite(PIN_LED_STEAM,HIGH);
  }else{
    digitalWrite(PIN_LED_ON,LOW);
    if(CoffeeTemp>0)
      digitalWrite(PIN_LED_COFFEE,LOW);
    else
      digitalWrite(PIN_LED_COFFEE,HIGH);
    if(SteamTemp>0)
      digitalWrite(PIN_LED_STEAM,LOW);
    else
      digitalWrite(PIN_LED_STEAM,HIGH);
  }
}

//turn everything off
void shutDown(){
  CoffeeTime = 0;
  SteamTime = 0;
  setHeater(0);
  setPump(0);
}

//boot up the machine
void startUp(){
  CoffeeTime = 1;
  preInfusion();
}

//turns off the pump if brew time is up
void checkBrew(){
  if( (millisRollover() - PumpShutoffTime) < ONE_SECOND ){
    setPump(0);
    BrewTime = 0;
  }
}

//stops the steam heating if steam time is up
void checkSteam(){
  if( (millisRollover() - SteamShutoffTime) < ONE_SECOND ){
    SteamTime = 0;
  }
}

void loop() {
  if(CoffeeTime>0){
    //if machine is on do these
    //regulate temperature once per second
    if(millisRollover() - LoopTimer > TEMP_REGULATION_DELAY){
      LoopTimer = millisRollover();
      regulateTemp();
    }
    if(SteamTime>0)
      checkSteam();
    if(BrewTime>0);
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
