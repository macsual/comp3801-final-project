#include <stdio.h>  /* snprintf */
#include <stdint.h>
#include <string.h> /* memcmp, strlen */

#include <Key.h>
#include <Servo.h>

#include "Adafruit_FONA.h"
#include "LiquidCrystal.h"
#include "Keypad.h"

#define BAUD_RATE 9600

/*
 * Width of LCD row
 * 
 * The maximum length of PIN used for authentication.
 */
#define MAX_PIN_LEN 16

/* keypad */

#define KP_ROWS    4
#define KP_COLS    4

#define KP_PIN_8  49
#define KP_PIN_7  48
#define KP_PIN_6  47
#define KP_PIN_5  46
#define KP_PIN_4  53
#define KP_PIN_3  52
#define KP_PIN_2  51
#define KP_PIN_1  50

/* ultrasonic sensor */

#define TRIG_PIN  8
#define ECHO_PIN  9

/* LED */

#define GREEN_LED   28
#define RED_LED     24
#define WARN_LED    26

/* motor */
#define SERVO_PIN  10

/* LCD */

#define LCD_RS_PIN  12
#define LCD_EN_PIN  11
#define LCD_D4_PIN   5
#define LCD_D5_PIN   4
#define LCD_D6_PIN   3
#define LCD_D7_PIN   2

/* Adafruit FONA (GSM modem) */

#define FONA_SERIAL   Serial3
#define FONA_RST_PIN  7

/* email notifications */

#define SMTP_SRV        "smtp.gmail.com"
#define SMTP_PORT       "465"
#define SMTP_USERNAME   "webmaster@example.com"
#define SMTP_PASSWORD   "password"

#define AUTH_PASS           100
#define AUTH_FAIL           200
#define AUTH_MAX_ATTEMPTS   300
#define AUTH_TIMEOUT        400

static Adafruit_FONA fona = Adafruit_FONA(FONA_RST_PIN);

static unsigned char KP_ROW_PINS[KP_ROWS] = {KP_PIN_8, KP_PIN_7, KP_PIN_6, KP_PIN_5};
static unsigned char KP_COL_PINS[KP_COLS] = {KP_PIN_4, KP_PIN_3, KP_PIN_2, KP_PIN_1};
static char KP_KEYS[KP_ROWS][KP_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
static Keypad keypad = Keypad(makeKeymap(KP_KEYS), KP_ROW_PINS, KP_COL_PINS, KP_ROWS, KP_COLS);

static LiquidCrystal lcd(LCD_RS_PIN, LCD_EN_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);

static char PIN[MAX_PIN_LEN];

static int8_t ndigits;
static int8_t maxdigits;
static int8_t nattempts;
static int8_t maxattempts;

/* flags */
static int8_t show_passwd;
static int8_t object_in_range;
static int8_t start_timer;
static int8_t keypad_entering;
static int8_t access_granted;
static int8_t access_denied;
static int8_t lock_open;
static int8_t waiting_on_close_cmd;

static Servo Servo1;

static void open_lock(void);
static void close_lock(void);

static void ultrasonic(void);
static void poll_keypad(void);
static unsigned long usec_to_centimeters(unsigned long);
static int8_t notify(int8_t);
static int8_t send_email(const char *subject, const char *body, size_t len);

void
setup()
{
    maxdigits = 4;
    show_passwd = 0;
    maxattempts = 3;

    lcd.begin(16, 2);
    lcd.print("Initialising ...");

    Serial.begin(BAUD_RATE);

    DDRA |= B01010100;  /* set LED pins for ouput */
    
    Servo1.attach(SERVO_PIN);

    FONA_SERIAL.begin(BAUD_RATE);

    if (!fona.begin(FONA_SERIAL)) {
        Serial.println("Couldn't find FONA");
        for (;;);
    }

    fona.setGPRSNetworkSettings(F("ppinternet"));
    Serial.println("delay start");
    delay(10000);
    Serial.println("delay stop");

    Serial.println("enabling GPRS");
    while (!fona.enableGPRS(true));
    Serial.println("GPRS enabled");

    lcd.clear();
    lcd.print("Enter PIN:");
}

void
loop()
{
    poll_keypad();
    
    ultrasonic();

    if (access_granted) {
        access_granted = 0;
        waiting_on_close_cmd = 1;

        lcd.clear();
        lcd.print("ACCESS GRANTED");
        lcd.setCursor(0, 1);
        lcd.print("Press # to lock");

        PORTA = B01000000;  /* turn on green LED */

        if (!lock_open)
            open_lock();

        notify(AUTH_PASS);
    }

    if (access_denied) {
        access_denied = 0;

        lcd.clear();
        lcd.print("ACCESS DENIED");

        PORTA = B00000100;  /* turn on red LED */

        notify(AUTH_FAIL);

        delay(3000);

        lcd.clear();
        lcd.print("Enter PIN:");
    }

    if (nattempts == maxattempts) {
        lcd.clear();
        lcd.print("MAX ATTEMPTS");
        lcd.setCursor(0, 1);
        lcd.print("EXCEEDED");

        notify(AUTH_MAX_ATTEMPTS);

        nattempts = 0;
    }
}

/* moves the motor unidirectionally to open the lock */
static void
open_lock(void)
{
    int angle;

    for (angle = 180; angle >= 0; angle--)
        Servo1.write(angle);
    
    delay(200);
    Servo1.write(1500);
    
    lock_open = 1;
}

/* moves the motor unidirectionally to close the lock */
static void
close_lock(void)
{
    int angle;
 
    for (angle = 0; angle <= 180; angle++)
        Servo1.write(angle);

    delay(200);
    Servo1.write(1500);
    
    lock_open = 0;
}

static void
ultrasonic(void)
{
    unsigned long duration; /* ping duration */
    unsigned long distance; /* object distance in centimeters */
    static unsigned long timer;
    
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

    distance = usec_to_centimeters(duration);

    Serial.print(distance);
    Serial.println(" cm");

    if (distance <= 10) {
        object_in_range = 1;
        Serial.println("Object In Range");
    } else {
        Serial.println("Out of Range");
        object_in_range = 0;
    }

    /* someone is around the safe and a timer will begin until they walk away or start entering a PIN */
    if (object_in_range && !start_timer) {
        Serial.println("Start Timer");

        PORTA = B00010000;  /* turn on yellow LED */

        start_timer = 1;
        timer = millis();
    }

    if (start_timer) {
        /* someone is loitering around the safe */
        if (millis() - timer >= 60000) {
            notify(AUTH_TIMEOUT);

            PORTA = B00000100;  /* turn on red LED */

            Serial.println("Alarm");
        }

        /* person standing around the safe walked away or they started entering a PIN */
        if (!object_in_range || keypad_entering) {
            timer = 0;
            start_timer = 0;

            PORTA = B00000000;  /* turn off all LEDs */
        }
    }
}

static void
poll_keypad(void)
{
    char key;

    if (waiting_on_close_cmd) {
        key = keypad.getKey();

        if (key) {
            if (key == '#') {
                waiting_on_close_cmd = 0;
                close_lock();

                lcd.clear();
                lcd.print("Enter PIN:");

                return;
            }
        }
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
            access_granted = 1;
            nattempts = 0;
            ndigits = 0;
        } else {
            access_denied = 1;
            ndigits = 0;
            nattempts++;
        }
    }
}

