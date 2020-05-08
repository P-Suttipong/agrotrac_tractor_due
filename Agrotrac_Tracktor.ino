#include "NewPing.h"
#include "I2Cdev.h"
#include "HMC5983.h"
#include "Wire.h"
#include "EasyScheduler.h"
#include "Servo.h"

HMC5983 compass;
Servo myservo;

#define Motor_Left_0    47
#define Motor_Left_1    46
#define Motor_Right_0   48
#define Motor_Right_1   49

#define Ultra_Right_TX  31
#define Ultra_Right_RX  30
#define Ultra_Mid_TX    33
#define Ultra_Mid_RX    32
#define Ultra_Left_TX   35
#define Ultra_Left_RX   34
#define MAX_Distance    200

#define encoderPinA     3
#define encoderPinB     4
#define servo           9
int encoder0Pos = 0;
int encoder0PinALast = LOW;
int n = LOW;
int track_distance = 0;

float c = -999;
int initial_compass = 0;
int compass_val = 0;
boolean setCompass = false;

NewPing Ultra_mid(Ultra_Mid_TX, Ultra_Mid_RX, MAX_Distance);
NewPing Ultra_left(Ultra_Left_TX, Ultra_Left_RX, MAX_Distance);
NewPing Ultra_right(Ultra_Right_TX, Ultra_Right_RX, MAX_Distance);

unsigned long previousMillis = 0;
const long Turn_interval = 2500;

Schedular Task1, Task2, Task3, Task4, Task5, Task6;


int ULTRA_MID = 0;
int ULTRA_LEFT = 0;
int ULTRA_RIGHT = 0;


String state = "";
int count = 0;

int value1 = 0;
int value2 = 0;
int val1_h = 0;
int val1_l = 0;
int val2_h = 0;
int val2_l = 0;


void setup() {
  pinMode(5, OUTPUT);
  Wire.begin(17711);
  Wire.onReceive(receiveEvent);

  Serial.begin(9600);
  pinMode(Motor_Left_0, OUTPUT);
  pinMode(Motor_Left_1, OUTPUT);
  pinMode(Motor_Right_0, OUTPUT);
  pinMode(Motor_Right_1, OUTPUT);
  pinMode(encoderPinA, INPUT);
  pinMode(encoderPinB, INPUT);
  myservo.attach(servo);
  myservo.write(180);
  Task1.start();
  Task2.start();
  Task3.start();
}


void loop() {
  Task1.check(mid_checker, 200);
  Task2.check(left_checker, 200);
  Task3.check(right_checker, 200);

  if (state == "Forward") {
    delay(2000);
    motorForward(value1);
  }
  else if (state == "Backward") {
    delay(2000);
    motorBackward(value1);
  }
  else if (state == "Left") {
    delay(2000);
    Turn_left(value1);
  }
  else if (state == "Right") {
    delay(2000);
    Turn_right(value1);
  }
  else if (state == "Stop") {
    Motor_Left_driver(0, 0);
    Motor_Right_driver(0, 0);
  }
  else if (state == "Up"){
    myservo.write(180);
  }
  else if (state == "Down"){
    myservo.write(0);
  }
  else {
    Motor_Left_driver(0, 0);
    Motor_Right_driver(0, 0);
  }
}


void motorForward(int dist) {
  while ((track_distance < dist) && (ULTRA_MID > 15)){
    encoder();
    Motor_Left_driver(1, 130);
    Motor_Right_driver(1, 255);
    Serial.print("Track : ");
    Serial.print(track_distance);
    Serial.print(" / ");
    Serial.println(dist);
  }
  track_distance = 0;
  encoder0Pos = 0;
  state = "Stop";
}

void motorBackward(int dist) {
  while (track_distance > dist) {
    encoder();
    Motor_Left_driver(0, 255);
    Motor_Right_driver(0, 255);
  }
  track_distance = 0;
  encoder0Pos = 0;
  state = "Stop";
}

void mid_checker() {
  ULTRA_MID = Ultra_mid.ping_cm();
  //Serial.print("MID : ");
  //Serial.print(ULTRA_MID);
}

void left_checker() {
  ULTRA_LEFT = Ultra_left.ping_cm();
  //Serial.print("    LEFT : ");
  //Serial.print(ULTRA_LEFT);
}

void right_checker() {
  ULTRA_RIGHT = Ultra_right.ping_cm();
  //Serial.print("    RIGHT : ");
  //Serial.println(ULTRA_RIGHT);
}

