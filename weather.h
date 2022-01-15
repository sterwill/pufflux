#ifndef __WEATHER_H_
#define __WEATHER_H_

// Arbitrary categories of types of VTEC phenomena ("pp" field).
typedef enum {
  CAT_UNKNOWN,
  CAT_AIR_QUALITY,
  CAT_COLD,
  CAT_HEAT,
  CAT_FLOOD,
  CAT_LOW_WATER,
  CAT_MARINE,
  CAT_SNOW,
  CAT_WIND,
  CAT_DUST,
  CAT_FOG,
  CAT_FREEZE,
  CAT_FIRE,
  CAT_STORM,
  CAT_ICE,
  CAT_TORNADO,
} phen_cat_t;

void weather_setup(void);
void weather_loop(void);

#endif /* __WEATER_H_ */
