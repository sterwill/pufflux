/*
 * Pufflux Receiver
 *
 * Copyright 2013 Shaw Terwilliger <sterwill@tinfig.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <float.h>
#include <avr/pgmspace.h>

// Turns on some serial port output
//#define DEBUG

/* 
 * Choose the port (pin group) and matching direction control register that 
 * contains the pins which control your 5 LEDs strips.  Choices are PORTB with 
 * DDRB, PORTC with DDRC, and PORTD with DDRD.
 *
 * All your LED control pins must be in one port, the first LED control pin must 
 * be the first in the port, and the pins must be contiguous.
 *
 * See http://www.arduino.cc/en/Reference/PortManipulation for help on
 * determining which ports map to which pins on your device.
 *
 * I use PORTC on my ATmega328P-based Ardweeny, which is digital pins 
 * 0-5.
 *
 * Figuring all this out is easier than it sounds: if you have 5 LED 
 * control wires and want to use PORTC, just hook them to digital pins
 * 0, 1, 2, 3, and 4.  That's it.
 */
#define LED_PORT PORTC
#define LED_DDR  DDRC

/*
 * Where the LEDs are:
 *
 *  -0,1---------3,4-
 *       |  2  |
 *       |  7  |
 *  -5,6---------8,9-
 *           ^
 *     front |
 */

/*
 * Addresses of the LEDs in order 0-9; make them match that map.
 *
 * The high nibble is the 0-based pin in the port.  The low nibble 
 * is the 0-based offset of the LED in the strip controlled by the specified pin.
 */
const byte leds[10] = { 0x31, 0x30, 0x41, 0x10, 0x11, 0x21, 0x20, 0x40, 0x00, 0x01 };

/*
 * The order in which the LEDs must be programmed.  All the offset-0 LEDs
 * must immediately precede their offset-1 partner on the same pin.
 */
const byte led_send_order[10] = { 1, 0, 7, 2, 3, 4, 6, 5, 8, 9 };

/*
 * Digital pins used by the radio interface.  rx goes high when there's an 
 * incoming transmission, and chan0, chan1, chan2, and chan3 are the 
 * lowest four bits (in that order) of the data nibble.
 */
#define RX       9
#define RX_LED   13 
#define CHAN_0   10
#define CHAN_1   11
#define CHAN_2   12
#define CHAN_3   4

/*
 * How long a transmitted pulse is in milliseconds.  rx goes high this long
 * so we can read a nibble, then rx goes low this long before the next nibble.
 */
#define PULSE_DURATION 500

/************************************************************************/
/* You probably don't need to change anything below here.               */
/************************************************************************/

/*
 * You can change the HSV values of these colors, but don't change the
 * count or IDs without changing the transmitter software to match.
 */
#define COLOR_BLACK_ID          0
#define COLOR_WHITE_ID          1
#define COLOR_RED_ID            2
#define COLOR_GREEN_ID          3
#define COLOR_BLUE_ID           4
#define COLOR_LIGHT_BLUE_ID     5
#define COLOR_DARK_BLUE_ID      6
#define COLOR_LIGHT_GRAY_ID     7
#define COLOR_DARK_GRAY_ID      8
#define COLOR_YELLOW_ID         9
#define COLOR_ORANGE_ID         10
#define MAX_COLOR_ID            10

static uint32_t hsv_values[11];

/*
 * Reserve one hue value to mean "don't interpolate hue values to this one if
 * it's the target."
 */
#define NULL_HUE 0xff

#define COLOR_ID_MASK           0b0000000000000111

/*
 * Don't change these unless you change the transmitter software to match.
 */
#define ANIM_DEFAULT_ID       0
#define ANIM_PRECIPITATION_ID 1
#define ANIM_FLOOD_ID         2
#define ANIM_PULSE_ID         3
#define ANIM_SWIRL_ID         4

#define RGB2GBR(c)  (((c & 0xff0000) >> 16) | ((c & 0x00ff00) << 8) | ((c & 0x0000ff) << 8)) 

