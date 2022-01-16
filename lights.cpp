#include <Adafruit_NeoPixel.h>
#include <float.h>

#include "config.h"
#include "lights.h"

// Each channel is 0-1
typedef struct {
  float r;
  float g;
  float b;
} rgb;


// All colors are RGB order in this struct
typedef struct {
  anim animation;
  bool fast;
  color base_color;
  color highlight_color;

  // LED states, RGB color space
  rgb target_colors[LED_COUNT];
  rgb current_colors[LED_COUNT];
  rgb source_colors[LED_COUNT];
  int fade_in_steps;
  int fade_out_steps;
} cloud_state;

// Defines an rgb
#define RGB_HEX(R,G,B)  ((rgb) { ((R) / 255.0), ((G) / 255.0), ((B) / 255.0) })

#define MIN3(X,Y,Z)  (X < Y ? (X < Z ? X : Z) : (Y < Z ? Y : Z))
#define MAX3(X,Y,Z)  (X > Y ? (X > Z ? X : Z) : (Y > Z ? Y : Z))

static rgb colors[COLOR_MAX + 1];
static cloud_state state;
static byte flood_queue[LED_COUNT];
static byte swirl_queue[LED_COUNT];
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void lights_setup(void) {
  strip.begin();
  strip.show();
  strip.setBrightness(255);

  // Initialize the flood queue
  for (int i = 0; i < LED_COUNT; i++) {
    flood_queue[i] = 0;
  }
  flood_queue[0]  = 1;
  flood_queue[1]  = 1;
  flood_queue[2]  = 1;
  flood_queue[3]  = 1;
  flood_queue[4]  = 1;
  flood_queue[5]  = 1;
  flood_queue[6]  = 1;
  flood_queue[7]  = 1;
  flood_queue[8]  = 2;
  flood_queue[9]  = 2;
  flood_queue[10] = 2;
  flood_queue[11] = 2;
  flood_queue[12] = 2;
  flood_queue[13] = 2;
  flood_queue[14] = 2;
  flood_queue[15] = 2;
  flood_queue[67] = 1;
  flood_queue[66] = 1;
  flood_queue[65] = 1;
  flood_queue[64] = 1;
  flood_queue[63] = 1;
  flood_queue[62] = 1;
  flood_queue[61] = 1;
  flood_queue[60] = 1;
  flood_queue[59] = 2;
  flood_queue[58] = 2;
  flood_queue[57] = 2;
  flood_queue[56] = 2;
  flood_queue[55] = 2;
  flood_queue[54] = 2;
  flood_queue[53] = 2;
  flood_queue[52] = 2;

  // Initialize the swirl queue
  for (int i = 0; i < LED_COUNT; i++) {
    swirl_queue[i] = 0;
  }
  swirl_queue[0]  = 1;
  swirl_queue[1]  = 1;
  swirl_queue[2]  = 1;
  swirl_queue[3]  = 1;
  swirl_queue[4]  = 1;
  swirl_queue[5]  = 1;
  swirl_queue[6]  = 1;
  swirl_queue[7]  = 1;
  swirl_queue[8]  = 1;
  swirl_queue[9]  = 2;
  swirl_queue[10] = 2;
  swirl_queue[11] = 2;
  swirl_queue[12] = 2;
  swirl_queue[13] = 2;
  swirl_queue[14] = 2;
  swirl_queue[15] = 2;

  // Initialize our named colors
  colors[COLOR_BLACK]      = RGB_HEX(0x00, 0x00, 0x00);
  colors[COLOR_WHITE]      = RGB_HEX(0xff, 0xff, 0xff);
  colors[COLOR_RED]        = RGB_HEX(0xff, 0x00, 0x00);
  colors[COLOR_GREEN]      = RGB_HEX(0x00, 0xff, 0x00);
  colors[COLOR_BLUE]       = RGB_HEX(0x00, 0x00, 0xff);
  colors[COLOR_LIGHT_BLUE] = RGB_HEX(0xb5, 0xdd, 0xff);
  colors[COLOR_DARK_BLUE]  = RGB_HEX(0x16, 0x88, 0xfa);
  colors[COLOR_LIGHT_GRAY] = RGB_HEX(0xaa, 0xaa, 0xaa);
  colors[COLOR_DARK_GRAY]  = RGB_HEX(0x30, 0x30, 0x30);
  colors[COLOR_YELLOW]     = RGB_HEX(0xfc, 0xec, 0x5b);
  colors[COLOR_ORANGE]     = RGB_HEX(0xff, 0x89, 0x00);

  // 90 starts with a pleasing purple
  randomSeed(90);

  for (int i = 0; i < LED_COUNT; i++) {
    state.target_colors[i] = colors[COLOR_BLACK];
    state.current_colors[i] = colors[COLOR_BLACK];
    state.source_colors[i] = colors[COLOR_BLACK];
  }

  // The default animation doesn't care about the other fields
  state.animation = ANIM_DEFAULT;
  state.fast = true;
  state.base_color = COLOR_BLUE;
  state.highlight_color = COLOR_WHITE;
}

