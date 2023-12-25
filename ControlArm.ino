#include <Servo.h>
#include <Dabble.h>
#define CUSTOM_SETTINGS
#define INCLUDE_TERMINAL_MODULE
#define INCLUDE_GAMEPAD_MODULE
byte BPH = 41;
byte BEN = 45;
byte APH = 42; //This sets the pins of the arduino
byte AEN = 46;
//const int Lift = 3;
//const int Tilt = 10;
//const int Grip = 11;
Servo tilt;
Servo lift;
Servo grip; //declaring my objects
double x = 127.5;
int i = 1;int arm = 35;
bool P = false;bool n = false; bool help = true; //these are state variables to check later in the code
int R=0;double avg = 0;double gripper = 500; //theese variables are declared here and used later
unsigned long check = 0;
void setup()
{
  pinMode(BPH, OUTPUT); pinMode(BEN, OUTPUT); pinMode(APH, OUTPUT); pinMode(AEN, OUTPUT);
  Serial.begin(115200);
  Dabble.begin(9600); // i set the output pins and state the baud rate
  lift.attach(3); tilt.attach(11); grip.attach(10); tilt.write(100);
}
void setpower()
{
  if (GamePad.isStartPressed() == 1)
  {
    if (i == 1 && x < 255)
    {
      x += 25.5;
      i = 0;
    }
  }
  if (GamePad.isSelectPressed() == 1) // this function "Setpower" is how I adjust the duty cycle it loops one time through when
                                      //the button is pushed based on flipping a number between 1 and 0
  {
    if (i == 1 && x > 0)
    {
      x -= 25.5;
      i = 0;
    }
  }
  if (GamePad.isSelectPressed() == 0 && GamePad.isStartPressed() == 0)
  {
    i = 1; // it resest when no bbutton is pushed
  }
}
void dirofmotor()
{
  if (GamePad.isUpPressed() == 1)
  {
    analogWrite(AEN, x);
    digitalWrite(APH, LOW);
    digitalWrite(BPH, HIGH);
    analogWrite(BEN, x);
  }
  else if (GamePad.isDownPressed() == 1)
  {
    analogWrite(AEN, x);
    digitalWrite(APH, HIGH);
    digitalWrite(BPH, LOW);   // all of this code is how i drive the motors based on the duty cycle
    analogWrite(BEN, x); 
  }
  else if (GamePad.isLeftPressed() == 1)
  {
    analogWrite(AEN, x);
    digitalWrite(APH, LOW);
    digitalWrite(BPH, LOW);
    analogWrite(BEN, x);
  }
  else if (GamePad.isRightPressed() == 1)
  {
    analogWrite(AEN, x);
    digitalWrite(APH, HIGH);
    digitalWrite(BPH, HIGH);
    analogWrite(BEN, x);
  }
  else
  {
    analogWrite(BEN, 0);
    analogWrite(AEN, 0); // this turns everything off when nothing is pressed
  }
}
void robotarm()
{
  if (GamePad.isCrossPressed() == 1)
  {
    if (arm < 165)
    {
      arm += 5;
      delay(100);             // both of these cross and triangle control the arm and delay it
    }
  }
  if (GamePad.isTrianglePressed() == 1)
  {
    if (arm > 35)
    {
      arm -= 5;
      delay(100);
    }
  }
  if (GamePad.isSquarePressed() == 1)
  {
    if (gripper < 2400 && !P)
    {
      gripper = 2400;
      grip.writeMicroseconds(gripper); // the way i did the gripper was either set it open or closed based on guidance in class
      delay(1500);
      check = millis(); //if it is set closed i delay it and flip a state of the variable so it starts checking the feed back
      P = true;
    }
  }
  if (GamePad.isCirclePressed() == 1)
  {
    if (gripper > 500 && P)
    {
      gripper = 500;
      delay(100);
      avg = 0;
      R =0;
      help = true;
      P = false;
    }
  }

  if (P && arm > 35 && help ) {
    if (millis() - check >= 1500 ) {
      check = millis(); //when it starts looping here it waits 1.5 seconds before actually checking in this time i am adding up the feedback values to get a average
      n = true;
    }
    if (!n) {
      avg = avg +  map(analogRead(A0), 98.21, 429.66, 500, 2400); // this adjust my bounds to microseconds based on how the gripper reads
      R = R + 1;
    }
    if (n) {
      avg = avg / R; // after 1.5 seconds it starts to run this code
      while (avg < 2100 && arm > 35) {
        arm -= 5;
        lift.write(arm); // based on the values recieved it flips the arm back to 35
        delay(100);
      }
      help = false; // this resets the states
      n = false;
    }
  }
  grip.writeMicroseconds(gripper);
  lift.write(arm);

}
void loop()
{
  Dabble.processInput(); // this loops through everything
  setpower();
  dirofmotor();
  robotarm();
}
