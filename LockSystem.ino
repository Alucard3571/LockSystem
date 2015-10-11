#include <Servo.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Arduino.h>  // for type definitions
// Get the LCD I2C Library here:
// https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads
// Move any other LCD libraries to another folder or delete them
// See Library "Docs" folder for possible commands etc.
#include <LiquidCrystal_I2C.h>
#include "SimpleTimer.h"
#include "EEPROMData.h"
#include "i2ckeypadreader.h"

// Variables Definition
#define BUZZER_PIN 9
#define SERVO_PIN 10
#define DOOR_SENSOR_ANALOG_PIN A0

#define STATE_CONFIG_PASS 1
#define WAITING_FOR_INPUT 2

#define EEPROM_START_ADDRESS 0x00

//#define DEBUG

#define LCD_NUMBER_OF_CHARACTERS 20
#define LCD_NUMBER_OF_LINES 4

#define PASSCODE_ROW 3
#define MESSAGES_ROW 2
#define INSTRUCTIONS_ROW 1
#define STATUS_ROW 0

#define DELAY_BETWEEN_MESSAGES 2000 // 2 seconds
#define NEW_LINE '\n'

#define PASSWORD_ALLOWED_LENGTH 9
#define PASSWORD_EMPTY_LENGTH "         "

#define SPECIAL_CHAR '#'
#define APPLY_PASSWORD '#'
#define DEBUG_CHAR '*'
#define LOCK_DOOR '*'
#define UNLOCK_DOOR SPECIAL_CHAR
#define OPEN_AND_MONITOR_DOOR '1'
#define RESET_PASSWORD '0'

#define UNKNOWN_STATE                       "STATUS: UNKNOWN"
#define DOOR_LOCKED_STATE                   "STATUS: LOCKED"
#define DOOR_UNLOCKED_STATE                 "STATUS: UNLOCKED"
#define DOOR_UNLOCKED_WITH_MONITORING_STATE "STATUS: UNLOCKED&MNT"

#define ROWS 4
#define COLS 3
#define NO_KEY_VALUE '\0'

// With A0, A1 and A2 of PCF8574AP to ground, I2C address is 0x38
#define PCF8574_ADDR 0x38

i2ckeypad keypad = i2ckeypad(PCF8574_ADDR, ROWS, COLS);

// set the LCD address to 0x27 for a 20 chars 4 line display
// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

Servo myServo; // Define our Servo
EEPROMData eeprom(EEPROM_START_ADDRESS);
SimpleTimer timer;

String tempPassword = "";
String emptyPassword = "NO DATA";
String maxPasswordLength(PASSWORD_ALLOWED_LENGTH);
unsigned int state;
int timer_id;
bool doorOpenedAlarm = false;
bool isSpecialCharacter = false;
bool lcdBacklistOff = false;
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
void printCode(unsigned int location, char key, bool show);
void clearScreen(unsigned int row);
void lcdPrintMessage(String message, unsigned int row, bool clearAfterDelay, bool delayMessage);
void turnLCDBacklightOff();

// Start
void setup()
{
  Serial.begin(9600);
  myServo.attach(SERVO_PIN); // servo on digital pin 10
  Wire.begin();
  keypad.init();
  lcd.begin(LCD_NUMBER_OF_CHARACTERS, LCD_NUMBER_OF_LINES); // initialize the lcd for 20 chars 4 lines
  lcd.backlight(); // finish with backlight on
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(DOOR_SENSOR_ANALOG_PIN, LOW);
  // Enable before sending it to customer.
  //writeEEPROMData("NO DATA");
  setStatus(UNKNOWN_STATE);
  printMessage("Done With Setup");
  setState(getState());
  timer_id = timer.setInterval(60000, turnLCDBacklightOff);
}

