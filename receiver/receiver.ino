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
 *  -1,2---------4,5-
 *       |  3  |
 *       |  8  |
 *  -6,7---------9,10
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
#define COLOR_LIGHT_BLUE_ID     2
#define COLOR_DARK_BLUE_ID      3
#define COLOR_LIGHT_GRAY_ID     4
#define COLOR_DARK_GRAY_ID      5
#define COLOR_YELLOW_ID         6
#define COLOR_ORANGE_ID         7
#define COLOR_RED_ID            8

const uint32_t rgb_values[9] = {
  0x000000, // COLOR_BLACK_ID
  0xffffff, // COLOR_WHITE_ID
  0xb5ddff, // COLOR_LIGHT_BLUE_ID
  0x1688fa, // COLOR_DARK_BLUE_ID
  0xaaaaaa, // COLOR_LIGHT_GRAY_ID
  0x303030, // COLOR_DARK_GRAY_ID
  0xfcec5b, // COLOR_YELLOW_ID
  0xff8900, // COLOR_ORANGE_ID
  0xff0000  // COLOR_RED_ID
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

  // Timing  
  unsigned long last_animated;
};
static struct cloud_state state;

void setup() {
  // For debugging
  Serial.begin(115200);
    
  // Configure which pins in the port are in vs. out
  CONFIG_DDR();
  
  // Reset all the controllers and turn off the LEDs
  reset_leds();
}

void loop() {
  state.animation = ANIM_SWIRL_ID;
  state.fast = false;
  state.base_color = COLOR_YELLOW_ID;
  state.highlight_color = COLOR_DARK_BLUE_ID;
  memset(state.target_colors, 0, sizeof(state.target_colors));
  memset(state.current_colors, 0, sizeof(state.current_colors));
  state.last_animated;
  
  uint16_t command;
  
  print_state();
  while (true) {
    if (read_command(command)) {
      decode_command(command);
      print_state();
    }
    
    animate();
  }
}

void print_state() {
  Serial.println("----------");
  Serial.print("Animation: ");
  Serial.println(state.animation);
  Serial.print("Fast: ");
  Serial.println(state.fast);
  Serial.print("Base color: ");
  Serial.println(state.base_color);
  Serial.print("Highlight color: ");
  Serial.println(state.highlight_color);
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
  boolean enabled[10];  
  for (byte i = 0; i < 10; i++) {
    enabled[i] = (boolean) random(2);
  }
  
  uint32_t base_color_bgr = RGB2GBR(rgb_values[state.base_color]);
  uint32_t highlight_color_bgr = RGB2GBR(rgb_values[state.highlight_color]);

  noInterrupts();
  // For each pin
  for (byte i = 0; i < 5; i++) {
    // Send two colors (for near and far LED modules)
    for (byte j = 0; j < 2; j++) {
      send(0b00000001 << i, enabled[i + j] ? base_color_bgr : highlight_color_bgr);
    }    
  }
  interrupts();

  delay(state.fast ? 100 : 300);
}

// Colors move in waves from one end to the other
void animate_flood() {
  const byte queue_size = 5;
  static boolean flood_queue[queue_size] = {false, false, false, false, false};
  uint32_t base_color_bgr = RGB2GBR(rgb_values[state.base_color]);
  uint32_t highlight_color_bgr = RGB2GBR(rgb_values[state.highlight_color]);

  // See the map at the top of this file.
  
  noInterrupts();

  // left side
  send(0b00001000, flood_queue[3] ? highlight_color_bgr : base_color_bgr);
  send(0b00001000, flood_queue[4] ? highlight_color_bgr : base_color_bgr);
  
  send(0b00000100, flood_queue[3] ? highlight_color_bgr : base_color_bgr);
  send(0b00000100, flood_queue[4] ? highlight_color_bgr : base_color_bgr);

  // middle
  send(0b00010000, flood_queue[2] ? highlight_color_bgr : base_color_bgr);
  send(0b00010000, flood_queue[2] ? highlight_color_bgr : base_color_bgr);

  // right side
  send(0b00000010, flood_queue[1] ? highlight_color_bgr : base_color_bgr);
  send(0b00000010, flood_queue[0] ? highlight_color_bgr : base_color_bgr);
  
  send(0b00000001, flood_queue[1] ? highlight_color_bgr : base_color_bgr);
  send(0b00000001, flood_queue[0] ? highlight_color_bgr : base_color_bgr);
 
  interrupts();

  // Shift items toward the head of the queue
  for (int i = 0; i < queue_size - 1; i++) {
    flood_queue[i] = flood_queue[i + 1];
  }

  // Compute new tail element
  flood_queue[queue_size - 1] = random(6) == 0;

  delay(state.fast ? 200 : 500);
}