// Array of [a,b,c,d] becomes [d,a,b,c]
void rotate_right(byte array[], byte size) {
  if (size < 2) {
    return;
  }
  byte tail = array[size - 1];
  for (int i = size - 1; i > 0; i--) {
    array[i] = array[i - 1];
  }
  array[0] = tail;
}


// Array of [a,b,c,d] becomes [b,c,d,a]
void rotate_left(byte array[], byte size) {
  if (size < 2) {
    return;
  }
  byte head = array[0];
  for (int i = 0; i < size; i++) {
    array[i] = array[i + 1];
  }
  array[size - 1] = head;
}

/*
   Sets the desired RGB color for the specified LED.
*/
void set_color_rgb(const byte led, const rgb c) {
  if (state.target_colors[led].r != c.r || state.target_colors[led].g != c.g || state.target_colors[led].b != c.b) {
    state.target_colors[led] = c;
    // Reset the source so the next color step knows where we started from
    state.source_colors[led] = state.current_colors[led];
  }
}
/*
   Sets the desired indexed color for the specified LED.
*/
void set_color(const byte led, const color color_id) {
  set_color_rgb(led, colors[color_id]);
}

// A randomly twinkling animation
void animate_precipitation() {
  static unsigned long last_time = 0;
  unsigned long time = millis();

  state.fade_in_steps = 4;
  state.fade_out_steps = 128;

  if (last_time == 0 || time - last_time > (state.fast ? 100 : 256)) {
    for (byte i = 0; i < LED_COUNT; i++) {
      byte r = random(LED_COUNT);
      color color = COLOR_BLACK;
      if (r == 0) {
        color = state.base_color;
      } else if (r == 1) {
        color = state.highlight_color;
      }
      set_color(i, color);
    }
    last_time = time;
  }
}

// Colors move in waves from one end to the other
void animate_flood() {
  static unsigned long last_time = 0;
  unsigned long time = millis();

  // Set the colors from the queue
  color color_ids[3];
  color_ids[0] = COLOR_BLACK;
  color_ids[1] = state.base_color;
  color_ids[2] = state.highlight_color;

  for (uint8_t i; i < LED_COUNT; i++) {
    set_color(i, color_ids[flood_queue[i]]);
  }

  state.fade_in_steps = 1;
  state.fade_out_steps = 1;

  // Rotate the bottom half to the right, top half to the left
  if (time - last_time > (state.fast ? 15 : 40)) {
    rotate_right(flood_queue, LED_COUNT / 2);
    rotate_left(flood_queue + (LED_COUNT / 2), LED_COUNT / 2);
    last_time = time;
  }
}

void animate_pulse() {
  static unsigned long last_time = 0;
  static boolean flip = false;
  unsigned long time = millis();

  state.fade_in_steps = 128;
  state.fade_out_steps = 128;

  if (last_time == 0 || time - last_time > (state.fast ? 1024 : 4096)) {
    for (int i = 0; i < LED_COUNT; i++) {
      if (flip) {
        set_color(i, state.base_color);
      } else {
        set_color(i, state.highlight_color);
      }
    }
    flip = !flip;
    last_time = time;
  }
}

// Colors move in a clockwise circle around the model
void animate_swirl() {
  static unsigned long last_time = 0;
  unsigned long time = millis();

  // Set the colors from the queue
  color color_ids[3];
  color_ids[0] = COLOR_BLACK;
  color_ids[1] = state.base_color;
  color_ids[2] = state.highlight_color;

  for (uint8_t i; i < LED_COUNT; i++) {
    set_color(i, color_ids[swirl_queue[i]]);
  }

  // No fade on swirl
  state.fade_in_steps = 1;
  state.fade_out_steps = 1;

  // Rotate the queue one place if the period has elapsed
  if (time - last_time > (state.fast ? 10 : 30)) {
    rotate_right(swirl_queue, LED_COUNT);
    last_time = time;
  }
}

