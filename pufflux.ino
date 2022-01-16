/*
   Pufflux for the Adafruit Feather M0 WiFi - ATSAMD21 + ATWINC1500
   (Product ID: 3010)

   Copyright 2013-2021 Shaw Terwilliger <sterwill@tinfig.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include <WiFi101.h>

#include "config.h"
#include "weather.h"
#include "lights.h"

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
  // Uncomment if you want to guarantee seeing early debug messages.
  // If you leave this wait enabled, the microcontroller will hang
  // until a serial connection is available (this is not desirable when
  // it's installed on the wall).
  //while (!Serial) {}
#endif
  WiFi.setPins(8, 7, 4, 2);
  WiFi.begin(WIFI_SSID, WIFI_PASSPHRASE);

  lights_setup();
  weather_setup();
}

void loop() {
  weather_loop();
  lights_loop();
}
