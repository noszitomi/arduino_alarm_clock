#include "ds3231.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#define ACTIVE LOW
#define NOT_ACTIVE HIGH

// Buttons (used later as index in buttons struct)
#define B_ALARM 0
#define B_TIME  1
#define B_UP    2
#define B_DOWN  3

// Events: button pressed SHORT/LONG
#define E_NOTHING 0
#define E_PRESS_SHORT 10
#define E_PRESS_LONG  20

#define DEBOUNCE_DELAY 50

#define BUFF_MAX 128
#define CONFIG_UNIXTIME 

//run state values
#define NORMAL   0
#define TIMESET  1
#define ALARMSET 2
#define LEDPIN 13
struct s_butt {
  int pin;
  int active;
  int longpressed;
  unsigned long presstime;
  unsigned long lastDebounceTime;
};

struct s_butt buttons[4];
struct ts newt;
struct ts actualt;
struct ts alarmt;
struct ts newalarmt;

int alarmON = false;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 
int index = 0;
uint8_t time[8];
char recv[BUFF_MAX];
unsigned int recv_size = 0;
unsigned long prev, interval = 5000;

void parse_cmd(char *cmd, int cmdsize);

int runState = NORMAL;

//------------------------------------------------------------------------
//
//  void setup()
//
//------------------------------------------------------------------------
void setup() {
  int i;
  char buff[100];

  Serial.begin(9600);
  buttons[B_ALARM].pin = 2;
  buttons[B_TIME].pin  = 3;
  buttons[B_UP].pin    = 4;
  buttons[B_DOWN].pin  = 5;

  for (i = 0; i < 4; i++) {
    buttons[i].active = false;
    buttons[i].longpressed = false;
    buttons[i].presstime = 0;
    pinMode(buttons[i].pin, INPUT);
  }

  lcd.begin(16,2);

  DS3231_init(DS3231_INTCN);
  memset(recv, 0, BUFF_MAX);
  DS3231_get(&newt);
  DS3231_get(&alarmt);

  pinMode(LEDPIN, OUTPUT);
}

//------------------------------------------------------------------------
//
//  void loop()
//
//------------------------------------------------------------------------
void loop() {
  int event = E_NOTHING;
  int action = 0;
  int actual_min = 0;
  int alarm_min = 0;
  char buff[BUFF_MAX];
  unsigned long now = millis();
  struct ts t;
  char onbuff[3] = "  ";
  char setbuff[4] = "   ";
  
  // check ALARM button
  event = getButtonEvent(&(buttons[B_ALARM]));

  if (event != E_NOTHING)
    action = (100 * B_ALARM) + event;
  else {
    event = getButtonEvent(&(buttons[B_TIME]));
    if (event != E_NOTHING)
      action = (100 * B_TIME) + event;
    else {
      event = getButtonEvent(&(buttons[B_UP]));
      if (event != E_NOTHING)
        action = (100 * B_UP) + event;
      else {
        event = getButtonEvent(&(buttons[B_DOWN]));
        if (event != E_NOTHING)
          action = (100 * B_DOWN) + event;
      }     
    }
  } 

  switch(runState)
  {
    case NORMAL:
      process_NORMAL(action);
      break;
      
    case ALARMSET:
      process_ALARM(action);
      break;
      
    case TIMESET: 
      process_TIME(action);
      break; 
      
    default:
      break;
  }

  //fuggveny --> checks time ha nagyobb mint az alarm time es engedelyezve van  --> ZENEBONA
  //ha timenow - alarmtime > 2 perc --> KIKAPCS

  if(runState == TIMESET){
    snprintf(buff, BUFF_MAX, "SET: %02d:%02d", newt.hour, newt.min);
    lcd.setCursor(0,0);
    lcd.print(buff);
  }
  else {
    DS3231_get(&actualt);
    snprintf(buff, BUFF_MAX, "     %02d:%02d", actualt.hour, actualt.min);
    lcd.setCursor(0,0);
    lcd.print(buff);
  } 
    
  if(alarmON == true)
    strncpy(onbuff,"ON",2);

  if(runState == ALARMSET) {
    strncpy(setbuff,"SET",3);    
    snprintf(buff, BUFF_MAX, "%s  %02d:%02d    %s",setbuff,newalarmt.hour, newalarmt.min,onbuff);
  }
  else
    snprintf(buff, BUFF_MAX, "%s  %02d:%02d    %s",setbuff, alarmt.hour, alarmt.min,onbuff);
  
  lcd.setCursor(0,1);
  lcd.print(buff);

//switch on alarming if the time has come 
  if (actualt.hour == 0)
    actual_min = (24 * 60) + actualt.min;
  else
    actual_min = (actualt.hour * 60) + actualt.min;

  alarm_min = (alarmt.hour * 60) + alarmt.min;

  if ( (actual_min >= alarm_min) && ((actual_min - alarm_min) <= 2) )
    digitalWrite(LEDPIN, HIGH);
  else 
    digitalWrite(LEDPIN, LOW);
}

