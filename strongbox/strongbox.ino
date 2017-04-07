const int trigPin = 2;
const int echoPin = 4;
const int ledPin = 6;
const int redPin = 7;
boolean objectInRange = false;
boolean startTimer = false;
long timer;

void setup() {
  // initialize serial communication:
  Serial.begin(9600);
  pinMode(ledPin,OUTPUT);
  pinMode(redPin,OUTPUT);
}

void loop()
{
  // establish variables for duration of the ping, 
  // and the distance result in inches and centimeters:
  long duration, inches, cm;

  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);

  cm = microsecondsToCentimeters(duration);

  if(cm<=10)
  {
    objectInRange = true;
    Serial.print("Object In Range");
  }else{
    Serial.println("Out of Range");
    objectInRange = false;
  }

  if(objectInRange && !startTimer)
  {
    digitalWrite(ledPin,HIGH);
    startTimer = true;
    timer= millis();
   
  }

  if(startTimer)
  {
    if(millis()-timer>=3000)
    {
        Serial.println("Alarm");
        digitalWrite(redPin,HIGH);
        digitalWrite(ledPin,LOW);
    }

    if(!objectInRange)
    {
        digitalWrite(redPin,LOW);
        digitalWrite(ledPin,LOW);
        timer = 0;
        startTimer = false;
    }

  }

  
  
  Serial.print(cm);
  Serial.print("cm");
  Serial.println();
  
  delay(100);
}


long microsecondsToCentimeters(long microseconds)
{
  return microseconds / 29 / 2;
}
