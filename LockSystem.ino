#include <Servo.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Arduino.h>  // for type definitions
#include "EEPROMData.h"
#include "i2ckeypadreader.h"

// Variables Definition
#define BUZZER_PIN 9
#define SERVO_PIN 10

#define STATE_CONFIG_PASS 1
#define WAITING_FOR_INPUT 2

#define EEPROM_START_ADDRESS 0x00

#define DEBUG
// TODO: define once we have an LCD
//#define LCD_ENABLED

#define PASSWORD_ALLOWED_LENGTH 9
#define PASSWORD_EMPTY_LENGTH "         "

#define SPECIAL_CHAR '#'
#define APPLY_PASSWORD '*'

#define ROWS 4
#define COLS 3
#define NO_KEY_VALUE '\0'

// With A0, A1 and A2 of PCF8574AP to ground, I2C address is 0x38
#define PCF8574_ADDR 0x38

i2ckeypad keypad = i2ckeypad(PCF8574_ADDR, ROWS, COLS);

Servo myServo; // Define our Servo
EEPROMData eeprom(EEPROM_START_ADDRESS);

String tempPassword = "";
String emptyPassword = "NO DATA";
String maxPasswordLength(PASSWORD_ALLOWED_LENGTH);
int state;

// Function Declaration
int getState();
void printMessage(String message);
void correctPassword();
void wrongPassword();
void openDoor();
void closeDoor();
void buzz(int delayTimeInms);
void writeEEPROMData(String data);
void readEEPROMData(String& data);


// Start
void setup()
{
  Serial.begin(9600);
  myServo.attach(SERVO_PIN); // servo on digital pin 10
  Wire.begin();
  keypad.init();
  pinMode(BUZZER_PIN, OUTPUT);
  writeEEPROMData("NO DATA");
  state = getState();
  // TODO: remove this once we figure out what is causing the servo to act crazy at first.
  for(int pos = 0; pos <= 90; pos += 1) // goes from 0 degrees to 180 degrees 
  {                                  // in steps of 1 degree 
    myServo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(5);                       // waits 15ms for the servo to reach the position 
  }
  for(int pos = 90; pos>=0; pos-=1)     // goes from 180 degrees to 0 degrees 
  {                                
    myServo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(5);                       // waits 15ms for the servo to reach the position 
  }
  //////////////////////////////////////

  printMessage("Done With Setup");
}
  
void loop()
{
   // read pressed key
   char key = keypad.getkey();

   if (key != NO_KEY_VALUE)
   {
     Serial.println(key);
     buzz(50);
     // state machine
     switch(state)
     {
       case STATE_CONFIG_PASS:
        if(key != SPECIAL_CHAR && key != APPLY_PASSWORD)
        {
          tempPassword += key;
        }
        // TODO: handle '*' and '#' properly.
        
        if(tempPassword.length() > PASSWORD_ALLOWED_LENGTH && key != SPECIAL_CHAR)
        {
          String message = "Password Max Size Is: " + maxPasswordLength;
          printMessage(message);
          printMessage("Try Again");
          tempPassword = "";
        }
        if(key == SPECIAL_CHAR)
        {
          printMessage("Saving Password...");
          // save password to EEPROM.
          writeEEPROMData(tempPassword);
          tempPassword = "";
          state = WAITING_FOR_INPUT;
          printMessage("Password Saved.");
        }
        break;
       case WAITING_FOR_INPUT:
        if(key != SPECIAL_CHAR && key != APPLY_PASSWORD)
        {
          tempPassword += key;
        }
        
        if(key == APPLY_PASSWORD)
        {
          // read password from EEPROM.
          String password(PASSWORD_EMPTY_LENGTH);
          readEEPROMData(password);
          if(password == tempPassword)
          {
            correctPassword();
            tempPassword = "";
          }
          else
          {
            wrongPassword();
            state = WAITING_FOR_INPUT;
            tempPassword = "";
          }
        }
        else if(tempPassword.length() > PASSWORD_ALLOWED_LENGTH)
        {
          wrongPassword();
          state = WAITING_FOR_INPUT;
          tempPassword = "";
        }
#ifdef DEBUG
        if(key == SPECIAL_CHAR)
        {
          printMessage("Printing Password...");
          String password(PASSWORD_EMPTY_LENGTH);
          eeprom.readData(password);
          printMessage(password);
        }
#endif
        break;
       default:
          
        break;
     }
  }
}

int getState()
{
  String temp(PASSWORD_EMPTY_LENGTH);
  readEEPROMData(temp);
  if(temp.equals(emptyPassword))
  {
    printMessage("Choose A Password");
    return STATE_CONFIG_PASS;
  }
  else
  {
    printMessage("Enter Password");
    return WAITING_FOR_INPUT;
  }
}

void printMessage(String message)
{
#ifdef LCD_ENABLED
 Serial.println("IS THE LCD ON?!!!");
#else
 Serial.println(message);
#endif

}

void correctPassword()
{
  printMessage("Password Matched");
  printMessage("Press # to unlock and * to lock");
  bool commandReceived = false;
  while(!commandReceived)
  {
     char key = keypad.getkey();    
     if (key != NO_KEY_VALUE)
     {
        if(key == '#')
        {
          printMessage("Openning Lock");
          openDoor();
          commandReceived = true;
        }
        else if(key == '*')
        {
          printMessage("Closing Lock");
          closeDoor();
          commandReceived = true;
        }
        else
        {
          printMessage("Invalid Command");
          printMessage("Press # to unlock and * to lock");
        }
     }
  }
}

void wrongPassword()
{
  printMessage("Wrong Password !!!");
  printMessage("Try Again");
  buzz(2000);
}

void openDoor()
{
  for(int pos = 0; pos <= 90; pos += 1) // goes from 0 degrees to 180 degrees 
  {                                  // in steps of 1 degree 
    myServo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(5);                       // waits 15ms for the servo to reach the position 
  }
}

void closeDoor()
{
  for(int pos = 90; pos>=0; pos-=1)     // goes from 180 degrees to 0 degrees 
  {                                
    myServo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(5);                       // waits 15ms for the servo to reach the position 
  }
}

void buzz(int delayTimeInms)
{
  digitalWrite(BUZZER_PIN, HIGH);
  delay(delayTimeInms);
  digitalWrite(BUZZER_PIN, LOW);
}

void writeEEPROMData(String data)
{
  eeprom.writeData(data);
}

void readEEPROMData(String& data)
{
  if(!eeprom.readData(data))
  {
    printMessage("ERROR READING EEPROM DATA!!!");
    printMessage("READ DATA: " + data);
  }
}
