#include <stdint.h> /* uint8_t */

#include <Key.h>
#include <Keypad.h>

#define ROWS 4
#define COLS 4

static const char KEYS[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

static const uint8_t ROW_PINS[ROWS] = {5, 4, 3, 2};
static const uint8_t COL_PINS[COLS] = {9, 8, 7, 6};

static Keypad keypad = Keypad(makeKeymap(KEYS), ROW_PINS, COL_PINS, ROWS, COLS);

void 
setup(void)
{
  Serial.begin(9600);
}

void
loop(void)
{
  char key = keypad.getKey();

  if (key) {
    Serial.println(key);
  }
}
