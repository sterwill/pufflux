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
 * Where the LEDs are:
 *
 * -4B,4A---------2A,2B-
 *        |     |
 *        |5B,5A|
 *        |     |
 * -3B,3A---------1A,1B-
 *           ^
 *     front |
 */
 
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

/************************************************************************/
/* You probably don't need to change anything below here.               */
/************************************************************************/

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

struct cloud_state {
  byte animation;
  bool fast;
  uint32_t base_color;
  uint32_t highlight_color;  
  
  byte frame;
  unsigned long last_frame_time;
};

void setup() {
  // For debugging
  Serial.begin(115200);
    
  // Configure which pins in the port are in vs. out
  CONFIG_DDR();
  
  // Reset all the controllers and turn off the LEDs
  reset_leds();
}

void loop() {
  struct cloud_state state;
  state.animation = ANIM_FLOOD_ID;
  state.fast = true;
  state.base_color = COLOR_WHITE_ID;
  state.highlight_color = COLOR_RED_ID;
  state.frame = 0;
  state.last_frame_time = 0;

  uint16_t command;
  
  print_state(state);
  while (true) {
    if (read_command(command)) {
      decode_command(command, state);
      print_state(state);
    }
    
    animate(state);
  }
}

void print_state(const struct cloud_state & state) {
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

void decode_command(uint16_t command, struct cloud_state & state) {
  state.animation = (command & ANIM_GET_MASK);
  state.fast = (command & SPEED_GET_MASK) >> 3;
  state.base_color = (command & BASE_COLOR_GET_MASK) >> 8;
  state.highlight_color = (command & HIGHLIGHT_COLOR_GET_MASK) >> 12;
  
  state.frame = 0;
  state.last_frame_time = 0;
}

bool read_command(uint16_t & command) {
  //command = 0b0111001000001001;
  // TODO read from radio
  return false;
}

void animate(struct cloud_state & state) {
  switch (state.animation) {
    case ANIM_PRECIPITATION_ID:
      animate_precipitation(state);
      break;
    case ANIM_FLOOD_ID:
      animate_flood(state);
      break;
    case ANIM_PULSE_ID:
      animate_pulse(state);
      break;
    case ANIM_SWIRL_ID:
      animate_swirl(state);
      break;
    case ANIM_DEFAULT_ID:
    default:
      animate_default(state);
      break;
  }
}

// A randomly twinkling animation
void animate_precipitation(struct cloud_state & state) {
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
void animate_flood(const struct cloud_state & state) {
  static boolean flood_queue[6] = {false, false, false, false, false, false};
  uint32_t base_color_bgr = RGB2GBR(rgb_values[state.base_color]);
  uint32_t highlight_color_bgr = RGB2GBR(rgb_values[state.highlight_color]);

  // See the map at the top of this file.
  
  noInterrupts();

  // left side
  send(0b00001000, flood_queue[4] ? highlight_color_bgr : base_color_bgr);
  send(0b00001000, flood_queue[5] ? highlight_color_bgr : base_color_bgr);
  
  send(0b00000100, flood_queue[4] ? highlight_color_bgr : base_color_bgr);
  send(0b00000100, flood_queue[5] ? highlight_color_bgr : base_color_bgr);

  // middle
  send(0b00010000, flood_queue[2] ? highlight_color_bgr : base_color_bgr);
  send(0b00010000, flood_queue[3] ? highlight_color_bgr : base_color_bgr);

  // right side
  send(0b00000010, flood_queue[1] ? highlight_color_bgr : base_color_bgr);
  send(0b00000010, flood_queue[0] ? highlight_color_bgr : base_color_bgr);
  
  send(0b00000001, flood_queue[1] ? highlight_color_bgr : base_color_bgr);
  send(0b00000001, flood_queue[0] ? highlight_color_bgr : base_color_bgr);
 
  interrupts();

  // Shift items toward the head of the queue
  for (int i = 0; i < 5; i++) {
    flood_queue[i] = flood_queue[i + 1];
  }

  // Compute new tail element
  flood_queue[5] = random(6) == 0;

  delay(state.fast ? 200 : 500);
}

void animate_pulse(const struct cloud_state & state) {
}

// Colors move in a circle, down one side, then down the other in the opposite direction
void animate_swirl(const struct cloud_state & state) {
  // The middle segment uses 4 indices in this animation
  static byte swirl_queue[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1};

  uint32_t colors[3];
  colors[0] = RGB2GBR(rgb_values[COLOR_BLACK_ID]);
  colors[1] = RGB2GBR(rgb_values[state.base_color]);
  colors[2] = RGB2GBR(rgb_values[state.highlight_color]);

  // See the map at the top of this file.
  
  noInterrupts();

  // front left
  send(0b00000100, colors[swirl_queue[10]]);
  send(0b00000100, colors[swirl_queue[11]]);
  
  // middle
  send(0b00010000, max(colors[swirl_queue[8]], colors[swirl_queue[3]]));
  send(0b00010000, max(colors[swirl_queue[9]], colors[swirl_queue[2]]));

  // front right
  send(0b00000001, colors[swirl_queue[7]]);
  send(0b00000001, colors[swirl_queue[6]]);
  
  // rear right 
  send(0b00000010, colors[swirl_queue[4]]);
  send(0b00000010, colors[swirl_queue[5]]);
  
  // already handled these incides for the middle
  
  // rear left
  send(0b00001000, colors[swirl_queue[1]]);
  send(0b00001000, colors[swirl_queue[0]]);
 
  interrupts();

  // Rotate the queue one place toward the head
  byte head = swirl_queue[0];
  for (byte i = 0; i < 11; i++) {
    swirl_queue[i] = swirl_queue[i + 1];
  }
  swirl_queue[11] = head;

  delay(state.fast ? 120 : 300);

}

void animate_default(const struct cloud_state & state) {
}

/*
  short deltaRed = 1;
  byte red = minVal;
  
  short deltaGreen = 2;
  byte green = minVal + 20;
  
  short deltaBlue = 3;
  byte blue = minVal + 30;
  
  while (true) {
    fade(red, deltaRed);
    fade(green, deltaGreen);
    fade(blue, deltaBlue);
    uint32_t color = ((uint32_t) green << 16) | ((uint32_t) blue << 8) | red;
//    Serial.println(color);

    noInterrupts();
    for (int j = 0; j < 10; j++) {
      send_strip(color);
    }
    interrupts();
    delay(50);
  }

}
*/

/*
 * Resets the LED controllers on all pins.  LEDs turn off.
 */
void reset_leds() {
  // Reset
  LEDS_LOW(0b00011111);
  delayMicroseconds(20);

  // All off (two deep)
  noInterrupts();
  send(0b11111111, 0x000000);
  send(0b11111111, 0x000000);
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