int Motor_Left_driver(int Direct, int Speed) {
  if (Direct == 1) {
    analogWrite(Motor_Left_0, Speed);
    analogWrite(Motor_Left_1, 0);
  }
  else {
    analogWrite(Motor_Left_0, 0);
    analogWrite(Motor_Left_1, Speed);
  }
}

int Motor_Right_driver(int Direct, int Speed) {
  if (Direct == 1) {
    analogWrite(Motor_Right_0, Speed);
    analogWrite(Motor_Right_1, 0);

  }
  else {
    analogWrite(Motor_Right_0, 0);
    analogWrite(Motor_Right_1, Speed);
  }
}


void Turn_left(int degree) {
  while ( (abs(compass_val - initial_compass) < degree) && (ULTRA_LEFT > 15)) {
    Motor_Left_driver(0, 255);
    Motor_Right_driver(1, 255);
  }
  state = "Stop";
  initial_compass = 0;
  compass_val = 0;
}


void Turn_right(int degree) {
  while ( (abs(compass_val - initial_compass) < degree) && (ULTRA_RIGHT > 15)) {
    Motor_Left_driver(1, 255);
    Motor_Right_driver(0, 255);
  }
  state = "Stop";
  initial_compass = 0;
  compass_val = 0;
}

void encoder() {
  delay(1);
  n = digitalRead(encoderPinA);
  if ((encoder0PinALast == LOW) && (n == HIGH)) {
    if (digitalRead(encoderPinB) == LOW) {
      encoder0Pos--;
    } else {
      encoder0Pos++;
    }
    // Serial.println (encoder0Pos);
    track_distance = encoder0Pos;
    //Serial.print("Distance in function : ");
    //Serial.println(distance);
  }
  encoder0PinALast = n;
  //track_distance = distance;
}

void receiveEvent( int bytes )
{
  int x = Wire.read();
    Serial.print("Command: ");
    Serial.println(x);
  if (x == 0) {
    state = "Stop";
  }
  else if (x == 1) {
    state = "Forward";
    Wire.onReceive(receiveEvent2);
  }
  else if (x == 2) {
    state = "Backward";
    Wire.onReceive(receiveEvent2);
  }
  else if (x == 3) {
    state = "Left";
    setCompass = true;
    Wire.onReceive(receiveEvent2);
  }
  else if (x == 4) {
    state = "Right";
    setCompass = true;
    Wire.onReceive(receiveEvent2);
  }
  else if (x == 5) {
    state = "Area";
    Wire.onReceive(receiveEvent2);
  }
  else if (x == 6) {
    state = "Setting";
    Wire.onReceive(receiveEvent2);
  }
  else if (x == 7) {
    state = "Start";
    setCompass = true;
  }
  else if (x == 8) {
    state = "Up";
  }
  else if (x == 9) {
    state = "Down";
  }
  else {
    compass_val = x;
    Serial.print("Compass : ");
    Serial.println(compass_val);
  }
}

void receiveEvent2( int bytes )
{
  int y = Wire.read();
  val1_h = y;
  if (setCompass) {
    initial_compass = y;
    Serial.print("Initial : ");
    Serial.println(compass_val);
    setCompass = false;
  }
  //    Serial.print("Value 1 High: ");
  //    Serial.println(y);
  Wire.onReceive(receiveEvent3);
}

void receiveEvent3( int bytes )
{
  int z = Wire.read();
  val1_l = z;
  value1 = (val1_h * 256) + val1_l;
  //  Serial.print("Value 1 Low : ");
  //  Serial.println(z);
  Serial.print("Value 1: ");
  Serial.println(value1);
  if (state == "Area" || state == "Setting") {
    Wire.onReceive(receiveEvent4);
  } else {
    Wire.onReceive(receiveEvent);
  }
}

void receiveEvent4( int bytes )
{
  int m = Wire.read();
  val2_h = m;
  //  Serial.print("Value 2 High: ");
  //  Serial.println(m);
  Wire.onReceive(receiveEvent5);
}

void receiveEvent5( int bytes )
{
  int n = Wire.read();
  val2_l = n;
  value2 = (val2_h * 256) + val2_l;
  //  Serial.print("Value 2 Low : ");
  //  Serial.println(n);
  //  Serial.print("Value 2: ");
  //  Serial.println(value2);
  Wire.onReceive(receiveEvent);
}
