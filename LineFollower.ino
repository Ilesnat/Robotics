#include <QTRSensors.h>
#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <Dabble.h>
#include <VL6180X.h>
#include <Wire.h>
const int lpwm = 45;
const int rpwm = 46;
const int ldir = 41;
const int rdir = 42;
VL6180X vl;
bool turn = false;

//  This code uses with eight analog QTR sensors connected to analog pins A0 to A7.
//  Two emitter pins are specicifed by calling setEmitterPins().
//
//  Calibration is setup to run for 10 seconds.  During this phase, you should expose
//  each reflectance sensor to the lightest and darkest readings they will encounter.
//  For example, if you are making a line follower, you should slide the sensors across
//  the line during the calibration phase so that each sensor can get a reading of how
//  dark the line is and how light the ground is. Improper calibration will result in
//  poor readings.
//
//  The loop() routine the calibrated sensor values and uses them to estimate the position
//  of a line. You can test this by taping a piece of 3/4" black electrical tape to a piece
//  of white paper and sliding the sensor across it.  The sensor values range from
//  0 (maximum reflectance) to 1000 (minimum reflectance) followed
//  by the estimated location of the line as a number from 0 to 5000. 1000 means
//  the line is directly under sensor 1, 2000 means directly under sensor 2,
//  etc. 0 means the line is directly under sensor 0 or was last seen by sensor
//  0 before being lost. 5000 means the line is directly under sensor 5 or was
//  last seen by sensor 5 before being lost.
//
QTRSensors qtr;
const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];
bool gStartMaze = false;
double x = 1.2;
double y = 1.325;
bool okay = false;
double left;
double right;
int P = 1;


//**** the way i setup the VL6180x sensor was through the GPIO interrupt pin. However, this is not a true interuppt.
// I never connected the pin to the Arduino. All the setup for the snesor is doing is checking the GPIO pin
//if something has been stored on the interuppt pin.
//it still has to check the pin every loop or two. Its only checking for a status.
//If a reading has been stored it reads the pin not untill then.
// This will speed up my bot quite well becasue a reading will never be stored in the pin untill something is within 20cm which is almost never
// on our maze. This is a easier way to get around messing with interrupt but it is still able to save time.
bool sensorReady()
{
  return ((vl.readReg(VL6180X::RESULT__INTERRUPT_STATUS_GPIO) & 0x04) != 0); // this si checking th estatus of the pin to see if a reading is avaiable
}
uint8_t readRange()
{
  uint8_t reading = vl.readReg(VL6180X::RESULT__RANGE_VAL); // if the reading is avaible i run this in the void loop. It reads the pin and then clears the interuppt
  vl.writeReg(VL6180X::SYSTEM__INTERRUPT_CLEAR, 0x01);
  return reading;
}
uint8_t reading()
{
  return readRange() * vl.getScaling(); // this scales the bove reading to what i want  - default
}
void setup()
{
  pinMode(rpwm, OUTPUT);
  pinMode(rpwm, OUTPUT);
  pinMode(ldir, OUTPUT);
  pinMode(rdir, OUTPUT);
  Wire.begin();
  vl.init();
  vl.configureDefault();
  vl.setTimeout(500);
  vl.stopContinuous();
  delay(250);
  vl.writeReg(VL6180X::SYSTEM__MODE_GPIO1, 0x10);      // enable GPIO1 pin on VL6180X
  vl.writeReg(VL6180X::SYSTEM__INTERRUPT_CLEAR, 0x03); // intially clears the pin
  vl.startRangeContinuous(100);
  Serial.begin(115200); // Set your Serial Monitor is set at 250000
  Dabble.begin(9600);   // This is the baude rate of the HM-10
  qtr.setTypeRC();
  qtr.setSensorPins((const uint8_t[]){A0, A1, A2, A3, A4, A5, A6, A7}, SensorCount);
  //qtr.setEmitterPins(34,35);
}

