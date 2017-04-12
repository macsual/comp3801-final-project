#include <stdint.h>
#include <string.h>

#include <Servo.h>

#include <LiquidCrystal.h>

#include <Key.h>
#include <Keypad.h>

#define ROWS 4
#define COLS 4

#define TRIG_PIN  8
#define ECHO_PIN  9

#define GREEN_LED 28
#define RED_LED   24
#define WARN_LED  26

#define servoPin  10

static char KEYS[ROWS][COLS] = {
    {'A', '3', '2', '1'},
    {'B', '6', '5', '4'},
    {'C', '9', '8', '7'},
    {'D', '#', '0', '*'}
};

static unsigned char ROW_PINS[ROWS] = {49, 48, 47, 46};
static unsigned char COL_PINS[COLS] = {53, 52, 51, 50};

static Keypad keypad = Keypad(makeKeymap(KEYS), ROW_PINS, COL_PINS, ROWS, COLS);

static LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

static char key;
static char PIN[10];

static int8_t ndigits;
static int8_t maxdigits;
static int8_t show_passwd;
static int8_t nattempts;
static int8_t maxattempts;

static int8_t objectInRange;
static int8_t startTimer;
static int8_t keypad_entering;
static int8_t access_granted;
static int8_t access_denied;
static int8_t lock_opened;
static long timer;
static Servo Servo1;

static void open_lock(void);
static void ultrasonic(void);
static void acceptInput(void);
static long microsecondsToCentimeters(long);

void
setup()
{
    maxdigits = 4;
    show_passwd = 1;
    maxattempts = 3;
    
    lcd.begin(16, 2);
    lcd.print("Enter PIN:");

    Serial.begin(9600);
    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(WARN_LED, OUTPUT);
    Servo1.attach(servoPin);
}

void
loop()
{
    acceptInput();
    ultrasonic();

    if (access_granted) {
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(WARN_LED, LOW);
        digitalWrite(RED_LED, LOW);
        if (!lock_opened)
            open_lock();
    }

    if (access_denied) {
        digitalWrite(RED_LED, HIGH);
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(WARN_LED, LOW);    
    }
}

static void
open_lock(void)
{
    //moves the motor one direction to open lock
    int angle;

    for (angle = 0; angle <= 180; angle++)
        Servo1.write(angle);
    
    delay(200);
    Servo1.write(1500);
    lock_opened = 1;
}

static void
ultrasonic(void)
{
    // establish variables for duration of the ping, 
    // and the distance result in inches and centimeters:
    long duration, inches, cm;
    
    // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
    // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
    pinMode(TRIG_PIN, OUTPUT);
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    // Read the signal from the sensor: a HIGH pulse whose
    // duration is the time (in microseconds) from the sending
    // of the ping to the reception of its echo off of an object.
    pinMode(ECHO_PIN, INPUT);
    duration = pulseIn(ECHO_PIN, HIGH);
    
    cm = microsecondsToCentimeters(duration);

    if (cm <= 10) {
        objectInRange = 1;
        Serial.print("Object In Range");
    } else {
        Serial.println("Out of Range");
        objectInRange = 0;
    }

    if (objectInRange && !startTimer) {
        Serial.println("Start Timer");
        digitalWrite(WARN_LED,HIGH);
        startTimer = 1;
        timer = millis();
    }

    if (startTimer) {
        if (millis() - timer >= 3000) {
            Serial.println("Alarm");
            digitalWrite(RED_LED, HIGH);
            digitalWrite(WARN_LED, LOW);
        }

        if (!objectInRange || keypad_entering) {
            // Serial.println("Hello");
            
            digitalWrite(RED_LED, LOW);
            digitalWrite(WARN_LED, LOW);
            timer = 0;
            startTimer = 0;
        }
    }
  
    Serial.print(cm);
    Serial.print("cm");
    Serial.println();
    
    delay(100);
}

static void
acceptInput(void)
{
    if (nattempts == maxattempts) {
        lcd.clear();
        lcd.print("MAX ATTEMPTS");
        lcd.setCursor(0, 1);
        lcd.print("EXCEEDED");

        nattempts = 0;
    }
    
    if (ndigits < maxdigits) {
        key = keypad.getKey();
    
        if (key) {
            PIN[ndigits] = key;
            lcd.setCursor(ndigits, 1);
            
            if (show_passwd)
                lcd.print(key);
            else
                lcd.print('*');
            
            ndigits++;   
        }
    }

    if (ndigits >= 1)
        keypad_entering = 1;
    else
        keypad_entering = 0;

    if (ndigits == maxdigits) {
        if (!memcmp(PIN, "1234", 4)) {
            nattempts = 0;
            ndigits = 0;

            lcd.clear();
            lcd.print("ACCESS GRANTED");
            access_granted = 1;
            
        } else {
            ndigits = 0;
            nattempts++;

            lcd.clear();
            lcd.print("ACCESS DENIED");
            access_denied = 1;           
        }
    }
}

static long
microsecondsToCentimeters(long usec)
{
    return usec / 29 / 2;
}