void animate_pulse() {
  static unsigned long last_time = 0;
  static boolean phase = false;
  unsigned long time = millis();
  
  if (time - last_time > 1000) {
    for (int i = 0; i < 10; i++) {
      if (phase) {
        set_color(i, state.base_color);
      } else {
        set_color(i, state.highlight_color);        
      }
    }
    phase = !phase;
    last_time = time;
  }
}

// Colors move in a circle, down one side, then down the other in the opposite direction
void animate_swirl() {
  static unsigned long last_time = 0;
  static byte swirl_queue[10] = {0, 0, 0, 0, 0, 0, 0, 1, 2, 1};
  unsigned long time = millis();
  const short period = state.fast ? 60 : 200;
  
  // Set the colors from the queue
  byte color_ids[3];
  color_ids[0] = COLOR_BLACK_ID;
  color_ids[1] = state.base_color;
  color_ids[2] = state.highlight_color;

  // See the map at the top of this file.
  set_color(0, color_ids[swirl_queue[9]]);
  set_color(1, color_ids[swirl_queue[8]]);
  set_color(2, color_ids[swirl_queue[7]]);
  set_color(3, color_ids[swirl_queue[6]]);
  set_color(4, color_ids[swirl_queue[5]]);
  set_color(9, color_ids[swirl_queue[4]]);
  set_color(8, color_ids[swirl_queue[3]]);
  set_color(7, color_ids[swirl_queue[2]]);
  set_color(6, color_ids[swirl_queue[1]]);
  set_color(5, color_ids[swirl_queue[0]]);
  
  // Rotate the queue one place if the period has elapsed
  if (time - last_time > period) {
    byte head = swirl_queue[0];
    for (byte i = 0; i < 9; i++) {
      swirl_queue[i] = swirl_queue[i + 1];
    }
    swirl_queue[9] = head;
    
    last_time = time;
  }
}

void animate_default() {
}

/*
 * Sets the desired indexed color for the specified LED.
 */
void set_color(const byte led, const byte color_id) {
  state.target_colors[led] = rgb_values[color_id];
}

/*
 * Changes the "current" colors to be one step closer to the "target" colors.
 * Colors are interpolated linearly.
 */
void step_colors() {
  const byte delta = state.fast ? 2 : 1;
  for (int i = 0; i < 10; i++){
    uint32_t current = state.current_colors[i];
    uint32_t target = state.target_colors[i];

    byte cur_r = (current >> 16) & 0xff;
    byte cur_g = (current >> 8) & 0xff;
    byte cur_b = (current) & 0xff;
    byte tar_r = (target >> 16) & 0xff;
    byte tar_g = (target >> 8) & 0xff;
    byte tar_b = (target) & 0xff;
    
    short diff_r = tar_r - cur_r;
    short diff_g = tar_g - cur_g;
    short diff_b = tar_b - cur_b;
    
    byte adjust;
   
    adjust = min(abs(diff_r), delta);
    cur_r += (diff_r >= 0) ? adjust : -adjust;
    
    adjust = min(abs(diff_g), delta);
    cur_g += (diff_g >= 0) ? adjust : -adjust;
    
    adjust = min(abs(diff_b), delta);
    cur_b += (diff_b >= 0) ? adjust : -adjust;

    state.current_colors[i] = ((uint32_t) cur_r << 16) | ((uint32_t) cur_g << 8) | ((uint32_t) cur_b);
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

void linear_fade(byte & channel, short & delta, const byte minVal, const byte maxVal) {
  // Up-cast to detect overflow / underflow
  short val = (short) channel + delta;
  if (val > maxVal) {
    val = maxVal;
    delta *= -1;
  } else if (val < minVal) {
    val = minVal;
    delta *= -1;
  } else {
    channel += delta;
  }
}


