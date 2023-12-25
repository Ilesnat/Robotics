#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <Dabble.h>
#include <VL6180X.h>
#include <Wire.h>
#include <NewPing.h>
VL6180X vl;
int BPH = 41;
int BEN = 45;
int APH = 42; //This sets the pins of the arduino
int AEN = 46;
int front;
bool StartMaze = false;
const int rtrigPin = 34;
const int rechoPin = 35;
const int ltrigPin = 36;
const int lechoPin = 37;
const int ela = 3;
const int era = 2;
volatile long elacount = 0;
volatile long eracount = 0;
float leftSensor, rightSensor, frontSensor;
float oldLeftSensor, oldRightSensor, oldFrontSensor; // these are global variables for the following code
float side_threshold = 20.0;
float side_target = 8.0;
float front_threshold = 10.0;
int speed = 70;
float turnfactor = 1.0;
float kp = 0.325*speed/side_target;
float ki = 0.0;   
float kd = 15; //2.0*kp;  
int lastError = 0;  // For storing PID error
float rturnadjust = 0.75;
float lturnadjust = 1.00;
float rwheeladjust = 1.00;
float lwheeladjust = 1.05;
int countperrevolution = 720;
String strLastState = "";
long loopCounter = 0;
float t;
#define MAX_DISTANCE 50
NewPing sonarfront(ltrigPin, lechoPin, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing sonarright(rtrigPin, rechoPin, MAX_DISTANCE);
// How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.
unsigned int pingSpeed = 30; 
// Holds the next ping time.
unsigned long pingTimer; 
//**** the way i setup the VL6180x sensor was through the GPIO interrupt pin. However, this is not a true interuppt.
// I never connected the pin to the Arduino. All the setup for the snesor is doing is checking the GPIO pin
//if something has been stored on the interuppt pin.
//it still has to check the pin every loop or two. Its only checking for a status.
//If a reading has been stored it reads the pin not untill then.
// This will speed up my bot quite well becasue a reading will never be stored in the pin untill something is within 20cm which is almost never
// on our maze. This is a easier way to get around messing with interrupt but it is still able to save time.
// this description above isnt really needed. However, this is how i used it on the last prject
// I just copied and pasted from my last project it makes my setup go by faster   
bool sensorReady()
{
    return ((vl.readReg(VL6180X::RESULT__INTERRUPT_STATUS_GPIO) & 0x04) != 0); // this is checking th estatus of the pin to see if a reading is avaiable
}
uint8_t readRange()
{
    float reading = vl.readReg(VL6180X::RESULT__RANGE_VAL); // if the reading is avaible i run this in the void loop. It reads the pin and then clears the interuppt
    vl.writeReg(VL6180X::SYSTEM__INTERRUPT_CLEAR, 0x01);
    return reading;
}
uint8_t reading()
{
    return readRange() * vl.getScaling(); // this scales the bove reading to what i want  - default
}
void setup()
{
    Serial.begin(115200);
    Dabble.begin(9600);
    Wire.begin();
    vl.init();
    vl.configureDefault();
    vl.setTimeout(500);
    vl.stopContinuous();
    delay(250);
    vl.writeReg(VL6180X::SYSTEM__MODE_GPIO1, 0x10);      // enable GPIO1 pin on VL6180X
    vl.writeReg(VL6180X::SYSTEM__INTERRUPT_CLEAR, 0x03); // intially clears the pin
    vl.startRangeContinuous(100);
    pinMode(ltrigPin, OUTPUT);
    pinMode(lechoPin, INPUT);
    pinMode(rtrigPin, OUTPUT); // delcARE the echo pins
    pinMode(rechoPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(ela), ela_down, CHANGE);
    attachInterrupt(digitalPinToInterrupt(era), era_down, CHANGE);
    pinMode(BPH, OUTPUT);pinMode(BEN, OUTPUT);pinMode(APH, OUTPUT);pinMode(AEN, OUTPUT);
    pinMode(ela, INPUT_PULLUP);
    pinMode(era, INPUT_PULLUP);
}
void loop()
{
    // ReadSensors();
    Dabble.processInput();
    if (GamePad.isStartPressed())
    {
        StartMaze = true; // starts the maze
    }
    if (GamePad.isCrossPressed())
    {
        StartMaze = false;
        analogWrite(AEN, 0);
        digitalWrite(APH,LOW); // cross is pressed turns everything off
        digitalWrite(BPH,HIGH);
        analogWrite(BEN, 0);
    }
    if (StartMaze == true)
    {
        ReadSensors(); // runs theese functions
        Navigate();
    }
}
void Navigate(){
  if (frontSensor > front_threshold && leftSensor < side_threshold) 
  {
    strLastState = "FORWARD";
    forward();
  }
  else if (frontSensor < (front_threshold - 1) && leftSensor < side_threshold &&  rightSensor > side_threshold)  // these are the if and else statments used to check the maze
  {
    strLastState = "RIGHT";
    turn_right(); 
  }
    else if (frontSensor >= front_threshold  &&  leftSensor > side_threshold && strLastState == "FORWARD") 
  {
    strLastState = "LEFT";
    float lSensor = reading()/10;
    if(lSensor > side_threshold){
        gofor(0.5);
        turnleft();
        gofor(1.15);
        lSensor = reading()/10;
        if(lSensor > side_threshold){
            turnleft();
            gofor(0.6);
        }

     }
    }
}
void ReadSensors() {
  float lSensor = reading()/10; 
  float fSensor = sonarfront.ping_cm(); // this is how I read the sensors
    float rSensor = sonarright.ping_cm();


  //  Set sensor values high if they are reported as zero.
  if (lSensor < 0.01) lSensor = 100.0;
  if (rSensor < 0.01) rSensor = 100.0;
  if (fSensor < 0.01) fSensor = 100.0;

  leftSensor = lSensor;
  rightSensor = rSensor;
  frontSensor = fSensor;
    // Serial.println(fSensor);
    // Serial.print(" ");
    // Serial.print(lSensor);
    // Serial.print(" ");
    // Serial.print(rSensor);
}
void forward(){
    float error = side_target -  leftSensor;
    float delta = kp*error + kd*(error - lastError); // this the PD controller used for the forward motion
    float rspeed = speed-delta;
    float lspeed = speed+delta;
    lastError = error;
    if (lspeed > speed)
        lspeed = speed;
    else if (lspeed < 0)
        lspeed = 0;
    if (rspeed > speed)
        rspeed = speed;
    else if (rspeed < 0)
        rspeed = 0;  
    analogWrite(AEN,rspeed);
    digitalWrite(APH,LOW);
    digitalWrite(BPH,HIGH);
    analogWrite(BEN, lspeed);
}
void turn_right(){
    bool bolStopRight = false;
    bool bolStopLeft = false;
    stop();
    delay(250);
    long lold = elacount;
    long rold = eracount;

    analogWrite(AEN, (turnfactor*speed));
    digitalWrite(APH, HIGH);
    analogWrite(BEN, (turnfactor*speed));
    digitalWrite(BPH, HIGH);

    while(bolStopLeft == false && bolStopRight == false){
        if (elacount-lold > 0.445*lturnadjust*countperrevolution){ // this is the code used to turn right it uses the encoders
            stopleft();
            bolStopLeft = true;
        }    
        else if (eracount-rold > 0.445*lturnadjust*countperrevolution){
            stopright(); 
            bolStopRight = true;
        }    
    }
}
void stopright()
 {
    analogWrite(AEN, 0);
    digitalWrite(APH, HIGH);
}

void stopleft()
{  
    analogWrite(BEN, 0); // stop functions
    digitalWrite(BPH, HIGH);
}
void gofor(float dis){
    bool bolStopRight = false;
    bool bolStopLeft = false;
    stop();
    delay(250);
    long lold = elacount;
    long rold = eracount;
    analogWrite(AEN,speed);
    digitalWrite(APH,LOW);
    digitalWrite(BPH,HIGH);
    analogWrite(BEN, speed);
 while(bolStopLeft == false && bolStopRight == false){ // this goses forward a set distance for the left turns
        if (elacount-lold > dis*countperrevolution){
            stop();
            bolStopLeft = true;
        }    
        else if (eracount-rold > dis*countperrevolution){
            stop(); 
            bolStopRight = true;
        }    
    }
}
void turnleft(){
    bool bolStopRight = false;
    bool bolStopLeft = false;
    stop();
    delay(250);
    long lold = elacount;
    long rold = eracount;
    analogWrite(AEN, (turnfactor*speed)); // turn left function
    digitalWrite(APH,LOW );
    analogWrite(BEN, (turnfactor*speed));
    digitalWrite(BPH, LOW);
    while(bolStopLeft == false && bolStopRight == false){
        if (elacount-lold > 0.62*rturnadjust*countperrevolution){
            stopleft();
            bolStopLeft = true;
        }    
        else if (eracount-rold > 0.62*rturnadjust*countperrevolution){
            stopright(); 
            bolStopRight = true;
        }    
    }
}

void stop()
{
    stopright(); // stop function
    stopleft();
}
void ela_down(){
  elacount++;  //interuppts
}
void era_down(){
  eracount++;
}
// analogWrite(AEN, 50);
// digitalWrite(APH,LOW);
// digitalWrite(BPH,HIGH);
// analogWrite(BEN, 50);