//------------------------------------------------------------------------
//
//  int getButtonEvent(struct s_butt *butt)
//
//------------------------------------------------------------------------
int getButtonEvent(struct s_butt *butt)
{
  char buff[100];
  int buttonstate = NOT_ACTIVE;
  int event = E_NOTHING;
  unsigned long timenow;
  timenow = millis();
  buttonstate = digitalRead(butt->pin);
  
    if (buttonstate == ACTIVE) {  // button is pressed now
      if (butt->active == false) { // button was NOT pressed until now
        butt->active = true;
        butt->presstime = timenow;
        butt->lastDebounceTime = timenow;
      }
      else {
        if ( (timenow - butt->presstime) >= 1000) {
          event = E_PRESS_LONG;
          butt->presstime = timenow;
          butt->longpressed = true;
        }
      }
    } // end of button is pressed
    else {                               // button is not pressed now
      if(butt->active == true) {         // button was pressed until now
        butt->active = false;
        if(butt->longpressed == false) {
          if ((timenow - butt->lastDebounceTime) > 50)  
            event = E_PRESS_SHORT;
        }
        else {
          //event = E_PRESS_LONG;
          butt->longpressed = false;
        }
      }
    }
  
  return event;
}

//------------------------------------------------------------------------
//
//  void processNORMAL(int action)
//
//------------------------------------------------------------------------
void process_NORMAL(int action)
{
  switch(action)
  {
    case((100 * B_ALARM) + E_PRESS_SHORT):
      if (alarmON == false)
        alarmON = true;
      else
        alarmON = false;
    break;
    
    case((100 * B_ALARM) + E_PRESS_LONG):
      newalarmt.hour = alarmt.hour;
      newalarmt.min  = alarmt.min;
      runState = ALARMSET;
      alarmON = true;
    break; 
    
    case((100 * B_TIME) + E_PRESS_LONG):
      DS3231_get(&newt);
      runState = TIMESET;
     //timesor villog
    break;
    
    case((100 * B_TIME) + E_PRESS_SHORT):
    case((100 * B_UP) + E_PRESS_SHORT):
    case((100 * B_UP) + E_PRESS_LONG):
    case((100 * B_DOWN) + E_PRESS_SHORT):
    case((100 * B_DOWN) + E_PRESS_LONG):
    break;
  }
}

//------------------------------------------------------------------------
//
//  void processTIME(int action)
//
//------------------------------------------------------------------------
void process_TIME(int action)
{
  switch(action)
  {
    case((100 * B_ALARM) + E_PRESS_SHORT):
    case((100 * B_ALARM) + E_PRESS_LONG):
      
    break; 
    
    case((100 * B_TIME) + E_PRESS_SHORT):
    case((100 * B_TIME) + E_PRESS_LONG):
      DS3231_set(newt);
      runState = NORMAL;
    break;
    
    case((100 * B_UP) + E_PRESS_SHORT):
     newt.min++;
     if (newt.min > 59) {
      newt.hour++;
      newt.min = 0;
     }
     if (newt.hour > 23)
      newt.hour = 0;
    break;
    
    case((100 * B_UP) + E_PRESS_LONG):
      newt.min+= 10;
      if (newt.min > 59) {
        newt.hour++;
        newt.min-= 60;
      }
      if (newt.hour > 23)
        newt.hour = 0;
    break;
    
    case((100 * B_DOWN) + E_PRESS_SHORT):
      newt.min--;
      if (newt.min > 59) {
        newt.hour--;
        newt.min = 59;
      }
      if (newt.hour > 23) 
        newt.hour = 23;
    break;
    
    case((100 * B_DOWN) + E_PRESS_LONG):
      newt.min-= 10;
      if (newt.min > 59) {
        newt.hour--;
        newt.min += 60;
      }
      if (newt.hour > 23) 
        newt.hour+= 24;
    break;
  }
}

//------------------------------------------------------------------------
//
//  void processALARM(int action)
//
//------------------------------------------------------------------------
void process_ALARM(int action)
{
  switch(action)
  {
    case((100 * B_ALARM) + E_PRESS_SHORT):
    case((100 * B_ALARM) + E_PRESS_LONG):
      alarmt.hour = newalarmt.hour;
      alarmt.min  = newalarmt.min;
      alarmON     = true;
      runState = NORMAL;
    break; 
    
    case((100 * B_UP) + E_PRESS_SHORT):
      newalarmt.min++;
      if (newalarmt.min > 59) {
        newalarmt.hour++;
        newalarmt.min = 0;
      }
      if (newalarmt.hour > 23)
        newalarmt.hour = 0;
    break;
    
    case((100 * B_UP) + E_PRESS_LONG):
      newalarmt.min+= 10;
      if (newalarmt.min > 59) {
        newalarmt.hour++;
        newalarmt.min-= 60;
      }
      if (newalarmt.hour > 23)
        newalarmt.hour = 0;
    break;
    
    case((100 * B_DOWN) + E_PRESS_SHORT):
      newalarmt.min--;
      if (newalarmt.min > 59) {
        newalarmt.hour--;
        newalarmt.min = 59;
      }
      if (newalarmt.hour > 23) 
        newalarmt.hour = 23;
    break;
    
    case((100 * B_DOWN) + E_PRESS_LONG):
      newalarmt.min-= 10;
      if (newalarmt.min > 59) {
        newalarmt.hour--;
        newalarmt.min += 60;
      }
      if (newalarmt.hour > 23) 
        newalarmt.hour+= 24;
    break;

    case((100 * B_TIME) + E_PRESS_LONG):
    case((100 * B_TIME) + E_PRESS_SHORT):
    break;
  }
}






