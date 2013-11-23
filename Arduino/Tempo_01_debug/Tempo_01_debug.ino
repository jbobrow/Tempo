#include <AFMotor.h>

// Stepper Motor
AF_Stepper motor(200, 2);

//From bildr article: http://bildr.org/2012/08/rotary-encoder-arduino/

//these pins can not be changed 2/3 are special pins
int encoderPin1 = 2;
int encoderPin2 = 3;

char messageBuffer[12];
int  index = 0;

int resetPin = 5;
int modePin = 4;
boolean bMotorMode = true;

float valuePerMinute = 172/60.f;  //8;
int valuePerHour = 172;  //480;
int valuePerDay = 4128;  //11520;

int targetHour = 0;
int targetMinute = 0;

boolean bLandedOnTarget = true;
boolean bMovedForward = false;
boolean bMoving = false;

volatile int lastEncoded = 0;
volatile long encoderValue = 0;

long noonEncoderValue = 0;
long prevEncoderValue = 0;

int lastMSB = 0;
int lastLSB = 0;

void setup() {
  Serial.begin (9600);

  pinMode(encoderPin1, INPUT); 
  pinMode(encoderPin2, INPUT);

  digitalWrite(encoderPin1, HIGH); //turn pullup resistor on
  digitalWrite(encoderPin2, HIGH); //turn pullup resistor on

  //call updateEncoder() when any high/low changed seen
  //on interrupt 0 (pin 2), or interrupt 1 (pin 3) 
  attachInterrupt(0, updateEncoder, CHANGE); 
  attachInterrupt(1, updateEncoder, CHANGE);
  
  // set speed of stepper
  motor.setSpeed(60);  // XX rpm   
  motor.release();

}

void loop(){ 
  //Do stuff here
  readSerial();
  
  if(analogRead(resetPin) > 200)
    noonReset();
    
  
  if(prevEncoderValue != encoderValue){
    
    if( !bMoving ){
    
      bMovedForward = prevEncoderValue > encoderValue;
      
      Serial.print(getHourStandard());
      Serial.print(":");
      if(getMinute() < 10)
        Serial.print("0");
      Serial.print(getMinute());
      if(bMovedForward)
        Serial.println(" am");
      else
        Serial.println(" pm");
      
      prevEncoderValue = encoderValue;
    }
    // call function to deliver time to Phillip
  }
  
  
  if(analogRead(modePin) > 200)
    bMotorMode = false;
  else
    bMotorMode = true;

  if(bMotorMode){
    
    // Move the motor
    if(bLandedOnTarget) {
      motor.release();
      bMoving = false;
    }
    else {
      if( getHourStandard() != targetHour ){
        forward();
        delay(10);
        bMoving = true;
      }
      
      if( getMinute() != targetMinute ){
        forward();
        delay(20);
        bMoving = true;
      }
      
      if( getHourStandard() == targetHour && getMinute() == targetMinute )
        bLandedOnTarget = true;
        
      // only for debug
      Serial.print("target -> ");  
      Serial.print(targetHour);
      Serial.print(":");
      Serial.println(targetMinute);
      
      Serial.print("actual -> ");
      Serial.print(getHourStandard());
      Serial.print(":");
      if(getMinute() < 10)
        Serial.print("0");
      Serial.println(getMinute());
    }
  
    //forward();
    //delay(100);
    //motor.step(1000, BACKWARD, MICROSTEP); 
    //motor.onestep(BACKWARD, SINGLE);
    //delay(100);
  }
  motor.release();
}


void updateEncoder(){
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit

  int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;

  lastEncoded = encoded; //store this value for next time
}

// Serial Listener
void readSerial() {
  while(Serial.available() > 0) {
    char x = Serial.read();
    if (x == 't') index = 0;      // start
    else if (x == '\r') process(); // end
    else messageBuffer[index++] = x;
  }
}

void process() {
  int hour = 0, min = 0;
  const char *ptr = messageBuffer;
  sscanf(ptr, "%d:%d%d", &hour, &min);
  while(hour > 12){
    hour-= 12;
  }
  while(min > 59){
    min-=60;
  }
  setTime(hour, min);
}

// Reset the position of noon
void noonReset(){
  noonEncoderValue = encoderValue;
}

// Move forward
void forward(){
  motor.onestep(BACKWARD, SINGLE);
}

// Move backward
void back(){
  motor.onestep(FORWARD, SINGLE);  
}

// Check if it is AM or PM
boolean isMorning(){
  return(getHour() < 12);
}

// Set the time, the motor will drive us there
void setTime(int h, int m){
   bLandedOnTarget = false;
   targetHour = h;
   targetMinute = m;
}

// Get the hour(24), read the current time value
int getHour(){
  int hour = 0;
  while((noonEncoderValue - encoderValue) < 0 ){
    noonEncoderValue += valuePerDay;
  }
  hour = (int)((noonEncoderValue - encoderValue) % valuePerDay) / valuePerHour;
  if(hour < 0) hour = 24 + hour;
  return hour;
}

// Get the hour(12h), read the current time value
int getHourStandard(){
  int hour = 0;
  hour = getHour() % 12;
  if( hour == 0 ) hour = 12;
  return hour;
}

// Get the minute, read the current time value
int getMinute(){
  int minute = 0;
  minute = (int) ((noonEncoderValue - encoderValue) % valuePerHour) / valuePerMinute;
  if(minute < 0) minute = 60 + minute;
  return minute;
}
