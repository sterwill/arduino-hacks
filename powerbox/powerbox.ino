#include <Arduino.h>

/* HARDWARE */

// Pin must support PWM.
#define POWER_RELAY_PIN        9

// Define this value for normally closed relays; #undef if normally open.
#define POWER_RELAY_NORMALLY_CLOSED

// Pin must support PWM.
#define POWER_LED_PIN          11

// Linear taper potentiometer / variable resistor.
#define POT_PIN                A1

// Measured min and max readings
#define POT_CALIB_MIN          100
#define POT_CALIB_MAX          1023
#define POT_CALIB_RANGE        (POT_CALIB_MAX - POT_CALIB_MIN)

void print(const char * format, ...) {
  char buf[1024];
  va_list argptr;
  
  va_start(argptr, format);
  int len = vsnprintf(buf, sizeof(buf), format, argptr);
  va_end(argptr);
  Serial.write(buf, len);
}

/* CONTROL PARAMETERS */
void set_led(uint8_t value) {
  analogWrite(POWER_LED_PIN, value);
}

void set_power(uint8_t value) {
#ifdef POWER_RELAY_NORMALLY_CLOSED
  value = 255 - value;
#endif
  analogWrite(POWER_RELAY_PIN, value);
}

/* Returns a normalized value (in the range 0-255) from POT_PIN. */
uint8_t read_pot() {
  uint16_t raw = (uint16_t) analogRead(POT_PIN);
  print("raw = %04d [%d-%d], ", raw, POT_CALIB_MIN, POT_CALIB_MAX);

  // Scale from calibration min and max.
  if (raw < POT_CALIB_MIN) {
    raw = POT_CALIB_MIN;
  } else if (raw > POT_CALIB_MAX) {
    raw = POT_CALIB_MAX;
  } else {
    raw = raw - POT_CALIB_MIN;
  }
  uint8_t scaled = 255 * ((float) raw / (float) POT_CALIB_RANGE);

  print("scaled = %03d\n", scaled);
  return scaled;
}

void setup() {
  Serial.begin(115200);

  pinMode(POT_PIN, INPUT);
  pinMode(POWER_RELAY_PIN, OUTPUT);
  pinMode(POWER_LED_PIN, OUTPUT);

  set_power(0);
  set_led(0);
}

void loop() {
  uint8_t value = read_pot();
  set_power(value);
  set_led(value);
}

