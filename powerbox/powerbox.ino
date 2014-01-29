#include <math.h>

/* HARDWARE */

// Normally CLOSED relay
#define POWER_RELAY_PIN        9
#define POWER_LED_PIN          11

#define THERMAL_RESISTOR_PIN   A0
#define POT_PIN                A1

// Measured min and max readings
#define POT_CALIB_MIN          105
#define POT_CALIB_MAX          1023

/* CONTROL PARAMETERS */

// The power cycle length is the on plus off parts
#define POWER_CYCLE_MILLIS     10000

static unsigned long prev_loop_time = 0;

// Counts from 0 to POWER_CYCLE_MILLIS and resets
static unsigned long power_cycle_time = 0;

// True when the switched outlets are on, false when they are off 
// (relay is normally CLOSED so default is true)
static boolean power_enabled = true;

void setup() {
  Serial.begin(115200);

  pinMode(POWER_RELAY_PIN, OUTPUT);
  pinMode(POWER_LED_PIN, OUTPUT);

  pinMode(THERMAL_RESISTOR_PIN, INPUT);
  pinMode(POT_PIN, INPUT);
  
  power(false);
}

void loop() {
  // Reads [0.0,1.0], higher values mean longer duty cycles
  float pot_percent = read_pot();
  
  // Calculate how much of the total cycle is "on"
  float power_cycle_on_millis = pot_percent * POWER_CYCLE_MILLIS;

  unsigned long time = millis();
  unsigned long delta = time - prev_loop_time;
  
  // Account for elapsed time this loop
  power_cycle_time += delta;
  if (power_cycle_time > POWER_CYCLE_MILLIS) {
    power_cycle_time = 0;
  }
  
  // Turn on power if in the "on" part
  power(power_cycle_time <= power_cycle_on_millis);
  
  prev_loop_time = time;
  
  delay(100);
}

/* Turns the switched power outlets and indicator LED on or off. */
void power(boolean on) {
  if (on == power_enabled) {
    return;
  }
  
  if (on) {
    // Relay is normally closed, so go LOW to let the juice flow
    digitalWrite(POWER_RELAY_PIN, LOW);
    digitalWrite(POWER_LED_PIN, HIGH);
  } else {
    // Relay is normally closed, so go HIGH to cut the juice
    digitalWrite(POWER_RELAY_PIN, HIGH);
    digitalWrite(POWER_LED_PIN, LOW);
  }
  power_enabled = on;
}

float cbrt(float f) {
  return pow(f, 1.0/3.0);
}

/* Returns a value in the range [0.0,1.0] from POT_PIN, normalized to linear response. */
float read_pot() {
  int raw = analogRead(POT_PIN);
  Serial.print("Pot raw: ");
  Serial.println(raw);

  const float scale = (1023.0 / (float) (POT_CALIB_MAX - POT_CALIB_MIN));
  float scaled = scale * (raw - POT_CALIB_MIN);
  if (scaled < 0) {
    scaled = 0;
  } else if (scaled > 1023) {
    scaled = 1023;
  }
  Serial.print("Pot scaled: ");
  Serial.println(scaled);

  float percent = scaled / 1024.0;
  Serial.print("Pot percent: ");
  Serial.println(percent);
  
  return percent;
}