static int8_t
notify(int8_t event)
{
    char buf[23];
    const char *subject;
    char body[128];
    int body_len;

     if (!fona.getTime(buf, sizeof buf))
        Serial.println(F("Failed to get time"));
    else
        Serial.println(buf);

    switch (event) {
    case AUTH_PASS:
        subject = "Successful PIN unlock";
        body_len = snprintf(body, sizeof body, "Strongbox authentication success at %s", buf);
        break;

    case AUTH_FAIL:
        subject = "Unsuccessful PIN unlock";
        body_len = snprintf(body, sizeof body, "Strongbox authentication failure at %s", buf);
        break;

    case AUTH_MAX_ATTEMPTS:
        subject = "Number of PIN unlock attempts exceeded";
        body_len = snprintf(body, sizeof body, "Strongbox authentication failure after too many attempts at %s", buf);
        break;

    case AUTH_TIMEOUT:
        subject = "PIN unlock attempt timeout exceeded";
        body_len = snprintf(body, sizeof body, "Strongbox authentication failure after PIN entry time limit exceeded at %s", buf);
        break;

    default:
        break;
    }

    return send_email(subject, body, body_len);
}

static int8_t
send_email(const char *subject, const char *body, size_t len)
{
    if (!fona.sendCheckReply(F("AT+EMAILCID=1"), F("OK")))
        return -1;

    if (!fona.sendCheckReply(F("AT+EMAILTO=30"), F("OK")))
        return -1;

    if (!fona.sendCheckReply(F("AT+EMAILSSL=1"), F("OK")))
        return -1;

    if (!fona.sendCheckReply(F("AT+SMTPSRV=\"" SMTP_SRV "\"," SMTP_PORT), F("OK")))
        return -1;

    if (!fona.sendCheckReply(F("AT+SMTPAUTH=1,\"" SMTP_USERNAME "\",\"" SMTP_PASSWORD "\""), F("OK")))
        return -1;

    if (!fona.sendCheckReply(F("AT+SMTPFROM=\"" SMTP_USERNAME "\",\"Strongbox\""), F("OK")))
        return -1;

    if (!fona.sendCheckReply(F("AT+SMTPRCPT=0,0,\"webmaster@example.com\""), F("OK")))
        return -1;
    
    fona.print(F("AT+SMTPSUB=\""));
    fona.print(subject);
    fona.println("\"");

    if (!fona.expectReply(F("OK")))
        return -1;

    fona.print(F("AT+SMTPBODY="));
    fona.println(len);

    if (!fona.expectReply(F("DOWNLOAD")))
        return -1;

    fona.println(body);

    if (!fona.expectReply(F("OK")))
        return -1;

    if (!fona.sendCheckReply(F("AT+SMTPSEND"), F("OK")))
        return -1;

    return 0;
}

static unsigned long
usec_to_centimeters(unsigned long usec)
{
    return usec / 29 / 2;
}

