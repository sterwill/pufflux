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

#ifndef __UTIL_H_
#define __UTIL_H_

// Find the token index of the value of the specified property of the specified object 
inline int find_json_prop(const char *json, jsmntok_t *tokens, int num_tokens, int object_tok, const char *prop_name) {
  // Easy way: scan through all tokens looking for parentage
  for (int i = 0; i < num_tokens; i++) {
    jsmntok_t *tok = &tokens[i];
    if (tok->parent == object_tok) {
      if (strlen(prop_name) == tok->end - tok->start &&
          strncmp(json + tok->start, prop_name, (size_t) tok->end - tok->start) == 0) {
        // The next token is the value
        return i + 1;
      }
    }
  }
  return -1;
}

#endif /* __UTIL_H_ */