void loop()
{
  timer.run();

  // read pressed key
  char key = keypad.getkey();

  if (key != NO_KEY_VALUE)
  {
    if (lcdBacklistOff)
    {
      lcd.backlight();
      lcdBacklistOff = false;
    }

    timer.restartTimer(timer_id);

    Serial.println(key);
    buzz(50);
    isSpecialCharacter = (key == SPECIAL_CHAR || key == DEBUG_CHAR);
    // state machine
    switch (state)
    {
      case STATE_CONFIG_PASS:
        if (!isSpecialCharacter)
        {
          tempPassword += key;
        }
        if (tempPassword.length() <= PASSWORD_ALLOWED_LENGTH)
        {
          if (!isSpecialCharacter)
          {
            printCode(tempPassword.length(), key, true);
          }
        }
        else
        {
          clearScreen(PASSCODE_ROW);
          String message = "Max Size Is: " + maxPasswordLength;
          printMessage(message);
          printMessage("Try Again");
          lcdPrintMessage(message, MESSAGES_ROW, true/*clear*/, true/*delay*/);
          lcdPrintMessage("Try Again", MESSAGES_ROW, true/*clear*/, true/*delay*/);
          tempPassword = "";
          setState(STATE_CONFIG_PASS);
        }

        if (key == SPECIAL_CHAR)
        {
          clearScreen(INSTRUCTIONS_ROW);
          String message = "Saving Password...";
          printMessage(message);
          lcdPrintMessage(message, MESSAGES_ROW, true/*clear*/, true/*delay*/);
          // save password to EEPROM.
          writeEEPROMData(tempPassword);
          tempPassword = "";
          message = "Password Saved.";
          printMessage(message);
          lcdPrintMessage(message, MESSAGES_ROW, true/*clear*/, true/*delay*/);
          clearScreen(PASSCODE_ROW);
          setState(WAITING_FOR_INPUT);
        }
        break;
      case WAITING_FOR_INPUT:

        if (!isSpecialCharacter)
        {
          tempPassword += key;
        }

        if (tempPassword.length() <= PASSWORD_ALLOWED_LENGTH)
        {
          if (!isSpecialCharacter)
          {
            printCode(tempPassword.length(), key, false);
          }
        }

        if (tempPassword.length() > PASSWORD_ALLOWED_LENGTH)
        {
          wrongPassword();
          setState(WAITING_FOR_INPUT);
          tempPassword = "";
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
#ifdef DEBUG
        if (key == DEBUG_CHAR)
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
  String message;
  String temp(PASSWORD_EMPTY_LENGTH);
  readEEPROMData(temp);
  if (temp.equals(emptyPassword))
  {
    message = "Choose Password";
    printMessage(message);
    lcdPrintMessage(message, MESSAGES_ROW, false, false);
    return STATE_CONFIG_PASS;
  }
  else
  {
    message = "Enter Password";
    printMessage(message);
    lcdPrintMessage(message, MESSAGES_ROW, false, false);
    return WAITING_FOR_INPUT;
  }
}

void setState(unsigned int stat)
{
  String message;
  switch (stat)
  {
    case STATE_CONFIG_PASS:
      printMessage("STATE_CONFIG_PASS");
      state = STATE_CONFIG_PASS;
      message = "Choose Password";
      lcdPrintMessage(message, MESSAGES_ROW, false, false);
      lcdPrintMessage("Press # to Save", INSTRUCTIONS_ROW, false, false);
      break;
    case WAITING_FOR_INPUT:
      printMessage("WAITING_FOR_INPUT");
      state = WAITING_FOR_INPUT;
      message = "Enter Password";
      lcdPrintMessage(message, MESSAGES_ROW, false, false);
      lcdPrintMessage("Press # to Apply", INSTRUCTIONS_ROW, false, false);
      break;
    default:
      printMessage("This should not happen!");
      break;
  }
}

void correctPassword()
{
  clearScreen(PASSCODE_ROW);
  clearScreen(INSTRUCTIONS_ROW);
  String message = "Password Matched";
  printMessage(message);
  lcdPrintMessage(message, MESSAGES_ROW, true, true);
  printMessage("Press # to unlock, * to lock, or 1 to unlock and monitor");
  lcdPrintMessage("*:LOCK     #:UNLOCK", INSTRUCTIONS_ROW, false, false);
  lcdPrintMessage("1:UNLOCK & MONITOR", MESSAGES_ROW, false, false);
  lcdPrintMessage("0:Reset Password", PASSCODE_ROW, false, false);
  bool commandReceived = false;
  disablePinInterrupt(DOOR_SENSOR_ANALOG_PIN);
  digitalWrite(BUZZER_PIN, LOW);
  unsigned int state = WAITING_FOR_INPUT;
  while (!commandReceived)
  {
    char key = keypad.getkey();
    if (key != NO_KEY_VALUE)
    {
      buzz(50);

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
      else if(key == RESET_PASSWORD)
      {
        writeEEPROMData("NO DATA");
        commandReceived = true;
        state = STATE_CONFIG_PASS;
      }
      else
      {
        printMessage("Invalid Command");
        printMessage("Press # to unlock and * to lock");
      }
    }
  }

  clearScreen(MESSAGES_ROW);
  clearScreen(INSTRUCTIONS_ROW);
  clearScreen(PASSCODE_ROW);
  setState(state);
}

void setStatus(String stat)
{
  clearScreen(STATUS_ROW);
  lcd.setCursor(0, 0);
  lcd.print(stat);
}

void lcdPrintMessage(String message, unsigned int row, bool clearAfterDelay, bool delayMessage)
{
  clearScreen(row);
  lcd.setCursor(0, row);
  if ((message.length() - 1) > LCD_NUMBER_OF_CHARACTERS)
  {
    lcd.print("long message !!!");
  }
  else
  {
    lcd.print(message);
  }

  if (delayMessage)
  {
    delay(DELAY_BETWEEN_MESSAGES);
  }

  if (clearAfterDelay)
  {
    clearScreen(row);
  }
}

void printCode(unsigned int location, char key, bool show)
{
  lcd.setCursor(0, PASSCODE_ROW);
  String pass = "Password: ";
  lcd.print(pass);
  lcd.setCursor((location + pass.length() - 1), 3);
  if (show)
    lcd.print(key);
  else
    lcd.print('*');
}

void clearScreen(unsigned int row)
{
  for (unsigned int i = 0; i < LCD_NUMBER_OF_CHARACTERS ; i++)
  {
    lcd.setCursor(i, row);
    lcd.print(' ');
  }
}

void printMessage(String message)
{
  Serial.println(message);
}

void wrongPassword()
{
  clearScreen(PASSCODE_ROW);
  String message = "Wrong Password !!!";
  printMessage(message);
  lcdPrintMessage(message, MESSAGES_ROW, false/*clear*/, false);
  buzz(2000);
  message = "Try Again";
  printMessage(message);
  lcdPrintMessage(message, MESSAGES_ROW, true/*clear*/, true);
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

void turnLCDBacklightOff()
{
  lcd.noBacklight();
  lcdBacklistOff = true;
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
