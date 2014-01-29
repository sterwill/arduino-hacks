#include <stdarg.h>

#define LIGHTS_SIZE 20
#define GROUPS_SIZE 5

#define PHASE_HIGH_AMBIENT_LIGHT     0
#define PHASE_MEDIUM_AMBIENT_LIGHT   1
#define PHASE_LOW_AMBIENT_LIGHT      2

void p(char *fmt, ... ) {
  char tmp[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(tmp, 128, fmt, args);
  va_end(args);
  Serial.print(tmp);
}

// Analog light sensor pin
static byte LDR = 4;

// Digital charliplexed LED pins
static byte A = 9, B = 10, C = 11, D = 12, E = 13;

// Pin pairs (HIGH, LOW) that each drive one LED
static byte pin_pairs[LIGHTS_SIZE][2] = {
  {A, B}, {A, C}, {A, D}, {A, E},
  {B, A}, {B, C}, {B, D}, {B, E},
  {C, A}, {C, B}, {C, D}, {C, E},
  {D, A}, {D, B}, {D, C}, {D, E},
  {E, A}, {E, B}, {E, C}, {E, D},
};

// State of a light (on or off), indexes match pin_pairs
static boolean lights[LIGHTS_SIZE];

// Which animation phase we're in
static byte phase = PHASE_HIGH_AMBIENT_LIGHT;

/* Groups */

struct light_group {
  byte count;
  byte * indexes;
  
  void (*timer)(unsigned long time, struct light_group * group);
  void * timer_data;
  
  void (*animator)(unsigned long time, struct light_group * group);
  void * animator_data;
};

static struct light_group groups[GROUPS_SIZE];

void light_group_init(struct light_group * group, byte count, byte * indexes, 
    void (*timer)(unsigned long time, struct light_group * group),
    void (*animator)(unsigned long time, struct light_group * group)) {
  group->count = count;
  group->indexes = indexes;
  group->timer = timer;
  group->timer_data = NULL;
  group->animator = animator;
  group->animator_data = NULL;
}

/* Arduino */

void setup() {
  Serial.begin(9600);
  
  // Configure light sensor
  pinMode(LDR, INPUT);
  
  // Turn off all lights
  for (byte i = 0; i < LIGHTS_SIZE; i++) {
    lights[i] = false;
  }
  
  static byte left_indexes[] = { 0, 8, 5, 4, 9 };
  light_group_init(&groups[0], 5, left_indexes, timer_monotonic, animate_increment);
  timer_monotonic_init(&groups[0], 517000, 0, 0);
  animate_increment_init(&groups[0]);
 
  static byte mid_left_indexes[] = { 2, 1, 12 };
  light_group_init(&groups[1], 3, mid_left_indexes, timer_monotonic, animate_increment);
  timer_monotonic_init(&groups[1], 1000000, 3000000, 0);
  animate_increment_init(&groups[1]);
  
  static byte nipples_indexes[] = { 16, 3 };
  light_group_init(&groups[2], 2, nipples_indexes, timer_monotonic, animate_increment);
  timer_monotonic_init(&groups[2], 1000000, 2000000, 3000000);
  animate_increment_init(&groups[2]);
  
  static byte snake_indxes[] = { 19, 15, 18, 11, 17 };
  light_group_init(&groups[3], 5, snake_indxes, timer_monotonic, animate_increment);
  timer_monotonic_init(&groups[3], 110000, 220000, 0);
  animate_increment_init(&groups[3]);
  
  static byte spikes_indexes[] = { 7, 14, 10, 13, 6 };
  light_group_init(&groups[4], 5, spikes_indexes, timer_monotonic, animate_flash);
  timer_monotonic_init(&groups[4], 130000, 0, 0);
  animate_flash_init(&groups[4]);
}

void loop() {
  sense_ambient_light();

  unsigned long time = micros();
  for (byte i = 0; i < GROUPS_SIZE; i++) {
    groups[i].timer(time, &groups[i]);
  }
  //Serial.println(micros() - time);
  
  update_lights();
/*
  unsigned long start = micros();  
  update_lights();
  byte lit_count = 0;
  for (byte i = 0; i < LIGHTS_SIZE; i++) {
    if (lights[i]) { lit_count++; }
  }
  p("update %d = %lu\n", lit_count, micros() - start);
*/

//  static unsigned long loops = 0;
//  static unsigned long last_time = 0;
//  loops++;
//  unsigned long diff = time - last_time;
//  if (diff > 1000000) {
//    p("%lu\n", loops);
//    loops = 0;
//    last_time = time;
//  }
}

/* Ambient light sensor updates phase with its results */
void sense_ambient_light() {
  int val = analogRead(LDR);
  //Serial.println(val);

  if (val > 700) {
    phase = PHASE_HIGH_AMBIENT_LIGHT;
  } else if (val > 300) {
    phase = PHASE_MEDIUM_AMBIENT_LIGHT;
  } else {
    phase = PHASE_LOW_AMBIENT_LIGHT;
  }
}


/* Increment Animator: Lights the next light each time, wrapping back to 0 */

struct animate_increment_data {
  byte lit;
};

void animate_increment_init(struct light_group * group) {
  struct animate_increment_data * data = (struct animate_increment_data *) 
    malloc(sizeof(struct animate_increment_data));
  data->lit = 0;
  group->animator_data = data;
}

void animate_increment(unsigned long time, struct light_group * group) {
  struct animate_increment_data * data = (struct animate_increment_data *) group->animator_data;

  lights[group->indexes[data->lit]] = false;
  
  if (data->lit == group->count - 1) {
    data->lit = 0;
  } else {
    data->lit++;
  }
  
  lights[group->indexes[data->lit]] = true;
}

/* Random Animator: Lights a random number of lights each time */

struct animate_random_data {
};

void animate_random_init(struct light_group * group) {
  struct animate_random_data * data = (struct animate_random_data *) 
    malloc(sizeof(struct animate_random_data));
  group->animator_data = data;
}

void animate_random(unsigned long time, struct light_group * group) {
  struct animate_random_data * data = (struct animate_random_data *) group->animator_data;
  for (byte i = 0; i < group->count; i++) {
    lights[group->indexes[i]] = (boolean) random(2);
  }
}

/* Flash Animator: Alternates between all lights on and off */

struct animate_flash_data {
  boolean lit;
};

void animate_flash_init(struct light_group * group) {
  struct animate_flash_data * data = (struct animate_flash_data *) 
    malloc(sizeof(struct animate_flash_data));
  data->lit = true;
  group->animator_data = data;
}

void animate_flash(unsigned long time, struct light_group * group) {
  struct animate_flash_data * data = (struct animate_flash_data *) group->animator_data;
  for (byte i = 0; i < group->count; i++) {
    lights[group->indexes[i]] = data->lit;
  }
  data->lit = !data->lit;
}

/* Monotonic Timer: Animates at a fixed frequency */

struct timer_monotonic_data {
  unsigned long low_ambient_light_period;
  unsigned long medium_ambient_light_period;
  unsigned long high_ambient_light_period;
  unsigned long last_animated;
};

void timer_monotonic_init(struct light_group * group, unsigned long low_ambient_light_period, 
  unsigned long medium_ambient_light_period, unsigned long high_ambient_light_period) {
  struct timer_monotonic_data * data = (struct timer_monotonic_data *) 
    malloc(sizeof(struct timer_monotonic_data));
  data->low_ambient_light_period = low_ambient_light_period;
  data->medium_ambient_light_period = medium_ambient_light_period;
  data->high_ambient_light_period = high_ambient_light_period;
  data->last_animated = 0;
  group->timer_data = data;
}

void timer_monotonic(unsigned long time, struct light_group * group) {
  struct timer_monotonic_data * data = (struct timer_monotonic_data *) group->timer_data;

  unsigned long period;
  switch (phase) {
    case PHASE_LOW_AMBIENT_LIGHT:
      period = data->low_ambient_light_period;
      break;
    case PHASE_MEDIUM_AMBIENT_LIGHT:
      period = data->medium_ambient_light_period;
      break;
    case PHASE_HIGH_AMBIENT_LIGHT:
      period = data->high_ambient_light_period;
      break;
  }
  
  // Disable animation for this group, turn all off
  if (period == 0) {
    for (byte i = 0; i < group->count; i++) {
      lights[group->indexes[i]] = 0;
    }
    return;
  }
  
  if (time - data->last_animated >= period) {
    if (group->animator != NULL) {
      group->animator(time, group);
    }
    data->last_animated = time;
  }
}

/* Light Update Utilities */

void pair_disable_all() {
  pinMode(A, INPUT);
  pinMode(B, INPUT);
  pinMode(C, INPUT);
  pinMode(D, INPUT);
  pinMode(E, INPUT);
}

void update_lights() {
  // Tracks whether there is one lit so we can elide resets for speed
  static boolean any_lit = false;

  if (any_lit) {
    pair_disable_all();
    any_lit = false;
  }
  
  for (byte i = 0; i < LIGHTS_SIZE; i++) {
    if (lights[i]) {
      if (any_lit) {
        pair_disable_all();
      }
      pinMode(pin_pairs[i][0], OUTPUT);
      digitalWrite(pin_pairs[i][0], HIGH);
      pinMode(pin_pairs[i][1], OUTPUT);
      digitalWrite(pin_pairs[i][1], LOW);
      any_lit = true;
    }
  }
}
