#include <Servo.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Arduino.h>  // for type definitions
#include "EEPROMData.h"
#include "i2ckeypadreader.h"

// Variables Definition
#define BUZZER_PIN 9
#define SERVO_PIN 10
#define DOOR_SENSOR_ANALOG_PIN A0

#define STATE_CONFIG_PASS 1
#define WAITING_FOR_INPUT 2

#define EEPROM_START_ADDRESS 0x00

#define DEBUG
// TODO: define once we have an LCD
//#define LCD_ENABLED
#define NEW_LINE '\n'

#define PASSWORD_ALLOWED_LENGTH 9
#define PASSWORD_EMPTY_LENGTH "         "

#define SPECIAL_CHAR '#'
#define APPLY_PASSWORD '*'
#define LOCK_DOOR APPLY_PASSWORD
#define UNLOCK_DOOR SPECIAL_CHAR
#define OPEN_AND_MONITOR_DOOR '1'

#define UNKNOWN_STATE "STATUS: Unknown State"
#define DOOR_LOCKED_STATE "STATUS: LOCKED"
#define DOOR_UNLOCKED_STATE "STATUS: UNLOCKED"
#define DOOR_UNLOCKED_WITH_MONITORING_STATE "STATUS: UNLOCKED & MNTR"

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
unsigned int state;
bool doorOpenedAlarm = false;
String Status;

// Function Declaration
void printMessage(String message);
void correctPassword();
void wrongPassword();
void openDoor();
void closeDoor();
void buzz(int delayTimeInms);
void writeEEPROMData(String data);
void readEEPROMData(String& data);
unsigned int getState();
void setState(unsigned int stat);
void setStatus(String stat);


// Start
void setup()
{
  Serial.begin(9600);
  myServo.attach(SERVO_PIN); // servo on digital pin 10
  Wire.begin();
  keypad.init();
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(DOOR_SENSOR_ANALOG_PIN, LOW);
  // Enable before sending it to customer.
  writeEEPROMData("NO DATA");
  setStatus(UNKNOWN_STATE);
  printMessage("Done With Setup");
  setState(getState());
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
    switch (state)
    {
      case STATE_CONFIG_PASS:
        if (key != SPECIAL_CHAR && key != APPLY_PASSWORD)
        {
          tempPassword += key;
        }
        // TODO: handle '*' and '#' properly.
        if (tempPassword.length() > PASSWORD_ALLOWED_LENGTH)
        {
          String message = "Password Max Size Is: " + maxPasswordLength;
          printMessage(message);
          printMessage("Try Again");
          tempPassword = "";
        }
        if (key == SPECIAL_CHAR)
        {
          printMessage("Saving Password...");
          // save password to EEPROM.
          writeEEPROMData(tempPassword);
          tempPassword = "";
          setState(WAITING_FOR_INPUT);
          printMessage("Password Saved.");
        }
        break;
      case WAITING_FOR_INPUT:
        if (key != SPECIAL_CHAR && key != APPLY_PASSWORD)
        {
          tempPassword += key;
        }

        if (key == APPLY_PASSWORD)
        {
          // read password from EEPROM.
          String password(PASSWORD_EMPTY_LENGTH);
          readEEPROMData(password);
          if (password == tempPassword)
          {
            correctPassword();
            tempPassword = "";
          }
          else
          {
            wrongPassword();
            setState(WAITING_FOR_INPUT);
            tempPassword = "";
          }
        }
        else if (tempPassword.length() > PASSWORD_ALLOWED_LENGTH)
        {
          wrongPassword();
          setState(WAITING_FOR_INPUT);
          tempPassword = "";
        }
#ifdef DEBUG
        if (key == SPECIAL_CHAR)
        {
          printMessage("Printing Password...");
          String password(PASSWORD_EMPTY_LENGTH);
          eeprom.readData(password);
          printMessage(password);
        }
#endif
        break;
      default:
           printMessage("This should not happen!!!");
           Serial.print(state);
        break;
    }
  }
}

unsigned int getState()
{
  String temp(PASSWORD_EMPTY_LENGTH);
  readEEPROMData(temp);
  if (temp.equals(emptyPassword))
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

void setState(unsigned int stat)
{
  switch (stat)
  {
    case STATE_CONFIG_PASS:
      printMessage("STATE_CONFIG_PASS");
      state = STATE_CONFIG_PASS;
      break;
    case WAITING_FOR_INPUT:
      printMessage("WAITING_FOR_INPUT");
      state = WAITING_FOR_INPUT;
      break;
    default:
      printMessage("This should not happen!");
      break;
  }
}

void setStatus(String stat)
{
  // TODO: make this its own line in the lCD.
  printMessage(stat);
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
  printMessage("Press # to unlock, * to lock, or 1 to unlock and monitor");
  bool commandReceived = false;
  disablePinInterrupt(DOOR_SENSOR_ANALOG_PIN);
  digitalWrite(BUZZER_PIN, LOW);
  while (!commandReceived)
  {
    char key = keypad.getkey();
    if (key != NO_KEY_VALUE)
    {
      if (key == UNLOCK_DOOR)
      {
        printMessage("Openning Lock");
        openDoor();
        commandReceived = true;
        setStatus(DOOR_UNLOCKED_STATE);
      }
      else if (key == LOCK_DOOR)
      {
        printMessage("Closing Lock");
        closeDoor();
        enablePinInterrupt(DOOR_SENSOR_ANALOG_PIN);
        commandReceived = true;
        setStatus(DOOR_LOCKED_STATE);
      }
      else if (key == OPEN_AND_MONITOR_DOOR)
      {
        printMessage("Openning Lock and Monitoring the door");
        openDoor();
        enablePinInterrupt(DOOR_SENSOR_ANALOG_PIN);
        commandReceived = true;
        setStatus(DOOR_UNLOCKED_WITH_MONITORING_STATE);
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
  myServo.write(0); // clock wise.
  delay(600);
  myServo.write(94); // stop rotation.
}

void closeDoor()
{
  myServo.write(255); // anti clock wise.
  delay(600);
  myServo.write(94);  // stop rotation.
}

void buzz(int delayTimeInms)
{
  // buzzer is already in use by the alarm.
  if (doorOpenedAlarm) return;

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
  if (!eeprom.readData(data))
  {
    printMessage("ERROR READING EEPROM DATA!!!");
    printMessage("READ DATA: " + data);
  }
}

void monitorDoorStatus()
{
  // TODO: use interrupts.
  delay(50);

}

void enablePinInterrupt(byte pin)
{
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

void disablePinInterrupt(byte pin)
{
  *digitalPinToPCMSK(pin) &= ~bit (digitalPinToPCMSKbit(pin));  // disable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

ISR (PCINT1_vect) // handle pin change interrupt for A0 to A5 here
{
  int value = digitalRead(DOOR_SENSOR_ANALOG_PIN);
  Serial.print(value);
  Serial.print(NEW_LINE);
  // 0: closed - 1: open.
  if (value == 0)
  {
    // door is closed.
    // do nothing.
  }
  else
  {
    // door opened.
    Serial.print("DOOR WAS OPENED!!! SOUND THE ALARM");
    // buzz until password is entered.
    digitalWrite(BUZZER_PIN, HIGH);
    doorOpenedAlarm = true;
  }
}