#define MIN3(X,Y,Z)  (X < Y ? (X < Z ? X : Z) : (Y < Z ? Y : Z))
#define MAX3(X,Y,Z)  (X > Y ? (X > Z ? X : Z) : (Y > Z ? Y : Z))

#define ANIM_GET_MASK 0b0000000000000111
#define ANIM_SET_MASK ~ANIM_GET_MASK

#define SPEED_GET_MASK 0b0000000000001000
#define SPEED_SET_MASK ~SPEED_GET_MASK

// TODO define masks for new features that use bits 5-8

#define BASE_COLOR_GET_MASK 0b0000111100000000
#define BASE_COLOR_SET_MASK ~BASE_COLOR_GET_MASK

#define HIGHLIGHT_COLOR_GET_MASK 0b1111000000000000
#define HIGHLIGHT_COLOR_SET_MASK ~HIGHLIGHT_COLOR_GET_MASK

// Set the 5 lowest bits as write pins
#define CONFIG_DDR()    (LED_DDR  |= 0b00011111)
#define LEDS_HIGH(pins) (LED_PORT |= (0b00011111 & pins))
#define LEDS_LOW(pins)  (LED_PORT &= (0b11100000 & pins))

// All colors are RGB order in this struct
struct cloud_state {
  // Command fields
  byte animation;
  bool fast;
  uint32_t base_color;
  uint32_t highlight_color;

  // LED states, HSV color space
  uint32_t target_colors[10];
  uint32_t current_colors[10];
  uint32_t source_colors[10];
  byte fade_steps;
};
static struct cloud_state state;

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
    
  hsv_values[COLOR_BLACK_ID] = rgb_to_hsv(0x000000);
  hsv_values[COLOR_WHITE_ID] = rgb_to_hsv(0xffffff);
  hsv_values[COLOR_RED_ID] = rgb_to_hsv(0xff0000);
  hsv_values[COLOR_GREEN_ID] = rgb_to_hsv(0x00ff00);
  hsv_values[COLOR_BLUE_ID] = rgb_to_hsv(0x0000ff);
  hsv_values[COLOR_LIGHT_BLUE_ID] = rgb_to_hsv(0xb5ddff);
  hsv_values[COLOR_DARK_BLUE_ID] = rgb_to_hsv(0x1688fa);
  hsv_values[COLOR_LIGHT_GRAY_ID] = rgb_to_hsv(0xaaaaaa);
  hsv_values[COLOR_DARK_GRAY_ID] = rgb_to_hsv(0x303030);
  hsv_values[COLOR_YELLOW_ID] = rgb_to_hsv(0xfcec5b);
  hsv_values[COLOR_ORANGE_ID] = rgb_to_hsv(0xff8900);

  // Configure which pins in the port are in vs. out
  CONFIG_DDR();

  pinMode(RX, INPUT);
  pinMode(CHAN_0, INPUT);
  pinMode(CHAN_1, INPUT);
  pinMode(CHAN_2, INPUT);
  pinMode(CHAN_3, INPUT);
  
  // Reset all the controllers and turn off the LEDs
  reset_leds();

  randomSeed(2);
  
  for (int i = 0; i < 10; i++) {
   state.target_colors[i] = hsv_values[COLOR_BLACK_ID];
   state.current_colors[i] = hsv_values[COLOR_BLACK_ID];
   state.source_colors[i] = hsv_values[COLOR_BLACK_ID];
  }
  state.fade_steps = 64;

  // The default animation doesn't care about the other fields
  state.animation = ANIM_DEFAULT_ID;
  state.fast = false;
  state.base_color = COLOR_RED_ID;
  state.highlight_color = COLOR_ORANGE_ID;
  
  print_state();
}

