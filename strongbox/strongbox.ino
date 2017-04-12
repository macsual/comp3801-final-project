#include <stdint.h>
#include <string.h>

#include <Servo.h>

#include <LiquidCrystal.h>

#include <Key.h>
#include <Keypad.h>

#include "Adafruit_FONA.h"

/* width of LCD row */
#define MAX_PIN_LEN 16

#define ROWS 4
#define COLS 4

#define TRIG_PIN  8
#define ECHO_PIN  9

#define GREEN_LED 28
#define RED_LED   24
#define WARN_LED  26

#define SERVO_PIN  10

#define LCD_RS_PIN  12
#define LCD_EN_PIN  11
#define LCD_D4_PIN  5
#define LCD_D5_PIN  4
#define LCD_D6_PIN  3
#define LCD_D7_PIN  2

#define FONA_RX   14
#define FONA_TX   15
#define FONA_RST  7

#define URL "comp3801-final-project-macsual.c9users.io/cgi-bin/notify.py"

static char buf[255];

static HardwareSerial *fonaSerial = &Serial3;

static Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

static char KEYS[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

static unsigned char ROW_PINS[ROWS] = {49, 48, 47, 46};
static unsigned char COL_PINS[COLS] = {53, 52, 51, 50};

static Keypad keypad = Keypad(makeKeymap(KEYS), ROW_PINS, COL_PINS, ROWS, COLS);

static LiquidCrystal lcd(LCD_RS_PIN, LCD_EN_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);

static char key;
static char PIN[MAX_PIN_LEN];

static int8_t ndigits;
static int8_t maxdigits;
static int8_t show_passwd;
static int8_t nattempts;
static int8_t maxattempts;

static int8_t object_in_range;
static int8_t start_timer;
static int8_t keypad_entering;
static int8_t access_granted;
static int8_t access_denied;
static int8_t lock_opened;
static long timer;
static Servo Servo1;

static void open_lock(void);
static void close_lock(void);

static void ultrasonic(void);
static void accept_input(void);
static long microseconds_to_centimeters(long);
static void notify_server(const char *);

void
setup()
{
    maxdigits = 4;
    show_passwd = 1;
    maxattempts = 3;

    lcd.begin(16, 2);
    lcd.print("Initialising ...");

    Serial.begin(9600);
    
    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(WARN_LED, OUTPUT);
    
    Servo1.attach(SERVO_PIN);

    fonaSerial->begin(9600);

    if (!fona.begin(*fonaSerial)) {
        Serial.println("Couldn't find FONA");
        for (;;);
    }

    fona.setGPRSNetworkSettings(F("ppinternet"));
    Serial.println("delay start");
    delay(10000);
    Serial.println("delay stop");

    Serial.println("enabling GPRS");
    while(!fona.enableGPRS(true));
    Serial.println("GPRS enabled");

    lcd.clear();
    lcd.print("Enter PIN:");
}

void
loop()
{
    accept_input();
    
    ultrasonic();

    if (access_granted) {
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(WARN_LED, LOW);
        digitalWrite(RED_LED, LOW);
        if (!lock_opened)
            open_lock();

        notify_server("pass");
    }

    if (access_denied) {
        digitalWrite(RED_LED, HIGH);
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(WARN_LED, LOW);

        notify_server("fail");
    }
}

static void
open_lock(void)
{
    //moves the motor one direction to open lock
    int angle;

    for (angle = 180;angle>=0;angle--)
        Servo1.write(angle);
    
    delay(200);
    Servo1.write(1500);
    lock_opened = 1;
}

static void
close_lock(void) {
  //moves the motor one direction to close the lock
  int angle;
 
  for ( angle = 0;angle<=180;angle++){
      Servo1.write(angle);
    }

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
    
    cm = microseconds_to_centimeters(duration);

    if (cm <= 10) {
        object_in_range = 1;
        Serial.print("Object In Range");
    } else {
        Serial.println("Out of Range");
        object_in_range = 0;
    }

    if (object_in_range && !start_timer) {
        Serial.println("Start Timer");
        digitalWrite(WARN_LED, HIGH);
        start_timer = 1;
        timer = millis();
    }

    if (start_timer) {
        if (millis() - timer >= 3000) {
            Serial.println("Alarm");
            digitalWrite(RED_LED, HIGH);
            digitalWrite(WARN_LED, LOW);
        }

        if (!object_in_range || keypad_entering) {
            // Serial.println("Hello");
            
            digitalWrite(RED_LED, LOW);
            digitalWrite(WARN_LED, LOW);
            timer = 0;
            start_timer = 0;
        }
    }
  
    Serial.print(cm);
    Serial.print("cm");
    Serial.println();
    
    delay(100);
}

static void
accept_input(void)
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
        if (!memcmp(PIN, "1234", maxdigits)) {
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

static void
notify_server(const char *data)
{
    uint16_t statuscode;
    int16_t length;

    if (!fona.HTTP_POST_start(URL, F("text/plain"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *) &length)) {
        Serial.println("Failed!");
        return;
    }

    fona.HTTP_POST_end();
}

static long
microseconds_to_centimeters(long usec)
{
    return usec / 29 / 2;
}

