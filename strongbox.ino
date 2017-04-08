#include <stdint.h>
#include <string.h>

#include <LiquidCrystal.h>

#include <Key.h>
#include <Keypad.h>

#define ROWS 4
#define COLS 4

static char KEYS[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

static unsigned char ROW_PINS[ROWS] = {5, 4, 3, 2};
static unsigned char COL_PINS[COLS] = {9, 8, 7, 6};

static Keypad keypad = Keypad(makeKeymap(KEYS), ROW_PINS, COL_PINS, ROWS, COLS);

static LiquidCrystal lcd(47, 49, 23, 24, 25, 26);

static char key;
static char PIN[10];

static int8_t ndigits;
static int8_t maxdigits;
static int8_t show_passwd;
static int8_t nattempts;
static int8_t maxattempts;

void
setup(void)
{
    maxdigits = 4;
    show_passwd = 0;
    maxattempts = 3;
    
    lcd.begin(16, 2);
    lcd.print("Enter PIN:");
}

void
loop(void)
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

    if (ndigits == maxdigits) {
        if (!memcmp(PIN, "1234", 4)) {
            nattempts = 0;
            ndigits = 0;

            lcd.clear();
            lcd.print("ACCESS GRANTED");
        } else {
            ndigits = 0;
            nattempts++;

            lcd.clear();
            lcd.print("ACCESS DENIED");
        }
    }
}