// Colors fade in and out slowly at random locations
void animate_default() {
  static unsigned long next_time = 0;
  unsigned long time = millis();

  state.fade_in_steps = 512;
  state.fade_out_steps = 1024;

  if (next_time == 0 || time > next_time) {
    rgb new_color;
    new_color.r = random(256) / 255.0;
    new_color.g = random(256) / 255.0;
    new_color.b = random(256) / 255.0;

    // Cut one channel down so we get more vivid colors
    switch (random(3)) {
      case 0:
        new_color.r /= 3.0;
        break;
      case 1:
        new_color.g /= 3.0;
        break;
      case 2:
        new_color.b /= 3.0;
        break;
    }

    for (int i = 0; i < LED_COUNT; i++) {
      if (random(3) == 0) {
        set_color_rgb(i, new_color);
      } else {
        set_color(i, COLOR_BLACK);
      }
    }

    next_time = time + random(1000, 10000);
  }
}

/*
   Changes the "current" colors to be one step closer to the "target" colors.
   Colors are interpolated linearly by channel.
*/
void step_colors() {
  for (int i = 0; i < LED_COUNT; i++) {
    rgb tgt = state.target_colors[i];
    rgb cur = state.current_colors[i];
    rgb src = state.source_colors[i];

    /*
       Each step moves "cur" closer to "tgt" by an amount that's a fraction of
       the original difference between "src" and "tgt".  When other functions
       change the "tgt", they must also set the "src" to "cur" so we can calculate
       new step sizes.
    */

    float diff_r = tgt.r - cur.r;
    float diff_g = tgt.g - cur.g;
    float diff_b = tgt.b - cur.b;

    if (abs(diff_r) >= FLT_EPSILON) {
      int steps = diff_r > 0.0 ? state.fade_in_steps : state.fade_out_steps;
      float step = (tgt.r - src.r) / steps;
      if (diff_r < 0.0) {
        cur.r += max(diff_r, step);
      } else {
        cur.r += min(diff_r, step);
      }
    }

    if (abs(diff_g) >= FLT_EPSILON) {
      int steps = diff_g > 0.0 ? state.fade_in_steps : state.fade_out_steps;
      float step = (tgt.g - src.g) / steps;
      if (diff_g < 0.0) {
        cur.g += max(diff_g, step);
      } else {
        cur.g += min(diff_g, step);
      }
    }

    if (abs(diff_b) >= FLT_EPSILON) {
      int steps = diff_b > 0.0 ? state.fade_in_steps : state.fade_out_steps;
      float step = (tgt.b - src.b) / steps;
      if (diff_b < 0.0) {
        cur.b += max(diff_b, step);
      } else {
        cur.b += min(diff_b, step);
      }
    }

    state.current_colors[i] = cur;

    if (cur.r == tgt.r && cur.g == tgt.g && cur.b == tgt.b) {
      state.source_colors[i] = tgt;
    }
  }
}

/*
   Updates the colors of all the LEDs to their "current" colors.
*/
void update_leds() {
  for (int i = 0; i < LED_COUNT; i++) {
    rgb rgb = state.current_colors[i];
    strip.setPixelColor(i, rgb.r * 255, rgb.g * 255, rgb.b * 255);
  }
  strip.show();
}


void print_state() {
#ifdef DEBUG
  Serial.println("new state:");
  Serial.print("  animation: ");
  Serial.println(state.animation);
  Serial.print("  fast: ");
  Serial.println(state.fast);
  Serial.print("  base color: ");
  Serial.println(state.base_color);
  Serial.print("  highlight color: ");
  Serial.println(state.highlight_color);
#endif
}

void print_rgb(rgb rgb) {
#ifdef DEBUG
  Serial.print(rgb.r);
  Serial.print(",");
  Serial.print(rgb.g);
  Serial.print(",");
  Serial.print(rgb.b);
  Serial.print(" ");
#endif
}

void lights_loop(void) {
  switch (state.animation) {
    case ANIM_PRECIP:
      animate_precipitation();
      break;
    case ANIM_FLOOD:
      animate_flood();
      break;
    case ANIM_PULSE:
      animate_pulse();
      break;
    case ANIM_SWIRL:
      animate_swirl();
      break;
    case ANIM_DEFAULT:
    default:
      animate_default();
      break;
  }

  step_colors();
  update_leds();
}

void lights_configure(anim animation, bool fast, color base_color, color highlight_color) {
  state.animation = animation;
  state.fast = fast;
  state.base_color = base_color;
  state.highlight_color = highlight_color;
  print_state();
}
