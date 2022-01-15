#ifndef __WEATHER_H_
#define __WEATHER_H_

// Arbitrary categories of types of VTEC "phenomena" (pp) field
typedef enum {
  CAT_UNKNOWN, // our own value
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

// P-VTEC "significance" (s) field.  Ordered least- to most-significant.
typedef enum {
  SIG_UNKNOWN, // our own value
  SIG_SYNOPSIS,
  SIG_OUTLOOK,
  SIG_FORECAST,
  SIG_STATEMENT,
  SIG_ADVISTORY,
  SIG_WATCH,
  SIG_WARNING,
} phen_sig_t;

void weather_setup(void);
void weather_loop(void);
void weather_get_active_phenomena(phen_cat_t *cat, phen_sig_t *sig);

#endif /* __WEATER_H_ */
