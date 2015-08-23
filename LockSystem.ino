#include <Keypad.h>
#include <Servo.h>

// Variables Definition
#define BUZZER_PIN 9
#define SERVO_PIN 10
#define STATE_CONFIG_PASS 1
#define WAITING_FOR_INPUT 2

#define DEBUG
// TODO: define once we have an LCD
//#define LCD_ENABLED

#define PASSWORD_ALLOWED_LENGTH 9

#define SPECIAL_CHAR '#'
#define APPLY_PASSWORD '*'

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 7, 6}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
Servo myServo; // Define our Servo

String password = "1234";
String tempPassword = "";
int passwordLength;
int state;

// Function Declaration
void printMessage(String message);
void correctPassword();
void wrongPassword();
void openDoor();
void closeDoor();
void buzz(int delayTimeInms);


// Start
void setup(){
  Serial.begin(9600);
  myServo.attach(SERVO_PIN); // servo on digital pin 10
  pinMode(BUZZER_PIN, OUTPUT);
  state = STATE_CONFIG_PASS;
  passwordLength = password.length();
  
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
 
  printMessage("Please Choose A Password");
}
  
void loop()
{
   // read pressed key
   char key = keypad.getKey();
      
   if (key != NO_KEY)
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
          // TODO: report PASSWORD_ALLOWED_LENGTH as the number;
          String message = "Password Max Size Is: 9";
          printMessage(message);
          printMessage("Try Again");
          tempPassword = "";
        }
        if(key == SPECIAL_CHAR)
        {
          printMessage("Saving Password...");
          password = tempPassword;
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
          printMessage(password);
        }
#endif
        break;
       default:
          
        break;
     }
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
     char key = keypad.getKey();    
     if (key != NO_KEY)
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
