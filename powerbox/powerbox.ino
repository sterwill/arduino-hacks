#include <Arduino.h>

/* HARDWARE */

#define POWER_RELAY_PIN        9
#define POWER_LED_PIN          11

// Define this value for normally closed relays; #undef if normally open.
#define POWER_RELAY_NORMALLY_CLOSED

// Linear taper potentiometer / variable resistor.
#define POT_PIN                A1

// Measured min and max readings
#define POT_CALIB_MIN          100
#define POT_CALIB_MAX          1023
#define POT_CALIB_RANGE        (POT_CALIB_MAX - POT_CALIB_MIN)

// The shorter the better (so the heating element holds a steady temp), 
// but too short or SSR won't be able to switch in time.
#define POWER_CYCLE_MILLIS     50

// Counts from 0 to POWER_CYCLE_MILLIS and resets
static uint16_t power_loop_counter = 0;
static uint32_t prev_loop_time = 0;

void print(const char * format, ...) {
  char buf[1024];
  va_list argptr;
  
  va_start(argptr, format);
  int len = vsnprintf(buf, sizeof(buf), format, argptr);
  va_end(argptr);
  Serial.write(buf, len);
}

void set_led(bool energize) {
  digitalWrite(POWER_LED_PIN, energize ? HIGH : LOW);
}

void set_power(bool energize) {
#ifdef POWER_RELAY_NORMALLY_CLOSED
  int value = energize ? LOW : HIGH;
#else
  int value = energize ? HIGH : LOW;
#endif
  digitalWrite(POWER_RELAY_PIN, value);
}

/* Returns a normalized value (in the range 0-max_val) from POT_PIN. */
uint16_t read_pot(uint16_t max_val) {
  const uint16_t raw = (uint16_t) analogRead(POT_PIN);

  // Scale from calibration min and max.
  uint16_t corrected;
  if (raw < POT_CALIB_MIN) {
    corrected = POT_CALIB_MIN;
  } else if (raw > POT_CALIB_MAX) {
    corrected = POT_CALIB_MAX;
  } else {
    corrected = raw - POT_CALIB_MIN;
  }
  const uint16_t scaled = max_val * ((float) corrected / (float) POT_CALIB_RANGE);

  print("raw = %04d [%d-%d], corrected = %04d / %04d, scaled = %04d / %04d\n", raw, 
        POT_CALIB_MIN, POT_CALIB_MAX, corrected, POT_CALIB_RANGE, scaled, max_val);
  return scaled;
}

void setup() {
  pinMode(POT_PIN, INPUT);
  pinMode(POWER_RELAY_PIN, OUTPUT);
  pinMode(POWER_LED_PIN, OUTPUT);
  set_power(false);
  set_led(false);

  Serial.begin(115200);
}

void loop() {
  uint32_t this_loop_time = millis();
  uint32_t delta = this_loop_time - prev_loop_time;
  
  // Account for elapsed time this loop
  power_loop_counter += delta;
  if (power_loop_counter > POWER_CYCLE_MILLIS) {
    power_loop_counter = 0;
  }

  // Read the dial to see what proportion of the loop to energize in.
  bool energize = power_loop_counter < read_pot(POWER_CYCLE_MILLIS);
  set_power(energize);
  set_led(energize);
   
  prev_loop_time = this_loop_time;
}