void loop() {
  uint16_t command;
  if (read_command(command)) {
    decode_command(command);
    print_state();
  }
    
  animate();
    
  /*
   * A very small delay here helps our byte-sized step counters add up to meaningful
   * times.  A delay of 16 ms makes 255 steps take at least 4096 ms, a good long fade.
   */
  delay(16);
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

void decode_command(uint16_t command) {
  state.animation = (command & ANIM_GET_MASK);
  state.fast = (command & SPEED_GET_MASK) >> 3;
  state.base_color = (command & BASE_COLOR_GET_MASK) >> 8;
  state.highlight_color = (command & HIGHLIGHT_COLOR_GET_MASK) >> 12;
}

bool wait_for(int pin, int condition, unsigned long timeout) {
  timeout = millis() + timeout;
  while (millis() < timeout) {
    if (digitalRead(pin) == condition) {
      return true;
    }
  }
  return false;
}

bool read_command(uint16_t & command) {
  byte nibbles[4];        

  if (digitalRead(RX)) {
    digitalWrite(RX_LED, HIGH);
    for (int i = 0; i < 4; i++) {
      if (i == 0) {
#ifdef DEBUG
        Serial.println("incoming command:");
#endif
      }
      // Read the transmitted nibble      
      nibbles[i] = 
          ((byte) digitalRead(CHAN_3) << 3)
        | ((byte) digitalRead(CHAN_2) << 2) 
        | ((byte) digitalRead(CHAN_1) << 1) 
        | ((byte) digitalRead(CHAN_0));

#ifdef DEBUG
      Serial.print("  nibble ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(nibbles[i], HEX);
#endif

      // Wait for this pulse to be over, and half-way into the silence
      delay(PULSE_DURATION + (PULSE_DURATION / 2));
      
      if (i == 3) {
        break;
      }
      
      // Wait for the next pulse to start
      if (!wait_for(RX, HIGH, PULSE_DURATION)) {
#ifdef DEBUG
        Serial.print("time out waiting for start of of nibble ");
        Serial.println(i + 1);
#endif
        digitalWrite(RX_LED, LOW);
        return false;
      }
    }
    digitalWrite(RX_LED, LOW);

#ifdef DEBUG
    Serial.println("command read successfully");
#endif
    command = 
        ((uint16_t) nibbles[0] << 12)
      | ((uint16_t) nibbles[1] << 8)
      | ((uint16_t) nibbles[2] << 4)
      | ((uint16_t) nibbles[3]);
    return true;
  }
  return false;  
}

void animate() {
  switch (state.animation) {
    case ANIM_PRECIPITATION_ID:
      animate_precipitation();
      break;
    case ANIM_FLOOD_ID:
      animate_flood();
      break;
    case ANIM_PULSE_ID:
      animate_pulse();
      break;
    case ANIM_SWIRL_ID:
      animate_swirl();
      break;
    case ANIM_DEFAULT_ID:
    default:
      animate_default();
      break;
  }

  step_colors();
  update_leds();
}

// A randomly twinkling animation
void animate_precipitation() {
  static unsigned long last_time = 0;
  unsigned long time = millis();
  
  state.fade_steps = state.fast ? 10 : 20;

  if (last_time == 0 || time - last_time > (state.fast ? 32 : 64)) {
    for (byte i = 0; i < 10; i++) {
      byte r = random(10);
      byte color_id = COLOR_BLACK_ID;
      if (r == 0) {
        color_id = state.base_color;
      } else if (r == 1) {
        color_id = state.highlight_color;
      }
      set_color(i, color_id);
    }
    last_time = time;
  }
}

// Colors move in waves from one end to the other
void animate_flood() {
  static unsigned long last_time = 0;
  const byte queue_size = 5;
  static byte flood_queue[queue_size] = {1, 2, 0, 0, 0};
  unsigned long time = millis();
  
  // Set the colors from the queue
  byte color_ids[3];
  color_ids[0] = COLOR_BLACK_ID;
  color_ids[1] = state.base_color;
  color_ids[2] = state.highlight_color;

  // LEDs from left to right
  set_color(0, color_ids[flood_queue[0]]);
  set_color(5, color_ids[flood_queue[0]]);
  set_color(1, color_ids[flood_queue[1]]);
  set_color(6, color_ids[flood_queue[1]]);
  set_color(2, color_ids[flood_queue[2]]);
  set_color(7, color_ids[flood_queue[2]]);
  set_color(3, color_ids[flood_queue[3]]);
  set_color(8, color_ids[flood_queue[3]]);
  set_color(4, color_ids[flood_queue[4]]);
  set_color(9, color_ids[flood_queue[4]]);
  
  state.fade_steps = state.fast ? 8 : 16;

  // Rotate the queue one place if the period has elapsed
  if (time - last_time > (state.fast ? 128 : 256)) {
    rotate_right(flood_queue, queue_size);
    last_time = time;
  }
}

void animate_pulse() {
  static unsigned long last_time = 0;
  static boolean flip = false;
  unsigned long time = millis();
  
  state.fade_steps = state.fast ? 16 : 128;

  if (last_time == 0 || time - last_time > (state.fast ? 1024 : 2048)) {
    for (int i = 0; i < 10; i++) {
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
  const byte queue_size = 10;
  static byte swirl_queue[queue_size] = {1, 1, 2, 0, 0, 0, 0, 0, 0, 0};
  unsigned long time = millis();
  
  // Set the colors from the queue
  byte color_ids[3];
  color_ids[0] = COLOR_BLACK_ID;
  color_ids[1] = state.base_color;
  color_ids[2] = state.highlight_color;

  set_color(0, color_ids[swirl_queue[0]]);
  set_color(1, color_ids[swirl_queue[1]]);
  set_color(2, color_ids[swirl_queue[2]]);
  set_color(3, color_ids[swirl_queue[3]]);
  set_color(4, color_ids[swirl_queue[4]]);
  set_color(9, color_ids[swirl_queue[5]]);
  set_color(8, color_ids[swirl_queue[6]]);
  set_color(7, color_ids[swirl_queue[7]]);
  set_color(6, color_ids[swirl_queue[8]]);
  set_color(5, color_ids[swirl_queue[9]]);
  
  state.fade_steps = state.fast ? 8 : 16;

  // Rotate the queue one place if the period has elapsed
  if (time - last_time > (state.fast ? 128 : 256)) {
    rotate_right(swirl_queue, queue_size);
    last_time = time;
  }
}

void animate_default() {
  static unsigned long next_time = 0;
  unsigned long time = millis();

  state.fade_steps = 255;
  
  if (next_time == 0 || time > next_time) {
    // Avoid the NULL_HUE by only going to 255, because we want lots of hue
    // shifts in this animation.  Bias the saturation high for bright colors,
    // and bias the value up a bit.
    const uint32_t new_color = (random(255) << 16) | ((128 + random(128)) << 8) | ((64 + random(192)));
    
    for (int i = 0; i < 10; i++) {
      if (random(2) == 0) {
        set_color_hsv(i, new_color);
      }
    }

    next_time = time + random(1000, 10000);
  }
}

/*
 * Sets the desired indexed color for the specified LED.
 */
void set_color(const byte led, const byte color_id) {
  set_color_hsv(led, hsv_values[color_id]);
}

/*
 * Sets the desired HSV color for the specified LED.
 */
void set_color_hsv(const byte led, const uint32_t hsv) {
  if (state.target_colors[led] != hsv) {
    state.target_colors[led] = hsv;
    // Reset the source so the next color step knows where we started from
    state.source_colors[led] = state.current_colors[led];
  }
}

/*
 * Changes the "current" colors to be one step closer to the "target" colors.
 * Colors are interpolated linearly by channel.
 */
void step_colors() {
  // For each LED
  for (int i = 0; i < 10; i++) {
    uint32_t target = state.target_colors[i];
    uint32_t current = state.current_colors[i];
    uint32_t source = state.source_colors[i];
    byte tgt_h = (target >> 16) & 0xff;
    byte tgt_s = (target >> 8) & 0xff;
    byte tgt_v = (target) & 0xff;
    byte cur_h = (current >> 16) & 0xff;
    byte cur_s = (current >> 8) & 0xff;
    byte cur_v = (current) & 0xff;
    byte src_h = (source >> 16) & 0xff;
    byte src_s = (source >> 8) & 0xff;
    byte src_v = (source) & 0xff;

    /*
     * Each step moves "cur" closer to "tgt" by an amount that's a fraction of
     * the original difference between "src" and "tgt".  When other functions 
     * change the "tgt", they must also set the "src" to "cur" so we can calculate
     * new step sizes.
     */

    // Hue is an angle (0-255 corresponds to 0-360 degrees), and we need to wrap
    // around to find the best color transitions, so calculate diffs with larger 
    // signed values.
    short diff_h = tgt_h - cur_h;
    short diff_s = tgt_s - cur_s;
    short diff_v = tgt_v - cur_v;

    byte adjust;

#ifdef DEBUG
    Serial.print(source, HEX);
    Serial.print(" ");
    Serial.print(current, HEX);
    Serial.print(" ");
    Serial.print(target, HEX);
    Serial.println();
#endif

    // Don't interpolate to the target if it's the NULL_HUE
    if (diff_h != 0 && tgt_h != NULL_HUE) {
      /*
       * Because hue wraps around, choose the shortest path. step preserves the sign
       * of the difference.
       */
      short step = ((short) tgt_h - src_h) / state.fade_steps;
      if (diff_h < 0) {
        adjust = min(-1, max(diff_h, step));
      } else {
        adjust = max(1, min(diff_h, step));
      }
      cur_h += adjust;
    }
  
    // Avoid rollover for saturation and value
    if (diff_s != 0) {
      // Adjust by the step size between src and tgt, but always at least 1
      adjust = max(1, min(abs(diff_s), abs(tgt_s - src_s) / state.fade_steps));
      cur_s += (diff_s > 0) ? adjust : -adjust;
    }

    if (diff_v != 0) {    
      adjust = max(1, min(abs(diff_v), abs(tgt_v - src_v) / state.fade_steps));
      cur_v += (diff_v > 0) ? adjust : -adjust;
    }
 
    state.current_colors[i] = ((uint32_t) cur_h << 16) | ((uint32_t) cur_s << 8) | ((uint32_t) cur_v);

    if (current == target) {
      state.source_colors[i] = target;
    }
  }
}

/*
 * Updates the colors of all the LEDs to their "current" colors.
 */
void update_leds() {
  static uint32_t bgr[10];
  
  // hsv_to_rgb is too slow to run in the noInterrupts section
  for (int i = 0; i < 10; i++) {
    uint32_t rgb = hsv_to_rgb(state.current_colors[led_send_order[i]]);
    bgr[i] = RGB2GBR(rgb);
  }
  
  noInterrupts();
  for (int i = 0; i < 10; i++) {
    // Get the led index from the order array
    const byte led = led_send_order[i];
    const byte led_address = leds[led];
    
    // Crack the address into pin and offset
    byte pin_mask = 0x1 << ((led_address & 0b11110000) >> 4);
    byte offset = (led_address & 0b00001111);

    send(pin_mask, bgr[i]);
  }
  interrupts();
}

/*
 * Resets the LED controllers on all pins.  LEDs turn off.
 */
void reset_leds() {
  // Reset
  LEDS_LOW(0b00011111);
  delayMicroseconds(20);

  // All off (two deep)
  noInterrupts();
  send(0b00011111, 0x000000);
  send(0b00011111, 0x000000);
  interrupts();
}

/*
 * Sends the lowest 24-bits of the specified (color) data to all configured pins on 
 * the configured port.  
 *
 * Call this with interrupts disabled.
 */
void send(byte pin_mask, uint32_t data) {
  // Send the highest order bits first
  unsigned long dataMask = 0b100000000000000000000000;
  for (byte i = 0; i < 24; i++) {
    if ((data & dataMask) == dataMask) {
      LEDS_HIGH(pin_mask);
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");    
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      LEDS_LOW(pin_mask);
    } else {
      LEDS_HIGH(pin_mask);
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");
      __asm__("nop\n\t");    
      LEDS_LOW(pin_mask);
    }
    dataMask >>= 1;
  }
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

#define RGB(R,G,B)    (((uint32_t) (R) << 16) | ((uint32_t) (G) << 8) | ((uint32_t) (B)))
#define RGB_F(R,G,B)  RGB(((byte) (255.0 * (R))), ((byte) (255.0 * (G))), ((byte) (255.0 * (B))))

uint32_t hsv_to_rgb(uint32_t hsv) {
  float h = ((hsv >> 16) & 0xff) / 255.0;
  float s = ((hsv >> 8) & 0xff) / 255.0;
  float v = ((hsv) & 0xff) / 255.0;
  
  // Some shade of grey, use the value for all color channels
  if (s < FLT_EPSILON) {
    return RGB_F(v, v, v);
  }

  int region = floor(h * 6);
  float remainder = (h * 6) - region;
  float p = v * (1.0 - s);
  float q = v * (1.0 - (s * remainder));
  float t = v * (1.0 - (s * (1.0 - remainder)));

  switch (region) {
    case 0:
      return RGB_F(v, t, p);
    case 1:
      return RGB_F(q, v, p);
    case 2:
      return RGB_F(p, v, t);
    case 3:
      return RGB_F(p, q, v);
    case 4:
      return RGB_F(t, p, v);
    default:
      return RGB_F(v, p, q);
  }
}

#define HSV(H,S,V)    (((uint32_t) (H) << 16) | ((uint32_t) (S) << 8) | ((uint32_t) (V)))
// Hue is degrees 0-360, saturation is 0-1, value is 0-1
#define HSV_F(H,S,V)  HSV(((byte) (255.0 * ((H) / 360.0))), ((byte) (255.0 * (S))), ((byte) (255.0 * (V))))

uint32_t rgb_to_hsv(uint32_t rgb) {
  float r = ((rgb >> 16) & 0xff) / 255.0;
  float g = ((rgb >> 8) & 0xff) / 255.0;
  float b = ((rgb) & 0xff) / 255.0;
  float h, s, v;
  
  float rgb_min = MIN3(r, g, b);
  float rgb_max = MAX3(r, g, b);

  // v is the maximum channel
  v = rgb_max;

  // We can exit early for black
  if (v == 0) {
    h = 0.0;
    s = 0.0;
    return HSV_F(h, s, v);
  }
  
  // Normalize color channels against the value (the max color channel)
  r /= v;
  g /= v;
  b /= v;
  rgb_min = MIN3(r, g, b);
  rgb_max = MAX3(r, g, b);
  
  // Saturation is the distance between the extreme color channels
  s = rgb_max - rgb_min;

  // If the saturation is 0, the color is some shade of gray
  if (s < FLT_EPSILON) {
    h = 0.0;
    return HSV_F(h, s, v);
  }

  // Normalize color channels against value and saturation
  r = (r - rgb_min) / (rgb_max - rgb_min);
  g = (g - rgb_min) / (rgb_max - rgb_min);
  b = (b - rgb_min) / (rgb_max - rgb_min);
  rgb_min = MIN3(r, g, b);
  rgb_max = MAX3(r, g, b);
  
  // Calculate the hue angle closest to which color is the max
  if (rgb_max == r) {
    h = 0.0 + 60.0 * (g - b);
    if (h < 0.0) {
       h += 360.0;
    }
  } else if (rgb_max == g) {
    h = 120.0 + 60.0 * (b - r);
  } else {
    h = 240.0 + 60.0 * (r - g);
  }

  return HSV_F(h, s, v);
}
