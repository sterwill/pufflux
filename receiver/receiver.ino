/*
 * Pufflux Receiver
 *
 * Copyright 2013 Shaw Terwilliger <sterwill@tinfig.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */

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

/************************************************************************/
/* You probably don't need to change anything below here.               */
/************************************************************************/

/*
 * You can change the RGB values of these colors, but don't change the
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

const uint32_t rgb_values[11] = {
  0x000000, // COLOR_BLACK_ID
  0xffffff, // COLOR_WHITE_ID
  0xff0000, // COLOR_RED_ID
  0x00ff00, // COLOR_GREEN_ID
  0x0000ff, // COLOR_BLUE_ID
  0xb5ddff, // COLOR_LIGHT_BLUE_ID
  0x1688fa, // COLOR_DARK_BLUE_ID
  0xaaaaaa, // COLOR_LIGHT_GRAY_ID
  0x303030, // COLOR_DARK_GRAY_ID
  0xfcec5b, // COLOR_YELLOW_ID
  0xff8900  // COLOR_ORANGE_ID
};

#define COLOR_ID_MASK           0b0000000000000111

/*
 * Don't change these unless you change the transmitter software to match.
 */
#define ANIM_DEFAULT_ID       0
#define ANIM_PRECIPITATION_ID 1
#define ANIM_FLOOD_ID         2
#define ANIM_PULSE_ID         3
#define ANIM_SWIRL_ID         4

#define RGB2GBR(c) (((c & 0xff0000) >> 16) | ((c & 0x00ff00) << 8) | ((c & 0x0000ff) << 8)) 

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

  // LED states
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
    
  // Configure which pins in the port are in vs. out
  CONFIG_DDR();
  
  // Reset all the controllers and turn off the LEDs
  reset_leds();
}

void loop() {
  state.animation = ANIM_PRECIPITATION_ID;
  state.fast = false;
  state.base_color = COLOR_WHITE_ID;
  state.highlight_color = COLOR_RED_ID;
  memset(state.target_colors, 0, sizeof(state.target_colors));
  memset(state.current_colors, 0, sizeof(state.current_colors));
  memset(state.source_colors, 0, sizeof(state.source_colors));
  state.fade_steps = 64;
  
  uint16_t command;
  
  print_state();
  while (true) {
    if (read_command(command)) {
      decode_command(command);
      print_state();
    }
    
    animate();
    
    /*
     * A very small delay here helps our byte-sized step counters add up to meaningful
     * times.  A delay of 8 ms makes 255 steps take at least 2048 ms, a good long fade.
     */
    delay(8);
  }
}

void print_state() {
#ifdef DEBUG
  Serial.println("----------");
  Serial.print("Animation: ");
  Serial.println(state.animation);
  Serial.print("Fast: ");
  Serial.println(state.fast);
  Serial.print("Base color: ");
  Serial.println(state.base_color);
  Serial.print("Highlight color: ");
  Serial.println(state.highlight_color);
#endif
}

void decode_command(uint16_t command) {
  state.animation = (command & ANIM_GET_MASK);
  state.fast = (command & SPEED_GET_MASK) >> 3;
  state.base_color = (command & BASE_COLOR_GET_MASK) >> 8;
  state.highlight_color = (command & HIGHLIGHT_COLOR_GET_MASK) >> 12;
}

bool read_command(uint16_t & command) {
  //command = 0b0111001000001001;
  // TODO read from radio
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
  
  state.fade_steps = state.fast ? 8 : 16;

  if (time - last_time > (state.fast ? 128 : 256)) {
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
  
  state.fade_steps = state.fast ? 16 : 32;

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
  
  state.fade_steps = state.fast ? 128 : 255;

  if (time - last_time > (state.fast ? 1024 : 2048)) {
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
  
  state.fade_steps = state.fast ? 16 : 32;

  // Rotate the queue one place if the period has elapsed
  if (time - last_time > (state.fast ? 128 : 256)) {
    rotate_right(swirl_queue, queue_size);
    last_time = time;
  }
}

void animate_default() {
}

/*
 * Sets the desired indexed color for the specified LED.
 */
void set_color(const byte led, const byte color_id) {
  const uint32_t color = rgb_values[color_id];
  if (state.target_colors[led] != color) {
    state.target_colors[led] = color;
    // Reset the source so the next color step knows where we started from
    state.source_colors[led] = state.current_colors[led];
  }
}

/*
 * Changes the "current" colors to be one step closer to the "target" colors.
 * Colors are interpolated linearly by channel.
 */
void step_colors() {
  for (int i = 0; i < 10; i++){
    uint32_t target = state.target_colors[i];
    uint32_t current = state.current_colors[i];
    uint32_t source = state.source_colors[i];
    byte tgt_r = (target >> 16) & 0xff;
    byte tgt_g = (target >> 8) & 0xff;
    byte tgt_b = (target) & 0xff;
    byte cur_r = (current >> 16) & 0xff;
    byte cur_g = (current >> 8) & 0xff;
    byte cur_b = (current) & 0xff;
    byte src_r = (source >> 16) & 0xff;
    byte src_g = (source >> 8) & 0xff;
    byte src_b = (source) & 0xff;

    /*
     * Each step moves "cur" closer to "tgt" by an amount that's a fraction of
     * the original difference between "src" and "tgt".  When other functions 
     * change the "tgt", they must also set the "src" to "cur" so we can calculate
     * new step sizes.
     */
     
    short diff_r = tgt_r - cur_r;
    short diff_g = tgt_g - cur_g;
    short diff_b = tgt_b - cur_b;

    byte adjust;
   
    // If the channel needs an adjustment
    if (diff_r != 0) {
      // Adjust by the step size between src and tgt, but always at least 1
      adjust = max(1, min(abs(diff_r), abs(tgt_r - src_r) / state.fade_steps));
      cur_r += (diff_r >= 0) ? adjust : -adjust;
    }
  
    if (diff_g != 0) {
      adjust = max(1, min(abs(diff_g), abs(tgt_g - src_g) / state.fade_steps));
      cur_g += (diff_g >= 0) ? adjust : -adjust;
    }

    if (diff_b != 0) {    
      adjust = max(1, min(abs(diff_b), abs(tgt_b - src_b) / state.fade_steps));
      cur_b += (diff_b >= 0) ? adjust : -adjust;
    }

    state.current_colors[i] = ((uint32_t) cur_r << 16) | ((uint32_t) cur_g << 8) | ((uint32_t) cur_b);

    if (current == target) {
      state.source_colors[i] = target;
    }
  }
}

/*
 * Updates the colors of all the LEDs to their "current" colors.
 */
void update_leds() {
  noInterrupts();
  for (int i = 0; i < 10; i++) {
    // Get the led index from the order array
    const byte led = led_send_order[i];
    const byte led_address = leds[led];
    
    // Crack the address into pin and offset
    byte pin_mask = 0x1 << ((led_address & 0b11110000) >> 4);
    byte offset = (led_address & 0b00001111);

    send(pin_mask, RGB2GBR(state.current_colors[led]));
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
