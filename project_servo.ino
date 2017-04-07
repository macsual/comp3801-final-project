#include<Servo.h>

int servoPin = 9;
Servo Servo1;

int access;
int idle;

void setup() {
  // put your setup code here, to run once:
  Servo1. attach(servoPin);
  access = true;
  idle = false; //start or stops the motor loop

}

void loop() {
  // put your main code here, to run repeatedly:
//  delay(500);

 if (idle == false){

    if (access == true){
    openLock();
    delay(1000);
    Servo1.write(1500);
    
    }
    
    access = false;
  
    delay(3000);
    
   if (access == false){
    closeLock();
    delay(1000);
    Servo1.write(1500);
    
   }

   idle = true;
  
 }
 
  
}


void openLock(){
  //moves the motor one direction to open lock
  int angle;
    for (angle = 0;angle<=180;angle++){
      Servo1.write(angle);
    }
}

void closeLock(){
  //moves the motor one direction to close the lock
  int angle;
  for (angle = 180;angle>=0;angle--){
      Servo1.write(angle);
    }
}




/*
Adafruit Arduino - Lesson 14. Sweep
*/

//#include <Servo.h> 
//
//int servoPin = 9;
// 
//Servo servo;  
// 
//int angle = 0;   // servo position in degrees 
// 
//void setup() 
//{ 
//  servo.attach(servoPin); 
//} 
// 
// 
//void loop() 
//{ 
//  // scan from 0 to 180 degrees
//  for(angle = 0; angle < 180; angle++)  
//  {                                  
//    servo.write(angle);               
//    delay(15);                   
//  } 
//  // now scan back from 180 to 0 degrees
//  for(angle = 180; angle > 0; angle--)    
//  {                                
//    servo.write(angle);           
//    delay(15);       
//  } 
//} 
