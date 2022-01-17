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

#ifndef __CONFIG_H_
#define __CONFIG_H_

/*
 * If defined, enables serial port output.
 */
#define DEBUG

/*
 * Put your wifi network name and passphrase here.
 */
#define WIFI_SSID       "Your network here"
#define WIFI_PASSPHRASE "Your passphrase here"

/*
 * Set the forecast location.  This must be a string that can be resolved 
 * to a latitude and longitude using the ESRI ArcGIS web wervice, which is 
 * what powers the search box on the National Weather Service's web site 
 * (https://www.weather.gov/).  You can use a ZIP code, or a city and state
 * like "Durham NC".  Test your search string at the weather.gov web
 * site so you know it finds the right place.
 */
#define NWS_LOCATION   "Durham NC"
#define NWS_USER_AGENT "pufflux/1 https://github.com/sterwill/pufflux"

/* 
 * Fetch the weather forecast every this many minutes. 
 */
#define FORECAST_PERIOD_MINUTES 15

/*
 * LEDs are a single 68-element NeoPixel strip that forms a circle
 * around the chassis:
 *
 *  =67-56===========45-34=
 *         | 55-46 |
 *         |       | 
 *         | 12-21 |
 *  =00-11===========22-33=
 *             
 *             ^
 *       front |
 */

// Pin the NeoPixel strip is connected to
#define LED_PIN    1

// Number of NeoPixels on the strip
#define LED_COUNT  68

#endif /* __CONFIG_H_ */