void loop()
{
  Dabble.processInput();
  if (sensorReady())
  {
    while (reading() < 50) // this is how i check the 5 cm rule if it is it stops my motors
    {
      analogWrite(lpwm, 0);
      analogWrite(rpwm, 0);
      turn == true;
      gStartMaze == false;
      okay = false;
    }
  }
  if (turn == true)
  {
    gStartMaze == true; // once it has been released it turns everything back on
    turn = false;
  }

  if (gStartMaze == false && GamePad.isSelectPressed() == 1)
  {
    CalibrateSensorArray();
  }
  else if (GamePad.isCrossPressed() == 1)
  {
    analogWrite(lpwm, 0); //0 //stops everything if cross is pressed
    analogWrite(rpwm, 0); //0
    gStartMaze = false;
    okay = false;
  }
  if (GamePad.isStartPressed() == 1)
  {
    gStartMaze = true; //start is pressed it runs the line
  }
  if (gStartMaze == true)
  {
    // read calibrated sensor values and obtain a measure of the line position
    // from 0 to 7000 (for a white line, use readLineWhite() instead)
    uint16_t position = qtr.readLineBlack(sensorValues);
    if (position >= 0 && position <= 500) // this si the line follwer i used it was the one provided i just played with the duty cycle slightly
    {


      analogWrite(lpwm, 0.30 * 255.0); 
      digitalWrite(ldir, HIGH);
      analogWrite(rpwm, 0.15 * 255.0); 
      digitalWrite(rdir, HIGH);        
    }
    else if (position > 500 && position <= 1000)
    {

  
      analogWrite(lpwm, 0.30 * 255.0); 
      digitalWrite(ldir, HIGH);
      analogWrite(rpwm, 0.09 * 255.0); 
      digitalWrite(rdir, HIGH);        
    }
    else if (position > 1000 && position <= 1500)
    {

      analogWrite(lpwm, x * 0.30 * 255.0);
      digitalWrite(ldir, HIGH);
      analogWrite(rpwm, 0); 
      digitalWrite(rdir, LOW);
    }
    else if (position > 1500 && position <= 2000)
    {

      // State 4
      analogWrite(lpwm, 0.30 * 255.0); 
      digitalWrite(ldir, HIGH);
      analogWrite(rpwm, 0.09 * 255.0); 
      digitalWrite(rdir, LOW);
    }
    else if (position > 2000 && position <= 2500)
    {

      // State 5
      analogWrite(lpwm, x * 0.30 * 255.0);
      digitalWrite(ldir, HIGH);
      analogWrite(rpwm, x * 0.15 * 255.0); 
      digitalWrite(rdir, LOW);
    }
    else if (position > 2500 && position <= 3000)
    {


      analogWrite(lpwm, x * 0.30 * 255.0); 
      digitalWrite(ldir, HIGH);
      analogWrite(rpwm, x * 0.22 * 255.0); 
      digitalWrite(rdir, LOW);
    }
    else if (position > 3000 && position <= 3500)
    {

     
      analogWrite(lpwm, y * x * 0.30 * 255.0);
      digitalWrite(ldir, HIGH);
      analogWrite(rpwm, y * x * 0.30 * 255.0);
      digitalWrite(rdir, LOW);
    }
    else if (position > 3500 && position <= 4000)
    {

     
      analogWrite(lpwm, y * x * 0.30 * 255.0); 
      digitalWrite(ldir, HIGH);
      analogWrite(rpwm, y * x * 0.30 * 255.0); 
      digitalWrite(rdir, LOW);
    }
    else if (position > 4000 && position <= 4500)
    {

    
      analogWrite(lpwm, x * 0.22 * 255.0); 
      digitalWrite(ldir, HIGH);
      analogWrite(rpwm, x * 0.30 * 255.0); 
      digitalWrite(rdir, LOW);
    }
    else if (position > 4500 && position <= 5000)
    {

    
      analogWrite(lpwm, x * 0.15 * 255.0); 
      digitalWrite(ldir, HIGH);
      analogWrite(rpwm, x * 0.30 * 255.0); 
      digitalWrite(rdir, LOW);
    }
    else if (position > 5000 && position <= 5500)
    {

    
      analogWrite(lpwm, x * 0.09 * 255.0); 
      digitalWrite(ldir, HIGH);
      analogWrite(rpwm, x * 0.30 * 255.0); 
      digitalWrite(rdir, LOW);
    }
    else if (position > 5500 && position <= 6000)
    {

     
      analogWrite(lpwm, 0); 
      digitalWrite(ldir, HIGH);
      analogWrite(rpwm, x * 0.30 * 255.0); 
      digitalWrite(rdir, LOW);
    }
    else if (position > 6000 && position <= 6500)
    {

    
      analogWrite(lpwm, 0.09 * 255.0); 
      digitalWrite(ldir, LOW);         
      analogWrite(rpwm, 0.30 * 255.0); 
      digitalWrite(rdir, LOW);
    }
    else if (position > 6500 && position <= 7000)
    {
      // State 14
      analogWrite(lpwm, 0.15 * 255.0);
      digitalWrite(ldir, LOW);        
      analogWrite(rpwm, 0.30 * 255.0);
      digitalWrite(rdir, LOW);
    }
  }
  if (GamePad.isCirclePressed() == 1 || okay == true)
  {
    okay = true;
    left = 30; //this is the section i press when i get to the Jags. It just makes the robot go really slow
                // and uses a dumbed down line follower.
    right = 30;
    gStartMaze = false;
    qtr.read(sensorValues);
    if (sensorValues[0] >= 1500)
    {
      left += 15;
      right -= 15;
    }
    if (sensorValues[1] >= 1500) //this line follower checks each sensor rather than average
    {
      left += 10;
      right -= 5;
    }
    if (sensorValues[2] >= 1500)
    {
      left += 10;
    }
    if (sensorValues[7] >= 1500)
    {
      right += 15;
      left -= 15;
    }
    if (sensorValues[6] >= 1500)
    {
      right += 10;
      left -= 5;
    }
    if (sensorValues[5] >= 1500)
    {
      right += 10;
    }
    analogWrite(lpwm, left);
    digitalWrite(ldir, HIGH);
    analogWrite(rpwm, right);
    digitalWrite(rdir, LOW);
    int ilovebrockoff = sensorValues[0];
    for (int i = 0; i < 8; i++)
    {
      if (sensorValues[i] > ilovebrockoff) // this is used to check for the max sensor. it checks if I have fallen off a line
      {
        ilovebrockoff = sensorValues[i];
      }
    }
    if(ilovebrockoff < 1400 && P==1)
    {
      analogWrite(lpwm, 30);
      digitalWrite(ldir, HIGH);
      analogWrite(rpwm, 30);
      digitalWrite(rdir, HIGH);
    }
    else if (ilovebrockoff >1500 && P ==1 && sensorValues[4] >= 1500 ){ // based on that value I turn my robot back and forth untill it finds the line again
      P =0;
      ilovebrockoff = 1400;
    }
    if(ilovebrockoff < 1400 && P==0 )
    {
      analogWrite(lpwm, 30);
      digitalWrite(ldir, LOW);
      analogWrite(rpwm, 30);
      digitalWrite(rdir, LOW);
    }
    else if(ilovebrockoff >1500 && P ==0 && sensorValues[5] >= 1500){
      P =1;
      ilovebrockoff =1400;
    }
  }
}

void CalibrateSensorArray()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // turn on Arduino's LED to indicate we are in calibration mode
  // analogRead() takes about 0.1 ms on an AVR.
  // 0.1 ms per sensor * 4 samples per sensor read (default) * 8 sensors
  // * 10 reads per calibrate() call = ~32 ms per calibrate() call.
  // Call calibrate() 400 times to make calibration take about 13 seconds.
  for (uint16_t i = 0; i < 400; i++)
  {
    qtr.calibrate();
    analogWrite(lpwm, 50);
    analogWrite(rpwm, 50);
    if (i < 195)
    {
      digitalWrite(ldir, LOW); // this runs the calibration and turns my robot back and forth at the halve way point
      digitalWrite(rdir, LOW);
    }
    else if (i > 200)
    {
      digitalWrite(ldir, HIGH);
      digitalWrite(rdir, HIGH);
    }
  }
  analogWrite(lpwm, 0);
  analogWrite(rpwm, 0);
  digitalWrite(LED_BUILTIN, LOW); // turn off Arduino's LED to indicate we are through with calibration
}
