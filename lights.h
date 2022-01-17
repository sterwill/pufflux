/*
 * Pufflux for the Adafruit Feather M0 WiFi - ATSAMD21 + ATWINC1500 
 * (Product ID: 3010)
 *
 * Copyright 2013-2022 Shaw Terwilliger <sterwill@tinfig.com>
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

#ifndef __LIGHTS_H_
#define __LIGHTS_H_

typedef enum {
  ANIM_DEFAULT,
  ANIM_PRECIP,
  ANIM_FLOOD,
  ANIM_PULSE,
  ANIM_SWIRL,
} anim;

typedef enum {
  COLOR_BLACK,
  COLOR_WHITE,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_LIGHT_BLUE,
  COLOR_DARK_BLUE,
  COLOR_LIGHT_GRAY,
  COLOR_DARK_GRAY,
  COLOR_YELLOW,
  COLOR_ORANGE,
} color;

#define COLOR_MAX COLOR_ORANGE

void lights_setup(void);
void lights_loop(void);
void lights_configure(anim animation, bool fast, color base_color, color highlight_color);

#endif /* __LIGHTS_H_ */
