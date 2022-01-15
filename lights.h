#ifndef __LIGHTS_H_
#define __LIGHTS_H_

typedef enum {
  ANIM_DEFAULT,
  ANIM_PRECIP,
  ANIM_FLOOD,
  ANIM_PULSE,
  ANIM_SWIRL,
} anim_t;

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
} color_t;

#define COLOR_MAX COLOR_ORANGE

void lights_setup(void);
void lights_loop(void);
void lights_configure(anim_t animation, bool fast, color_t base_color, color_t highlight_color);

#endif /* __LIGHTS_H_ */
